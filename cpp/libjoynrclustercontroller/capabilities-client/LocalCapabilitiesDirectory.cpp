/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

#include "joynr/LocalCapabilitiesDirectory.h"

#include <algorithm>
#include <unordered_set>
#include <ostream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>

#include "joynr/access-control/IAccessController.h"

#include "joynr/CapabilityUtils.h"
#include "joynr/CallContextStorage.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/IMessageRouter.h"
#include "joynr/Util.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/Util.h"

#include "libjoynrclustercontroller/capabilities-client/ICapabilitiesClient.h"

namespace joynr
{

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(
        ClusterControllerSettings& clusterControllerSettings,
        std::shared_ptr<ICapabilitiesClient> capabilitiesClientPtr,
        const std::string& localAddress,
        std::weak_ptr<IMessageRouter> messageRouter,
        boost::asio::io_service& ioService,
        const std::string clusterControllerId)
        : joynr::system::DiscoveryAbstractProvider(),
          clusterControllerSettings(clusterControllerSettings),
          capabilitiesClient(std::move(capabilitiesClientPtr)),
          localAddress(localAddress),
          cacheLock(),
          pendingLookupsLock(),
          registeredGlobalCapabilities(),
          messageRouter(messageRouter),
          observers(),
          pendingLookups(),
          accessController(),
          checkExpiredDiscoveryEntriesTimer(ioService),
          isLocalCapabilitiesDirectoryPersistencyEnabled(
                  clusterControllerSettings.isLocalCapabilitiesDirectoryPersistencyEnabled()),
          freshnessUpdateTimer(ioService),
          clusterControllerId(clusterControllerId)
{
    scheduleCleanupTimer();
    scheduleFreshnessUpdate();
}

void LocalCapabilitiesDirectory::shutdown()
{
    checkExpiredDiscoveryEntriesTimer.cancel();
    freshnessUpdateTimer.cancel();
}

void LocalCapabilitiesDirectory::scheduleFreshnessUpdate()
{
    boost::system::error_code timerError = boost::system::error_code();
    freshnessUpdateTimer.expires_from_now(
            clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs(), timerError);
    if (timerError) {
        JOYNR_LOG_ERROR(logger(),
                        "Error from freshness update timer: {}: {}",
                        timerError.value(),
                        timerError.message());
    }
    freshnessUpdateTimer.async_wait(
            std::bind(&LocalCapabilitiesDirectory::sendAndRescheduleFreshnessUpdate,
                      this,
                      std::placeholders::_1));
}

void LocalCapabilitiesDirectory::sendAndRescheduleFreshnessUpdate(
        const boost::system::error_code& timerError)
{
    if (timerError == boost::asio::error::operation_aborted) {
        // Assume Destructor has been called
        JOYNR_LOG_DEBUG(logger(),
                        "freshness update aborted after shutdown, error code from freshness update "
                        "timer: {}",
                        timerError.message());
        return;
    } else if (timerError) {
        JOYNR_LOG_ERROR(
                logger(),
                "send freshness update called with error code from freshness update timer: {}",
                timerError.message());
    }

    auto onError = [](const joynr::exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger(), "error sending freshness update: {}", error.getMessage());
    };
    capabilitiesClient->touch(clusterControllerId, nullptr, std::move(onError));
    scheduleFreshnessUpdate();
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    freshnessUpdateTimer.cancel();
    checkExpiredDiscoveryEntriesTimer.cancel();
    clear();
}

void LocalCapabilitiesDirectory::addInternal(const types::DiscoveryEntry& discoveryEntry)
{
    const bool isGloballyVisible = isGlobal(discoveryEntry);

    // register locally
    this->insertInCache(discoveryEntry, true, isGloballyVisible);

    // Inform observers
    informObserversOnAdd(discoveryEntry);

    // register globally
    if (isGloballyVisible) {
        types::GlobalDiscoveryEntry globalDiscoveryEntry = toGlobalDiscoveryEntry(discoveryEntry);

        if (std::find(registeredGlobalCapabilities.begin(),
                      registeredGlobalCapabilities.end(),
                      globalDiscoveryEntry) == registeredGlobalCapabilities.end()) {

            std::function<void(const exceptions::JoynrException&)> onError =
                    [&](const exceptions::JoynrException& error) {
                JOYNR_LOG_ERROR(
                        logger(),
                        "Error occured during the execution of capabilitiesProxy->add. Error: {}",
                        error.getMessage());
            };

            std::function<void()> onSuccess = [this, globalDiscoveryEntry]() {
                JOYNR_LOG_TRACE(logger(),
                                "Global capability addedd successfully, adding it to list "
                                "of registered capabilities.");
                this->registeredGlobalCapabilities.push_back(globalDiscoveryEntry);
            };

            // Add globally
            capabilitiesClient->add(globalDiscoveryEntry, onSuccess, onError);
        }
    }

    updatePersistedFile();
    {
        std::lock_guard<std::mutex> lock(pendingLookupsLock);
        callPendingLookups(
                InterfaceAddress(discoveryEntry.getDomain(), discoveryEntry.getInterfaceName()));
    }
}

types::GlobalDiscoveryEntry LocalCapabilitiesDirectory::toGlobalDiscoveryEntry(
        const types::DiscoveryEntry& discoveryEntry) const
{
    return types::GlobalDiscoveryEntry(discoveryEntry.getProviderVersion(),
                                       discoveryEntry.getDomain(),
                                       discoveryEntry.getInterfaceName(),
                                       discoveryEntry.getParticipantId(),
                                       discoveryEntry.getQos(),
                                       discoveryEntry.getLastSeenDateMs(),
                                       discoveryEntry.getExpiryDateMs(),
                                       discoveryEntry.getPublicKeyId(),
                                       localAddress);
}

void LocalCapabilitiesDirectory::remove(const std::string& participantId)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    boost::optional<types::DiscoveryEntry> optionalEntry =
            localCapabilities.lookupByParticipantId(participantId);
    if (!optionalEntry) {
        JOYNR_LOG_INFO(logger(), "participantId '{}' not found, cannot be removed", participantId);
        return;
    }
    const types::DiscoveryEntry& entry = *optionalEntry;

    if (isGlobal(entry)) {
        JOYNR_LOG_TRACE(logger(), "Removing globally registered participantId: {}", participantId);
        removeFromGloballyRegisteredCapabilities(entry);
        globalCapabilities.removeByParticipantId(participantId);
        capabilitiesClient->remove(participantId);
    }
    JOYNR_LOG_TRACE(logger(), "Removing locally registered participantId: {}", participantId);
    localCapabilities.removeByParticipantId(participantId);
    informObserversOnRemove(entry);
    if (auto messageRouterSharedPtr = messageRouter.lock()) {
        messageRouterSharedPtr->removeNextHop(participantId);
    } else {
        JOYNR_LOG_FATAL(logger(),
                        "could not removeNextHop for {} because messageRouter is not available",
                        participantId);
    }
    updatePersistedFile();
}

void LocalCapabilitiesDirectory::removeFromGloballyRegisteredCapabilities(
        const types::DiscoveryEntry& discoveryEntry)
{
    auto compareFunc = [&discoveryEntry](const types::GlobalDiscoveryEntry& it) {
        return it.getProviderVersion() == discoveryEntry.getProviderVersion() &&
               it.getDomain() == discoveryEntry.getDomain() &&
               it.getInterfaceName() == discoveryEntry.getInterfaceName() &&
               it.getQos() == discoveryEntry.getQos() &&
               it.getParticipantId() == discoveryEntry.getParticipantId() &&
               it.getPublicKeyId() == discoveryEntry.getPublicKeyId();
    };

    while (registeredGlobalCapabilities.erase(std::remove_if(registeredGlobalCapabilities.begin(),
                                                             registeredGlobalCapabilities.end(),
                                                             compareFunc),
                                              registeredGlobalCapabilities.end()) !=
           registeredGlobalCapabilities.end()) {
    }
}

void LocalCapabilitiesDirectory::triggerGlobalProviderReregistration(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;

    {
        std::lock_guard<std::mutex> lock(cacheLock);
        for (const auto& globalCapability : globalCapabilities) {
            if (globalCapability.getQos().getScope() == types::ProviderScope::GLOBAL) {
                capabilitiesClient->add(toGlobalDiscoveryEntry(globalCapability), nullptr, nullptr);
            }
        }
    }

    onSuccess();
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedGlobalDiscoveryEntries()
        const
{
    return std::vector<types::DiscoveryEntry>(
            globalCapabilities.cbegin(), globalCapabilities.cend());
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const joynr::types::DiscoveryQos& discoveryQos,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    std::vector<types::DiscoveryEntry> localCapabilities =
            searchCache(interfaceAddresses, std::chrono::milliseconds(-1), true);
    std::vector<types::DiscoveryEntry> globalCapabilities = searchCache(
            interfaceAddresses, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()), false);

    return callReceiverIfPossible(scope,
                                  std::move(localCapabilities),
                                  std::move(globalCapabilities),
                                  std::move(callback));
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    boost::optional<types::DiscoveryEntry> globalCapability = searchCache(
            participantId, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()), false);

    return callReceiverIfPossible(scope,
                                  getCachedLocalCapabilities(participantId),
                                  optionalToVector(std::move(globalCapability)),
                                  std::move(callback));
}

bool LocalCapabilitiesDirectory::callReceiverIfPossible(
        joynr::types::DiscoveryScope::Enum& scope,
        std::vector<types::DiscoveryEntry>&& localCapabilities,
        std::vector<types::DiscoveryEntry>&& globalCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // return only local capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_ONLY) {
        std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
                util::convert(true, localCapabilities);
        callback->capabilitiesReceived(std::move(localCapabilitiesWithMetaInfo));
        return true;
    }

    // return local then global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
                util::convert(true, localCapabilities);
        std::vector<types::DiscoveryEntryWithMetaInfo> globalCapabilitiesWithMetaInfo =
                util::convert(false, globalCapabilities);
        if (!localCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(localCapabilitiesWithMetaInfo));
            return true;
        }
        if (!globalCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(globalCapabilitiesWithMetaInfo));
            return true;
        }
    }

    // return local and global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        // return if global entries
        if (!globalCapabilities.empty()) {
            std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
                    util::convert(true, localCapabilities);
            std::vector<types::DiscoveryEntryWithMetaInfo> globalCapabilitiesWithMetaInfo =
                    util::convert(false, globalCapabilities);

            // remove duplicates
            std::unordered_set<types::DiscoveryEntryWithMetaInfo> resultSet(
                    std::make_move_iterator(localCapabilitiesWithMetaInfo.begin()),
                    std::make_move_iterator(localCapabilitiesWithMetaInfo.end()));
            resultSet.insert(std::make_move_iterator(globalCapabilitiesWithMetaInfo.begin()),
                             std::make_move_iterator(globalCapabilitiesWithMetaInfo.end()));
            std::vector<types::DiscoveryEntryWithMetaInfo> resultVec(
                    resultSet.begin(), resultSet.end());
            callback->capabilitiesReceived(std::move(resultVec));
            return true;
        }
    }

    // return the global cached entries
    if (scope == joynr::types::DiscoveryScope::GLOBAL_ONLY) {
        if (!globalCapabilities.empty()) {
            std::vector<types::DiscoveryEntryWithMetaInfo> globalCapabilitiesWithMetaInfo =
                    util::convert(false, globalCapabilities);
            callback->capabilitiesReceived(std::move(globalCapabilitiesWithMetaInfo));
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::capabilitiesReceived(
        const std::vector<types::GlobalDiscoveryEntry>& results,
        std::vector<types::DiscoveryEntry>&& cachedLocalCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback,
        joynr::types::DiscoveryScope::Enum discoveryScope)
{
    std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
    std::vector<types::DiscoveryEntryWithMetaInfo> mergedEntries;

    for (types::GlobalDiscoveryEntry globalDiscoveryEntry : results) {
        types::DiscoveryEntryWithMetaInfo convertedEntry =
                util::convert(false, globalDiscoveryEntry);
        capabilitiesMap.insert(
                {globalDiscoveryEntry.getAddress(), std::move(globalDiscoveryEntry)});
        mergedEntries.push_back(std::move(convertedEntry));
    }
    registerReceivedCapabilities(std::move(capabilitiesMap));

    if (discoveryScope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryScope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        std::vector<types::DiscoveryEntryWithMetaInfo> cachedLocalCapabilitiesWithMetaInfo =
                util::convert(false, cachedLocalCapabilities);
        // look if in the meantime there are some local providers registered
        // lookup in the local directory to get local providers which were registered in the
        // meantime.
        mergedEntries.insert(mergedEntries.end(),
                             std::make_move_iterator(cachedLocalCapabilitiesWithMetaInfo.begin()),
                             std::make_move_iterator(cachedLocalCapabilitiesWithMetaInfo.end()));
    }
    callback->capabilitiesReceived(std::move(mergedEntries));
}

void LocalCapabilitiesDirectory::lookup(const std::string& participantId,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(participantId, discoveryQos, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto onSuccess = [this, participantId, callback](
                const std::vector<joynr::types::GlobalDiscoveryEntry>& result) {
            this->capabilitiesReceived(result,
                                       getCachedLocalCapabilities(participantId),
                                       callback,
                                       joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        };
        this->capabilitiesClient->lookup(participantId,
                                         std::move(onSuccess),
                                         std::bind(&ILocalCapabilitiesCallback::onError,
                                                   std::move(callback),
                                                   std::placeholders::_1));
    }
}

void LocalCapabilitiesDirectory::lookup(const std::vector<std::string>& domains,
                                        const std::string& interfaceName,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback,
                                        const joynr::types::DiscoveryQos& discoveryQos)
{
    std::vector<InterfaceAddress> interfaceAddresses;
    interfaceAddresses.reserve(domains.size());
    for (std::size_t i = 0; i < domains.size(); i++) {
        interfaceAddresses.push_back(InterfaceAddress(domains.at(i), interfaceName));
    }

    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(interfaceAddresses, discoveryQos, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto onSuccess = [this, interfaceAddresses, callback, discoveryQos](
                std::vector<joynr::types::GlobalDiscoveryEntry> capabilities) {
            std::lock_guard<std::mutex> lock(pendingLookupsLock);
            if (!(isCallbackCalled(interfaceAddresses, callback, discoveryQos))) {
                this->capabilitiesReceived(capabilities,
                                           getCachedLocalCapabilities(interfaceAddresses),
                                           callback,
                                           discoveryQos.getDiscoveryScope());
            }
            callbackCalled(interfaceAddresses, callback);
        };

        auto onError = [this, interfaceAddresses, callback, discoveryQos](
                const exceptions::JoynrRuntimeException& error) {
            std::lock_guard<std::mutex> lock(pendingLookupsLock);
            if (!(isCallbackCalled(interfaceAddresses, callback, discoveryQos))) {
                callback->onError(error);
            }
            callbackCalled(interfaceAddresses, callback);
        };

        if (discoveryQos.getDiscoveryScope() == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
            std::lock_guard<std::mutex> lock(pendingLookupsLock);
            registerPendingLookup(interfaceAddresses, callback);
        }
        this->capabilitiesClient->lookup(domains,
                                         interfaceName,
                                         discoveryQos.getDiscoveryTimeout(),
                                         std::move(onSuccess),
                                         std::move(onError));
    }
}

void LocalCapabilitiesDirectory::callPendingLookups(const InterfaceAddress& interfaceAddress)
{
    if (pendingLookups.find(interfaceAddress) == pendingLookups.cend()) {
        return;
    }
    std::vector<types::DiscoveryEntry> localCapabilities =
            searchCache({interfaceAddress}, std::chrono::milliseconds(-1), true);
    if (localCapabilities.empty()) {
        return;
    }
    std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
            util::convert(true, localCapabilities);

    for (const std::shared_ptr<ILocalCapabilitiesCallback>& callback :
         pendingLookups[interfaceAddress]) {
        callback->capabilitiesReceived(localCapabilitiesWithMetaInfo);
    }
    pendingLookups.erase(interfaceAddress);
}

void LocalCapabilitiesDirectory::registerPendingLookup(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback)
{
    for (const InterfaceAddress& address : interfaceAddresses) {
        pendingLookups[address].push_back(callback); // if no entry exists for key address, an
                                                     // empty list is automatically created
    }
}

bool LocalCapabilitiesDirectory::hasPendingLookups()
{
    return !pendingLookups.empty();
}

bool LocalCapabilitiesDirectory::isCallbackCalled(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback,
        const joynr::types::DiscoveryQos& discoveryQos)
{
    // only if discovery scope is joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL, the
    // callback can potentially already be called, as a matching capability has been added
    // to the local capabilities directory while waiting for capabilitiesclient->lookup result
    if (discoveryQos.getDiscoveryScope() != joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        return false;
    }
    for (const InterfaceAddress& address : interfaceAddresses) {
        if (pendingLookups.find(address) == pendingLookups.cend()) {
            return true;
        }
        if (std::find(pendingLookups[address].cbegin(), pendingLookups[address].cend(), callback) ==
            pendingLookups[address].cend()) {
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::callbackCalled(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback)
{
    for (const InterfaceAddress& address : interfaceAddresses) {
        if (pendingLookups.find(address) != pendingLookups.cend()) {
            std::vector<std::shared_ptr<ILocalCapabilitiesCallback>>& callbacks =
                    pendingLookups[address];
            util::removeAll(callbacks, callback);
            if (pendingLookups[address].empty()) {
                pendingLookups.erase(address);
            }
        }
    }
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::string& participantId)
{
    return optionalToVector(localCapabilities.lookupByParticipantId(participantId));
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    return searchCache(interfaceAddresses, std::chrono::milliseconds(-1), true);
}

void LocalCapabilitiesDirectory::clear()
{
    localCapabilities.clear();
    globalCapabilities.clear();
}

void LocalCapabilitiesDirectory::registerReceivedCapabilities(
        const std::unordered_multimap<std::string, types::DiscoveryEntry>&& capabilityEntries)
{
    for (auto it = capabilityEntries.cbegin(); it != capabilityEntries.cend(); ++it) {
        const std::string& serializedAddress = it->first;
        const types::DiscoveryEntry& currentEntry = it->second;
        std::shared_ptr<const system::RoutingTypes::Address> address;
        const bool isGloballyVisible = isGlobal(currentEntry);
        try {
            joynr::serializer::deserializeFromJson(address, serializedAddress);
            if (auto messageRouterSharedPtr = messageRouter.lock()) {
                constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
                const bool isSticky = false;
                messageRouterSharedPtr->addNextHop(currentEntry.getParticipantId(),
                                                   address,
                                                   isGloballyVisible,
                                                   expiryDateMs,
                                                   isSticky,
                                                   true);
            } else {
                JOYNR_LOG_FATAL(
                        logger(),
                        "could not addNextHop {} to {} because messageRouter is not available",
                        currentEntry.getParticipantId(),
                        serializedAddress);
            }
            this->insertInCache(currentEntry, false, true);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger(),
                            "could not deserialize Address from {} - error: {}",
                            serializedAddress,
                            e.what());
        }
    }
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (hasProviderPermission(discoveryEntry)) {
        addInternal(discoveryEntry);
        onSuccess();
        return;
    }
    onError(joynr::exceptions::ProviderRuntimeException(
            "Provider does not have permissions to register domain/interface."));
}

bool LocalCapabilitiesDirectory::hasProviderPermission(const types::DiscoveryEntry& discoveryEntry)
{
    if (!clusterControllerSettings.enableAccessController()) {
        return true;
    }

    if (auto gotAccessController = accessController.lock()) {
        const CallContext& callContext = CallContextStorage::get();
        const std::string& ownerId = callContext.getPrincipal();
        const bool result = gotAccessController->hasProviderPermission(
                ownerId,
                infrastructure::DacTypes::TrustLevel::HIGH,
                discoveryEntry.getDomain(),
                discoveryEntry.getInterfaceName());
        if (clusterControllerSettings.aclAudit()) {
            if (!result) {
                JOYNR_LOG_ERROR(logger(),
                                "ACL AUDIT: owner '{}' is not allowed to register "
                                "interface '{}' on domain '{}'",
                                ownerId,
                                discoveryEntry.getInterfaceName(),
                                discoveryEntry.getDomain());
            } else {
                JOYNR_LOG_DEBUG(logger(),
                                "ACL AUDIT: owner '{}' is allowed to register interface "
                                "'{}' on domain '{}'",
                                ownerId,
                                discoveryEntry.getInterfaceName(),
                                discoveryEntry.getDomain());
            }
            return true;
        }
        return result;
    }

    // return false in case AC ptr and setting do not match
    return false;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::optionalToVector(
        boost::optional<types::DiscoveryEntry> optionalEntry)
{
    std::vector<types::DiscoveryEntry> vec;
    if (optionalEntry) {
        vec.push_back(std::move(*optionalEntry));
    }
    return vec;
}

void LocalCapabilitiesDirectory::setAccessController(
        std::weak_ptr<IAccessController> accessController)
{
    this->accessController = std::move(accessController);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const types::DiscoveryQos& discoveryQos,
        std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>& result)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (domains.size() != 1) {
        onError(joynr::exceptions::ProviderRuntimeException(
                "LocalCapabilitiesDirectory does not yet support lookup on multiple domains."));
        return;
    }

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    lookup(domains, interfaceName, std::move(localCapabilitiesCallback), discoveryQos);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& participantId,
        std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto callback = [ onSuccess = std::move(onSuccess), onError, this, participantId ](
            const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities)
    {
        if (capabilities.size() == 0) {
            joynr::exceptions::ProviderRuntimeException exception(
                    "No capabilities found for participandId \"" + participantId + "\"");
            onError(exception);
            return;
        }
        if (capabilities.size() > 1) {
            JOYNR_LOG_ERROR(this->logger(),
                            "participantId {} has more than 1 capability entry:\n {}\n {}",
                            participantId,
                            capabilities[0].toString(),
                            capabilities[1].toString());
        }

        onSuccess(capabilities[0]);
    };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(callback), std::move(onError));
    lookup(participantId, std::move(localCapabilitiesCallback));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::remove(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    remove(participantId);
    onSuccess();
}

void LocalCapabilitiesDirectory::addProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    observers.push_back(std::move(observer));
}

void LocalCapabilitiesDirectory::removeProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    util::removeAll(observers, observer);
}

void LocalCapabilitiesDirectory::updatePersistedFile()
{
    saveLocalCapabilitiesToFile(
            clusterControllerSettings.getLocalCapabilitiesDirectoryPersistenceFilename());
}

void LocalCapabilitiesDirectory::saveLocalCapabilitiesToFile(const std::string& fileName)
{
    if (!isLocalCapabilitiesDirectoryPersistencyEnabled) {
        return;
    }

    if (fileName.empty()) {
        return;
    }

    try {
        joynr::util::saveStringToFile(
                fileName, joynr::serializer::serializeToJson(localCapabilities));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }
}

void LocalCapabilitiesDirectory::loadPersistedFile()
{
    if (!isLocalCapabilitiesDirectoryPersistencyEnabled) {
        return;
    }

    const std::string persistencyFile =
            clusterControllerSettings.getLocalCapabilitiesDirectoryPersistenceFilename();

    if (persistencyFile.empty()) { // Persistency disabled
        return;
    }

    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(persistencyFile);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger(), ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    try {
        joynr::serializer::deserializeFromJson(localCapabilities, jsonString);
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }

    // insert all global capability entries into global cache
    for (const auto& entry : localCapabilities) {
        if (entry.getQos().getScope() == types::ProviderScope::GLOBAL) {
            globalCapabilities.insert(entry);
        }
    }
}

void LocalCapabilitiesDirectory::injectGlobalCapabilitiesFromFile(const std::string& fileName)
{
    if (fileName.empty()) {
        JOYNR_LOG_WARN(
                logger(), "Empty file name provided in input: cannot load global capabilities.");
        return;
    }

    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(fileName);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    std::vector<joynr::types::GlobalDiscoveryEntry> injectedGlobalCapabilities;
    try {
        joynr::serializer::deserializeFromJson(injectedGlobalCapabilities, jsonString);
    } catch (const std::invalid_argument& e) {
        std::string errorMessage("could not deserialize injected global capabilities from " +
                                 jsonString + " - error: " + e.what());
        JOYNR_LOG_FATAL(logger(), errorMessage);
        return;
    }

    if (injectedGlobalCapabilities.empty()) {
        return;
    }

    std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
    for (const auto& globalDiscoveryEntry : injectedGlobalCapabilities) {
        // insert in map for messagerouter
        capabilitiesMap.insert(
                {globalDiscoveryEntry.getAddress(), std::move(globalDiscoveryEntry)});
    }

    // insert found capabilities in messageRouter
    registerReceivedCapabilities(std::move(capabilitiesMap));
}

/**
 * Private convenience methods.
 */
void LocalCapabilitiesDirectory::insertInCache(const types::DiscoveryEntry& entry,
                                               bool localCache,
                                               bool globalCache)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    // add entry to local cache
    if (localCache) {
        localCapabilities.insert(entry);
    }

    // add entry to global cache
    if (globalCache) {
        globalCapabilities.insert(entry);
    }
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchCache(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        std::chrono::milliseconds maxCacheAge,
        bool localEntries)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    std::vector<types::DiscoveryEntry> result;
    for (std::size_t i = 0; i < interfaceAddresses.size(); i++) {
        const InterfaceAddress& interfaceAddress = interfaceAddresses.at(i);
        const std::string& domain = interfaceAddress.getDomain();
        const std::string& interface = interfaceAddress.getInterface();

        std::vector<types::DiscoveryEntry> entries =
                localEntries ? localCapabilities.lookupByDomainAndInterface(domain, interface)
                             : globalCapabilities.lookupCacheByDomainAndInterface(
                                       domain, interface, maxCacheAge);
        result.insert(result.end(),
                      std::make_move_iterator(entries.begin()),
                      std::make_move_iterator(entries.end()));
    }
    return result;
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchCache(
        const std::string& participantId,
        std::chrono::milliseconds maxCacheAge,
        bool localEntries)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    // search in local
    if (localEntries) {
        return localCapabilities.lookupByParticipantId(participantId);
    } else {
        if (maxCacheAge == std::chrono::milliseconds(-1)) {
            return globalCapabilities.lookupByParticipantId(participantId);
        }
        return globalCapabilities.lookupCacheByParticipantId(participantId, maxCacheAge);
    }
}

void LocalCapabilitiesDirectory::informObserversOnAdd(const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : observers) {
        observer->onProviderAdd(discoveryEntry);
    }
}

void LocalCapabilitiesDirectory::informObserversOnRemove(
        const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : observers) {
        observer->onProviderRemove(discoveryEntry);
    }
}

bool LocalCapabilitiesDirectory::isGlobal(const types::DiscoveryEntry& discoveryEntry) const
{
    return discoveryEntry.getQos().getScope() == types::ProviderScope::GLOBAL;
}

void LocalCapabilitiesDirectory::scheduleCleanupTimer()
{
    boost::system::error_code timerError;
    auto intervalMs = clusterControllerSettings.getPurgeExpiredDiscoveryEntriesIntervalMs();
    checkExpiredDiscoveryEntriesTimer.expires_from_now(
            std::chrono::milliseconds(intervalMs), timerError);
    if (timerError) {
        JOYNR_LOG_FATAL(logger(),
                        "Error scheduling discovery entries check. {}: {}",
                        timerError.value(),
                        timerError.message());
    } else {
        checkExpiredDiscoveryEntriesTimer.async_wait(
                std::bind(&LocalCapabilitiesDirectory::checkExpiredDiscoveryEntries,
                          this,
                          std::placeholders::_1));
    }
}

void LocalCapabilitiesDirectory::checkExpiredDiscoveryEntries(
        const boost::system::error_code& errorCode)
{
    if (errorCode == boost::asio::error::operation_aborted) {
        // Assume Destructor has been called
        JOYNR_LOG_DEBUG(logger(),
                        "expired discovery entries check aborted after shutdown, error code from "
                        "expired discovery entries timer: {}",
                        errorCode.message());
        return;
    } else if (errorCode) {
        JOYNR_LOG_ERROR(logger(),
                        "Error triggering expired discovery entries check, error code: {}",
                        errorCode.message());
    }

    std::lock_guard<std::mutex> lock(cacheLock);

    auto removedLocalCapabilities = localCapabilities.removeExpired();
    auto removedGlobalCapabilities = globalCapabilities.removeExpired();

    if (!removedLocalCapabilities.empty()) {
        JOYNR_LOG_DEBUG(logger(),
                        "Following local discovery entries expired: {}",
                        joinToString(removedLocalCapabilities));
    }

    if (!removedGlobalCapabilities.empty()) {
        JOYNR_LOG_DEBUG(logger(),
                        "Following global discovery entries expired: {}",
                        joinToString(removedGlobalCapabilities));
    }

    if (!removedLocalCapabilities.empty() || !removedGlobalCapabilities.empty()) {
        updatePersistedFile();
    }

    scheduleCleanupTimer();
}

std::string LocalCapabilitiesDirectory::joinToString(
        const std::vector<types::DiscoveryEntry>& discoveryEntries) const
{
    std::ostringstream outputStream;

    std::transform(
            discoveryEntries.cbegin(),
            discoveryEntries.cend(),
            std::ostream_iterator<std::string>(outputStream, ", "),
            [](const types::DiscoveryEntry& discoveryEntry) { return discoveryEntry.toString(); });

    return outputStream.str();
}

LocalCapabilitiesCallback::LocalCapabilitiesCallback(
        std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>&& onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)>&& onError)
        : onSuccess(std::move(onSuccess)), onErrorCallback(std::move(onError))
{
}

void LocalCapabilitiesCallback::onError(const exceptions::JoynrRuntimeException& error)
{
    onErrorCallback(joynr::exceptions::ProviderRuntimeException(
            "Unable to collect capabilities from global capabilities directory. Error: " +
            error.getMessage()));
}

void LocalCapabilitiesCallback::capabilitiesReceived(
        const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities)
{
    onSuccess(capabilities);
}

} // namespace joynr

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <algorithm>
#include <unordered_set>

#include "joynr/LocalCapabilitiesDirectory.h"

#include <boost/algorithm/string/predicate.hpp>

#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"

#include "common/InterfaceAddress.h"

#include "joynr/DiscoveryQos.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/infrastructure/IGlobalCapabilitiesDirectory.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/MessageRouter.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/Util.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

INIT_LOGGER(LocalCapabilitiesDirectory);

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(
        MessagingSettings& messagingSettings,
        std::shared_ptr<ICapabilitiesClient> capabilitiesClientPtr,
        const std::string& localAddress,
        MessageRouter& messageRouter,
        LibjoynrSettings& libjoynrSettings)
        : joynr::system::DiscoveryAbstractProvider(),
          messagingSettings(messagingSettings),
          capabilitiesClient(capabilitiesClientPtr),
          localAddress(localAddress),
          cacheLock(),
          pendingLookupsLock(),
          interfaceAddress2GlobalCapabilities(),
          participantId2GlobalCapabilities(),
          interfaceAddress2LocalCapabilities(),
          participantId2LocalCapability(),
          registeredGlobalCapabilities(),
          messageRouter(messageRouter),
          observers(),
          mqttSettings(),
          libJoynrSettings(libjoynrSettings),
          pendingLookups(),
          thread(),
          ioService(),
          ioServiceWork(std::make_unique<boost::asio::io_service::work>(ioService)),
          checkExpiredDiscoveryEntriesTimer(ioService)
{
    // setting up the provisioned values for GlobalCapabilitiesClient
    // The GlobalCapabilitiesServer is also provisioned in MessageRouter
    types::ProviderQos providerQos;
    providerQos.setPriority(1);
    std::int64_t lastSeenDateMs = 0;
    std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    std::string defaultPublicKeyId("");
    types::Version providerVersion(infrastructure::IGlobalCapabilitiesDirectory::MAJOR_VERSION,
                                   infrastructure::IGlobalCapabilitiesDirectory::MINOR_VERSION);
    this->insertInCache(
            types::DiscoveryEntry(providerVersion,
                                  messagingSettings.getDiscoveryDirectoriesDomain(),
                                  infrastructure::IGlobalCapabilitiesDirectory::INTERFACE_NAME(),
                                  messagingSettings.getCapabilitiesDirectoryParticipantId(),
                                  providerQos,
                                  lastSeenDateMs,
                                  expiryDateMs,
                                  defaultPublicKeyId),
            true,
            false);

    thread = std::thread([this]() { this->ioService.run(); });
    scheduleCleanupTimer();
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    checkExpiredDiscoveryEntriesTimer.cancel();
    ioServiceWork.reset();
    ioService.stop();
    if (thread.joinable()) {
        thread.join();
    }
    cleanCaches();
}

void LocalCapabilitiesDirectory::cleanCaches()
{
    const auto zero = std::chrono::milliseconds::zero();
    cleanCache(zero);
}

void LocalCapabilitiesDirectory::add(const types::DiscoveryEntry& discoveryEntry)
{
    bool isGlobal = discoveryEntry.getQos().getScope() == types::ProviderScope::GLOBAL;

    // register locally
    this->insertInCache(discoveryEntry, true, isGlobal);

    // Inform observers
    informObserversOnAdd(discoveryEntry);

    // register globally
    if (isGlobal) {
        types::GlobalDiscoveryEntry globalDiscoveryEntry(discoveryEntry.getProviderVersion(),
                                                         discoveryEntry.getDomain(),
                                                         discoveryEntry.getInterfaceName(),
                                                         discoveryEntry.getParticipantId(),
                                                         discoveryEntry.getQos(),
                                                         discoveryEntry.getLastSeenDateMs(),
                                                         discoveryEntry.getExpiryDateMs(),
                                                         discoveryEntry.getPublicKeyId(),
                                                         localAddress);
        if (std::find(registeredGlobalCapabilities.begin(),
                      registeredGlobalCapabilities.end(),
                      globalDiscoveryEntry) == registeredGlobalCapabilities.end()) {
            registeredGlobalCapabilities.push_back(globalDiscoveryEntry);
        }
        this->capabilitiesClient->add(registeredGlobalCapabilities);
    }

    updatePersistedFile();
    {
        std::lock_guard<std::mutex> lock(pendingLookupsLock);
        callPendingLookups(
                InterfaceAddress(discoveryEntry.getDomain(), discoveryEntry.getInterfaceName()));
    }
}

void LocalCapabilitiesDirectory::remove(const std::string& domain,
                                        const std::string& interfaceName,
                                        const types::ProviderQos& qos)
{
    // TODO does it make sense to remove any capability for a domain/interfaceName
    // without knowing which provider registered the capability
    std::vector<types::DiscoveryEntry> entries = searchCache(
            {InterfaceAddress(domain, interfaceName)}, std::chrono::milliseconds(-1), false);

    // first, update cache
    {
        std::lock_guard<std::mutex> lock(cacheLock);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const types::DiscoveryEntry& entry = entries.at(i);
            if (isGlobal(entry)) {
                auto compareFunc = [&entry, &domain, &interfaceName, &qos](
                        const types::GlobalDiscoveryEntry& it) {
                    return it.getProviderVersion() == entry.getProviderVersion() &&
                           it.getDomain() == domain && it.getInterfaceName() == interfaceName &&
                           it.getQos() == qos &&
                           it.getParticipantId() == entry.getParticipantId() &&
                           it.getPublicKeyId() == entry.getPublicKeyId();
                };

                while (registeredGlobalCapabilities.erase(
                               std::remove_if(registeredGlobalCapabilities.begin(),
                                              registeredGlobalCapabilities.end(),
                                              compareFunc),
                               registeredGlobalCapabilities.end()) !=
                       registeredGlobalCapabilities.end()) {
                }
                participantId2GlobalCapabilities.remove(entry.getParticipantId(), entry);
                interfaceAddress2GlobalCapabilities.remove(
                        InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
            }
            participantId2LocalCapability.remove(entry.getParticipantId(), entry);
            interfaceAddress2LocalCapabilities.remove(
                    InterfaceAddress(domain, interfaceName), entry);
        }
    }

    // second, do final cleanup and observer call
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const types::DiscoveryEntry& entry = entries.at(i);
        if (isGlobal(entry)) {
            capabilitiesClient->remove(entry.getParticipantId());
        }
        informObserversOnRemove(entry);
    }

    updatePersistedFile();
}

void LocalCapabilitiesDirectory::remove(const std::string& participantId)
{

    std::vector<types::DiscoveryEntry> entries =
            searchCache(participantId, std::chrono::milliseconds(-1), true);
    remove(entries);
}

void LocalCapabilitiesDirectory::remove(const std::vector<types::DiscoveryEntry>& discoveryEntries)
{
    // first, update cache
    {
        for (std::size_t i = 0; i < discoveryEntries.size(); ++i) {
            const types::DiscoveryEntry& entry = discoveryEntries.at(i);
            std::lock_guard<std::mutex> lock(cacheLock);
            participantId2LocalCapability.remove(entry.getParticipantId(), entry);
            interfaceAddress2LocalCapabilities.remove(
                    InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
            if (isGlobal(entry)) {
                participantId2GlobalCapabilities.remove(entry.getParticipantId(), entry);
                interfaceAddress2GlobalCapabilities.remove(
                        InterfaceAddress(entry.getDomain(), entry.getInterfaceName()), entry);
            }
        }
    }

    // second, do final cleanup and observer call
    for (std::size_t i = 0; i < discoveryEntries.size(); ++i) {
        const types::DiscoveryEntry& entry = discoveryEntries.at(i);
        if (isGlobal(entry)) {
            capabilitiesClient->remove(entry.getParticipantId());
        }
        informObserversOnRemove(entry);
    }

    updatePersistedFile();
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

    std::vector<types::DiscoveryEntry> localCapabilities =
            searchCache(participantId, std::chrono::milliseconds(-1), true);
    std::vector<types::DiscoveryEntry> globalCapabilities = searchCache(
            participantId, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()), false);

    return callReceiverIfPossible(scope,
                                  std::move(localCapabilities),
                                  std::move(globalCapabilities),
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
        callback->capabilitiesReceived(std::move(localCapabilities));
        return true;
    }

    // return local then global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        if (!localCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(localCapabilities));
            return true;
        }
        if (!globalCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(globalCapabilities));
            return true;
        }
    }

    // return local and global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        // return if global entries
        if (!globalCapabilities.empty()) {
            // remove duplicates
            std::unordered_set<types::DiscoveryEntry> resultSet(
                    std::make_move_iterator(localCapabilities.begin()),
                    std::make_move_iterator(localCapabilities.end()));
            resultSet.insert(std::make_move_iterator(globalCapabilities.begin()),
                             std::make_move_iterator(globalCapabilities.end()));
            std::vector<types::DiscoveryEntry> resultVec(resultSet.begin(), resultSet.end());
            callback->capabilitiesReceived(std::move(resultVec));
            return true;
        }
    }

    // return the global cached entries
    if (scope == joynr::types::DiscoveryScope::GLOBAL_ONLY) {
        if (!globalCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(globalCapabilities));
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::capabilitiesReceived(
        const std::vector<types::GlobalDiscoveryEntry>& results,
        std::vector<types::DiscoveryEntry>&& cachedLocalCapabilies,
        std::shared_ptr<ILocalCapabilitiesCallback> callback,
        joynr::types::DiscoveryScope::Enum discoveryScope)
{
    std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
    std::vector<types::DiscoveryEntry> mergedEntries;

    for (types::GlobalDiscoveryEntry globalDiscoveryEntry : results) {
        capabilitiesMap.insert({globalDiscoveryEntry.getAddress(), globalDiscoveryEntry});
        mergedEntries.push_back(std::move(globalDiscoveryEntry));
    }
    registerReceivedCapabilities(std::move(capabilitiesMap));

    if (discoveryScope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryScope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        // look if in the meantime there are some local providers registered
        // lookup in the local directory to get local providers which were registered in the
        // meantime.
        mergedEntries.insert(mergedEntries.end(),
                             std::make_move_iterator(cachedLocalCapabilies.begin()),
                             std::make_move_iterator(cachedLocalCapabilies.end()));
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
        this->capabilitiesClient->lookup(
                participantId,
                onSuccess,
                std::bind(&ILocalCapabilitiesCallback::onError, callback, std::placeholders::_1));
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
        this->capabilitiesClient->lookup(
                domains, interfaceName, discoveryQos.getDiscoveryTimeout(), onSuccess, onError);
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
    for (const std::shared_ptr<ILocalCapabilitiesCallback>& callback :
         pendingLookups[interfaceAddress]) {
        callback->capabilitiesReceived(localCapabilities);
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
    return searchCache(participantId, std::chrono::milliseconds(-1), true);
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    return searchCache(interfaceAddresses, std::chrono::milliseconds(-1), true);
}

void LocalCapabilitiesDirectory::cleanCache(std::chrono::milliseconds maxAge)
{
    std::lock_guard<std::mutex> lock(cacheLock);
    interfaceAddress2GlobalCapabilities.cleanup(maxAge);
    participantId2GlobalCapabilities.cleanup(maxAge);
    interfaceAddress2LocalCapabilities.cleanup(maxAge);
    participantId2LocalCapability.cleanup(maxAge);
}

void LocalCapabilitiesDirectory::registerReceivedCapabilities(
        const std::unordered_multimap<std::string, types::DiscoveryEntry>&& capabilityEntries)
{
    for (auto it = capabilityEntries.cbegin(); it != capabilityEntries.cend(); ++it) {
        const std::string& serializedAddress = it->first;
        const types::DiscoveryEntry& currentEntry = it->second;
        // TODO: check joynrAddress for nullptr instead of string.find after the deserialization
        // works as expected.
        // Currently, JsonDeserializer.deserialize<T> always returns an instance of T
        std::size_t foundTypeNameKey = serializedAddress.find("\"_typeName\"");
        std::size_t foundTypeNameValue =
                serializedAddress.find("\"joynr.system.RoutingTypes.MqttAddress\"");
        if (boost::starts_with(serializedAddress, "{") && foundTypeNameKey != std::string::npos &&
            foundTypeNameValue != std::string::npos && foundTypeNameKey < foundTypeNameValue) {
            try {
                using system::RoutingTypes::MqttAddress;
                MqttAddress joynrAddress;
                joynr::serializer::deserializeFromJson(joynrAddress, serializedAddress);
                auto addressPtr = std::make_shared<MqttAddress>(joynrAddress);
                messageRouter.addNextHop(currentEntry.getParticipantId(), addressPtr);
            } catch (const std::invalid_argument& e) {
                JOYNR_LOG_FATAL(logger,
                                "could not deserialize MqttAddress from {} - error: {}",
                                serializedAddress,
                                e.what());
            }
        } else {
            try {
                using system::RoutingTypes::ChannelAddress;

                ChannelAddress channelAddress;
                joynr::serializer::deserializeFromJson(channelAddress, serializedAddress);
                auto channelAddressPtr = std::make_shared<const ChannelAddress>(channelAddress);

                messageRouter.addNextHop(currentEntry.getParticipantId(), channelAddressPtr);
            } catch (const std::invalid_argument& e) {
                JOYNR_LOG_FATAL(logger,
                                "could not deserialize ChannelAddress from {} - error: {}",
                                serializedAddress,
                                e.what());
            }
        }
        this->insertInCache(currentEntry, false, true);
    }
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    add(discoveryEntry);
    onSuccess();
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const types::DiscoveryQos& discoveryQos,
        std::function<void(const std::vector<types::DiscoveryEntry>& result)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (domains.size() != 1) {
        onError(joynr::exceptions::ProviderRuntimeException(
                "LocalCapabilitiesDirectory does not yet support lookup on multiple domains."));
        return;
    }

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(onSuccess, onError);

    lookup(domains, interfaceName, localCapabilitiesCallback, discoveryQos);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& participantId,
        std::function<void(const types::DiscoveryEntry&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto callback = [onSuccess, onError, this, participantId](
            const std::vector<types::DiscoveryEntry>& capabilities) {
        if (capabilities.size() == 0) {
            joynr::exceptions::ProviderRuntimeException exception(
                    "No capabilities found for participandId \"" + participantId + "\"");
            onError(exception);
            return;
        }
        if (capabilities.size() > 1) {
            JOYNR_LOG_ERROR(this->logger,
                            "participantId {} has more than 1 capability entry:\n {}\n {}",
                            participantId,
                            capabilities[0].toString(),
                            capabilities[1].toString());
        }

        onSuccess(capabilities[0]);
    };

    auto localCapabilitiesCallback = std::make_shared<LocalCapabilitiesCallback>(callback, onError);
    lookup(participantId, localCapabilitiesCallback);
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
    observers.push_back(observer);
}

void LocalCapabilitiesDirectory::removeProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    util::removeAll(observers, observer);
}

void LocalCapabilitiesDirectory::updatePersistedFile()
{
    saveLocalCapabilitiesToFile(
            libJoynrSettings.getLocalCapabilitiesDirectoryPersistenceFilename());
}

void LocalCapabilitiesDirectory::saveLocalCapabilitiesToFile(const std::string& fileName)
{
    try {
        joynr::util::saveStringToFile(fileName, serializeLocalCapabilitiesToJson());
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }
}

std::string LocalCapabilitiesDirectory::serializeLocalCapabilitiesToJson() const
{
    const std::vector<std::string>& capEntriesKeys = participantId2LocalCapability.getKeys();

    // put all entries in a vector to be serialized
    std::vector<types::DiscoveryEntry> toBeSerialized;

    // convert each entry with Joynr::serialize
    for (const auto& key : capEntriesKeys) {
        auto capEntriesAtKey = participantId2LocalCapability.lookUpAll(key);
        toBeSerialized.insert(toBeSerialized.end(),
                              std::make_move_iterator(capEntriesAtKey.begin()),
                              std::make_move_iterator(capEntriesAtKey.end()));
    }

    return joynr::serializer::serializeToJson(toBeSerialized);
}

void LocalCapabilitiesDirectory::loadPersistedFile()
{
    const std::string persistencyFile =
            libJoynrSettings.getLocalCapabilitiesDirectoryPersistenceFilename();
    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(persistencyFile);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger, ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    std::vector<types::DiscoveryEntry> persistedCapabilities;
    try {
        joynr::serializer::deserializeFromJson(persistedCapabilities, jsonString);
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }

    if (persistedCapabilities.empty()) {
        return;
    }

    // move all capability entries into local cache and avoid duplicates
    for (const auto& entry : persistedCapabilities) {
        insertInCache(entry, true, isGlobal(entry));
    }
}

void LocalCapabilitiesDirectory::injectGlobalCapabilitiesFromFile(const std::string& fileName)
{
    if (fileName.empty()) {
        JOYNR_LOG_WARN(
                logger, "Empty file name provided in input: cannot load global capabilities.");
        return;
    }

    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(fileName);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    std::vector<joynr::types::GlobalDiscoveryEntry> injectedGlobalCapabilities;
    joynr::serializer::deserializeFromJson(injectedGlobalCapabilities, jsonString);

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
    bool insertLocal = localCache && !hasEntryInCache(entry, localCache);
    std::lock_guard<std::mutex> lock(cacheLock);
    InterfaceAddress addr(entry.getDomain(), entry.getInterfaceName());

    // add entry to local cache
    if (insertLocal) {
        interfaceAddress2LocalCapabilities.insert(addr, entry);
        participantId2LocalCapability.insert(entry.getParticipantId(), entry);
    }

    // add entry to global cache
    if (globalCache) {
        interfaceAddress2GlobalCapabilities.insert(addr, entry);
        participantId2GlobalCapabilities.insert(entry.getParticipantId(), entry);
    }
}

bool LocalCapabilitiesDirectory::hasEntryInCache(const types::DiscoveryEntry& entry,
                                                 bool localEntries)
{
    // the combination participantId is unique for [domain, interfaceName, authtoken]
    std::vector<types::DiscoveryEntry> entryList =
            searchCache(entry.getParticipantId(), std::chrono::milliseconds(-1), localEntries);

    bool found = std::find(entryList.cbegin(), entryList.cend(), entry) != entryList.cend();
    return found;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchCache(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        std::chrono::milliseconds maxCacheAge,
        bool localEntries)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    std::vector<types::DiscoveryEntry> result;
    for (std::size_t i = 0; i < interfaceAddresses.size(); i++) {
        // search in local
        std::vector<types::DiscoveryEntry> entries =
                localEntries
                        ? interfaceAddress2LocalCapabilities.lookUpAll(interfaceAddresses.at(i))
                        : interfaceAddress2GlobalCapabilities.lookUp(
                                  interfaceAddresses.at(i), maxCacheAge);
        result.insert(result.end(),
                      std::make_move_iterator(entries.begin()),
                      std::make_move_iterator(entries.end()));
    }
    return result;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchCache(
        const std::string& participantId,
        std::chrono::milliseconds maxCacheAge,
        bool localEntries)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    // search in local
    if (localEntries) {
        return participantId2LocalCapability.lookUpAll(participantId);
    } else {
        return participantId2GlobalCapabilities.lookUp(participantId, maxCacheAge);
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
    bool isGlobal = discoveryEntry.getQos().getScope() == types::ProviderScope::GLOBAL;
    return isGlobal;
}

void LocalCapabilitiesDirectory::scheduleCleanupTimer()
{
    boost::system::error_code timerError;
    auto intervalMs = messagingSettings.getPurgeExpiredDiscoveryEntriesIntervalMs();
    checkExpiredDiscoveryEntriesTimer.expires_from_now(
            std::chrono::milliseconds(intervalMs), timerError);
    if (timerError) {
        JOYNR_LOG_FATAL(logger,
                        "Error schduling discovery entries check. {}: {}",
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
    if (!errorCode) {
        std::vector<types::DiscoveryEntry> expiredEntries;
        TypedClientMultiCache<std::string, types::DiscoveryEntry>* caches[] = {
                &participantId2LocalCapability, &participantId2GlobalCapabilities};
        auto now = TypeUtil::toMilliseconds(std::chrono::system_clock::now());
        for (auto& cache : caches) {
            for (auto& key : cache->getKeys()) {
                for (auto& discoveryEntry : participantId2LocalCapability.lookUpAll(key)) {
                    if ((std::uint64_t)discoveryEntry.getExpiryDateMs() < now) {
                        expiredEntries.push_back(discoveryEntry);
                    }
                }
            }
        }
        if (expiredEntries.size() > 0) {
            remove(expiredEntries);
        }
    } else {
        JOYNR_LOG_ERROR(logger, "Error triggering expired discovery entries check.");
    }
    scheduleCleanupTimer();
}

LocalCapabilitiesCallback::LocalCapabilitiesCallback(
        std::function<void(const std::vector<types::DiscoveryEntry>&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
        : onSuccess(onSuccess), onErrorCallback(onError)
{
}

void LocalCapabilitiesCallback::onError(const exceptions::JoynrRuntimeException& error)
{
    if (onErrorCallback) {
        onErrorCallback(joynr::exceptions::ProviderRuntimeException(
                "Unable to collect capabilities from global capabilities directory. Error: " +
                error.getMessage()));
    }
}

void LocalCapabilitiesCallback::capabilitiesReceived(
        const std::vector<types::DiscoveryEntry>& capabilities)
{
    if (onSuccess) {
        onSuccess(capabilities);
    }
}

} // namespace joynr

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
#include <cassert>
#include <chrono>
#include <limits>
#include <tuple>
#include <unordered_set>
#include <ostream>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/range/join.hpp>
#include <spdlog/fmt/fmt.h>

#include "joynr/access-control/IAccessController.h"

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/CapabilitiesStorage.h"
#include "joynr/CapabilityUtils.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/IMessageRouter.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderScope.h"

#include "IGlobalCapabilitiesDirectoryClient.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"

namespace joynr
{

struct DiscoveryEntryHash
{
    std::size_t operator()(const types::DiscoveryEntry& entry) const
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, entry.getParticipantId());
        return seed;
    }
};

struct DiscoveryEntryKeyEq
{
    bool operator()(const types::DiscoveryEntry& lhs, const types::DiscoveryEntry& rhs) const
    {
        // there is no need to check typeid because entries are of the same type.
        return joynr::util::compareValues(lhs.getParticipantId(), rhs.getParticipantId());
    }
};

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(
        ClusterControllerSettings& clusterControllerSettings,
        std::shared_ptr<IGlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient,
        std::shared_ptr<capabilities::Storage> locallyRegisteredCapabilities,
        std::shared_ptr<capabilities::CachingStorage> globalLookupCache,
        const std::string& localAddress,
        std::weak_ptr<IMessageRouter> messageRouter,
        boost::asio::io_service& ioService,
        const std::string clusterControllerId,
        std::vector<std::string> knownGbids,
        std::int64_t defaultExpiryIntervalMs)
        : joynr::system::DiscoveryAbstractProvider(),
          joynr::system::ProviderReregistrationControllerProvider(),
          std::enable_shared_from_this<LocalCapabilitiesDirectory>(),
          _clusterControllerSettings(clusterControllerSettings),
          _globalCapabilitiesDirectoryClient(std::move(globalCapabilitiesDirectoryClient)),
          _locallyRegisteredCapabilities(locallyRegisteredCapabilities),
          _globalLookupCache(globalLookupCache),
          _localAddress(localAddress),
          _cacheLock(),
          _pendingLookupsLock(),
          _messageRouter(messageRouter),
          _observers(),
          _pendingLookups(),
          _accessController(),
          _checkExpiredDiscoveryEntriesTimer(ioService),
          _isLocalCapabilitiesDirectoryPersistencyEnabled(
                  clusterControllerSettings.isLocalCapabilitiesDirectoryPersistencyEnabled()),
          _freshnessUpdateTimer(ioService),
          _clusterControllerId(clusterControllerId),
          _knownGbids(knownGbids),
          _knownGbidsSet(knownGbids.cbegin(), knownGbids.cend()),
          _defaultExpiryIntervalMs(defaultExpiryIntervalMs)
{
}

void LocalCapabilitiesDirectory::init()
{
    scheduleCleanupTimer();
    scheduleFreshnessUpdate();
}

void LocalCapabilitiesDirectory::shutdown()
{
    _checkExpiredDiscoveryEntriesTimer.cancel();
    _freshnessUpdateTimer.cancel();
}

void LocalCapabilitiesDirectory::scheduleFreshnessUpdate()
{
    boost::system::error_code timerError = boost::system::error_code();
    _freshnessUpdateTimer.expires_from_now(
            _clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs(), timerError);
    if (timerError) {
        JOYNR_LOG_ERROR(logger(),
                        "Error from freshness update timer: {}: {}",
                        timerError.value(),
                        timerError.message());
    }
    _freshnessUpdateTimer.async_wait([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
            const boost::system::error_code& localTimerError) {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            thisSharedPtr->sendAndRescheduleFreshnessUpdate(localTimerError);
        }
    });
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

    std::vector<std::string> participantIds;
    std::vector<capabilities::LocalDiscoveryEntry> entries;

    const std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch()).count();
    const std::int64_t newExpiryDateMs = now + _defaultExpiryIntervalMs;
    {
        std::lock_guard<std::recursive_mutex> lock14(_cacheLock);
        for (auto entry : *_locallyRegisteredCapabilities) {
            if (entry.getQos().getScope() == types::ProviderScope::GLOBAL) {
                entry.setLastSeenDateMs(now);
                entry.setExpiryDateMs(newExpiryDateMs);
                participantIds.push_back(entry.getParticipantId());
                entries.push_back(entry);
            }
        }
        for (const auto& entry : entries) {
            _locallyRegisteredCapabilities->insert(entry, entry.gbids);
            _globalLookupCache->insert(entry);
        }
    }

    auto onSuccess = [ ccId = _clusterControllerId, participantIds ]()
    {
        if (logger().getLogLevel() == LogLevel::Trace) {
            const std::string participantIdConcat = boost::algorithm::join(participantIds, ", ");
            JOYNR_LOG_TRACE(logger(),
                            "touch(ccId={}, participantIds={}) succeeded.",
                            ccId,
                            participantIdConcat);
        } else {
            JOYNR_LOG_DEBUG(logger(), "touch succeeded.");
        }
    };

    auto onError = [ ccId = _clusterControllerId, participantIds ](
            const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_ERROR(logger(),
                        "touch(ccId={}, participantIds={}) failed: {}",
                        ccId,
                        boost::algorithm::join(participantIds, ", "),
                        error.getMessage());
    };

    _globalCapabilitiesDirectoryClient->touch(
            _clusterControllerId, participantIds, std::move(onSuccess), std::move(onError));
    scheduleFreshnessUpdate();
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    _freshnessUpdateTimer.cancel();
    _checkExpiredDiscoveryEntriesTimer.cancel();
    clear();
}

LocalCapabilitiesDirectory::ValidateGBIDsEnum::Enum LocalCapabilitiesDirectory::validateGbids(
        std::vector<std::string> gbids,
        std::unordered_set<std::string> validGbids)
{
    std::unordered_set<std::string> gbidSet;
    for (auto gbid : gbids) {
        if (gbid.empty() || (gbidSet.find(gbid) != gbidSet.cend())) {
            JOYNR_LOG_ERROR(
                    logger(), "INVALID_GBID: provided GBID is empty or duplicate: >{}<.", gbid);
            return ValidateGBIDsEnum::INVALID;
        }
        gbidSet.insert(gbid);
        if (validGbids.find(gbid) == validGbids.cend()) {
            JOYNR_LOG_ERROR(logger(), "UNKNOWN_GBID: provided GBID is unknown: >{}<.", gbid);
            return ValidateGBIDsEnum::UNKNOWN;
        }
    }
    return ValidateGBIDsEnum::OK;
}

void LocalCapabilitiesDirectory::addInternal(
        const types::DiscoveryEntry& discoveryEntry,
        bool awaitGlobalRegistration,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const types::DiscoveryError::Enum&)> onError)
{
    const bool isGloballyVisible = isGlobal(discoveryEntry);

    if (!isGloballyVisible || !awaitGlobalRegistration) {
        std::lock_guard<std::recursive_mutex> lock1(_cacheLock);
        if (isGloballyVisible) {
            insertInGlobalLookupCache(discoveryEntry, gbids);
        }
        // register locally
        insertInLocalCapabilitiesStorage(discoveryEntry);
        // Inform observers
        informObserversOnAdd(discoveryEntry);

        updatePersistedFile();
        {
            std::lock_guard<std::mutex> lock(_pendingLookupsLock);
            callPendingLookups(InterfaceAddress(
                    discoveryEntry.getDomain(), discoveryEntry.getInterfaceName()));
        }
    }

    // register globally
    if (isGloballyVisible) {
        types::GlobalDiscoveryEntry globalDiscoveryEntry = toGlobalDiscoveryEntry(discoveryEntry);

        auto onRuntimeError = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            globalDiscoveryEntry,
            gbids,
            awaitGlobalRegistration,
            onError
        ](const exceptions::JoynrRuntimeException& error)
        {
            JOYNR_LOG_ERROR(logger(),
                            "Exception occurred during the execution of capabilitiesProxy->add for "
                            "'{}' for GBIDs >{}<. Error: {} ({})",
                            globalDiscoveryEntry.toString(),
                            boost::algorithm::join(gbids, ", "),
                            error.getMessage(),
                            error.getTypeName());
            if (awaitGlobalRegistration && onError) {
                // no need to remove entry as in this case the entry was not yet added
                onError(types::DiscoveryError::INTERNAL_ERROR);
            }
            // in case awaitGlobalRegistration == false, the provider discovery
            // entry will not be deleted, so the provider continues to be available
            // locally, even if this makes little sense except for testing purposes.
            // It will never be informed about the failure to be registered globally
            // since it already got a reply after the local registration succeeded.
        };

        auto onErrorWrapper = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            globalDiscoveryEntry,
            gbids,
            awaitGlobalRegistration,
            onError
        ](const types::DiscoveryError::Enum& error)
        {
            JOYNR_LOG_ERROR(logger(),
                            "DiscoveryError occurred during the execution of "
                            "capabilitiesProxy->add for "
                            "'{}' for GBIDs >{}<. Error: {}",
                            globalDiscoveryEntry.toString(),
                            boost::algorithm::join(gbids, ", "),
                            types::DiscoveryError::getLiteral(error));
            if (awaitGlobalRegistration && onError) {
                // no need to remove entry as in this case the entry was not yet added
                onError(error);
            }
            // in case awaitGlobalRegistration == false, the provider discovery
            // entry will not be deleted, so the provider continues to be available
            // locally, even if this makes little sense except for testing purposes.
            // It will never be informed about the failure to be registered globally
            // since it already got a reply after the local registration succeeded.
        };

        std::function<void()> onSuccessWrapper = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            globalDiscoveryEntry,
            awaitGlobalRegistration,
            gbids,
            onSuccess
        ]()
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::recursive_mutex> lock2(thisSharedPtr->_cacheLock);
                JOYNR_LOG_INFO(logger(),
                               "Global capability '{}' added successfully for GBIDs >{}<, "
                               "#registeredGlobalCapabilities {}",
                               globalDiscoveryEntry.toString(),
                               boost::algorithm::join(gbids, ", "),
                               thisSharedPtr->countGlobalCapabilities());
                if (awaitGlobalRegistration) {
                    thisSharedPtr->insertInGlobalLookupCache(globalDiscoveryEntry, gbids);
                    thisSharedPtr->insertInLocalCapabilitiesStorage(globalDiscoveryEntry);
                    if (onSuccess) {
                        onSuccess();
                    }

                    // Inform observers
                    thisSharedPtr->informObserversOnAdd(globalDiscoveryEntry);

                    thisSharedPtr->updatePersistedFile();
                    {
                        std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                        thisSharedPtr->callPendingLookups(
                                InterfaceAddress(globalDiscoveryEntry.getDomain(),
                                                 globalDiscoveryEntry.getInterfaceName()));
                    }
                }
            }
        };

        _globalCapabilitiesDirectoryClient->add(globalDiscoveryEntry,
                                                std::move(gbids),
                                                std::move(onSuccessWrapper),
                                                std::move(onErrorWrapper),
                                                std::move(onRuntimeError));
    }

    if (!isGloballyVisible || !awaitGlobalRegistration) {
        onSuccess();
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
                                       _localAddress);
}

void LocalCapabilitiesDirectory::triggerGlobalProviderReregistration(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;

    {
        std::lock_guard<std::recursive_mutex> lock3(_cacheLock);
        JOYNR_LOG_DEBUG(logger(), "triggerGlobalProviderReregistration");
        std::vector<types::DiscoveryEntry> entries;
        const std::int64_t now =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
        const std::int64_t newExpiryDateMs = now + _defaultExpiryIntervalMs;

        // copy existing global entries, update lastSeenDateMs and
        // increase expiryDateMs unless it already references a time
        // which is beyond newExpiryDate/updatedExpiryDate
        for (auto capability : *_locallyRegisteredCapabilities) {
            if (capability.getExpiryDateMs() < newExpiryDateMs) {
                capability.setExpiryDateMs(newExpiryDateMs);
            }
            if (capability.getLastSeenDateMs() < now) {
                capability.setLastSeenDateMs(now);
            }
            entries.push_back(capability);
        }
        for (const auto& capability : entries) {
            if (capability.getQos().getScope() == types::ProviderScope::GLOBAL) {
                const std::string& participantId = capability.getParticipantId();
                auto foundGbids = _globalParticipantIdsToGbidsMap.find(participantId);
                if (foundGbids != _globalParticipantIdsToGbidsMap.cend()) {
                    // update local store
                    auto gbids = foundGbids->second;
                    _locallyRegisteredCapabilities->insert(capability, gbids);
                    // update global cache
                    _globalLookupCache->insert(capability);
                    // send entries to JDS again
                    auto onApplicationError =
                            [participantId, gbids](const types::DiscoveryError::Enum& error) {
                        JOYNR_LOG_WARN(logger(),
                                       "Global provider reregistration for participantId {} and "
                                       "gbids >{}< failed: {} (DiscoveryError)",
                                       participantId,
                                       boost::algorithm::join(gbids, ", "),
                                       types::DiscoveryError::getLiteral(error));
                    };
                    auto onRuntimeError = [participantId, gbids](
                            const exceptions::JoynrRuntimeException& exception) {
                        JOYNR_LOG_WARN(logger(),
                                       "Global provider reregistration for participantId {} and "
                                       "gbids >{}< failed: {} ({})",
                                       participantId,
                                       boost::algorithm::join(gbids, ", "),
                                       exception.getMessage(),
                                       exception.getTypeName());
                    };
                    _globalCapabilitiesDirectoryClient->add(toGlobalDiscoveryEntry(capability),
                                                            gbids,
                                                            nullptr,
                                                            std::move(onApplicationError),
                                                            std::move(onRuntimeError));
                } else {
                    JOYNR_LOG_FATAL(logger(),
                                    "Global provider reregistration failed because participantId "
                                    "to GBIDs mapping is missing for participantId {}",
                                    participantId);
                }
            } else {
                // update local cache
                _locallyRegisteredCapabilities->insert(capability);
            }
        }
    }

    onSuccess();
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedGlobalDiscoveryEntries()
        const
{
    std::lock_guard<std::recursive_mutex> lock4(_cacheLock);

    return std::vector<types::DiscoveryEntry>(
            _globalLookupCache->cbegin(), _globalLookupCache->cend());
}

std::size_t LocalCapabilitiesDirectory::countGlobalCapabilities() const
{
    std::size_t counter = 0;
    std::lock_guard<std::recursive_mutex> lock4(_cacheLock);
    for (const auto& capability : *_locallyRegisteredCapabilities) {
        if (capability.getQos().getScope() == types::ProviderScope::GLOBAL) {
            counter++;
        }
    }
    return counter;
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    auto localCapabilities = searchLocalCache(interfaceAddresses);
    auto cachedCapabilities =
            scope == types::DiscoveryScope::LOCAL_ONLY
                    ? std::vector<types::DiscoveryEntry>{}
                    : searchGlobalCache(interfaceAddresses,
                                        gbids,
                                        std::chrono::milliseconds(discoveryQos.getCacheMaxAge()));

    return callReceiverIfPossible(scope,
                                  std::move(localCapabilities),
                                  std::move(cachedCapabilities),
                                  std::move(callback));
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    boost::optional<types::DiscoveryEntry> localOrCachedCapability = searchCaches(
            participantId, scope, gbids, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()));
    auto localCapabilities = optionalToVector(std::move(localOrCachedCapability));
    std::vector<types::DiscoveryEntry> globalCapabilities(localCapabilities);

    return callReceiverIfPossible(scope,
                                  std::move(localCapabilities),
                                  std::move(globalCapabilities),
                                  std::move(callback));
}

bool LocalCapabilitiesDirectory::isEntryForGbid(
        const std::unique_lock<std::recursive_mutex>& cacheLock,
        const types::DiscoveryEntry& entry,
        const std::unordered_set<std::string> gbids)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;

    const auto foundMapping = _globalParticipantIdsToGbidsMap.find(entry.getParticipantId());
    if (foundMapping != _globalParticipantIdsToGbidsMap.cend() && !foundMapping->second.empty()) {
        for (const auto& entryGbid : foundMapping->second) {
            if (gbids.find(entryGbid) != gbids.cend()) {
                return true;
            }
        }
    } else {
        JOYNR_LOG_WARN(logger(), "No GBIDs found for DiscoveryEntry {}", entry.getParticipantId());
    }
    return false;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::filterDiscoveryEntriesByGbids(
        const std::unique_lock<std::recursive_mutex>& cacheLock,
        const std::vector<types::DiscoveryEntry>& entries,
        const std::unordered_set<std::string>& gbids)
{
    assert(cacheLock.owns_lock());
    std::vector<types::DiscoveryEntry> result;

    for (const auto& entry : entries) {
        if (isEntryForGbid(cacheLock, entry, gbids)) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<types::DiscoveryEntryWithMetaInfo> LocalCapabilitiesDirectory::filterDuplicates(
        std::vector<types::DiscoveryEntryWithMetaInfo>&& localCapabilitiesWithMetaInfo,
        std::vector<types::DiscoveryEntryWithMetaInfo>&& globalCapabilitiesWithMetaInfo)
{
    // use custom DiscoveryEntryHash and custom DiscoveryEntryKeyEq to compare only the
    // participantId and to ignore the isLocal flag of DiscoveryEntryWithMetaInfo.
    // prefer local entries if there are local and global entries for the same provider.
    std::unordered_set<types::DiscoveryEntryWithMetaInfo,
                       joynr::DiscoveryEntryHash,
                       joynr::DiscoveryEntryKeyEq>
            resultSet(std::make_move_iterator(localCapabilitiesWithMetaInfo.begin()),
                      std::make_move_iterator(localCapabilitiesWithMetaInfo.end()));
    resultSet.insert(std::make_move_iterator(globalCapabilitiesWithMetaInfo.begin()),
                     std::make_move_iterator(globalCapabilitiesWithMetaInfo.end()));
    std::vector<types::DiscoveryEntryWithMetaInfo> resultVec(resultSet.begin(), resultSet.end());
    return resultVec;
}

bool LocalCapabilitiesDirectory::callReceiverIfPossible(
        joynr::types::DiscoveryScope::Enum& scope,
        std::vector<types::DiscoveryEntry>&& localCapabilities,
        std::vector<types::DiscoveryEntry>&& globalCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // return only local capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_ONLY) {
        auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
        callback->capabilitiesReceived(std::move(localCapabilitiesWithMetaInfo));
        return true;
    }

    // return local then global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
        auto globalCapabilitiesWithMetaInfo = util::convert(false, globalCapabilities);
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
        if (!globalCapabilities.empty()) {
            auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
            auto globalCapabilitiesWithMetaInfo = util::convert(false, globalCapabilities);

            // remove duplicates
            auto resultVec = filterDuplicates(std::move(localCapabilitiesWithMetaInfo),
                                              std::move(globalCapabilitiesWithMetaInfo));
            callback->capabilitiesReceived(std::move(resultVec));
            return true;
        }
    }

    // return the global cached entries
    if (scope == joynr::types::DiscoveryScope::GLOBAL_ONLY) {
        auto resultWithDuplicates = util::convert(false, globalCapabilities);
        auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
        for (const auto& entry : localCapabilitiesWithMetaInfo) {
            if (entry.getQos().getScope() == joynr::types::ProviderScope::GLOBAL) {
                resultWithDuplicates.push_back(entry);
            }
        }
        if (!resultWithDuplicates.empty()) {
            // remove duplicates
            std::unordered_set<types::DiscoveryEntryWithMetaInfo,
                               joynr::DiscoveryEntryHash,
                               joynr::DiscoveryEntryKeyEq>
                    resultSet(std::make_move_iterator(resultWithDuplicates.begin()),
                              std::make_move_iterator(resultWithDuplicates.end()));
            std::vector<types::DiscoveryEntryWithMetaInfo> result(
                    resultSet.begin(), resultSet.end());
            callback->capabilitiesReceived(std::move(result));
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::capabilitiesReceived(
        const std::vector<types::GlobalDiscoveryEntry>& results,
        std::vector<types::DiscoveryEntry>&& localEntries,
        std::shared_ptr<ILocalCapabilitiesCallback> callback,
        joynr::types::DiscoveryScope::Enum discoveryScope)
{
    std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
    std::vector<types::DiscoveryEntryWithMetaInfo> globalEntries;

    for (types::GlobalDiscoveryEntry globalDiscoveryEntry : results) {
        types::DiscoveryEntryWithMetaInfo convertedEntry =
                util::convert(false, globalDiscoveryEntry);
        capabilitiesMap.insert(
                {globalDiscoveryEntry.getAddress(), std::move(globalDiscoveryEntry)});
        globalEntries.push_back(std::move(convertedEntry));
    }
    registerReceivedCapabilities(std::move(capabilitiesMap));

    if (discoveryScope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryScope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        auto localEntriesWithMetaInfo = util::convert(true, localEntries);
        // look if in the meantime there are some local providers registered
        // lookup in the local directory to get local providers which were registered in the
        // meantime.
        globalEntries =
                filterDuplicates(std::move(localEntriesWithMetaInfo), std::move(globalEntries));
    }
    callback->capabilitiesReceived(std::move(globalEntries));
}

void LocalCapabilitiesDirectory::lookup(const std::string& participantId,
                                        const joynr::types::DiscoveryQos& discoveryQos,
                                        const std::vector<std::string>& gbids,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // get the local and cached entries
    bool receiverCalled =
            getLocalAndCachedCapabilities(participantId, discoveryQos, gbids, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entries in the global capabilities directory
        auto onSuccess = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            participantId,
            discoveryScope = discoveryQos.getDiscoveryScope(),
            callback,
            replaceGdeGbid = containsOnlyEmptyString(gbids)
        ](std::vector<joynr::types::GlobalDiscoveryEntry> result)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                if (replaceGdeGbid) {
                    thisSharedPtr->replaceGbidWithEmptyString(result);
                }
                thisSharedPtr->capabilitiesReceived(
                        result,
                        thisSharedPtr->getCachedLocalCapabilities(participantId),
                        callback,
                        discoveryScope);
            }
        };

        auto onRuntimeError =
                [callback, participantId](const exceptions::JoynrRuntimeException& exception) {
            JOYNR_LOG_DEBUG(logger(),
                            "Global lookup for participantId {} failed with exception: {} ({})",
                            participantId,
                            exception.getMessage(),
                            exception.TYPE_NAME());
            callback->onError(types::DiscoveryError::INTERNAL_ERROR);
        };

        _globalCapabilitiesDirectoryClient->lookup(participantId,
                                                   std::move(gbids),
                                                   discoveryQos.getDiscoveryTimeout(),
                                                   std::move(onSuccess),
                                                   std::bind(&ILocalCapabilitiesCallback::onError,
                                                             std::move(callback),
                                                             std::placeholders::_1),
                                                   std::move(onRuntimeError));
    }
}

bool LocalCapabilitiesDirectory::containsOnlyEmptyString(const std::vector<std::string> gbids)
{
    return gbids.size() == 1 && gbids[0] == "";
}

void LocalCapabilitiesDirectory::replaceGbidWithEmptyString(
        std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)
{
    JOYNR_LOG_TRACE(logger(), "replacing GBID of GDEs with empty string");
    for (auto& cap : capabilities) {
        const auto& serializedAddress = cap.getAddress();
        std::shared_ptr<system::RoutingTypes::Address> address;
        try {
            joynr::serializer::deserializeFromJson(address, serializedAddress);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(
                    logger(),
                    "could not deserialize Address for GBID replacement from {} - error: {}",
                    serializedAddress,
                    e.what());
            continue;
        }
        if (auto mqttAddress = dynamic_cast<system::RoutingTypes::MqttAddress*>(address.get())) {
            mqttAddress->setBrokerUri("");
            cap.setAddress(joynr::serializer::serializeToJson(*mqttAddress));
        }
        // other address types do not contain a GBID, default GBID will be used then for
        // globalParticipantIdsToGbidsMap
    }
}

void LocalCapabilitiesDirectory::lookup(const std::vector<std::string>& domains,
                                        const std::string& interfaceName,
                                        const std::vector<std::string>& gbids,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback,
                                        const joynr::types::DiscoveryQos& discoveryQos)
{
    std::vector<InterfaceAddress> interfaceAddresses;
    interfaceAddresses.reserve(domains.size());
    for (const auto& domain : domains) {
        interfaceAddresses.push_back(InterfaceAddress(domain, interfaceName));
    }

    // get the local and cached entries
    bool receiverCalled =
            getLocalAndCachedCapabilities(interfaceAddresses, discoveryQos, gbids, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entries in the global capabilities directory
        auto onSuccess = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            interfaceAddresses,
            callback,
            discoveryQos,
            replaceGdeGbid = containsOnlyEmptyString(gbids)
        ](std::vector<joynr::types::GlobalDiscoveryEntry> result)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                if (!(thisSharedPtr->isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    if (replaceGdeGbid) {
                        thisSharedPtr->replaceGbidWithEmptyString(result);
                    }
                    thisSharedPtr->capabilitiesReceived(
                            result,
                            thisSharedPtr->getCachedLocalCapabilities(interfaceAddresses),
                            callback,
                            discoveryQos.getDiscoveryScope());
                }
                thisSharedPtr->callbackCalled(interfaceAddresses, callback);
            }
        };

        auto onError = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            interfaceAddresses,
            domain = domains[0],
            interfaceName,
            callback,
            discoveryQos
        ](const types::DiscoveryError::Enum& error)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                JOYNR_LOG_DEBUG(logger(),
                                "Global lookup for domain {} and interface {} failed with "
                                "DiscoveryError: {}",
                                domain,
                                interfaceName,
                                types::DiscoveryError::getLiteral(error));
                std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                if (!(thisSharedPtr->isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    callback->onError(error);
                }
                thisSharedPtr->callbackCalled(interfaceAddresses, callback);
            }
        };

        auto onRuntimeError = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            interfaceAddresses,
            domain = domains[0],
            interfaceName,
            callback,
            discoveryQos
        ](const exceptions::JoynrRuntimeException& exception)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                JOYNR_LOG_DEBUG(logger(),
                                "Global lookup for domain {} and interface {} failed with "
                                "exception: {} ({})",
                                domain,
                                interfaceName,
                                exception.getMessage(),
                                exception.TYPE_NAME());
                std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                if (!(thisSharedPtr->isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    callback->onError(types::DiscoveryError::INTERNAL_ERROR);
                }
                thisSharedPtr->callbackCalled(interfaceAddresses, callback);
            }
        };

        if (discoveryQos.getDiscoveryScope() == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
            std::lock_guard<std::mutex> lock(_pendingLookupsLock);
            registerPendingLookup(interfaceAddresses, callback);
        }
        _globalCapabilitiesDirectoryClient->lookup(domains,
                                                   interfaceName,
                                                   std::move(gbids),
                                                   discoveryQos.getDiscoveryTimeout(),
                                                   std::move(onSuccess),
                                                   std::move(onError),
                                                   std::move(onRuntimeError));
    }
}

void LocalCapabilitiesDirectory::callPendingLookups(const InterfaceAddress& interfaceAddress)
{
    if (_pendingLookups.find(interfaceAddress) == _pendingLookups.cend()) {
        return;
    }
    auto localCapabilities = searchLocalCache({interfaceAddress});
    if (localCapabilities.empty()) {
        return;
    }
    auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);

    for (const std::shared_ptr<ILocalCapabilitiesCallback>& callback :
         _pendingLookups[interfaceAddress]) {
        callback->capabilitiesReceived(localCapabilitiesWithMetaInfo);
    }
    _pendingLookups.erase(interfaceAddress);
}

void LocalCapabilitiesDirectory::registerPendingLookup(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback)
{
    for (const InterfaceAddress& address : interfaceAddresses) {
        _pendingLookups[address].push_back(callback); // if no entry exists for key address, an
                                                      // empty list is automatically created
    }
}

bool LocalCapabilitiesDirectory::hasPendingLookups()
{
    return !_pendingLookups.empty();
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
        if (_pendingLookups.find(address) == _pendingLookups.cend()) {
            return true;
        }
        if (std::find(_pendingLookups[address].cbegin(),
                      _pendingLookups[address].cend(),
                      callback) == _pendingLookups[address].cend()) {
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
        if (_pendingLookups.find(address) != _pendingLookups.cend()) {
            std::vector<std::shared_ptr<ILocalCapabilitiesCallback>>& callbacks =
                    _pendingLookups[address];
            util::removeAll(callbacks, callback);
            if (_pendingLookups[address].empty()) {
                _pendingLookups.erase(address);
            }
        }
    }
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::string& participantId)
{
    std::lock_guard<std::recursive_mutex> lock5(_cacheLock);
    return optionalToVector(_locallyRegisteredCapabilities->lookupByParticipantId(participantId));
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    return searchLocalCache(interfaceAddresses);
}

void LocalCapabilitiesDirectory::clear()
{
    std::lock_guard<std::recursive_mutex> lock6(_cacheLock);
    _locallyRegisteredCapabilities->clear();
    _globalLookupCache->clear();
    _globalParticipantIdsToGbidsMap.clear();
}

void LocalCapabilitiesDirectory::registerReceivedCapabilities(
        const std::unordered_multimap<std::string, types::DiscoveryEntry>&& capabilityEntries)
{
    for (auto it = capabilityEntries.cbegin(); it != capabilityEntries.cend(); ++it) {
        const std::string& serializedAddress = it->first;
        std::shared_ptr<const system::RoutingTypes::Address> address;
        try {
            joynr::serializer::deserializeFromJson(address, serializedAddress);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger(),
                            "could not deserialize Address from {} - error: {}",
                            serializedAddress,
                            e.what());
            continue;
        }

        const types::DiscoveryEntry& currentEntry = it->second;
        const bool isGloballyVisible = isGlobal(currentEntry);
        if (auto messageRouterSharedPtr = _messageRouter.lock()) {
            constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
            const bool isSticky = false;
            messageRouterSharedPtr->addNextHop(currentEntry.getParticipantId(),
                                               address,
                                               isGloballyVisible,
                                               expiryDateMs,
                                               isSticky);
        } else {
            JOYNR_LOG_FATAL(logger(),
                            "could not addNextHop {} to {} because messageRouter is not available",
                            currentEntry.getParticipantId(),
                            serializedAddress);
            return;
        }

        std::vector<std::string> gbids;
        if (auto mqttAddress =
                    dynamic_cast<const system::RoutingTypes::MqttAddress*>(address.get())) {
            gbids.push_back(mqttAddress->getBrokerUri());
        } else {
            // use default GBID for all other address types
            gbids.push_back(_knownGbids[0]);
        }
        insertInGlobalLookupCache(std::move(currentEntry), std::move(gbids));
    }
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    const bool awaitGlobalRegistration = false;
    return add(discoveryEntry, awaitGlobalRegistration, std::move(onSuccess), std::move(onError));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        const bool& awaitGlobalRegistration,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto onErrorWrapper = [
        onError = std::move(onError),
        discoveryEntry,
        participantId = discoveryEntry.getParticipantId()
    ](const types::DiscoveryError::Enum& errorEnum)
    {
        onError(joynr::exceptions::ProviderRuntimeException(
                fmt::format("Error registering provider {} in default backend: {}",
                            participantId,
                            types::DiscoveryError::getLiteral(errorEnum))));
    };
    add(discoveryEntry,
        awaitGlobalRegistration,
        std::vector<std::string>(),
        std::move(onSuccess),
        std::move(onErrorWrapper));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        const bool& awaitGlobalRegistration,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError)
{
    auto result = validateGbids(gbids, _knownGbidsSet);
    switch (result) {
    case ValidateGBIDsEnum::OK:
        break;
    case ValidateGBIDsEnum::INVALID:
        onError(types::DiscoveryError::INVALID_GBID);
        return;
    case ValidateGBIDsEnum::UNKNOWN:
        onError(types::DiscoveryError::UNKNOWN_GBID);
        return;
    default:
        onError(types::DiscoveryError::INTERNAL_ERROR);
        break;
    }

    if (!hasProviderPermission(discoveryEntry)) {
        throw exceptions::ProviderRuntimeException(fmt::format(
                "Provider does not have permissions to register interface {} on domain {}.",
                discoveryEntry.getInterfaceName(),
                discoveryEntry.getDomain()));
        return;
    }
    const auto gbidsForAdd = gbids.size() == 0 ? std::vector<std::string>{_knownGbids[0]} : gbids;
    addInternal(discoveryEntry,
                awaitGlobalRegistration,
                std::move(gbidsForAdd),
                std::move(onSuccess),
                std::move(onError));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::addToAll(
        const joynr::types::DiscoveryEntry& discoveryEntry,
        const bool& awaitGlobalRegistration,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
{
    add(discoveryEntry,
        awaitGlobalRegistration,
        _knownGbids,
        std::move(onSuccess),
        std::move(onError));
}

bool LocalCapabilitiesDirectory::hasProviderPermission(const types::DiscoveryEntry& discoveryEntry)
{
    if (!_clusterControllerSettings.enableAccessController()) {
        return true;
    }

    if (auto gotAccessController = _accessController.lock()) {
        const CallContext& callContext = CallContextStorage::get();
        const std::string& ownerId = callContext.getPrincipal();
        JOYNR_LOG_TRACE(logger(), "hasProviderPermission for ownerId={}", ownerId);
        const bool result = gotAccessController->hasProviderPermission(
                ownerId,
                infrastructure::DacTypes::TrustLevel::HIGH,
                discoveryEntry.getDomain(),
                discoveryEntry.getInterfaceName());
        if (_clusterControllerSettings.aclAudit()) {
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
    this->_accessController = std::move(accessController);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const types::DiscoveryQos& discoveryQos,
        std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>& result)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto onErrorWrapper = [ onError = std::move(onError), domains = domains, interfaceName ](
            const types::DiscoveryError::Enum& error)
    {
        onError(exceptions::ProviderRuntimeException(
                fmt::format("Error looking up provider for domain {} and interface {} in all known "
                            "backends: {}",
                            domains.size() != 0 ? domains[0] : "",
                            interfaceName,
                            types::DiscoveryError::getLiteral(error))));
    };
    lookup(domains,
           interfaceName,
           discoveryQos,
           std::vector<std::string>(),
           std::move(onSuccess),
           std::move(onErrorWrapper));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
{
    if (domains.empty()) {
        throw joynr::exceptions::ProviderRuntimeException("Domains must not be empty.");
    }

    auto result = validateGbids(gbids, _knownGbidsSet);
    switch (result) {
    case ValidateGBIDsEnum::OK:
        break;
    case ValidateGBIDsEnum::INVALID:
        onError(types::DiscoveryError::INVALID_GBID);
        return;
    case ValidateGBIDsEnum::UNKNOWN:
        onError(types::DiscoveryError::UNKNOWN_GBID);
        return;
    default:
        onError(types::DiscoveryError::INTERNAL_ERROR);
        break;
    }

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    const auto& gbidsForLookup = gbids.size() == 0 ? _knownGbids : gbids;
    lookup(domains,
           interfaceName,
           std::move(gbidsForLookup),
           std::move(localCapabilitiesCallback),
           discoveryQos);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& participantId,
        std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto onErrorWrapper = [ onError = std::move(onError), participantId ](
            const types::DiscoveryError::Enum& error)
    {
        onError(exceptions::ProviderRuntimeException(
                fmt::format("Error looking up provider {} in all known backends: {}",
                            participantId,
                            types::DiscoveryError::getLiteral(error))));
    };

    auto onSuccessWrapper = [
        thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
        onSuccess = std::move(onSuccess),
        onError,
        participantId
    ](const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities)
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            if (capabilities.size() == 0) {
                joynr::exceptions::ProviderRuntimeException exception(
                        "No capabilities found for participantId \"" + participantId +
                        "\" and default GBID: " + thisSharedPtr->_knownGbids[0]);
                onError(exception);
                return;
            }
            if (capabilities.size() > 1) {
                JOYNR_LOG_FATAL(thisSharedPtr->logger(),
                                "participantId {} has more than 1 capability entry:\n {}\n {}",
                                participantId,
                                capabilities[0].toString(),
                                capabilities[1].toString());
            }

            onSuccess(capabilities[0]);
        }
    };

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    auto localCapabilitiesCallback = std::make_shared<LocalCapabilitiesCallback>(
            std::move(onSuccessWrapper), std::move(onErrorWrapper));
    lookup(participantId, discoveryQos, _knownGbids, std::move(localCapabilitiesCallback));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
{
    auto result = validateGbids(gbids, _knownGbidsSet);
    switch (result) {
    case ValidateGBIDsEnum::OK:
        break;
    case ValidateGBIDsEnum::INVALID:
        onError(types::DiscoveryError::INVALID_GBID);
        return;
    case ValidateGBIDsEnum::UNKNOWN:
        onError(types::DiscoveryError::UNKNOWN_GBID);
        return;
    default:
        onError(types::DiscoveryError::INTERNAL_ERROR);
        break;
    }

    const auto gbidsForLookup = gbids.size() == 0 ? _knownGbids : gbids;
    auto onSuccessWrapper = [
        thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
        onSuccess = std::move(onSuccess),
        onError,
        participantId,
        gbidsForLookup
    ](const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities)
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            if (capabilities.size() == 0) {
                const std::string gbidString = boost::algorithm::join(gbidsForLookup, ", ");
                JOYNR_LOG_DEBUG(logger(),
                                "participantId {} has no capability entry "
                                "(DiscoveryError::NO_ENTRY_FOR_PARTICIPANT) for GBIDs: >{}<",
                                participantId,
                                gbidString);
                onError(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
                return;
            }
            if (capabilities.size() > 1) {
                JOYNR_LOG_FATAL(thisSharedPtr->logger(),
                                "participantId {} has more than 1 capability entry:\n {}\n {}",
                                participantId,
                                capabilities[0].toString(),
                                capabilities[1].toString());
            }

            onSuccess(capabilities[0]);
        }
    };

    auto localCapabilitiesCallback = std::make_shared<LocalCapabilitiesCallback>(
            std::move(onSuccessWrapper), std::move(onError));
    lookup(participantId,
           discoveryQos,
           std::move(gbidsForLookup),
           std::move(localCapabilitiesCallback));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::remove(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    {
        std::lock_guard<std::recursive_mutex> lock7(_cacheLock);

        boost::optional<types::DiscoveryEntry> optionalEntry =
                _locallyRegisteredCapabilities->lookupByParticipantId(participantId);
        if (!optionalEntry) {
            JOYNR_LOG_INFO(
                    logger(), "participantId '{}' not found, cannot be removed", participantId);
            exceptions::ProviderRuntimeException exception(
                    fmt::format("Failed to remove participantId: {}. ParticipantId is not "
                                "registered in cluster controller.",
                                participantId));
            onError(exception);
            return;
        }
        const types::DiscoveryEntry& entry = *optionalEntry;

        if (isGlobal(entry)) {
            auto foundGbids = _globalParticipantIdsToGbidsMap.find(participantId);
            std::vector<std::string> gbids;
            if (foundGbids == _globalParticipantIdsToGbidsMap.cend()) {
                JOYNR_LOG_FATAL(logger(),
                                "Global remove failed because participantId to GBIDs mapping is "
                                "missing for participantId {}",
                                participantId);
            } else {
                gbids = foundGbids->second;
                const std::string gbidString = boost::algorithm::join(gbids, ", ");
                JOYNR_LOG_INFO(logger(),
                               "Removing globally registered participantId: {} from GBIDs: >{}<",
                               participantId,
                               gbidString);

                _globalParticipantIdsToGbidsMap.erase(participantId);
                _globalLookupCache->removeByParticipantId(participantId);
                auto onApplicationError =
                        [participantId, gbids](const types::DiscoveryError::Enum& error) {
                    JOYNR_LOG_WARN(logger(),
                                   "Error removing participantId {} globally for GBIDs >{}<: {}",
                                   participantId,
                                   boost::algorithm::join(gbids, ", "),
                                   types::DiscoveryError::getLiteral(error));
                };
                auto onRuntimeError =
                        [participantId, gbids](const exceptions::JoynrRuntimeException& exception) {
                    JOYNR_LOG_WARN(logger(),
                                   "Failed to remove participantId {} globally for GBIDs >{}<: "
                                   "{} ({})",
                                   participantId,
                                   boost::algorithm::join(gbids, ", "),
                                   exception.getMessage(),
                                   exception.getTypeName());
                };

                _globalCapabilitiesDirectoryClient->remove(participantId,
                                                           gbids,
                                                           nullptr,
                                                           std::move(onApplicationError),
                                                           std::move(onRuntimeError));
            }
        }
        JOYNR_LOG_INFO(logger(), "Removing locally registered participantId: {}", participantId);
        _locallyRegisteredCapabilities->removeByParticipantId(participantId);
        JOYNR_LOG_INFO(logger(),
                       "After removal of participantId {}: #localCapabilities {}, "
                       "#registeredGlobalCapabilities: {}, #globalLookupCache: {}",
                       participantId,
                       _locallyRegisteredCapabilities->size(),
                       countGlobalCapabilities(),
                       _globalLookupCache->size());
        informObserversOnRemove(entry);

        if (auto messageRouterSharedPtr = _messageRouter.lock()) {
            messageRouterSharedPtr->removeNextHop(participantId);
        } else {
            JOYNR_LOG_FATAL(logger(),
                            "could not removeNextHop for {} because messageRouter is not available",
                            participantId);
        }
    }
    if (onSuccess) {
        onSuccess();
    }
    updatePersistedFile();
}

void LocalCapabilitiesDirectory::addProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    _observers.push_back(std::move(observer));
}

void LocalCapabilitiesDirectory::removeProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    util::removeAll(_observers, observer);
}

void LocalCapabilitiesDirectory::updatePersistedFile()
{
    saveLocalCapabilitiesToFile(
            _clusterControllerSettings.getLocalCapabilitiesDirectoryPersistenceFilename());
}

void LocalCapabilitiesDirectory::saveLocalCapabilitiesToFile(const std::string& fileName)
{
    if (!_isLocalCapabilitiesDirectoryPersistencyEnabled) {
        return;
    }

    if (fileName.empty()) {
        return;
    }

    try {
        std::lock_guard<std::recursive_mutex> lock8(_cacheLock);
        joynr::util::saveStringToFile(
                fileName, joynr::serializer::serializeToJson(_locallyRegisteredCapabilities));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }
}

void LocalCapabilitiesDirectory::loadPersistedFile()
{
    if (!_isLocalCapabilitiesDirectoryPersistencyEnabled) {
        return;
    }

    const std::string persistencyFile =
            _clusterControllerSettings.getLocalCapabilitiesDirectoryPersistenceFilename();

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

    std::lock_guard<std::recursive_mutex> lock9(_cacheLock);

    try {
        joynr::serializer::deserializeFromJson(_locallyRegisteredCapabilities, jsonString);
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }

    // insert all global capability entries into global cache
    for (const auto& entry : *_locallyRegisteredCapabilities) {
        if (entry.getQos().getScope() == types::ProviderScope::GLOBAL) {
            insertInGlobalLookupCache(entry, entry.gbids);
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
void LocalCapabilitiesDirectory::insertInLocalCapabilitiesStorage(
        const types::DiscoveryEntry& entry)
{
    std::lock_guard<std::recursive_mutex> lock10(_cacheLock);

    auto found = _globalParticipantIdsToGbidsMap.find(entry.getParticipantId());
    _locallyRegisteredCapabilities->insert(entry,
                                           found != _globalParticipantIdsToGbidsMap.cend()
                                                   ? found->second
                                                   : std::vector<std::string>{});
    JOYNR_LOG_INFO(logger(),
                   "Added local capability to cache {}, #localCapabilities: {}",
                   entry.toString(),
                   _locallyRegisteredCapabilities->size());
}

/**
 * Private convenience methods.
 */
void LocalCapabilitiesDirectory::insertInGlobalLookupCache(const types::DiscoveryEntry& entry,
                                                           const std::vector<std::string>& gbids)
{
    std::lock_guard<std::recursive_mutex> lock11(_cacheLock);

    _globalLookupCache->insert(entry);

    const std::string& participantId = entry.getParticipantId();
    std::vector<std::string> allGbids(gbids);
    auto foundMapping = _globalParticipantIdsToGbidsMap.find(participantId);
    if (foundMapping != _globalParticipantIdsToGbidsMap.cend()) {
        const auto oldGbids = foundMapping->second;
        for (const auto gbid : oldGbids) {
            if (std::find(allGbids.cbegin(), allGbids.cend(), gbid) == allGbids.cend()) {
                allGbids.emplace_back(gbid);
            }
        }
    }
    _globalParticipantIdsToGbidsMap.insert(std::make_pair(participantId, allGbids));

    JOYNR_LOG_INFO(logger(),
                   "Added global capability to cache {}, registered GBIDs: >{}<, "
                   "#globalLookupCache: {}",
                   entry.toString(),
                   boost::algorithm::join(allGbids, ", "),
                   _globalLookupCache->size());
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchGlobalCache(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::vector<std::string>& gbids,
        std::chrono::milliseconds maxCacheAge)
{
    std::unique_lock<std::recursive_mutex> lock(_cacheLock);

    const std::unordered_set<std::string> gbidsSet(gbids.cbegin(), gbids.cend());
    std::vector<types::DiscoveryEntry> result;
    for (const auto& interfaceAddress : interfaceAddresses) {
        const std::string& domain = interfaceAddress.getDomain();
        const std::string& interface = interfaceAddress.getInterface();

        const auto entries =
                _globalLookupCache->lookupCacheByDomainAndInterface(domain, interface, maxCacheAge);
        const auto filteredEntries = filterDiscoveryEntriesByGbids(lock, entries, gbidsSet);
        result.insert(result.end(),
                      std::make_move_iterator(filteredEntries.begin()),
                      std::make_move_iterator(filteredEntries.end()));
    }
    return result;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchLocalCache(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    std::lock_guard<std::recursive_mutex> lock12(_cacheLock);

    std::vector<types::DiscoveryEntry> result;
    for (const auto& interfaceAddress : interfaceAddresses) {
        const std::string& domain = interfaceAddress.getDomain();
        const std::string& interface = interfaceAddress.getInterface();

        auto entries =
                _locallyRegisteredCapabilities->lookupByDomainAndInterface(domain, interface);
        result.insert(result.end(),
                      std::make_move_iterator(entries.begin()),
                      std::make_move_iterator(entries.end()));
    }
    return result;
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchCaches(
        const std::string& participantId,
        joynr::types::DiscoveryScope::Enum scope,
        const std::vector<std::string>& gbids,
        std::chrono::milliseconds maxCacheAge)
{
    std::unique_lock<std::recursive_mutex> lock(_cacheLock);

    // first search locally
    auto entry = _locallyRegisteredCapabilities->lookupByParticipantId(participantId);
    if (scope == types::DiscoveryScope::LOCAL_ONLY ||
        (entry && scope != types::DiscoveryScope::GLOBAL_ONLY)) {
        return entry;
    }

    if (maxCacheAge == std::chrono::milliseconds(-1)) {
        entry = _globalLookupCache->lookupByParticipantId(participantId);
    } else {
        entry = _globalLookupCache->lookupCacheByParticipantId(participantId, maxCacheAge);
    }
    if (entry) {
        const std::unordered_set<std::string> gbidsSet(gbids.cbegin(), gbids.cend());
        if (!isEntryForGbid(lock, *entry, gbidsSet)) {
            return boost::none;
        }
    }
    return entry;
}

void LocalCapabilitiesDirectory::informObserversOnAdd(const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : _observers) {
        observer->onProviderAdd(discoveryEntry);
    }
}

void LocalCapabilitiesDirectory::informObserversOnRemove(
        const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : _observers) {
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
    auto intervalMs = _clusterControllerSettings.getPurgeExpiredDiscoveryEntriesIntervalMs();
    _checkExpiredDiscoveryEntriesTimer.expires_from_now(
            std::chrono::milliseconds(intervalMs), timerError);
    if (timerError) {
        JOYNR_LOG_FATAL(logger(),
                        "Error scheduling discovery entries check. {}: {}",
                        timerError.value(),
                        timerError.message());
    } else {
        _checkExpiredDiscoveryEntriesTimer
                .async_wait([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
                        const boost::system::error_code& errorCode) {
                    if (auto thisSharedPtr = thisWeakPtr.lock()) {
                        thisSharedPtr->checkExpiredDiscoveryEntries(errorCode);
                    }
                });
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

    bool fileUpdateRequired = false;
    {
        std::lock_guard<std::recursive_mutex> lock13(_cacheLock);

        auto removedLocalCapabilities = _locallyRegisteredCapabilities->removeExpired();
        auto removedGlobalCapabilities = _globalLookupCache->removeExpired();

        if (!removedLocalCapabilities.empty() || !removedGlobalCapabilities.empty()) {
            fileUpdateRequired = true;
            if (auto messageRouterSharedPtr = _messageRouter.lock()) {
                JOYNR_LOG_INFO(logger(),
                               "Following discovery entries expired: local: {}, "
                               "#localCapabilities: {}, global: {}, #globalLookupCache: {}",
                               joinToString(removedLocalCapabilities),
                               _locallyRegisteredCapabilities->size(),
                               joinToString(removedGlobalCapabilities),
                               _globalLookupCache->size());

                for (const auto& capability :
                     boost::join(removedLocalCapabilities, removedGlobalCapabilities)) {
                    messageRouterSharedPtr->removeNextHop(capability.getParticipantId());
                    _globalParticipantIdsToGbidsMap.erase(capability.getParticipantId());
                }
            } else {
                JOYNR_LOG_FATAL(logger(),
                                "could not call removeNextHop because messageRouter is "
                                "not available");
            }
        }
    }

    if (fileUpdateRequired) {
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

void LocalCapabilitiesDirectory::removeStaleProvidersOfClusterController(
        const std::int64_t& clusterControllerStartDateMs)
{
    auto onSuccess = [ ccId = _clusterControllerId, clusterControllerStartDateMs ]()
    {
        JOYNR_LOG_TRACE(logger(),
                        "RemoveStale(ccId={}, maxLastSeenDateMs={}) succeeded.",
                        ccId,
                        clusterControllerStartDateMs);
    };

    auto onRuntimeError = [ ccId = _clusterControllerId, clusterControllerStartDateMs ](
            const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_ERROR(logger(),
                        "RemoveStale(ccId={}, maxLastSeenDateMs={}) failed: ",
                        ccId,
                        clusterControllerStartDateMs,
                        error.getMessage());
    };
    _globalCapabilitiesDirectoryClient->removeStale(_clusterControllerId,
                                                    clusterControllerStartDateMs,
                                                    std::move(onSuccess),
                                                    std::move(onRuntimeError));
}

LocalCapabilitiesCallback::LocalCapabilitiesCallback(
        std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>&& onSuccess,
        std::function<void(const types::DiscoveryError::Enum&)>&& onError)
        : _onSuccess(std::move(onSuccess)), _onErrorCallback(std::move(onError))
{
}

void LocalCapabilitiesCallback::onError(const types::DiscoveryError::Enum& error)
{
    std::call_once(_onceFlag, _onErrorCallback, error);
    //"Unable to collect capabilities from global capabilities directory. Error: " +
    // error.getMessage()));
}

void LocalCapabilitiesCallback::capabilitiesReceived(
        const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities)
{
    std::call_once(_onceFlag, _onSuccess, capabilities);
}

} // namespace joynr

#pragma GCC diagnostic pop

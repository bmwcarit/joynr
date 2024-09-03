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
#include <mutex>
#include <ostream>
#include <tuple>
#include <unordered_set>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/range/join.hpp>
#include <spdlog/fmt/fmt.h>

#include "joynr/access-control/IAccessController.h"

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/IMessageRouter.h"
#include "joynr/InterfaceAddress.h"
#include "joynr/LCDUtil.h"
#include "joynr/TimePoint.h"
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
#include "joynr/Future.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"

namespace joynr
{

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(
        ClusterControllerSettings& clusterControllerSettings,
        std::shared_ptr<IGlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient,
        std::shared_ptr<LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore,
        const std::string& localAddress,
        std::weak_ptr<IMessageRouter> messageRouter,
        boost::asio::io_service& ioService,
        const std::string clusterControllerId,
        std::vector<std::string> knownGbids,
        std::int64_t defaultExpiryIntervalMs,
        const std::chrono::milliseconds reAddInterval)
        : joynr::system::DiscoveryAbstractProvider(),
          joynr::system::ProviderReregistrationControllerProvider(),
          std::enable_shared_from_this<LocalCapabilitiesDirectory>(),
          _clusterControllerSettings(clusterControllerSettings),
          _globalCapabilitiesDirectoryClient(std::move(globalCapabilitiesDirectoryClient)),
          _localCapabilitiesDirectoryStore(localCapabilitiesDirectoryStore),
          _localAddress(localAddress),
          _pendingLookupsLock(),
          _messageRouter(messageRouter),
          _lcdPendingLookupsHandler(),
          _accessController(),
          _checkExpiredDiscoveryEntriesTimer(ioService),
          _freshnessUpdateTimer(ioService),
          _reAddAllGlobalEntriesTimer(ioService),
          _clusterControllerId(clusterControllerId),
          _knownGbids(knownGbids),
          _knownGbidsSet(knownGbids.cbegin(), knownGbids.cend()),
          _defaultExpiryIntervalMs(defaultExpiryIntervalMs),
          _reAddInterval(reAddInterval)
{
}

void LocalCapabilitiesDirectory::init()
{
    scheduleCleanupTimer();
    scheduleFreshnessUpdate();
    scheduleReAddAllGlobalDiscoveryEntries();
}

void LocalCapabilitiesDirectory::shutdown()
{
    _checkExpiredDiscoveryEntriesTimer.cancel();
    _freshnessUpdateTimer.cancel();
    _reAddAllGlobalEntriesTimer.cancel();
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
    const std::int64_t newLastSeenDateMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
    const std::int64_t newExpiryDateMs = newLastSeenDateMs + _defaultExpiryIntervalMs;
    {
        std::unique_lock<std::recursive_mutex> expiryDateUpdateLock(
                _localCapabilitiesDirectoryStore->getCacheLock());
        participantIds = _localCapabilitiesDirectoryStore
                                 ->touchAndReturnGlobalParticipantIdsFromLocalCapabilities(
                                         expiryDateUpdateLock, newLastSeenDateMs, newExpiryDateMs);
        _localCapabilitiesDirectoryStore->touchSelectedGlobalParticipant(
                expiryDateUpdateLock, participantIds, newLastSeenDateMs, newExpiryDateMs);
    }

    if (participantIds.empty()) {
        JOYNR_LOG_DEBUG(logger(),
                        "touch(clusterControllerId={}) has not been called, because there are no "
                        "providers to touch",
                        _clusterControllerId);
        scheduleFreshnessUpdate();
        return;
    }

    // Get map of gbids to participantIds to touch
    std::map<std::string, std::vector<std::string>> gbidsToParticipantIdsMap;
    for (const std::string& gbid : _knownGbids) {
        gbidsToParticipantIdsMap[gbid];
    }

    for (const std::string& participantIdToTouch : participantIds) {
        std::unique_lock<std::recursive_mutex> cacheLock(
                _localCapabilitiesDirectoryStore->getCacheLock());
        std::vector<std::string> gbids = _localCapabilitiesDirectoryStore->getGbidsForParticipantId(
                participantIdToTouch, cacheLock);
        cacheLock.unlock();

        if (gbids.empty()) {
            JOYNR_LOG_WARN(logger(),
                           "touch(clusterControllerId={}) cannot be called for provider with "
                           "participantId {}, no GBID found.",
                           _clusterControllerId,
                           participantIdToTouch);
            break;
        }

        // Get first gbid mapped to participantIdToTouch
        std::string gbidToTouch = gbids[0];

        // Update map of gbids to participantIds
        if (gbidsToParticipantIdsMap.find(gbidToTouch) == gbidsToParticipantIdsMap.cend()) {
            JOYNR_LOG_ERROR(logger(),
                            "touch: found GBID {} for particpantId {} is unknown.",
                            gbidToTouch,
                            participantIdToTouch);
            continue;
        }
        gbidsToParticipantIdsMap[gbidToTouch].emplace_back(participantIdToTouch);
    }

    for (const auto& entity : gbidsToParticipantIdsMap) {
        std::string gbid = entity.first;
        std::vector<std::string> participantIdsToTouch = entity.second;

        if (participantIdsToTouch.empty()) {
            JOYNR_LOG_DEBUG(logger(),
                            "touch(clusterControllerId={}, gbid={}) has not been called, because "
                            "there are no providers to touch for it.",
                            _clusterControllerId,
                            gbid);
            continue;
        }

        auto onSuccess = [ccId = _clusterControllerId, participantIdsToTouch, gbid]() {
            if (logger().getLogLevel() == LogLevel::Trace) {
                const std::string participantIdConcat =
                        boost::algorithm::join(participantIdsToTouch, ", ");
                JOYNR_LOG_TRACE(logger(),
                                "touch(ccId={}, participantIds={}, gbid={}) succeeded.",
                                ccId,
                                participantIdConcat,
                                gbid);
            } else {
                JOYNR_LOG_DEBUG(logger(), "touch(gbid={}) succeeded.", gbid);
            }
        };

        auto onError = [ccId = _clusterControllerId, participantIdsToTouch, gbid](
                               const joynr::exceptions::JoynrRuntimeException& error) {
            JOYNR_LOG_ERROR(logger(),
                            "touch(ccId={}, participantIds={}, gbid={}) failed: {}",
                            ccId,
                            boost::algorithm::join(participantIdsToTouch, ", "),
                            gbid,
                            error.getMessage());
        };

        _globalCapabilitiesDirectoryClient->touch(_clusterControllerId,
                                                  participantIdsToTouch,
                                                  gbid,
                                                  std::move(onSuccess),
                                                  std::move(onError));
    }

    scheduleFreshnessUpdate();
}

void LocalCapabilitiesDirectory::scheduleReAddAllGlobalDiscoveryEntries()
{
    boost::system::error_code timerError = boost::system::error_code();
    _reAddAllGlobalEntriesTimer.expires_from_now(_reAddInterval, timerError);
    if (timerError) {
        JOYNR_LOG_ERROR(logger(),
                        "Error from reAdd all GDEs timer: {}: {}",
                        timerError.value(),
                        timerError.message());
    }
    _reAddAllGlobalEntriesTimer.async_wait(
            [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
                    const boost::system::error_code& localTimerError) {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    thisSharedPtr->triggerAndRescheduleReAdd(localTimerError);
                }
            });
}

void LocalCapabilitiesDirectory::triggerAndRescheduleReAdd(
        const boost::system::error_code& timerError)
{
    if (timerError == boost::asio::error::operation_aborted) {
        // Assume Destructor has been called
        JOYNR_LOG_DEBUG(logger(),
                        "Re-Add aborted after shutdown, error code from timer: {}",
                        timerError.message());
        return;
    }
    if (timerError) {
        JOYNR_LOG_ERROR(
                logger(), "Re-Add called with error code from timer: {}", timerError.message());
    }
    _globalCapabilitiesDirectoryClient->reAdd(_localCapabilitiesDirectoryStore, _localAddress);

    scheduleReAddAllGlobalDiscoveryEntries();
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    _freshnessUpdateTimer.cancel();
    _checkExpiredDiscoveryEntriesTimer.cancel();
}

void LocalCapabilitiesDirectory::addInternal(
        types::DiscoveryEntry discoveryEntry,
        bool awaitGlobalRegistration,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const types::DiscoveryError::Enum&)> onError)
{
    const bool isGloballyVisible = LCDUtil::isGlobal(discoveryEntry);
    discoveryEntry.setLastSeenDateMs(TimePoint::now().toMilliseconds());

    if (!isGloballyVisible || !awaitGlobalRegistration) {
        std::lock_guard<std::recursive_mutex> lock1(
                _localCapabilitiesDirectoryStore->getCacheLock());
        // register locally
        _localCapabilitiesDirectoryStore->insertInLocalCapabilitiesStorage(
                discoveryEntry, awaitGlobalRegistration, gbids);

        {
            std::lock_guard<std::mutex> lock(_pendingLookupsLock);
            InterfaceAddress interfaceAddress(
                    discoveryEntry.getDomain(), discoveryEntry.getInterfaceName());
            _lcdPendingLookupsHandler.callPendingLookups(
                    interfaceAddress,
                    _localCapabilitiesDirectoryStore->searchLocal({interfaceAddress}));
        }
    }

    // register globally
    if (isGloballyVisible) {
        types::GlobalDiscoveryEntry globalDiscoveryEntry =
                LCDUtil::toGlobalDiscoveryEntry(discoveryEntry, _localAddress);

        auto onRuntimeError = [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                               globalDiscoveryEntry,
                               gbids,
                               awaitGlobalRegistration,
                               onError](const exceptions::JoynrRuntimeException& error) {
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

        auto onErrorWrapper = [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                               globalDiscoveryEntry,
                               gbids,
                               awaitGlobalRegistration,
                               onError](const types::DiscoveryError::Enum& error) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "DiscoveryError occurred during the execution of capabilitiesProxy->add for "
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

        std::function<void()> onSuccessWrapper = [thisWeakPtr = joynr::util::as_weak_ptr(
                                                          shared_from_this()),
                                                  globalDiscoveryEntry,
                                                  awaitGlobalRegistration,
                                                  gbids,
                                                  onSuccess]() {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::recursive_mutex> cacheInsertionLock(
                        thisSharedPtr->_localCapabilitiesDirectoryStore->getCacheLock());
                if (awaitGlobalRegistration) {
                    thisSharedPtr->_localCapabilitiesDirectoryStore
                            ->insertInLocalCapabilitiesStorage(
                                    globalDiscoveryEntry, awaitGlobalRegistration, gbids);
                    JOYNR_LOG_INFO(logger(),
                                   "Global capability '{}' added successfully for GBIDs >{}<, "
                                   "#registeredGlobalCapabilities {}",
                                   globalDiscoveryEntry.toString(),
                                   boost::algorithm::join(gbids, ", "),
                                   thisSharedPtr->_localCapabilitiesDirectoryStore
                                           ->countGlobalCapabilities());
                    if (onSuccess) {
                        onSuccess();
                    }

                    {
                        std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                        InterfaceAddress interfaceAddress(globalDiscoveryEntry.getDomain(),
                                                          globalDiscoveryEntry.getInterfaceName());
                        thisSharedPtr->_lcdPendingLookupsHandler.callPendingLookups(
                                interfaceAddress,
                                thisSharedPtr->_localCapabilitiesDirectoryStore->searchLocal(
                                        {interfaceAddress}));
                    }
                } else {
                    JOYNR_LOG_INFO(logger(),
                                   "Global capability '{}' added successfully for GBIDs >{}<, "
                                   "#registeredGlobalCapabilities {}",
                                   globalDiscoveryEntry.toString(),
                                   boost::algorithm::join(gbids, ", "),
                                   thisSharedPtr->_localCapabilitiesDirectoryStore
                                           ->countGlobalCapabilities());
                }
            }
        };

        _globalCapabilitiesDirectoryClient->add(globalDiscoveryEntry,
                                                awaitGlobalRegistration,
                                                std::move(gbids),
                                                std::move(onSuccessWrapper),
                                                std::move(onErrorWrapper),
                                                std::move(onRuntimeError));
    }

    if (!isGloballyVisible || !awaitGlobalRegistration) {
        onSuccess();
    }
}

void LocalCapabilitiesDirectory::triggerGlobalProviderReregistration(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;

    {
        std::unique_lock<std::recursive_mutex> providerReregistrationLock(
                _localCapabilitiesDirectoryStore->getCacheLock());
        JOYNR_LOG_DEBUG(logger(), "triggerGlobalProviderReregistration");
        const std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::system_clock::now().time_since_epoch())
                                         .count();
        const std::int64_t newExpiryDateMs = now + _defaultExpiryIntervalMs;

        // copy existing global entries, update lastSeenDateMs and
        // increase expiryDateMs unless it already references a time
        // which is beyond newExpiryDate/updatedExpiryDate
        std::vector<types::DiscoveryEntry> entries =
                _localCapabilitiesDirectoryStore->insertLocallyRegisteredCapabilitesToEntryList(
                        providerReregistrationLock, now, newExpiryDateMs);
        for (const auto& capability : entries) {
            if (capability.getQos().getScope() == types::ProviderScope::GLOBAL) {
                const std::string& participantId = capability.getParticipantId();
                auto foundGbids = _localCapabilitiesDirectoryStore->getGbidsForParticipantId(
                        participantId, providerReregistrationLock);
                if (!foundGbids.empty()) {
                    // update local store
                    _localCapabilitiesDirectoryStore->insertIntoLocallyRegisteredCapabilities(
                            providerReregistrationLock, capability, foundGbids);
                    // update global cache
                    _localCapabilitiesDirectoryStore->insertIntoGlobalCachedCapabilities(
                            providerReregistrationLock, capability);
                    // send entries to JDS again
                    auto onApplicationError = [participantId, foundGbids](
                                                      const types::DiscoveryError::Enum& error) {
                        JOYNR_LOG_WARN(logger(),
                                       "Global provider reregistration for participantId {} and "
                                       "gbids >{}< failed: {} (DiscoveryError)",
                                       participantId,
                                       boost::algorithm::join(foundGbids, ", "),
                                       types::DiscoveryError::getLiteral(error));
                    };
                    auto onRuntimeError =
                            [participantId,
                             foundGbids](const exceptions::JoynrRuntimeException& exception) {
                                JOYNR_LOG_WARN(
                                        logger(),
                                        "Global provider reregistration for participantId {} and "
                                        "gbids >{}< failed: {} ({})",
                                        participantId,
                                        boost::algorithm::join(foundGbids, ", "),
                                        exception.getMessage(),
                                        exception.getTypeName());
                            };
                    _globalCapabilitiesDirectoryClient->add(
                            LCDUtil::toGlobalDiscoveryEntry(capability, _localAddress),
                            false,
                            foundGbids,
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
                _localCapabilitiesDirectoryStore->insertIntoLocallyRegisteredCapabilities(
                        providerReregistrationLock, capability);
            }
        }
    }

    onSuccess();
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedGlobalDiscoveryEntries()
        const
{
    return _localCapabilitiesDirectoryStore->getCachedGlobalDiscoveryEntries();
}

void LocalCapabilitiesDirectory::capabilitiesReceived(
        const std::vector<types::GlobalDiscoveryEntry>& receivedGlobalEntries,
        std::vector<types::DiscoveryEntry>&& localEntries,
        std::shared_ptr<ILocalCapabilitiesCallback> callback,
        joynr::types::DiscoveryScope::Enum discoveryScope)
{
    std::vector<types::GlobalDiscoveryEntry> capabilities;
    for (types::GlobalDiscoveryEntry globalDiscoveryEntry : receivedGlobalEntries) {
        // check whether this entry exists in the local store. if so, then skip it
        auto localEntries2 = _localCapabilitiesDirectoryStore->getLocalCapabilities(
                globalDiscoveryEntry.getParticipantId());
        if (localEntries2.empty()) {
            capabilities.push_back(std::move(globalDiscoveryEntry));
        }
    }
    // stores remote entries in global cache
    auto result = registerReceivedCapabilities(std::move(capabilities));

    if (discoveryScope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryScope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        auto localEntriesWithMetaInfo = LCDUtil::convert(true, localEntries);
        // combine local and global entries (duplicates are already filtered)
        result.insert(
                result.end(), localEntriesWithMetaInfo.begin(), localEntriesWithMetaInfo.end());
    } else { // GLOBAL_ONLY
        for (const auto& localEntry : localEntries) {
            // add globally registered local entries to the result
            if (types::ProviderScope::GLOBAL == localEntry.getQos().getScope()) {
                auto localEntryWithMetaInfo = LCDUtil::convert(true, localEntry);
                result.push_back(localEntryWithMetaInfo);
            }
        }
    }

    callback->capabilitiesReceived(result);
}

// base lookup by particiapntId
void LocalCapabilitiesDirectory::lookup(const std::string& participantId,
                                        const joynr::types::DiscoveryQos& discoveryQos,
                                        const std::vector<std::string>& gbids,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // get the local and cached entries
    bool receiverCalled = _localCapabilitiesDirectoryStore->getLocalAndCachedCapabilities(
            participantId, discoveryQos, gbids, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entries in the global capabilities directory
        auto onSuccess = [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                          participantId,
                          discoveryScope = discoveryQos.getDiscoveryScope(),
                          callback,
                          replaceGdeGbid = LCDUtil::containsOnlyEmptyString(gbids)](
                                 std::vector<joynr::types::GlobalDiscoveryEntry> result) {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                if (replaceGdeGbid) {
                    LCDUtil::replaceGbidWithEmptyString(result);
                }
                std::lock_guard<std::recursive_mutex> cacheLock(
                        thisSharedPtr->_localCapabilitiesDirectoryStore->getCacheLock());
                thisSharedPtr->capabilitiesReceived(
                        result,
                        thisSharedPtr->_localCapabilitiesDirectoryStore->getLocalCapabilities(
                                participantId),
                        callback,
                        discoveryScope);
            }
        };

        auto onRuntimeError = [callback,
                               participantId](const exceptions::JoynrRuntimeException& exception) {
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

// base lookup by domains and interface
void LocalCapabilitiesDirectory::lookup(const std::vector<std::string>& domains,
                                        const std::string& interfaceName,
                                        const std::vector<std::string>& gbids,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback,
                                        const joynr::types::DiscoveryQos& discoveryQos)
{
    std::vector<InterfaceAddress> interfaceAddresses =
            LCDUtil::getInterfaceAddresses(domains, interfaceName);

    // get the local and cached entries
    bool receiverCalled = _localCapabilitiesDirectoryStore->getLocalAndCachedCapabilities(
            interfaceAddresses, discoveryQos, gbids, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entries in the global capabilities directory
        auto onSuccess = [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                          interfaceAddresses,
                          callback,
                          discoveryQos,
                          replaceGdeGbid = LCDUtil::containsOnlyEmptyString(gbids)](
                                 std::vector<joynr::types::GlobalDiscoveryEntry> result) {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::recursive_mutex> cacheLock(
                        thisSharedPtr->_localCapabilitiesDirectoryStore->getCacheLock());
                std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                if (!(thisSharedPtr->_lcdPendingLookupsHandler.isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    if (replaceGdeGbid) {
                        LCDUtil::replaceGbidWithEmptyString(result);
                    }
                    thisSharedPtr->capabilitiesReceived(
                            result,
                            thisSharedPtr->_localCapabilitiesDirectoryStore->getLocalCapabilities(
                                    interfaceAddresses),
                            callback,
                            discoveryQos.getDiscoveryScope());
                    thisSharedPtr->_lcdPendingLookupsHandler.callbackCalled(
                            interfaceAddresses, callback);
                }
            }
        };

        auto onError = [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                        interfaceAddresses,
                        domain = domains[0],
                        interfaceName,
                        callback,
                        discoveryQos](const types::DiscoveryError::Enum& error) {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                JOYNR_LOG_DEBUG(logger(),
                                "Global lookup for domain {} and interface {} failed with "
                                "DiscoveryError: {}",
                                domain,
                                interfaceName,
                                types::DiscoveryError::getLiteral(error));
                std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                if (!(thisSharedPtr->_lcdPendingLookupsHandler.isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    callback->onError(error);
                    thisSharedPtr->_lcdPendingLookupsHandler.callbackCalled(
                            interfaceAddresses, callback);
                }
            }
        };

        auto onRuntimeError = [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                               interfaceAddresses,
                               domain = domains[0],
                               interfaceName,
                               callback,
                               discoveryQos](const exceptions::JoynrRuntimeException& exception) {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                JOYNR_LOG_DEBUG(logger(),
                                "Global lookup for domain {} and interface {} failed with "
                                "exception: {} ({})",
                                domain,
                                interfaceName,
                                exception.getMessage(),
                                exception.TYPE_NAME());
                std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                if (!(thisSharedPtr->_lcdPendingLookupsHandler.isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    callback->onError(types::DiscoveryError::INTERNAL_ERROR);
                    thisSharedPtr->_lcdPendingLookupsHandler.callbackCalled(
                            interfaceAddresses, callback);
                }
            }
        };

        if (discoveryQos.getDiscoveryScope() == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
            std::lock_guard<std::mutex> lock(_pendingLookupsLock);
            _lcdPendingLookupsHandler.registerPendingLookup(interfaceAddresses, callback);
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

bool LocalCapabilitiesDirectory::hasPendingLookups()
{
    std::lock_guard<std::mutex> lock(_pendingLookupsLock);
    return _lcdPendingLookupsHandler.hasPendingLookups();
}

std::vector<types::DiscoveryEntryWithMetaInfo> LocalCapabilitiesDirectory::
        registerReceivedCapabilities(
                const std::vector<types::GlobalDiscoveryEntry>&& capabilityEntries)
{
    std::vector<types::DiscoveryEntryWithMetaInfo> validGlobalEntries;
    for (auto currentEntry : capabilityEntries) {
        const std::string& serializedAddress = currentEntry.getAddress();
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

        const bool isGloballyVisible = LCDUtil::isGlobal(currentEntry);
        if (auto messageRouterSharedPtr = _messageRouter.lock()) {
            constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
            const bool isSticky = false;
            Future<void> future;
            auto onSuccess = [&]() { future.onSuccess(); };
            auto onError = [&](const joynr::exceptions::ProviderRuntimeException& e) {
                future.onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
            };
            messageRouterSharedPtr->addNextHop(currentEntry.getParticipantId(),
                                               address,
                                               isGloballyVisible,
                                               expiryDateMs,
                                               isSticky,
                                               onSuccess,
                                               onError);

            try {
                future.get();

                types::DiscoveryEntryWithMetaInfo convertedEntry =
                        LCDUtil::convert(false, currentEntry);
                validGlobalEntries.push_back(std::move(convertedEntry));

                _localCapabilitiesDirectoryStore->insertRemoteEntriesIntoGlobalCache(
                        currentEntry, address, _knownGbids);

            } catch (const joynr::exceptions::JoynrException& e) {
                JOYNR_LOG_WARN(logger(),
                               "Failed to register capability entry for participantId: {}, "
                               "addNextHop failed: {}",
                               currentEntry.getParticipantId(),
                               e.what());
            }
        } else {
            JOYNR_LOG_FATAL(logger(),
                            "could not addNextHop {} to {} because messageRouter is not available",
                            currentEntry.getParticipantId(),
                            serializedAddress);
            return {};
        }
    }
    return validGlobalEntries;
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
    auto onErrorWrapper = [onError = std::move(onError),
                           discoveryEntry,
                           participantId = discoveryEntry.getParticipantId()](
                                  const types::DiscoveryError::Enum& errorEnum) {
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
    auto result = LCDUtil::validateGbids(gbids, _knownGbidsSet);
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
    }
    const auto gbidsForAdd = gbids.size() == 0 ? _knownGbids : gbids;
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
    auto onErrorWrapper = [onError = std::move(onError), domains = domains, interfaceName](
                                  const types::DiscoveryError::Enum& error) {
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

    auto result = LCDUtil::validateGbids(gbids, _knownGbidsSet);
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
    auto onErrorWrapper = [onError = std::move(onError),
                           participantId](const types::DiscoveryError::Enum& error) {
        onError(exceptions::ProviderRuntimeException(
                fmt::format("Error looking up provider {} in all known backends: {}",
                            participantId,
                            types::DiscoveryError::getLiteral(error))));
    };

    auto onSuccessWrapper =
            [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
             onSuccess = std::move(onSuccess),
             onError,
             participantId](const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities) {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    if (capabilities.empty()) {
                        joynr::exceptions::ProviderRuntimeException exception(
                                "No capabilities found for participantId \"" + participantId +
                                "\" and default GBID: " + thisSharedPtr->_knownGbids[0]);
                        onError(exception);
                        return;
                    }
                    if (capabilities.size() > 1) {
                        JOYNR_LOG_FATAL(
                                thisSharedPtr->logger(),
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
    auto result = LCDUtil::validateGbids(gbids, _knownGbidsSet);
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
    auto onSuccessWrapper =
            [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
             onSuccess = std::move(onSuccess),
             onError,
             participantId,
             gbidsForLookup](const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities) {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    if (capabilities.empty()) {
                        const std::string gbidString = boost::algorithm::join(gbidsForLookup, ", ");
                        JOYNR_LOG_DEBUG(
                                logger(),
                                "participantId {} has no capability entry "
                                "(DiscoveryError::NO_ENTRY_FOR_PARTICIPANT) for GBIDs: >{}<",
                                participantId,
                                gbidString);
                        onError(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
                        return;
                    }
                    if (capabilities.size() > 1) {
                        JOYNR_LOG_FATAL(
                                thisSharedPtr->logger(),
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
    std::ignore = onError;
    {
        std::unique_lock<std::recursive_mutex> removeLock(
                _localCapabilitiesDirectoryStore->getCacheLock());

        bool awaitGlobalRegistration = _localCapabilitiesDirectoryStore->getAwaitGlobalRegistration(
                participantId, removeLock);

        boost::optional<types::DiscoveryEntry> optionalEntry =
                _localCapabilitiesDirectoryStore->lookupLocalEntry(participantId);
        if (optionalEntry && !LCDUtil::isGlobal(optionalEntry.get())) {
            _localCapabilitiesDirectoryStore->removeLocallyRegisteredParticipant(
                    participantId, removeLock);
            JOYNR_LOG_INFO(
                    logger(),
                    "Removed locally registered participantId {}: #localCapabilities {}, "
                    "#registeredGlobalCapabilities: {}, #globalLookupCache: {}",
                    participantId,
                    _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilitiesCount(
                            removeLock),
                    _localCapabilitiesDirectoryStore->countGlobalCapabilities(),
                    _localCapabilitiesDirectoryStore->getGlobalCachedCapabilitiesCount(removeLock));
        } else {
            auto onGlobalRemoveSuccess = [participantId,
                                          awaitGlobalRegistration,
                                          lCDStoreWeakPtr = joynr::util::as_weak_ptr(
                                                  _localCapabilitiesDirectoryStore)](
                                                 const std::vector<std::string>& participantGbids) {
                const std::string gbidString = boost::algorithm::join(participantGbids, ", ");
                auto lCDStoreSharedPtr = lCDStoreWeakPtr.lock();
                if (lCDStoreSharedPtr && awaitGlobalRegistration) {
                    std::unique_lock<std::recursive_mutex> cacheLock(
                            lCDStoreSharedPtr->getCacheLock());
                    lCDStoreSharedPtr->removeParticipant(participantId, cacheLock);
                    JOYNR_LOG_INFO(
                            logger(),
                            "Removed globally registered participantId: {} from GBIDs: >{}< "
                            "#localCapabilities {}, "
                            "#registeredGlobalCapabilities: {}, #globalLookupCache: {}",
                            participantId,
                            gbidString,
                            lCDStoreSharedPtr->getLocallyRegisteredCapabilitiesCount(cacheLock),
                            lCDStoreSharedPtr->countGlobalCapabilities(),
                            lCDStoreSharedPtr->getGlobalCachedCapabilitiesCount(cacheLock));
                    return;
                }
                JOYNR_LOG_INFO(logger(),
                               "Removed globally registered participantId: {} from GBIDs: >{}< ",
                               participantId,
                               gbidString);
            };
            auto onApplicationError = [participantId,
                                       awaitGlobalRegistration,
                                       lCDStoreWeakPtr = joynr::util::as_weak_ptr(
                                               _localCapabilitiesDirectoryStore)](
                                              const types::DiscoveryError::Enum& error,
                                              const std::vector<std::string>& participantGbids) {
                using namespace types;
                const std::string gbidString = boost::algorithm::join(participantGbids, ", ");
                switch (error) {
                case DiscoveryError::Enum::NO_ENTRY_FOR_PARTICIPANT:
                case DiscoveryError::Enum::NO_ENTRY_FOR_SELECTED_BACKENDS: {
                    auto lCDStoreSharedPtr = lCDStoreWeakPtr.lock();
                    if (lCDStoreSharedPtr && awaitGlobalRegistration) {
                        std::unique_lock<std::recursive_mutex> cacheLock(
                                lCDStoreSharedPtr->getCacheLock());
                        JOYNR_LOG_WARN(logger(),
                                       "Error removing participantId {} globally for GBIDs >{}<: "
                                       "{}. Removing local entry.",
                                       participantId,
                                       gbidString,
                                       types::DiscoveryError::getLiteral(error));
                        lCDStoreSharedPtr->removeParticipant(participantId, cacheLock);
                        JOYNR_LOG_INFO(
                                logger(),
                                "After removal of participantId {}: #localCapabilities {}, "
                                "#registeredGlobalCapabilities: {}, #globalLookupCache: {}",
                                participantId,
                                lCDStoreSharedPtr->getLocallyRegisteredCapabilitiesCount(cacheLock),
                                lCDStoreSharedPtr->countGlobalCapabilities(),
                                lCDStoreSharedPtr->getGlobalCachedCapabilitiesCount(cacheLock));
                        return;
                    }
                    JOYNR_LOG_WARN(logger(),
                                   "Error removing participantId {} globally for GBIDs >{}<: "
                                   "{}",
                                   participantId,
                                   gbidString,
                                   types::DiscoveryError::getLiteral(error));
                    break;
                }
                case DiscoveryError::Enum::INVALID_GBID:
                case DiscoveryError::Enum::UNKNOWN_GBID:
                case DiscoveryError::Enum::INTERNAL_ERROR:
                default:
                    JOYNR_LOG_WARN(logger(),
                                   "Error removing participantId {} globally for GBIDs >{}<: {}",
                                   participantId,
                                   gbidString,
                                   types::DiscoveryError::getLiteral(error));
                    break;
                }
            };
            auto onRuntimeError = [participantId](
                                          const exceptions::JoynrRuntimeException& exception,
                                          const std::vector<std::string>& participantGbids) {
                const std::string gbidString = boost::algorithm::join(participantGbids, ", ");
                JOYNR_LOG_WARN(logger(),
                               "Failed to remove participantId {} globally for GBIDs >{}<: "
                               "{} ({})",
                               participantId,
                               gbidString,
                               exception.getMessage(),
                               exception.getTypeName());
            };

            auto gbidsToRemove = _localCapabilitiesDirectoryStore->getGbidsForParticipantId(
                    participantId, removeLock);
            // If last 'add' for that participant was invoked with awaitGlobalRegistration==false
            // delete the entry, then schedule removal at JDS
            if (!awaitGlobalRegistration) {
                const std::string gbidString = boost::algorithm::join(gbidsToRemove, ", ");
                _localCapabilitiesDirectoryStore->removeParticipant(participantId, removeLock);
                JOYNR_LOG_INFO(
                        logger(),
                        "Removed local entries for participantId: {}. GBIDs: >{}< "
                        "#localCapabilities {}, "
                        "#registeredGlobalCapabilities: {}, #globalLookupCache: {}",
                        participantId,
                        gbidString,
                        _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilitiesCount(
                                removeLock),
                        _localCapabilitiesDirectoryStore->countGlobalCapabilities(),
                        _localCapabilitiesDirectoryStore->getGlobalCachedCapabilitiesCount(
                                removeLock));
            }
            _globalCapabilitiesDirectoryClient->remove(participantId,
                                                       std::move(gbidsToRemove),
                                                       std::move(onGlobalRemoveSuccess),
                                                       std::move(onApplicationError),
                                                       std::move(onRuntimeError));
        }
    }
    if (onSuccess) {
        onSuccess();
    }
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
        _checkExpiredDiscoveryEntriesTimer.async_wait(
                [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
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

    {
        std::unique_lock<std::recursive_mutex> discoveryEntryExpiryCheckLock(
                _localCapabilitiesDirectoryStore->getCacheLock());

        auto removedLocalCapabilities =
                _localCapabilitiesDirectoryStore->removeExpiredLocallyRegisteredCapabilities(
                        discoveryEntryExpiryCheckLock);
        auto removedGlobalCapabilities =
                _localCapabilitiesDirectoryStore->removeExpiredCapabilitiesFromGlobalCache(
                        discoveryEntryExpiryCheckLock);

        if (!removedLocalCapabilities.empty() || !removedGlobalCapabilities.empty()) {
            if (auto messageRouterSharedPtr = _messageRouter.lock()) {
                JOYNR_LOG_INFO(
                        logger(),
                        "Following discovery entries expired: local: {}, "
                        "#localCapabilities: {}, global: {}, #globalLookupCache: {}",
                        LCDUtil::joinToString(removedLocalCapabilities),
                        _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilitiesCount(
                                discoveryEntryExpiryCheckLock),
                        LCDUtil::joinToString(removedGlobalCapabilities),
                        _localCapabilitiesDirectoryStore->getGlobalCachedCapabilitiesCount(
                                discoveryEntryExpiryCheckLock));

                for (const auto& capability :
                     boost::join(removedLocalCapabilities, removedGlobalCapabilities)) {
                    messageRouterSharedPtr->removeNextHop(capability.getParticipantId());
                    _localCapabilitiesDirectoryStore->eraseParticipantIdToGbidMapping(
                            capability.getParticipantId(), discoveryEntryExpiryCheckLock);
                }
            } else {
                JOYNR_LOG_FATAL(logger(),
                                "could not call removeNextHop because messageRouter is "
                                "not available");
            }
        }
    }

    scheduleCleanupTimer();
}

void LocalCapabilitiesDirectory::removeStaleProvidersOfClusterController(
        const std::int64_t& clusterControllerStartDateMs,
        const std::string gbid)
{
    auto onSuccess = [ccId = _clusterControllerId, clusterControllerStartDateMs, gbid]() {
        JOYNR_LOG_TRACE(logger(),
                        "RemoveStale(ccId={}, gbid={}, maxLastSeenDateMs={}) succeeded.",
                        ccId,
                        gbid,
                        clusterControllerStartDateMs);
    };

    auto onRuntimeError = [ccId = _clusterControllerId,
                           clusterControllerStartDateMs,
                           gbid,
                           thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
                                  const joynr::exceptions::JoynrRuntimeException& error) {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            std::int64_t durationForRetryCallMs = 3600000;
            JOYNR_LOG_ERROR(logger(),
                            "RemoveStale(ccId={}, gbid={}, maxLastSeenDateMs={}) failed: {}",
                            ccId,
                            gbid,
                            clusterControllerStartDateMs,
                            error.getMessage());
            if (TimePoint::now().toMilliseconds() - clusterControllerStartDateMs <=
                durationForRetryCallMs) {
                thisSharedPtr->removeStaleProvidersOfClusterController(
                        clusterControllerStartDateMs, gbid);
            } else {
                JOYNR_LOG_ERROR(logger(),
                                "RemoveStale(ccId={}, gbid={}, maxLastSeenDateMs={}) was not "
                                "retried because a duration to retry (60 minutes) after a start of "
                                "cluster controller was exceeded.",
                                ccId,
                                gbid,
                                clusterControllerStartDateMs);
            }
        }
    };
    _globalCapabilitiesDirectoryClient->removeStale(_clusterControllerId,
                                                    clusterControllerStartDateMs,
                                                    gbid,
                                                    std::move(onSuccess),
                                                    std::move(onRuntimeError));
}

void LocalCapabilitiesDirectory::removeStaleProvidersOfClusterController(
        const std::int64_t& clusterControllerStartDateMs)
{
    for (const auto& gbid : _knownGbids) {
        removeStaleProvidersOfClusterController(clusterControllerStartDateMs, gbid);
    }
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

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
#include "joynr/InterfaceAddress.h"
#include "joynr/TimePoint.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderScope.h"
#include "joynr/LCDUtil.h"

#include "IGlobalCapabilitiesDirectoryClient.h"

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
          _isLocalCapabilitiesDirectoryPersistencyEnabled(
                  clusterControllerSettings.isLocalCapabilitiesDirectoryPersistencyEnabled()),
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

    std::vector<std::string> participantIds = getParticipantIdsToTouch();

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
        std::vector<std::string> gbids =
                _localCapabilitiesDirectoryStore->getGbidsForParticipantId(participantIdToTouch);

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

        auto onSuccess = [ ccId = _clusterControllerId, participantIdsToTouch, gbid ]()
        {
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

        auto onError = [ ccId = _clusterControllerId, participantIdsToTouch, gbid ](
                const joynr::exceptions::JoynrRuntimeException& error)
        {
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
    _reAddAllGlobalEntriesTimer
            .async_wait([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
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
        if (isGloballyVisible) {
            _localCapabilitiesDirectoryStore->insertInGlobalLookupCache(discoveryEntry, gbids);
        }
        // register locally
        _localCapabilitiesDirectoryStore->insertInLocalCapabilitiesStorage(discoveryEntry);

        updatePersistedFile();
        {
            std::lock_guard<std::mutex> lock(_pendingLookupsLock);
            InterfaceAddress interfaceAddress(
                    discoveryEntry.getDomain(), discoveryEntry.getInterfaceName());
            _lcdPendingLookupsHandler.callPendingLookups(
                    interfaceAddress,
                    _localCapabilitiesDirectoryStore->searchLocalCache({interfaceAddress}));
        }
    }

    // register globally
    if (isGloballyVisible) {
        types::GlobalDiscoveryEntry globalDiscoveryEntry =
                LCDUtil::toGlobalDiscoveryEntry(discoveryEntry, _localAddress);

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

        std::function<void()> onSuccessWrapper = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            globalDiscoveryEntry,
            awaitGlobalRegistration,
            gbids,
            onSuccess
        ]()
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::recursive_mutex> cacheInsertionLock(
                        thisSharedPtr->_localCapabilitiesDirectoryStore->getCacheLock());
                JOYNR_LOG_INFO(
                        logger(),
                        "Global capability '{}' added successfully for GBIDs >{}<, "
                        "#registeredGlobalCapabilities {}",
                        globalDiscoveryEntry.toString(),
                        boost::algorithm::join(gbids, ", "),
                        thisSharedPtr->_localCapabilitiesDirectoryStore->countGlobalCapabilities());
                if (awaitGlobalRegistration) {
                    thisSharedPtr->_localCapabilitiesDirectoryStore->insertInGlobalLookupCache(
                            globalDiscoveryEntry, gbids);
                    thisSharedPtr->_localCapabilitiesDirectoryStore
                            ->insertInLocalCapabilitiesStorage(globalDiscoveryEntry);
                    if (onSuccess) {
                        onSuccess();
                    }

                    thisSharedPtr->updatePersistedFile();
                    {
                        std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                        InterfaceAddress interfaceAddress(globalDiscoveryEntry.getDomain(),
                                                          globalDiscoveryEntry.getInterfaceName());
                        thisSharedPtr->_lcdPendingLookupsHandler.callPendingLookups(
                                interfaceAddress,
                                thisSharedPtr->_localCapabilitiesDirectoryStore->searchLocalCache(
                                        {interfaceAddress}));
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

void LocalCapabilitiesDirectory::triggerGlobalProviderReregistration(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;

    {
        std::lock_guard<std::recursive_mutex> providerReregistrationLock(
                _localCapabilitiesDirectoryStore->getCacheLock());
        JOYNR_LOG_DEBUG(logger(), "triggerGlobalProviderReregistration");
        std::vector<types::DiscoveryEntry> entries;
        const std::int64_t now =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
        const std::int64_t newExpiryDateMs = now + _defaultExpiryIntervalMs;

        // copy existing global entries, update lastSeenDateMs and
        // increase expiryDateMs unless it already references a time
        // which is beyond newExpiryDate/updatedExpiryDate
        for (auto capability :
             *(_localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities())) {
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
                auto foundGbids =
                        _localCapabilitiesDirectoryStore->getGbidsForParticipantId(participantId);
                if (!foundGbids.empty()) {
                    // update local store
                    _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()->insert(
                            capability, foundGbids);
                    // update global cache
                    _localCapabilitiesDirectoryStore->getGlobalLookupCache()->insert(capability);
                    // send entries to JDS again
                    auto onApplicationError =
                            [participantId, foundGbids](const types::DiscoveryError::Enum& error) {
                        JOYNR_LOG_WARN(logger(),
                                       "Global provider reregistration for participantId {} and "
                                       "gbids >{}< failed: {} (DiscoveryError)",
                                       participantId,
                                       boost::algorithm::join(foundGbids, ", "),
                                       types::DiscoveryError::getLiteral(error));
                    };
                    auto onRuntimeError = [participantId, foundGbids](
                            const exceptions::JoynrRuntimeException& exception) {
                        JOYNR_LOG_WARN(logger(),
                                       "Global provider reregistration for participantId {} and "
                                       "gbids >{}< failed: {} ({})",
                                       participantId,
                                       boost::algorithm::join(foundGbids, ", "),
                                       exception.getMessage(),
                                       exception.getTypeName());
                    };
                    _globalCapabilitiesDirectoryClient->add(
                            LCDUtil::toGlobalDiscoveryEntry(capability, _localAddress),
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
                _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()->insert(
                        capability);
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
        globalEntries = LCDUtil::filterDuplicates(
                std::move(localEntriesWithMetaInfo), std::move(globalEntries));
    }
    callback->capabilitiesReceived(std::move(globalEntries));
}

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
        auto onSuccess = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            participantId,
            discoveryScope = discoveryQos.getDiscoveryScope(),
            callback,
            replaceGdeGbid = LCDUtil::containsOnlyEmptyString(gbids)
        ](std::vector<joynr::types::GlobalDiscoveryEntry> result)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                if (replaceGdeGbid) {
                    LCDUtil::replaceGbidWithEmptyString(result);
                }
                thisSharedPtr->capabilitiesReceived(
                        result,
                        thisSharedPtr->_localCapabilitiesDirectoryStore->getCachedLocalCapabilities(
                                participantId),
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
    bool receiverCalled = _localCapabilitiesDirectoryStore->getLocalAndCachedCapabilities(
            interfaceAddresses, discoveryQos, gbids, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entries in the global capabilities directory
        auto onSuccess = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            interfaceAddresses,
            callback,
            discoveryQos,
            replaceGdeGbid = LCDUtil::containsOnlyEmptyString(gbids)
        ](std::vector<joynr::types::GlobalDiscoveryEntry> result)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::mutex> lock(thisSharedPtr->_pendingLookupsLock);
                if (!(thisSharedPtr->_lcdPendingLookupsHandler.isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    if (replaceGdeGbid) {
                        LCDUtil::replaceGbidWithEmptyString(result);
                    }
                    thisSharedPtr->capabilitiesReceived(
                            result,
                            thisSharedPtr->_localCapabilitiesDirectoryStore
                                    ->getCachedLocalCapabilities(interfaceAddresses),
                            callback,
                            discoveryQos.getDiscoveryScope());
                }
                thisSharedPtr->_lcdPendingLookupsHandler.callbackCalled(
                        interfaceAddresses, callback);
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
                if (!(thisSharedPtr->_lcdPendingLookupsHandler.isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    callback->onError(error);
                }
                thisSharedPtr->_lcdPendingLookupsHandler.callbackCalled(
                        interfaceAddresses, callback);
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
                if (!(thisSharedPtr->_lcdPendingLookupsHandler.isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    callback->onError(types::DiscoveryError::INTERNAL_ERROR);
                }
                thisSharedPtr->_lcdPendingLookupsHandler.callbackCalled(
                        interfaceAddresses, callback);
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
    return _lcdPendingLookupsHandler.hasPendingLookups();
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
        const bool isGloballyVisible = LCDUtil::isGlobal(currentEntry);
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
        _localCapabilitiesDirectoryStore->insertInGlobalLookupCache(
                std::move(currentEntry), std::move(gbids));
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
        return;
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
    std::ignore = onError;
    {
        std::lock_guard<std::recursive_mutex> removeLock(
                _localCapabilitiesDirectoryStore->getCacheLock());

        boost::optional<types::DiscoveryEntry> optionalEntry =
                _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()
                        ->lookupByParticipantId(participantId);

        if (optionalEntry.has_value() && !LCDUtil::isGlobal(optionalEntry.get())) {
            JOYNR_LOG_INFO(
                    logger(), "Removing locally registered participantId: {}", participantId);
            _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()
                    ->removeByParticipantId(participantId);
        } else {
            auto foundGbids =
                    _localCapabilitiesDirectoryStore->getGbidsForParticipantId(participantId);
            if (foundGbids.empty()) {
                JOYNR_LOG_FATAL(logger(),
                                "Global remove failed because participantId to GBIDs mapping is "
                                "missing for participantId {}",
                                participantId);
                exceptions::ProviderRuntimeException exception(fmt::format(
                        "Global remove failed because participantId to GBIDs mapping is "
                        "missing for participantId {}",
                        participantId));
                if (onError) {
                    onError(exception);
                }
                return;
            } else {
                auto onGlobalRemoveSuccess = [
                    foundGbids,
                    optionalEntry,
                    participantId,
                    lCDStoreWeakPtr = joynr::util::as_weak_ptr(_localCapabilitiesDirectoryStore)
                ](/*add gbis here to log later*/)
                {
                    if (auto lCDStoreSharedPtr = lCDStoreWeakPtr.lock()) {
                        const std::string gbidString = boost::algorithm::join(foundGbids, ", ");
                        JOYNR_LOG_INFO(
                                logger(),
                                "Removing globally registered participantId: {} from GBIDs: >{}<",
                                participantId,
                                gbidString);
                        lCDStoreSharedPtr->eraseParticipantIdToGbidMapping(participantId);
                        lCDStoreSharedPtr->getGlobalLookupCache()->removeByParticipantId(
                                participantId);
                        JOYNR_LOG_INFO(logger(),
                                       "Removing locally registered participantId: {}",
                                       participantId);
                        lCDStoreSharedPtr->getLocallyRegisteredCapabilities()
                                ->removeByParticipantId(participantId);
                    }
                };

                auto onApplicationError =
                        [participantId, foundGbids](const types::DiscoveryError::Enum& error) {
                    JOYNR_LOG_WARN(logger(),
                                   "Error removing participantId {} globally for GBIDs >{}<: {}",
                                   participantId,
                                   boost::algorithm::join(foundGbids, ", "),
                                   types::DiscoveryError::getLiteral(error));
                };
                auto onRuntimeError = [participantId, foundGbids](
                        const exceptions::JoynrRuntimeException& exception) {
                    JOYNR_LOG_WARN(
                            logger(),
                            "Failed to remove participantId {} globally for GBIDs >{}<: {} ({})",
                            participantId,
                            boost::algorithm::join(foundGbids, ", "),
                            exception.getMessage(),
                            exception.getTypeName());
                };

                _globalCapabilitiesDirectoryClient->remove(participantId,
                                                           foundGbids,
                                                           std::move(onGlobalRemoveSuccess),
                                                           std::move(onApplicationError),
                                                           std::move(onRuntimeError));
            }
        }

        JOYNR_LOG_INFO(logger(),
                       "After removal of participantId {}: #localCapabilities {}, "
                       "#registeredGlobalCapabilities: {}, #globalLookupCache: {}",
                       participantId,
                       _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()->size(),
                       _localCapabilitiesDirectoryStore->countGlobalCapabilities(),
                       _localCapabilitiesDirectoryStore->getGlobalLookupCache()->size());
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
        std::lock_guard<std::recursive_mutex> filePersistencyStorageLock(
                _localCapabilitiesDirectoryStore->getCacheLock());
        joynr::util::saveStringToFile(
                fileName,
                joynr::serializer::serializeToJson(
                        _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()));
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

    std::lock_guard<std::recursive_mutex> filePersistencyRetrievalLock(
            _localCapabilitiesDirectoryStore->getCacheLock());

    try {
        std::shared_ptr<capabilities::Storage> locallyRegisteredCapabilities =
                _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities();
        joynr::serializer::deserializeFromJson(locallyRegisteredCapabilities, jsonString);
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }

    // insert all global capability entries into global cache
    for (const auto& entry :
         *(_localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities())) {
        if (entry.getQos().getScope() == types::ProviderScope::GLOBAL) {
            _localCapabilitiesDirectoryStore->insertInGlobalLookupCache(entry, entry.gbids);
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
        std::lock_guard<std::recursive_mutex> discoveryEntryExpiryCheckLock(
                _localCapabilitiesDirectoryStore->getCacheLock());

        auto removedLocalCapabilities =
                _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()
                        ->removeExpired();
        auto removedGlobalCapabilities =
                _localCapabilitiesDirectoryStore->getGlobalLookupCache()->removeExpired();

        if (!removedLocalCapabilities.empty() || !removedGlobalCapabilities.empty()) {
            fileUpdateRequired = true;
            if (auto messageRouterSharedPtr = _messageRouter.lock()) {
                JOYNR_LOG_INFO(logger(),
                               "Following discovery entries expired: local: {}, "
                               "#localCapabilities: {}, global: {}, #globalLookupCache: {}",
                               LCDUtil::joinToString(removedLocalCapabilities),
                               _localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities()
                                       ->size(),
                               LCDUtil::joinToString(removedGlobalCapabilities),
                               _localCapabilitiesDirectoryStore->getGlobalLookupCache()->size());

                for (const auto& capability :
                     boost::join(removedLocalCapabilities, removedGlobalCapabilities)) {
                    messageRouterSharedPtr->removeNextHop(capability.getParticipantId());
                    _localCapabilitiesDirectoryStore->eraseParticipantIdToGbidMapping(
                            capability.getParticipantId());
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

void LocalCapabilitiesDirectory::removeStaleProvidersOfClusterController(
        const std::int64_t& clusterControllerStartDateMs,
        const std::string gbid)
{
    auto onSuccess = [ ccId = _clusterControllerId, clusterControllerStartDateMs, gbid ]()
    {
        JOYNR_LOG_TRACE(logger(),
                        "RemoveStale(ccId={}, gbid={}, maxLastSeenDateMs={}) succeeded.",
                        ccId,
                        gbid,
                        clusterControllerStartDateMs);
    };

    auto onRuntimeError = [
        ccId = _clusterControllerId,
        clusterControllerStartDateMs,
        gbid,
        thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())
    ](const joynr::exceptions::JoynrRuntimeException& error)
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            JOYNR_LOG_ERROR(logger(),
                            "RemoveStale(ccId={}, gbid={}, maxLastSeenDateMs={}) failed: {}",
                            ccId,
                            gbid,
                            clusterControllerStartDateMs,
                            error.getMessage());
            thisSharedPtr->removeStaleProvidersOfClusterController(
                    clusterControllerStartDateMs, gbid);
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
    for (const auto gbid : _knownGbids) {
        removeStaleProvidersOfClusterController(clusterControllerStartDateMs, gbid);
    }
}

std::vector<std::string> LocalCapabilitiesDirectory::getParticipantIdsToTouch()
{
    std::vector<std::string> participantIds;
    const std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch()).count();
    const std::int64_t newExpiryDateMs = now + _defaultExpiryIntervalMs;
    {
        std::lock_guard<std::recursive_mutex> expiryDateUpdateLock(
                _localCapabilitiesDirectoryStore->getCacheLock());
        for (auto entry : *(_localCapabilitiesDirectoryStore->getLocallyRegisteredCapabilities())) {
            bool refresh_entry = false;
            if (now > entry.getLastSeenDateMs()) {
                entry.setLastSeenDateMs(now);
                refresh_entry = true;
            }
            if (newExpiryDateMs > entry.getExpiryDateMs()) {
                entry.setExpiryDateMs(newExpiryDateMs);
                refresh_entry = true;
            }
            if (refresh_entry) {
                if (entry.getQos().getScope() == types::ProviderScope::GLOBAL) {
                    participantIds.push_back(entry.getParticipantId());
                }
                _localCapabilitiesDirectoryStore->insertInLocalCapabilitiesStorage(entry);
            }
        }
        for (auto participantId : participantIds) {
            auto globalEntry =
                    _localCapabilitiesDirectoryStore->getGlobalLookupCache()->lookupByParticipantId(
                            participantId);
            if (!globalEntry) {
                continue;
            }
            bool refresh_entry = false;
            if (now > globalEntry->getLastSeenDateMs()) {
                globalEntry->setLastSeenDateMs(now);
                refresh_entry = true;
            }
            if (newExpiryDateMs > globalEntry->getExpiryDateMs()) {
                globalEntry->setExpiryDateMs(newExpiryDateMs);
                refresh_entry = true;
            }
            if (refresh_entry) {
                _localCapabilitiesDirectoryStore->insertInGlobalLookupCache(
                        *globalEntry,
                        _localCapabilitiesDirectoryStore->getGbidsForParticipantId(
                                globalEntry->getParticipantId()));
            }
        }
    }
    return participantIds;
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

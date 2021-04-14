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
#include "joynr/Arbitrator.h"

#include <cassert>
#include <vector>

#include <boost/algorithm/string/join.hpp>

#include "joynr/Future.h"
#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/NoCompatibleProviderFoundException.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryScope.h"

namespace joynr
{
Arbitrator::Arbitrator(
        const std::string& domain,
        const std::string& interfaceName,
        const joynr::types::Version& interfaceVersion,
        std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
        const DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::unique_ptr<const ArbitrationStrategyFunction> arbitrationStrategyFunction)
        : std::enable_shared_from_this<Arbitrator>(),
          _pendingFutureMutex(),
          _pendingFuture(),
          _discoveryProxy(discoveryProxy),
          _gbids(gbids),
          _gbidString(boost::algorithm::join(gbids, ", ")),
          _discoveryQos(discoveryQos),
          _systemDiscoveryQos(_discoveryQos.getCacheMaxAgeMs(),
                              _discoveryQos.getDiscoveryTimeoutMs(),
                              _discoveryQos.getDiscoveryScope(),
                              _discoveryQos.getProviderMustSupportOnChange()),
          _domains({domain}),
          _serializedDomainsList(boost::algorithm::join(_domains, ", ")),
          _interfaceName(interfaceName),
          _interfaceVersion(interfaceVersion),
          _discoveredIncompatibleVersions(),
          _arbitrationError("Arbitration could not be finished in time."),
          _arbitrationStrategyFunction(std::move(arbitrationStrategyFunction)),
          _semaphore(0),
          _arbitrationFinished(false),
          _arbitrationFailedForever(false),
          _arbitrationRunning(false),
          _arbitrationStopped(false),
          _arbitrationThread(),
          _startTimePoint(),
          _onceFlag(),
          _filterByVersionAndArbitrationStrategy(true)
{
}

Arbitrator::~Arbitrator()
{
    stopArbitration();
}

void Arbitrator::startArbitration(
        std::function<void(const joynr::ArbitrationResult& arbitrationResult)> onSuccess,
        std::function<void(const exceptions::DiscoveryException& exception)> onError,
        bool filterByVersionAndArbitrationStrategy)
{
    if (_arbitrationRunning) {
        JOYNR_LOG_ERROR(logger(),
                        "Arbitration already running for domain = [{}], interface = {}, GBIDs = "
                        ">{}<. A second "
                        "arbitration will not be started.",
                        _serializedDomainsList,
                        _interfaceName,
                        _gbidString);
        return;
    }

    _startTimePoint = std::chrono::steady_clock::now();
    JOYNR_LOG_INFO(
            logger(),
            "Arbitration started for domain = [{}], interface = {}, GBIDs = >{}<, version = {}.{}.",
            _serializedDomainsList,
            _interfaceName,
            _gbidString,
            std::to_string(_interfaceVersion.getMajorVersion()),
            std::to_string(_interfaceVersion.getMinorVersion()));

    _arbitrationRunning = true;
    _arbitrationStopped = false;

    _onSuccessCallback = onSuccess;
    _onErrorCallback = onError;

    _filterByVersionAndArbitrationStrategy = filterByVersionAndArbitrationStrategy;

    _arbitrationThread =
            std::thread([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())]() {
                auto thisSharedPtr = thisWeakPtr.lock();
                if (!thisSharedPtr) {
                    return;
                }

                thisSharedPtr->_arbitrationFinished = false;
                thisSharedPtr->_arbitrationFailedForever = false;

                JOYNR_LOG_TRACE(
                        logger(),
                        "Entering arbitration thread for domain: [{}], interface: {}, GBIDs = >{}<",
                        thisSharedPtr->_serializedDomainsList,
                        thisSharedPtr->_interfaceName,
                        thisSharedPtr->_gbidString);

                while (!thisSharedPtr->_arbitrationStopped) {
                    thisSharedPtr->attemptArbitration();

                    // exit if arbitration has finished successfully
                    if (thisSharedPtr->_arbitrationFinished) {
                        thisSharedPtr->assertNoPendingFuture();
                        return;
                    }

                    // check if we should break the loop and report errors to the user
                    if (thisSharedPtr->_arbitrationStopped) {
                        // stopArbitration has been invoked
                        break;
                    }

                    // If there are no suitable providers, retry the arbitration after the retry
                    // interval
                    // elapsed
                    const std::int64_t durationMs = thisSharedPtr->getDurationMs();

                    if (thisSharedPtr->_discoveryQos.getDiscoveryTimeoutMs() <= durationMs) {
                        // discovery timeout reached
                        break;
                    } else if (thisSharedPtr->_arbitrationFailedForever) {
                        // arbitration failed -> inform caller immediately
                        break;
                    } else if (thisSharedPtr->_discoveryQos.getDiscoveryTimeoutMs() - durationMs <=
                               thisSharedPtr->_discoveryQos.getRetryIntervalMs()) {
                        // no retry possible -> inform caller about cancelled arbitration
                        // immediately
                        break;
                    } else {
                        // wait for retry interval and attempt a new arbitration
                        JOYNR_LOG_TRACE(logger(),
                                        "Rescheduling arbitration with delay {}ms",
                                        thisSharedPtr->_discoveryQos.getRetryIntervalMs());
                        auto waitIntervalMs = std::chrono::milliseconds(
                                thisSharedPtr->_discoveryQos.getRetryIntervalMs());
                        thisSharedPtr->_semaphore.waitFor(waitIntervalMs);
                    }
                }

                if (thisSharedPtr->_onErrorCallback) {
                    if (thisSharedPtr->_arbitrationStopped) {
                        thisSharedPtr->_arbitrationError.setMessage(
                                "Shutting Down Arbitration for interface " +
                                thisSharedPtr->_interfaceName);
                        std::call_once(thisSharedPtr->_onceFlag,
                                       thisSharedPtr->_onErrorCallback,
                                       thisSharedPtr->_arbitrationError);
                        // If this point is reached the arbitration timed out
                    } else if (thisSharedPtr->_discoveredIncompatibleVersions.empty()) {
                        std::call_once(thisSharedPtr->_onceFlag,
                                       thisSharedPtr->_onErrorCallback,
                                       thisSharedPtr->_arbitrationError);
                    } else {
                        std::call_once(thisSharedPtr->_onceFlag,
                                       thisSharedPtr->_onErrorCallback,
                                       exceptions::NoCompatibleProviderFoundException(
                                               thisSharedPtr->_discoveredIncompatibleVersions));
                    }
                }

                thisSharedPtr->_arbitrationRunning = false;
                JOYNR_LOG_DEBUG(
                        logger(),
                        "Exiting arbitration thread for domain: [{}], interface: {}, GBIDs = >{}<",
                        thisSharedPtr->_serializedDomainsList,
                        thisSharedPtr->_interfaceName,
                        thisSharedPtr->_gbidString);
                thisSharedPtr->assertNoPendingFuture();
            });
}

void Arbitrator::stopArbitration()
{
    JOYNR_LOG_DEBUG(logger(),
                    "StopArbitrator for domain: [{}], interface: {}, GBIDs = >{}<",
                    _serializedDomainsList,
                    _interfaceName,
                    _gbidString);
    {
        std::unique_lock<std::mutex> lock(_pendingFutureMutex);
        _arbitrationStopped = true;

        // check if there is a pending future and stop it if still in progress
        auto error = std::make_shared<joynr::exceptions::JoynrRuntimeException>(
                "Shutting Down Arbitration for interface " + _interfaceName);
        boost::apply_visitor([error](auto& future) {
                                 if (future) {
                                     if (future->getStatus() == StatusCodeEnum::IN_PROGRESS) {
                                         future->onError(error);
                                     }
                                     future.reset();
                                 }
                             },
                             _pendingFuture);
    }

    _semaphore.notify();

    if (_arbitrationThread.joinable()) {
        JOYNR_LOG_DEBUG(logger(), "Thread can be joined. Joining thread ({}) ...", _interfaceName);
        _arbitrationThread.join();
    }
}

void Arbitrator::validatePendingFuture()
{
    std::unique_lock<std::mutex> lock(_pendingFutureMutex);
    boost::apply_visitor([](auto& future) {
                             if (future) {
                                 assert(future->getStatus() != StatusCodeEnum::IN_PROGRESS);
                                 future.reset();
                             }
                         },
                         _pendingFuture);
}

void Arbitrator::assertNoPendingFuture()
{
    std::unique_lock<std::mutex> lock(_pendingFutureMutex);
    boost::apply_visitor([](auto& future) {
                             assert(!future);
                             std::ignore = future;
                         },
                         _pendingFuture);
}

void Arbitrator::attemptArbitration()
{
    assertNoPendingFuture();
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> result;
    const bool isArbitrationStrateggyFixedParticipant =
            _discoveryQos.getArbitrationStrategy() ==
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT;
    const std::string fixedParticipantId =
            isArbitrationStrateggyFixedParticipant
                    // custom parameter is present in this case, checked in ArbitratorFactory
                    ? _discoveryQos.getCustomParameter("fixedParticipantId").getValue()
                    : "";

    JOYNR_LOG_DEBUG(logger(),
                    "DISCOVERY lookup for domain: [{}], interface: {}, GBIDs = >{}<",
                    _serializedDomainsList,
                    _interfaceName,
                    _gbidString);

    try {
        auto discoveryProxySharedPtr = _discoveryProxy.lock();
        if (!discoveryProxySharedPtr) {
            throw exceptions::JoynrRuntimeException("discoveryProxy not available");
        }
        const std::int64_t durationMs = getDurationMs();
        const std::int64_t waitTimeMs = _discoveryQos.getDiscoveryTimeoutMs() - durationMs;

        if (waitTimeMs <= 0) {
            throw exceptions::JoynrTimeOutException("arbitration timed out");
        }

        if (isArbitrationStrateggyFixedParticipant) {
            types::DiscoveryEntryWithMetaInfo fixedParticipantResult;

            auto future = discoveryProxySharedPtr->lookupAsync(
                    fixedParticipantId, _systemDiscoveryQos, _gbids);
            {
                std::unique_lock<std::mutex> lock(_pendingFutureMutex);
                if (_arbitrationStopped) {
                    return;
                } else {
                    _pendingFuture = future;
                }
            }

            future->get(waitTimeMs, fixedParticipantResult);
            validatePendingFuture();
            // _filterByVersionAndArbitrationStrategy allows to determine whether the
            // GuidedProxyBuilder is used. false => GuidedProxyBuilder
            if (_filterByVersionAndArbitrationStrategy &&
                fixedParticipantResult.getInterfaceName() != _interfaceName) {
                _arbitrationFailedForever = true;
                std::stringstream msg;
                msg << "incompatible interface returned, expected: " << _interfaceName
                    << " actual: " << fixedParticipantResult.getInterfaceName();
                throw exceptions::DiscoveryException(msg.str());
            }
            result.push_back(fixedParticipantResult);
        } else {
            auto future = discoveryProxySharedPtr->lookupAsync(
                    _domains, _interfaceName, _systemDiscoveryQos, _gbids);
            {
                std::unique_lock<std::mutex> lock(_pendingFutureMutex);
                if (_arbitrationStopped) {
                    return;
                } else {
                    _pendingFuture = future;
                }
            }

            future->get(waitTimeMs, result);
            validatePendingFuture();
        }

        receiveCapabilitiesLookupResults(result);

    } catch (const exceptions::JoynrException& e) {
        std::string errorMsg =
                "Unable to lookup provider (" +
                (isArbitrationStrateggyFixedParticipant
                         ? ("participantId: " + fixedParticipantId)
                         : ("domain: [" +
                            (_domains.empty() ? std::string("EMPTY") : _serializedDomainsList) +
                            "], interface: " + _interfaceName)) +
                (_gbids.empty() ? "" : ", GBIDs: " + _gbidString) + ") from discovery. ";
        if (exceptions::ApplicationException::TYPE_NAME() == e.getTypeName()) {
            const exceptions::ApplicationException& applicationException =
                    static_cast<const exceptions::ApplicationException&>(e);
            auto error = applicationException.getError<types::DiscoveryError::Enum>();
            switch (error) {
            case types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT:
            // fall through
            case types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS: {
                _discoveredIncompatibleVersions.clear();
                errorMsg += "DiscoveryError: " + types::DiscoveryError::getLiteral(error);
                JOYNR_LOG_INFO(logger(), errorMsg + ", continuing.");
                break;
            }
            case types::DiscoveryError::UNKNOWN_GBID:
            // fall through to default
            case types::DiscoveryError::INVALID_GBID:
            // fall through to default
            case types::DiscoveryError::INTERNAL_ERROR:
            // fall through to default
            default:
                _discoveredIncompatibleVersions.clear();
                errorMsg += "DiscoveryError: " + types::DiscoveryError::getLiteral(error);
                JOYNR_LOG_ERROR(logger(), errorMsg + ", giving up.");
                _arbitrationFailedForever = true;
                break;
            }
        } else {
            errorMsg += "JoynrException: " + e.getMessage();
            JOYNR_LOG_ERROR(logger(),
                            _arbitrationFailedForever ? errorMsg + ", giving up."
                                                      : errorMsg + ", continuing.");
        }
        _arbitrationError.setMessage(errorMsg);
        validatePendingFuture();
    }
    assertNoPendingFuture();
}

void Arbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& discoveryEntries)
{
    _discoveredIncompatibleVersions.clear();

    // Check for empty results
    if (discoveryEntries.empty()) {
        _arbitrationError.setMessage(
                "No entries found for domain: [" +
                (_domains.empty() ? std::string("EMPTY") : _serializedDomainsList) +
                "], interface: " + _interfaceName +
                (_gbids.empty() ? "" : ", GBIDs: " + _gbidString));
        return;
    }

    std::vector<types::DiscoveryEntryWithMetaInfo> filteredDiscoveryEntries =
            filterDiscoveryEntriesBySupportOnChange(discoveryEntries);

    if (filteredDiscoveryEntries.empty()) {
        std::string errorMsg =
                "There was more than one entries in capabilitiesEntries, but none supported "
                "on change subscriptions.";
        JOYNR_LOG_WARN(logger(), errorMsg);
        _arbitrationError.setMessage(errorMsg);
        return;
    }

    std::vector<types::DiscoveryEntryWithMetaInfo> selectedDiscoveryEntries;

    if (_filterByVersionAndArbitrationStrategy) {
        filteredDiscoveryEntries = filterDiscoveryEntriesByVersion(filteredDiscoveryEntries);

        if (filteredDiscoveryEntries.empty()) {
            std::string errorMsg =
                    "There was more than one entries in capabilitiesEntries, but none "
                    "was compatible.";
            JOYNR_LOG_WARN(logger(), errorMsg);
            _arbitrationError.setMessage(errorMsg);
            return;
        }

        types::DiscoveryEntryWithMetaInfo selectedDiscoveryEntry;
        try {
            selectedDiscoveryEntry = _arbitrationStrategyFunction->select(
                    _discoveryQos.getCustomParameters(), filteredDiscoveryEntries);
        } catch (const exceptions::DiscoveryException& e) {
            _arbitrationError = e;
        }

        if (!selectedDiscoveryEntry.getParticipantId().empty()) {
            selectedDiscoveryEntries.push_back(selectedDiscoveryEntry);
        }
    } else {
        selectedDiscoveryEntries = filteredDiscoveryEntries;
    }

    if (!selectedDiscoveryEntries.empty()) {
        joynr::ArbitrationResult arbitrationResult =
                joynr::ArbitrationResult(selectedDiscoveryEntries);
        if (_onSuccessCallback) {
            std::call_once(_onceFlag, _onSuccessCallback, arbitrationResult);
        }
        _arbitrationFinished = true;
    }
}

std::vector<types::DiscoveryEntryWithMetaInfo> Arbitrator::filterDiscoveryEntriesBySupportOnChange(
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries)
{
    std::vector<types::DiscoveryEntryWithMetaInfo> filteredDiscoveryEntries;
    if (_discoveryQos.getProviderMustSupportOnChange()) {
        for (const types::DiscoveryEntryWithMetaInfo& discoveryEntry : discoveryEntries) {
            const types::ProviderQos& providerQos = discoveryEntry.getQos();
            JOYNR_LOG_TRACE(
                    logger(), "Looping over capabilitiesEntry: {}", discoveryEntry.toString());
            if (!providerQos.getSupportsOnChangeSubscriptions()) {
                continue;
            }
            filteredDiscoveryEntries.push_back(discoveryEntry);
        }
    } else {
        filteredDiscoveryEntries = discoveryEntries;
    }

    return filteredDiscoveryEntries;
}

std::vector<types::DiscoveryEntryWithMetaInfo> Arbitrator::filterDiscoveryEntriesByVersion(
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries)
{
    std::vector<types::DiscoveryEntryWithMetaInfo> filteredDiscoveryEntries;
    types::Version providerVersion;
    for (const types::DiscoveryEntryWithMetaInfo& discoveryEntry : discoveryEntries) {
        providerVersion = discoveryEntry.getProviderVersion();
        if (providerVersion.getMajorVersion() != _interfaceVersion.getMajorVersion() ||
            providerVersion.getMinorVersion() < _interfaceVersion.getMinorVersion()) {
            JOYNR_LOG_TRACE(logger(),
                            "Skipping capabilitiesEntry with incompatible version, expected: " +
                                    std::to_string(_interfaceVersion.getMajorVersion()) + "." +
                                    std::to_string(_interfaceVersion.getMinorVersion()));
            _discoveredIncompatibleVersions.insert(providerVersion);
        } else {
            filteredDiscoveryEntries.push_back(discoveryEntry);
        }
    }

    return filteredDiscoveryEntries;
}

std::int64_t Arbitrator::getDurationMs() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _startTimePoint);

    return duration.count();
}

} // namespace joynr

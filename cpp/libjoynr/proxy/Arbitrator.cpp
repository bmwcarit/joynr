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
          pendingFutureMutex(),
          pendingFuture(),
          discoveryProxy(discoveryProxy),
          gbids(gbids),
          gbidString(util::vectorToString(gbids)),
          discoveryQos(discoveryQos),
          systemDiscoveryQos(discoveryQos.getCacheMaxAgeMs(),
                             discoveryQos.getDiscoveryTimeoutMs(),
                             discoveryQos.getDiscoveryScope(),
                             discoveryQos.getProviderMustSupportOnChange()),
          domains({domain}),
          interfaceName(interfaceName),
          interfaceVersion(interfaceVersion),
          discoveredIncompatibleVersions(),
          arbitrationError("Arbitration could not be finished in time."),
          arbitrationStrategyFunction(std::move(arbitrationStrategyFunction)),
          semaphore(0),
          arbitrationFinished(false),
          arbitrationRunning(false),
          arbitrationStopped(false),
          arbitrationThread()
{
}

Arbitrator::~Arbitrator()
{
    stopArbitration();
}

void Arbitrator::startArbitration(
        std::function<void(const types::DiscoveryEntryWithMetaInfo& discoveryEntry)> onSuccess,
        std::function<void(const exceptions::DiscoveryException& exception)> onError)
{
    if (arbitrationRunning) {
        JOYNR_LOG_ERROR(
                logger(),
                "Arbitration already running for domain = {}, interface = {}, GBIDs = {}. A second "
                "arbitration will not be started.",
                domains.at(0),
                interfaceName,
                gbidString);
        return;
    }

    startTimePoint = std::chrono::steady_clock::now();
    JOYNR_LOG_INFO(
            logger(),
            "Arbitration started for domain = {}, interface = {}, GBIDs = {}, version = {}.{}.",
            domains.at(0),
            interfaceName,
            gbidString,
            std::to_string(interfaceVersion.getMajorVersion()),
            std::to_string(interfaceVersion.getMinorVersion()));

    arbitrationRunning = true;
    arbitrationStopped = false;

    onSuccessCallback = onSuccess;
    onErrorCallback = onError;

    arbitrationThread = std::thread([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())]() {

        auto thisSharedPtr = thisWeakPtr.lock();
        if (!thisSharedPtr) {
            return;
        }

        thisSharedPtr->arbitrationFinished = false;

        std::string serializedDomainsList = boost::algorithm::join(thisSharedPtr->domains, ", ");
        JOYNR_LOG_DEBUG(logger(),
                        "DISCOVERY lookup for domain: [{}], interface: {}, GBIDs = {}",
                        serializedDomainsList,
                        thisSharedPtr->interfaceName,
                        thisSharedPtr->gbidString);

        while (!thisSharedPtr->arbitrationStopped) {
            thisSharedPtr->attemptArbitration();

            // exit if arbitration has finished successfully
            if (thisSharedPtr->arbitrationFinished) {
                thisSharedPtr->assertNoPendingFuture();
                return;
            }

            // check if we should break the loop and report errors to the user
            if (thisSharedPtr->arbitrationStopped) {
                // stopArbitration has been invoked
                break;
            }

            // If there are no suitable providers, retry the arbitration after the retry interval
            // elapsed
            const std::int64_t durationMs = thisSharedPtr->getDurationMs();

            if (thisSharedPtr->discoveryQos.getDiscoveryTimeoutMs() <= durationMs) {
                // discovery timeout reached
                break;
            } else if (thisSharedPtr->discoveryQos.getDiscoveryTimeoutMs() - durationMs <=
                       thisSharedPtr->discoveryQos.getRetryIntervalMs()) {
                /*
                 * no retry possible -> wait until discoveryTimeout is reached and inform caller
                 * about cancelled arbitration
                 */
                auto waitIntervalMs = std::chrono::milliseconds(
                        thisSharedPtr->discoveryQos.getDiscoveryTimeoutMs() - durationMs);
                thisSharedPtr->semaphore.waitFor(waitIntervalMs);
                break;
            } else {
                // wait for retry interval and attempt a new arbitration
                auto waitIntervalMs =
                        std::chrono::milliseconds(thisSharedPtr->discoveryQos.getRetryIntervalMs());
                thisSharedPtr->semaphore.waitFor(waitIntervalMs);
            }
        }

        if (thisSharedPtr->onErrorCallback) {
            if (thisSharedPtr->arbitrationStopped) {
                thisSharedPtr->arbitrationError.setMessage(
                        "Shutting Down Arbitration for interface " + thisSharedPtr->interfaceName);
                thisSharedPtr->onErrorCallback(thisSharedPtr->arbitrationError);
                // If this point is reached the arbitration timed out
            } else if (thisSharedPtr->discoveredIncompatibleVersions.empty()) {
                thisSharedPtr->onErrorCallback(thisSharedPtr->arbitrationError);
            } else {
                thisSharedPtr->onErrorCallback(exceptions::NoCompatibleProviderFoundException(
                        thisSharedPtr->discoveredIncompatibleVersions));
            }
        }

        thisSharedPtr->arbitrationRunning = false;
        JOYNR_LOG_DEBUG(logger(),
                        "Exiting arbitration thread for domain: [{}], interface: {}, GBIDs = {}",
                        serializedDomainsList,
                        thisSharedPtr->interfaceName,
                        thisSharedPtr->gbidString);
        thisSharedPtr->assertNoPendingFuture();
    });
}

void Arbitrator::stopArbitration()
{
    JOYNR_LOG_DEBUG(logger(),
                    "StopArbitrator for domain: [{}], interface: {}, GBIDs = {}",
                    domains.at(0),
                    interfaceName,
                    gbidString);
    {
        std::unique_lock<std::mutex> lock(pendingFutureMutex);
        arbitrationStopped = true;

        // check if there is a pending future and stop it if still in progress
        auto error = std::make_shared<joynr::exceptions::JoynrRuntimeException>(
                "Shutting Down Arbitration for interface " + interfaceName);
        boost::apply_visitor([error](auto& future) {
                                 if (future) {
                                     if (future->getStatus() == StatusCodeEnum::IN_PROGRESS) {
                                         future->onError(error);
                                     }
                                     future.reset();
                                 }
                             },
                             pendingFuture);
    }

    semaphore.notify();

    if (arbitrationThread.joinable()) {
        JOYNR_LOG_DEBUG(logger(), "Thread can be joined. Joining thread ({}) ...", interfaceName);
        arbitrationThread.join();
    }
}

void Arbitrator::validatePendingFuture()
{
    std::unique_lock<std::mutex> lock(pendingFutureMutex);
    boost::apply_visitor([](auto& future) {
                             if (future) {
                                 assert(future->getStatus() != StatusCodeEnum::IN_PROGRESS);
                                 future.reset();
                             }
                         },
                         pendingFuture);
}

void Arbitrator::assertNoPendingFuture()
{
    std::unique_lock<std::mutex> lock(pendingFutureMutex);
    boost::apply_visitor([](auto& future) { assert(!future); }, pendingFuture);
}

void Arbitrator::attemptArbitration()
{
    assertNoPendingFuture();
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> result;
    try {
        auto discoveryProxySharedPtr = discoveryProxy.lock();
        if (!discoveryProxySharedPtr) {
            throw exceptions::JoynrRuntimeException("discoveryProxy not available");
        }
        const std::int64_t durationMs = getDurationMs();
        const std::int64_t waitTimeMs = discoveryQos.getDiscoveryTimeoutMs() - durationMs;

        if (waitTimeMs <= 0) {
            throw exceptions::JoynrTimeOutException("arbitration timed out");
        }

        if (discoveryQos.getArbitrationStrategy() ==
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT) {
            types::DiscoveryEntryWithMetaInfo fixedParticipantResult;
            std::string fixedParticipantId =
                    discoveryQos.getCustomParameter("fixedParticipantId").getValue();

            auto future = discoveryProxySharedPtr->lookupAsync(
                    fixedParticipantId, systemDiscoveryQos, gbids);
            {
                std::unique_lock<std::mutex> lock(pendingFutureMutex);
                if (arbitrationStopped) {
                    return;
                } else {
                    pendingFuture = future;
                }
            }

            future->get(waitTimeMs, fixedParticipantResult);
            validatePendingFuture();
            result.push_back(fixedParticipantResult);
        } else {
            auto future = discoveryProxySharedPtr->lookupAsync(
                    domains, interfaceName, systemDiscoveryQos, gbids);
            {
                std::unique_lock<std::mutex> lock(pendingFutureMutex);
                if (arbitrationStopped) {
                    return;
                } else {
                    pendingFuture = future;
                }
            }

            future->get(waitTimeMs, result);
            validatePendingFuture();
        }

        receiveCapabilitiesLookupResults(result);

    } catch (const exceptions::JoynrException& e) {
        std::string errorMsg = "Unable to lookup provider (domain: " +
                               (domains.empty() ? std::string("EMPTY") : domains.at(0)) +
                               ", interface: " + interfaceName + ") from discovery. Error: " +
                               e.getMessage();
        JOYNR_LOG_ERROR(logger(), errorMsg);
        arbitrationError.setMessage(errorMsg);
        validatePendingFuture();
    }
    assertNoPendingFuture();
}

void Arbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& discoveryEntries)
{
    discoveredIncompatibleVersions.clear();

    // Check for empty results
    if (discoveryEntries.empty()) {
        arbitrationError.setMessage("No entries found for domain: " +
                                    (domains.empty() ? std::string("EMPTY") : domains.at(0)) +
                                    ", interface: " + interfaceName +
                                    (gbids.empty() ? "" : ", GBIDs: " + gbidString));
        return;
    }

    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> preFilteredDiscoveryEntries;
    joynr::types::Version providerVersion;
    std::size_t providersWithoutSupportOnChange = 0;
    std::size_t providersWithIncompatibleVersion = 0;
    for (const joynr::types::DiscoveryEntryWithMetaInfo& discoveryEntry : discoveryEntries) {
        const types::ProviderQos& providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger(), "Looping over capabilitiesEntry: {}", discoveryEntry.toString());
        providerVersion = discoveryEntry.getProviderVersion();

        if (discoveryQos.getProviderMustSupportOnChange() &&
            !providerQos.getSupportsOnChangeSubscriptions()) {
            ++providersWithoutSupportOnChange;
            continue;
        }

        if (providerVersion.getMajorVersion() != interfaceVersion.getMajorVersion() ||
            providerVersion.getMinorVersion() < interfaceVersion.getMinorVersion()) {
            JOYNR_LOG_TRACE(logger(),
                            "Skipping capabilitiesEntry with incompatible version, expected: " +
                                    std::to_string(interfaceVersion.getMajorVersion()) + "." +
                                    std::to_string(interfaceVersion.getMinorVersion()));
            discoveredIncompatibleVersions.insert(providerVersion);
            ++providersWithIncompatibleVersion;
            continue;
        }

        preFilteredDiscoveryEntries.push_back(discoveryEntry);
    }

    if (preFilteredDiscoveryEntries.empty()) {
        std::string errorMsg;
        if (providersWithoutSupportOnChange == discoveryEntries.size()) {
            errorMsg = "There was more than one entries in capabilitiesEntries, but none supported "
                       "on change subscriptions.";
            JOYNR_LOG_WARN(logger(), errorMsg);
            arbitrationError.setMessage(errorMsg);
        } else if ((providersWithoutSupportOnChange + providersWithIncompatibleVersion) ==
                   discoveryEntries.size()) {
            errorMsg = "There was more than one entries in capabilitiesEntries, but none "
                       "was compatible.";
            JOYNR_LOG_WARN(logger(), errorMsg);
            arbitrationError.setMessage(errorMsg);
        }
        return;
    } else {
        types::DiscoveryEntryWithMetaInfo res;

        try {
            res = arbitrationStrategyFunction->select(
                    discoveryQos.getCustomParameters(), preFilteredDiscoveryEntries);
        } catch (const exceptions::DiscoveryException& e) {
            arbitrationError = e;
        }
        if (!res.getParticipantId().empty()) {
            if (onSuccessCallback) {
                onSuccessCallback(res);
            }
            arbitrationFinished = true;
        }
    }
}

std::int64_t Arbitrator::getDurationMs() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTimePoint);

    return duration.count();
}

} // namespace joynr

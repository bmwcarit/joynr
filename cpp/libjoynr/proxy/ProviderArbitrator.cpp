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
#include "joynr/ProviderArbitrator.h"

#include <cassert>
#include <vector>
#include <chrono>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/NoCompatibleProviderFoundException.h"
#include "joynr/Logger.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/TypeUtil.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/Semaphore.h"

namespace joynr
{

INIT_LOGGER(ProviderArbitrator);

ProviderArbitrator::ProviderArbitrator(const std::string& domain,
                                       const std::string& interfaceName,
                                       const joynr::types::Version& interfaceVersion,
                                       joynr::system::IDiscoverySync& discoveryProxy,
                                       const DiscoveryQos& discoveryQos)
        : discoveryProxy(discoveryProxy),
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
          participantId(""),
          arbitrationStatus(ArbitrationStatus::ArbitrationRunning),
          listener(nullptr),
          listenerSemaphore(0)
{
}

void ProviderArbitrator::startArbitration()
{
    Semaphore semaphore;

    // Arbitrate until successful or timed out
    auto start = std::chrono::system_clock::now();

    while (true) {
        attemptArbitration();
        // Finish on success or failure
        if (arbitrationStatus != ArbitrationStatus::ArbitrationRunning)
            return;

        // If there are no suitable providers

        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        if (discoveryQos.getDiscoveryTimeoutMs() <= duration.count()) {
            // discovery timeout reached
            break;
        } else if (discoveryQos.getDiscoveryTimeoutMs() - duration.count() <=
                   discoveryQos.getRetryIntervalMs()) {
            /*
             * no retry possible -> wait until discoveryTimeout is reached and inform caller about
             * cancelled arbitration
             */
            semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() -
                                                        duration.count()));
            break;
        } else {
            // wait for retry interval and attempt a new arbitration
            semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getRetryIntervalMs()));
        }
    }

    // If this point is reached the arbitration timed out
    if (!discoveredIncompatibleVersions.empty()) {
        notifyArbitrationListener(
                exceptions::NoCompatibleProviderFoundException(discoveredIncompatibleVersions));
    } else {
        notifyArbitrationListener(arbitrationError);
    }
}

void ProviderArbitrator::attemptArbitration()
{
    std::vector<joynr::types::DiscoveryEntry> result;
    try {
        discoveryProxy.lookup(result, domains, interfaceName, systemDiscoveryQos);
        receiveCapabilitiesLookupResults(result);
    } catch (const exceptions::JoynrException& e) {
        std::string errorMsg = "Unable to lookup provider (domain: " +
                               (domains.size() > 0 ? domains.at(0) : std::string("EMPTY")) +
                               ", interface: " + interfaceName + ") from discovery. Error: " +
                               e.getMessage();
        JOYNR_LOG_ERROR(logger, errorMsg);
        arbitrationError.setMessage(errorMsg);
    }
}

void ProviderArbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    std::string res;
    discoveredIncompatibleVersions.clear();

    // Check for empty results
    if (discoveryEntries.size() == 0) {
        arbitrationError.setMessage("No entries found for domain: " +
                                    (domains.size() > 0 ? domains.at(0) : std::string("EMPTY")) +
                                    ", interface: " + interfaceName);
        return;
    }

    std::vector<joynr::types::DiscoveryEntry> preFilteredDiscoveryEntries;
    joynr::types::Version providerVersion;
    std::size_t providersWithoutSupportOnChange = 0;
    std::size_t providersWithIncompatibleVersion = 0;
    for (const joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        types::ProviderQos providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger, "Looping over capabilitiesEntry: {}", discoveryEntry.toString());
        providerVersion = discoveryEntry.getProviderVersion();

        if (discoveryQos.getProviderMustSupportOnChange() &&
            !providerQos.getSupportsOnChangeSubscriptions()) {
            ++providersWithoutSupportOnChange;
            continue;
        }

        if (providerVersion.getMajorVersion() != interfaceVersion.getMajorVersion() ||
            providerVersion.getMinorVersion() < interfaceVersion.getMinorVersion()) {
            JOYNR_LOG_TRACE(logger,
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
            JOYNR_LOG_WARN(logger, errorMsg);
            arbitrationError.setMessage(errorMsg);
        } else if ((providersWithoutSupportOnChange + providersWithIncompatibleVersion) ==
                   discoveryEntries.size()) {
            errorMsg = "There was more than one entries in capabilitiesEntries, but none "
                       "was compatible.";
            JOYNR_LOG_WARN(logger, errorMsg);
            arbitrationError.setMessage(errorMsg);
        }
        return;
    } else {
        res = filterDiscoveryEntries(preFilteredDiscoveryEntries);
        if (!res.empty())
            notifyArbitrationListener(res);
    }
}

std::string ProviderArbitrator::getParticipantId()
{
    if (participantId.empty()) {
        throw exceptions::DiscoveryException(
                "ParticipantId is empty: Called getParticipantId() before "
                "arbitration has finished / Arbitrator did not set "
                "participantId.");
    }
    return participantId;
}

void ProviderArbitrator::setParticipantId(std::string participantId)
{
    this->participantId = participantId;
    if (listenerSemaphore.waitFor()) {
        assert(listener != nullptr);
        listener->setParticipantId(participantId);
        listenerSemaphore.notify();
    }
}

void ProviderArbitrator::notifyArbitrationListener(const std::string& participantId)
{
    setParticipantId(participantId);
    setArbitrationStatus(ArbitrationStatus::ArbitrationSuccessful);
}

void ProviderArbitrator::notifyArbitrationListener(const exceptions::DiscoveryException& error)
{
    setArbitrationError(error);
    setArbitrationStatus(ArbitrationStatus::ArbitrationCanceledForever);
}

void ProviderArbitrator::setArbitrationStatus(
        ArbitrationStatus::ArbitrationStatusType arbitrationStatus)
{
    this->arbitrationStatus = arbitrationStatus;
    if (listenerSemaphore.waitFor()) {
        try {
            assert(listener != nullptr);
            listener->setArbitrationStatus(arbitrationStatus);
        } catch (const exceptions::DiscoveryException& e) {
            JOYNR_LOG_ERROR(
                    logger, "Exception while setting arbitration status: {}", e.getMessage());
            listenerSemaphore.notify();
            throw;
        }
        listenerSemaphore.notify();
    }
}

void ProviderArbitrator::setArbitrationError(const exceptions::DiscoveryException& error)
{
    if (listenerSemaphore.waitFor()) {
        assert(listener != nullptr);
        listener->setArbitrationError(error);
        listenerSemaphore.notify();
    }
}

void ProviderArbitrator::removeArbitrationListener()
{
    if (listener != nullptr) {
        this->listener = nullptr;
        listenerSemaphore.wait();
    }
}

void ProviderArbitrator::setArbitrationListener(IArbitrationListener* listener)
{
    this->listener = listener;
    listenerSemaphore.notify();
    if (arbitrationStatus == ArbitrationStatus::ArbitrationSuccessful) {
        listener->setParticipantId(participantId);
        listener->setArbitrationStatus(arbitrationStatus);
    }
}

} // namespace joynr

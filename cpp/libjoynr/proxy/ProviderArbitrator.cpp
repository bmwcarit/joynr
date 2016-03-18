/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include <vector>
#include <chrono>
#include "joynr/ProviderArbitrator.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Logger.h"
#include "joynr/system/IDiscovery.h"

#include "joynr/TypeUtil.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/Semaphore.h"

#include <cassert>

namespace joynr
{

INIT_LOGGER(ProviderArbitrator);

ProviderArbitrator::ProviderArbitrator(const std::string& domain,
                                       const std::string& interfaceName,
                                       joynr::system::IDiscoverySync& discoveryProxy,
                                       const DiscoveryQos& discoveryQos)
        : discoveryProxy(discoveryProxy),
          discoveryQos(discoveryQos),
          systemDiscoveryQos(discoveryQos.getCacheMaxAge(),
                             discoveryQos.getDiscoveryScope(),
                             discoveryQos.getProviderMustSupportOnChange()),
          domain(domain),
          interfaceName(interfaceName),
          participantId(""),
          connection(joynr::types::CommunicationMiddleware::NONE),
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
        // Attempt arbitration (overloaded in subclasses)
        attemptArbitration();

        // Finish on success or failure
        if (arbitrationStatus != ArbitrationStatus::ArbitrationRunning)
            return;

        // If there are no suitable providers

        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        if (discoveryQos.getDiscoveryTimeout() <= duration.count()) {
            // discovery timeout reached
            break;
        } else if (discoveryQos.getDiscoveryTimeout() - duration.count() <=
                   discoveryQos.getRetryInterval()) {
            /*
             * no retry possible -> wait until discoveryTimeout is reached and inform caller about
             * cancelled arbitration
             */
            semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeout() -
                                                        duration.count()));
            break;
        } else {
            // wait for retry interval and attempt a new arbitration
            semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getRetryInterval()));
        }
    }

    // If this point is reached the arbitration timed out
    updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationCanceledForever,
                                                   "",
                                                   joynr::types::CommunicationMiddleware::NONE);
}

joynr::types::CommunicationMiddleware::Enum ProviderArbitrator::
        selectPreferredCommunicationMiddleware(
                const std::vector<joynr::types::CommunicationMiddleware::Enum>& connections)
{
    if (std::find(connections.begin(),
                  connections.end(),
                  joynr::types::CommunicationMiddleware::IN_PROCESS) != connections.end()) {
        return joynr::types::CommunicationMiddleware::IN_PROCESS;
    }
    if (std::find(connections.begin(),
                  connections.end(),
                  joynr::types::CommunicationMiddleware::COMMONAPI_DBUS) != connections.end()) {
        return joynr::types::CommunicationMiddleware::COMMONAPI_DBUS;
    }
    if (std::find(connections.begin(),
                  connections.end(),
                  joynr::types::CommunicationMiddleware::WEBSOCKET) != connections.end()) {
        return joynr::types::CommunicationMiddleware::WEBSOCKET;
    }
    if (std::find(connections.begin(),
                  connections.end(),
                  joynr::types::CommunicationMiddleware::SOME_IP) != connections.end()) {
        return joynr::types::CommunicationMiddleware::SOME_IP;
    }
    if (std::find(connections.begin(),
                  connections.end(),
                  joynr::types::CommunicationMiddleware::JOYNR) != connections.end()) {
        return joynr::types::CommunicationMiddleware::JOYNR;
    }
    return joynr::types::CommunicationMiddleware::NONE;
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

joynr::types::CommunicationMiddleware::Enum ProviderArbitrator::getConnection()
{
    if (connection == joynr::types::CommunicationMiddleware::NONE) {
        throw exceptions::DiscoveryException("Connection is NULL: Called getConnection() before "
                                             "arbitration has finished / Arbitrator did not set "
                                             "connection.");
    }
    return connection;
}

void ProviderArbitrator::setConnection(
        const joynr::types::CommunicationMiddleware::Enum& connection)
{
    this->connection = connection;
    if (listenerSemaphore.waitFor()) {
        assert(listener != nullptr);
        listener->setConnection(connection);
        listenerSemaphore.notify();
    }
}

void ProviderArbitrator::updateArbitrationStatusParticipantIdAndAddress(
        ArbitrationStatus::ArbitrationStatusType arbitrationStatus,
        std::string participantId,
        const joynr::types::CommunicationMiddleware::Enum& connection)
{
    setParticipantId(participantId);
    setConnection(connection);
    setArbitrationStatus(arbitrationStatus);
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

void ProviderArbitrator::removeArbitationListener()
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
        listener->setConnection(connection);
        listener->setArbitrationStatus(arbitrationStatus);
    }
}

} // namespace joynr

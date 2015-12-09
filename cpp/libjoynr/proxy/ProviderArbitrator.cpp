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
#include <QTime>
#include <vector>

#include "joynr/ProviderArbitrator.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/joynrlogging.h"
#include "joynr/system/IDiscovery.h"

#include "joynr/TypeUtil.h"
#include "joynr/types/QtDiscoveryScope.h"
#include "joynr/Semaphore.h"

#include <cassert>

namespace joynr
{

joynr_logging::Logger* ProviderArbitrator::logger =
        joynr_logging::Logging::getInstance()->getLogger("Arb", "ProviderArbitrator");

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
          listener(NULL),
          listenerSemaphore(0)
{
}

ProviderArbitrator::~ProviderArbitrator()
{
}

void ProviderArbitrator::startArbitration()
{
    QTime timer;
    joynr::Semaphore semaphore;

    // Arbitrate until successful or timed out
    while (true) {

        timer.start();

        // Attempt arbitration (overloaded in subclasses)
        attemptArbitration();

        // Finish on success or failure
        if (arbitrationStatus != ArbitrationStatus::ArbitrationRunning)
            return;

        // Reduce the timeout by the elapsed time
        discoveryQos.setDiscoveryTimeout(discoveryQos.getDiscoveryTimeout() - timer.restart());
        if (discoveryQos.getDiscoveryTimeout() <= 0)
            break;

        // If there are no suitable providers, wait for a second before trying again
        semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getRetryInterval()));

        // Reduce the timeout again
        discoveryQos.setDiscoveryTimeout(discoveryQos.getDiscoveryTimeout() - timer.elapsed());
        if (discoveryQos.getDiscoveryTimeout() <= 0)
            break;
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
        assert(listener != NULL);
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
        assert(listener != NULL);
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
            assert(listener != NULL);
            listener->setArbitrationStatus(arbitrationStatus);
        } catch (exceptions::DiscoveryException& e) {
            LOG_ERROR(logger,
                      FormatString("Exception while setting arbitration status: %1")
                              .arg(e.getMessage())
                              .str());
            listenerSemaphore.notify();
            throw;
        }
        listenerSemaphore.notify();
    }
}

void ProviderArbitrator::removeArbitationListener()
{
    if (listener != NULL) {
        this->listener = NULL;
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

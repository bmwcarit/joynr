/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/RequestStatus.h"
#include "joynr/RequestStatusCode.h"

#include "joynr/system/DiscoveryEntry.h"
#include "joynr/system/DiscoveryQos.h"
#include "joynr/system/IRouting.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/system/DiscoveryProxy.h"

namespace joynr
{

LocalDiscoveryAggregator::LocalDiscoveryAggregator(
        IRequestCallerDirectory& requestCallerDirectory,
        const SystemServicesSettings& systemServicesSettings)
        : discoveryProxy(NULL),
          hasOwnershipOfDiscoveryProxy(false),
          requestCallerDirectory(requestCallerDirectory),
          provisionedDiscoveryEntries(),
          systemServicesSettings(systemServicesSettings)
{
    QList<joynr::system::CommunicationMiddleware::Enum> connections;
    connections << joynr::system::CommunicationMiddleware::JOYNR;
    joynr::system::DiscoveryEntry routingProviderDiscoveryEntry(
            systemServicesSettings.getDomain(),
            joynr::system::IRouting::getInterfaceName(),
            systemServicesSettings.getCcRoutingProviderParticipantId(),
            joynr::types::ProviderQos(),
            connections);
    provisionedDiscoveryEntries.insert(
            routingProviderDiscoveryEntry.getParticipantId(), routingProviderDiscoveryEntry);
    joynr::system::DiscoveryEntry discoveryProviderDiscoveryEntry(
            systemServicesSettings.getDomain(),
            joynr::system::IDiscovery::getInterfaceName(),
            systemServicesSettings.getCcDiscoveryProviderParticipantId(),
            joynr::types::ProviderQos(),
            connections);
    provisionedDiscoveryEntries.insert(
            discoveryProviderDiscoveryEntry.getParticipantId(), discoveryProviderDiscoveryEntry);
}

LocalDiscoveryAggregator::~LocalDiscoveryAggregator()
{
    if (hasOwnershipOfDiscoveryProxy) {
        delete discoveryProxy;
    }
}

void LocalDiscoveryAggregator::setDiscoveryProxy(joynr::system::IDiscoverySync* discoveryProxy)
{
    this->discoveryProxy = discoveryProxy;
    hasOwnershipOfDiscoveryProxy = true;
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::add(joynr::RequestStatus& joynrInternalStatus,
                                   const joynr::system::DiscoveryEntry& discoveryEntry)
{
    if (discoveryProxy == NULL) {
        joynrInternalStatus.setCode(RequestStatusCode::ERROR);
        joynrInternalStatus.addDescription(
                QString("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                        "local capabilitites directory."));
        return;
    }

    discoveryProxy->add(joynrInternalStatus, discoveryEntry);
}

void LocalDiscoveryAggregator::checkForLocalAvailabilityAndAddInProcessConnection(
        joynr::system::DiscoveryEntry& discoveryEntry)
{
    QString participantId(discoveryEntry.getParticipantId());
    if (requestCallerDirectory.containsRequestCaller(participantId)) {
        QList<joynr::system::CommunicationMiddleware::Enum> connections(
                discoveryEntry.getConnections());
        connections.prepend(joynr::system::CommunicationMiddleware::IN_PROCESS);
        discoveryEntry.setConnections(connections);
    }
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::lookup(joynr::RequestStatus& joynrInternalStatus,
                                      QList<joynr::system::DiscoveryEntry>& result,
                                      const QString& domain,
                                      const QString& interfaceName,
                                      const joynr::system::DiscoveryQos& discoveryQos)
{
    if (discoveryProxy == NULL) {
        joynrInternalStatus.setCode(RequestStatusCode::ERROR);
        joynrInternalStatus.addDescription(
                QString("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                        "local capabilitites directory."));
        return;
    }
    discoveryProxy->lookup(joynrInternalStatus, result, domain, interfaceName, discoveryQos);

    QMutableListIterator<joynr::system::DiscoveryEntry> i(result);
    while (i.hasNext()) {
        checkForLocalAvailabilityAndAddInProcessConnection(i.next());
    }
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::lookup(joynr::RequestStatus& joynrInternalStatus,
                                      joynr::system::DiscoveryEntry& result,
                                      const QString& participantId)
{
    if (provisionedDiscoveryEntries.contains(participantId)) {
        joynrInternalStatus.setCode(RequestStatusCode::OK);
        result = provisionedDiscoveryEntries.value(participantId);
    } else {
        if (discoveryProxy == NULL) {
            joynrInternalStatus.setCode(RequestStatusCode::ERROR);
            joynrInternalStatus.addDescription(
                    QString("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                            "local capabilitites directory."));
            return;
        }
        discoveryProxy->lookup(joynrInternalStatus, result, participantId);
    }
    checkForLocalAvailabilityAndAddInProcessConnection(result);
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::remove(joynr::RequestStatus& joynrInternalStatus,
                                      const QString& participantId)
{
    if (discoveryProxy == NULL) {
        joynrInternalStatus.setCode(RequestStatusCode::ERROR);
        joynrInternalStatus.addDescription(
                QString("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                        "local capabilitites directory."));
        return;
    }
    discoveryProxy->remove(joynrInternalStatus, participantId);
}

} // namespace joynr

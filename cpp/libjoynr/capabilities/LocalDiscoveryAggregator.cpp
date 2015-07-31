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

#include <vector>

#include "joynr/IRequestCallerDirectory.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/RequestStatus.h"
#include "joynr/RequestStatusCode.h"

#include "joynr/types/QtDiscoveryEntry.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/system/IRouting.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/TypeUtil.h"

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
    QList<joynr::types::QtCommunicationMiddleware::Enum> connections;
    connections << joynr::types::QtCommunicationMiddleware::JOYNR;
    joynr::types::QtDiscoveryEntry routingProviderDiscoveryEntry(
            systemServicesSettings.getDomain(),
            TypeUtil::toQt(joynr::system::IRouting::INTERFACE_NAME()),
            systemServicesSettings.getCcRoutingProviderParticipantId(),
            joynr::types::QtProviderQos(),
            connections);
    provisionedDiscoveryEntries.insert(
            routingProviderDiscoveryEntry.getParticipantId().toStdString(),
            routingProviderDiscoveryEntry);
    joynr::types::QtDiscoveryEntry discoveryProviderDiscoveryEntry(
            systemServicesSettings.getDomain(),
            TypeUtil::toQt(joynr::system::IDiscovery::INTERFACE_NAME()),
            systemServicesSettings.getCcDiscoveryProviderParticipantId(),
            joynr::types::QtProviderQos(),
            connections);
    provisionedDiscoveryEntries.insert(
            discoveryProviderDiscoveryEntry.getParticipantId().toStdString(),
            discoveryProviderDiscoveryEntry);
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
joynr::RequestStatus LocalDiscoveryAggregator::add(
        const joynr::types::DiscoveryEntry& discoveryEntry)
{
    if (discoveryProxy == NULL) {
        return RequestStatus(RequestStatusCode::ERROR,
                             "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                             "local capabilitites directory.");
    }

    return discoveryProxy->add(discoveryEntry);
}

void LocalDiscoveryAggregator::checkForLocalAvailabilityAndAddInProcessConnection(
        joynr::types::DiscoveryEntry& discoveryEntry)
{
    std::string participantId(discoveryEntry.getParticipantId());
    if (requestCallerDirectory.containsRequestCaller(participantId)) {
        std::vector<joynr::types::CommunicationMiddleware::Enum> connections(
                discoveryEntry.getConnections());
        connections.insert(connections.begin(), joynr::types::CommunicationMiddleware::IN_PROCESS);
        discoveryEntry.setConnections(connections);
    }
}

// inherited from joynr::system::IDiscoverySync
joynr::RequestStatus LocalDiscoveryAggregator::lookup(
        std::vector<joynr::types::DiscoveryEntry>& result,
        const std::string& domain,
        const std::string& interfaceName,
        const joynr::types::DiscoveryQos& discoveryQos)
{
    if (discoveryProxy == NULL) {
        return RequestStatus(RequestStatusCode::ERROR,
                             "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                             "local capabilitites directory.");
    }
    joynr::RequestStatus status(
            discoveryProxy->lookup(result, domain, interfaceName, discoveryQos));

    for (joynr::types::DiscoveryEntry& discoveryEntry : result) {
        checkForLocalAvailabilityAndAddInProcessConnection(discoveryEntry);
    }

    return status;
}

// inherited from joynr::system::IDiscoverySync
joynr::RequestStatus LocalDiscoveryAggregator::lookup(joynr::types::DiscoveryEntry& result,
                                                      const std::string& participantId)
{
    joynr::RequestStatus status(RequestStatusCode::OK);
    if (provisionedDiscoveryEntries.contains(participantId)) {
        result = joynr::types::QtDiscoveryEntry::createStd(
                provisionedDiscoveryEntries.value(participantId));
    } else {
        if (discoveryProxy == NULL) {
            return RequestStatus(RequestStatusCode::ERROR,
                                 "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                                 "local capabilitites directory.");
        }
        status = discoveryProxy->lookup(result, participantId);
    }
    checkForLocalAvailabilityAndAddInProcessConnection(result);
    return status;
}

// inherited from joynr::system::IDiscoverySync
joynr::RequestStatus LocalDiscoveryAggregator::remove(const std::string& participantId)
{
    if (discoveryProxy == NULL) {
        return RequestStatus(RequestStatusCode::ERROR,
                             "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                             "local capabilitites directory.");
    }
    return discoveryProxy->remove(participantId);
}

} // namespace joynr

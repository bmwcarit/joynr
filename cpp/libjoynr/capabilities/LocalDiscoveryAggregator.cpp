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
#include "joynr/LocalDiscoveryAggregator.h"

#include <utility>

#include "joynr/types/CommunicationMiddleware.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/SystemServicesSettings.h"

#include "joynr/types/ProviderQos.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/Version.h"
#include "joynr/system/IRouting.h"
#include "joynr/system/IDiscovery.h"

namespace joynr
{

LocalDiscoveryAggregator::LocalDiscoveryAggregator(
        IRequestCallerDirectory& requestCallerDirectory,
        const SystemServicesSettings& systemServicesSettings)
        : discoveryProxy(),
          requestCallerDirectory(requestCallerDirectory),
          provisionedDiscoveryEntries(),
          systemServicesSettings(systemServicesSettings)
{
    std::vector<joynr::types::CommunicationMiddleware::Enum> connections;
    connections.push_back(joynr::types::CommunicationMiddleware::JOYNR);
    joynr::types::Version providerVersion;
    joynr::types::DiscoveryEntry routingProviderDiscoveryEntry(
            providerVersion,
            systemServicesSettings.getDomain(),
            joynr::system::IRouting::INTERFACE_NAME(),
            systemServicesSettings.getCcRoutingProviderParticipantId(),
            joynr::types::ProviderQos(),
            connections);
    provisionedDiscoveryEntries.insert(std::make_pair(
            routingProviderDiscoveryEntry.getParticipantId(), routingProviderDiscoveryEntry));
    joynr::types::DiscoveryEntry discoveryProviderDiscoveryEntry(
            providerVersion,
            systemServicesSettings.getDomain(),
            joynr::system::IDiscovery::INTERFACE_NAME(),
            systemServicesSettings.getCcDiscoveryProviderParticipantId(),
            joynr::types::ProviderQos(),
            connections);
    provisionedDiscoveryEntries.insert(std::make_pair(
            discoveryProviderDiscoveryEntry.getParticipantId(), discoveryProviderDiscoveryEntry));
}

void LocalDiscoveryAggregator::setDiscoveryProxy(
        std::unique_ptr<joynr::system::IDiscoverySync> discoveryProxy)
{
    this->discoveryProxy = std::move(discoveryProxy);
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::add(const joynr::types::DiscoveryEntry& discoveryEntry)
{
    if (!discoveryProxy) {
        throw exceptions::JoynrRuntimeException(
                "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                "local capabilitites directory.");
    }

    discoveryProxy->add(discoveryEntry);
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
void LocalDiscoveryAggregator::lookup(std::vector<joynr::types::DiscoveryEntry>& result,
                                      const std::string& domain,
                                      const std::string& interfaceName,
                                      const joynr::types::DiscoveryQos& discoveryQos)
{
    if (!discoveryProxy) {
        throw exceptions::JoynrRuntimeException(
                "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                "local capabilitites directory.");
    }
    discoveryProxy->lookup(result, domain, interfaceName, discoveryQos);

    for (joynr::types::DiscoveryEntry& discoveryEntry : result) {
        checkForLocalAvailabilityAndAddInProcessConnection(discoveryEntry);
    }
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::lookup(joynr::types::DiscoveryEntry& result,
                                      const std::string& participantId)
{
    auto entry = provisionedDiscoveryEntries.find(participantId);
    if (entry != provisionedDiscoveryEntries.end()) {
        result = entry->second;
    } else {
        if (!discoveryProxy) {
            throw exceptions::JoynrRuntimeException(
                    "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                    "local capabilitites directory.");
        }
        discoveryProxy->lookup(result, participantId);
    }
    checkForLocalAvailabilityAndAddInProcessConnection(result);
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::remove(const std::string& participantId)
{
    if (!discoveryProxy) {
        throw exceptions::JoynrRuntimeException(
                "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                "local capabilitites directory.");
    }
    discoveryProxy->remove(participantId);
}

} // namespace joynr

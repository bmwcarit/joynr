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
#include <chrono>
#include <limits>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/infrastructure/IGlobalCapabilitiesDirectory.h"

#include "joynr/MessagingSettings.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/Version.h"
#include "joynr/system/IRouting.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/CapabilityUtils.h"

namespace joynr
{

LocalDiscoveryAggregator::LocalDiscoveryAggregator(
        const SystemServicesSettings& systemServicesSettings,
        const MessagingSettings& messagingSettings,
        bool provisionClusterControllerDiscoveryEntries)
        : discoveryProxy(), provisionedDiscoveryEntries()
{
    std::int64_t lastSeenDateMs = 0;
    std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    std::string defaultPublicKeyId("");

    joynr::types::Version routingProviderVersion(
            joynr::system::IRouting::MAJOR_VERSION, joynr::system::IRouting::MINOR_VERSION);
    joynr::types::Version discoveryProviderVersion(
            joynr::system::IDiscovery::MAJOR_VERSION, joynr::system::IDiscovery::MINOR_VERSION);
    joynr::types::DiscoveryEntryWithMetaInfo routingProviderDiscoveryEntry(
            routingProviderVersion,
            systemServicesSettings.getDomain(),
            joynr::system::IRouting::INTERFACE_NAME(),
            systemServicesSettings.getCcRoutingProviderParticipantId(),
            joynr::types::ProviderQos(),
            lastSeenDateMs,
            expiryDateMs,
            defaultPublicKeyId,
            true);
    provisionedDiscoveryEntries.insert(std::make_pair(
            routingProviderDiscoveryEntry.getParticipantId(), routingProviderDiscoveryEntry));
    joynr::types::DiscoveryEntryWithMetaInfo discoveryProviderDiscoveryEntry(
            discoveryProviderVersion,
            systemServicesSettings.getDomain(),
            joynr::system::IDiscovery::INTERFACE_NAME(),
            systemServicesSettings.getCcDiscoveryProviderParticipantId(),
            joynr::types::ProviderQos(),
            lastSeenDateMs,
            expiryDateMs,
            defaultPublicKeyId,
            true);
    provisionedDiscoveryEntries.insert(std::make_pair(
            discoveryProviderDiscoveryEntry.getParticipantId(), discoveryProviderDiscoveryEntry));

    if (provisionClusterControllerDiscoveryEntries) {
        // setting up the provisioned values for GlobalCapabilitiesClient
        // The GlobalCapabilitiesServer is also provisioned in MessageRouter
        types::ProviderQos capabilityProviderQos;
        capabilityProviderQos.setPriority(1);
        types::Version capabilityProviderVersion(
                infrastructure::IGlobalCapabilitiesDirectory::MAJOR_VERSION,
                infrastructure::IGlobalCapabilitiesDirectory::MINOR_VERSION);
        provisionedDiscoveryEntries.insert(std::make_pair(
                messagingSettings.getCapabilitiesDirectoryParticipantId(),
                types::DiscoveryEntryWithMetaInfo(
                        capabilityProviderVersion,
                        messagingSettings.getDiscoveryDirectoriesDomain(),
                        infrastructure::IGlobalCapabilitiesDirectory::INTERFACE_NAME(),
                        messagingSettings.getCapabilitiesDirectoryParticipantId(),
                        capabilityProviderQos,
                        lastSeenDateMs,
                        expiryDateMs,
                        defaultPublicKeyId,
                        false)));
    }
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

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::lookup(std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result,
                                      const std::vector<std::string>& domains,
                                      const std::string& interfaceName,
                                      const joynr::types::DiscoveryQos& discoveryQos)
{
    if (!discoveryProxy) {
        throw exceptions::JoynrRuntimeException(
                "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                "local capabilitites directory.");
    }
    discoveryProxy->lookup(result, domains, interfaceName, discoveryQos);
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::lookup(joynr::types::DiscoveryEntryWithMetaInfo& result,
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

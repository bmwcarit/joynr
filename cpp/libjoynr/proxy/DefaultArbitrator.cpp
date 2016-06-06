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
#include "joynr/DefaultArbitrator.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/exceptions/JoynrException.h"

#include <vector>

namespace joynr
{

INIT_LOGGER(DefaultArbitrator);

DefaultArbitrator::DefaultArbitrator(const std::string& domain,
                                     const std::string& interfaceName,
                                     const joynr::types::Version& interfaceVersion,
                                     joynr::system::IDiscoverySync& discoveryProxy,
                                     const DiscoveryQos& discoveryQos)
        : ProviderArbitrator(domain, interfaceName, interfaceVersion, discoveryProxy, discoveryQos)
{
}

void DefaultArbitrator::attemptArbitration()
{
    std::vector<joynr::types::DiscoveryEntry> result;
    try {
        discoveryProxy.lookup(result, domains, interfaceName, systemDiscoveryQos);
        receiveCapabilitiesLookupResults(result);
    } catch (const exceptions::JoynrException& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to lookup provider (domain: {}, interface: {}) "
                        "from discovery. Error: {}",
                        domains.size() > 0 ? domains.at(0) : "EMPTY",
                        interfaceName,
                        e.getMessage());
    }
}

void DefaultArbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    // Check for empty results
    if (discoveryEntries.size() == 0)
        return;

    // default arbitrator picks first entry with compatible version
    joynr::types::Version providerVersion;
    for (const joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        providerVersion = discoveryEntry.getProviderVersion();
        if (providerVersion.getMajorVersion() == interfaceVersion.getMajorVersion() &&
            providerVersion.getMinorVersion() >= interfaceVersion.getMinorVersion()) {
            updateArbitrationStatusParticipantIdAndAddress(
                    ArbitrationStatus::ArbitrationSuccessful, discoveryEntry.getParticipantId());
        }
    }
}

} // namespace joynr

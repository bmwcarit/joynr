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
#include "joynr/KeywordArbitrator.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/TypeUtil.h"

namespace joynr
{

INIT_LOGGER(KeywordArbitrator);

KeywordArbitrator::KeywordArbitrator(const std::string& domain,
                                     const std::string& interfaceName,
                                     const joynr::types::Version& interfaceVersion,
                                     joynr::system::IDiscoverySync& discoveryProxy,
                                     const DiscoveryQos& discoveryQos)
        : ProviderArbitrator(domain, interfaceName, interfaceVersion, discoveryProxy, discoveryQos),
          keyword(discoveryQos.getCustomParameter(DiscoveryQos::KEYWORD_PARAMETER()).getValue())
{
}

void KeywordArbitrator::attemptArbitration()
{
    std::vector<joynr::types::DiscoveryEntry> result;
    try {
        discoveryProxy.lookup(result, domains, interfaceName, systemDiscoveryQos);
        receiveCapabilitiesLookupResults(result);
    } catch (const exceptions::JoynrException& e) {
        JOYNR_LOG_ERROR(
                logger,
                "Unable to lookup provider (domain: {}, interface: {}from discovery. Error: {}",
                domains.size() > 0 ? domains.at(0) : "EMPTY",
                interfaceName,
                e.getMessage());
    }
}

void KeywordArbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    // Check for an empty list of results
    if (discoveryEntries.size() == 0) {
        return;
    }
    discoveredIncompatibleVersions.clear();

    // Loop through the result list
    joynr::types::Version providerVersion;
    for (joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        types::ProviderQos providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger, "Looping over capabilitiesEntry: {}", discoveryEntry.toString());
        providerVersion = discoveryEntry.getProviderVersion();

        // Check that the provider supports onChange subscriptions if this was requested
        if (discoveryQos.getProviderMustSupportOnChange() &&
            !providerQos.getSupportsOnChangeSubscriptions()) {
            continue;
        }

        // Search the QosParameters for the keyword field
        std::vector<types::CustomParameter> qosParameters = providerQos.getCustomParameters();
        for (types::CustomParameter parameter : qosParameters) {
            std::string name = parameter.getName();
            if (!(name == DiscoveryQos::KEYWORD_PARAMETER() && keyword == parameter.getValue())) {
                continue;
            } else if (providerVersion.getMajorVersion() != interfaceVersion.getMajorVersion() ||
                       providerVersion.getMinorVersion() < interfaceVersion.getMinorVersion()) {
                discoveredIncompatibleVersions.insert(providerVersion);
            } else {
                std::string res = discoveryEntry.getParticipantId();
                JOYNR_LOG_TRACE(logger, "setting res to {}", res);
                notifyArbitrationListener(res);
                return;
            }
        }
    }

    // If this point is reached, no provider with the keyword was found
}

} // namespace joynr

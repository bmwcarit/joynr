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
        std::string errorMsg = "Unable to lookup provider (domain: " +
                               (domains.size() > 0 ? domains.at(0) : std::string("EMPTY")) +
                               ", interface: " + interfaceName + ") from discovery. Error: " +
                               e.getMessage();
        JOYNR_LOG_ERROR(logger, errorMsg);
        arbitrationError.setMessage(errorMsg);
    }
}

void KeywordArbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    discoveredIncompatibleVersions.clear();

    // Check for an empty list of results
    if (discoveryEntries.size() == 0) {
        arbitrationError.setMessage("No entries found for domain: " +
                                    (domains.size() > 0 ? domains.at(0) : std::string("EMPTY")) +
                                    ", interface: " + interfaceName);
        return;
    }
    std::size_t providersWithoutSupportOnChange = 0;
    std::size_t providersWithIncompatibleVersion = 0;
    // Loop through the result list
    joynr::types::Version providerVersion;
    for (joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        types::ProviderQos providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger, "Looping over capabilitiesEntry: {}", discoveryEntry.toString());
        providerVersion = discoveryEntry.getProviderVersion();

        // Check that the provider supports onChange subscriptions if this was requested
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

        // Search the QosParameters for the keyword field
        std::vector<types::CustomParameter> qosParameters = providerQos.getCustomParameters();
        for (types::CustomParameter parameter : qosParameters) {
            std::string name = parameter.getName();
            if (!(name == DiscoveryQos::KEYWORD_PARAMETER() && keyword == parameter.getValue())) {
                continue;
            } else {
                std::string res = discoveryEntry.getParticipantId();
                JOYNR_LOG_TRACE(logger, "setting res to {}", res);
                notifyArbitrationListener(res);
                return;
            }
        }
    }

    // If this point is reached, no provider with the keyword was found
    std::string errorMsg;
    if (providersWithoutSupportOnChange == discoveryEntries.size()) {
        errorMsg = "There was more than one entries in capabilitiesEntries, but none supported "
                   "on change subscriptions.";
        JOYNR_LOG_WARN(logger, errorMsg);
        arbitrationError.setMessage(errorMsg);
    } else if ((providersWithoutSupportOnChange + providersWithIncompatibleVersion) <
               discoveryEntries.size()) {
        errorMsg = "There was more than one entries in capabilitiesEntries, but none of the "
                   "compatible entries had the correct keyword.";
        JOYNR_LOG_WARN(logger, errorMsg);
        arbitrationError.setMessage(errorMsg);
    } else {
        errorMsg = "There was more than one entries in capabilitiesEntries, but none "
                   "was compatible.";
        JOYNR_LOG_WARN(logger, errorMsg);
        arbitrationError.setMessage(errorMsg);
    }
}

} // namespace joynr

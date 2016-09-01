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

std::string KeywordArbitrator::filterDiscoveryEntries(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    std::string res;
    for (joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        types::ProviderQos providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger, "Looping over capabilitiesEntry: {}", discoveryEntry.toString());

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
            }
        }
    }

    if (res.empty()) {
        std::string errorMsg;
        errorMsg = "There was more than one entries in capabilitiesEntries, but none of the "
                   "compatible entries had the correct keyword.";
        JOYNR_LOG_WARN(logger, errorMsg);
        arbitrationError.setMessage(errorMsg);
    }
    return res;
}

} // namespace joynr

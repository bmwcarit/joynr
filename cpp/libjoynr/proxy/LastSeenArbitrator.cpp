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
#include "joynr/LastSeenArbitrator.h"

#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

INIT_LOGGER(LastSeenArbitrator);

LastSeenArbitrator::LastSeenArbitrator(const std::string& domain,
                                       const std::string& interfaceName,
                                       const joynr::types::Version& interfaceVersion,
                                       joynr::system::IDiscoverySync& discoveryProxy,
                                       const DiscoveryQos& discoveryQos)
        : Arbitrator(domain, interfaceName, interfaceVersion, discoveryProxy, discoveryQos)
{
}

std::string LastSeenArbitrator::filterDiscoveryEntries(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{

    std::string res;
    int64_t latestLastSeenDateMs = 0;

    for (const joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        JOYNR_LOG_TRACE(logger, "Looping over capabilitiesEntry: {}", discoveryEntry.toString());
        int64_t lastSeenDateMs = discoveryEntry.getLastSeenDateMs();

        if (lastSeenDateMs > latestLastSeenDateMs) {
            latestLastSeenDateMs = discoveryEntry.getLastSeenDateMs();
            res = discoveryEntry.getParticipantId();
        }
    }

    if (res.empty()) {
        std::string errorMsg;
        errorMsg = "There was more than one entries in capabilitiesEntries, but none of the "
                   "compatible entries had a acceptable LastSeenDateMs";
        JOYNR_LOG_WARN(logger, errorMsg);
        arbitrationError.setMessage(errorMsg);
    }
    return res;
}
} // namespace joynr

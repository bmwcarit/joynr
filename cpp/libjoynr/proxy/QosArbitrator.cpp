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
#include "joynr/QosArbitrator.h"

#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/TypeUtil.h"

#include <cassert>

namespace joynr
{

INIT_LOGGER(QosArbitrator);

QosArbitrator::QosArbitrator(const std::string& domain,
                             const std::string& interfaceName,
                             joynr::system::IDiscoverySync& discoveryProxy,
                             const DiscoveryQos& discoveryQos)
        : ProviderArbitrator(domain, interfaceName, discoveryProxy, discoveryQos)
{
}

void QosArbitrator::attemptArbitration()
{
    std::vector<joynr::types::DiscoveryEntry> result;
    try {
        discoveryProxy.lookup(result, domain, interfaceName, systemDiscoveryQos);
        receiveCapabilitiesLookupResults(result);
    } catch (const exceptions::JoynrException& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to lookup provider (domain: {}, interface: {}) "
                        "from discovery. Error: {}",
                        domain,
                        interfaceName,
                        e.getMessage());
    }
}

// Returns true if arbitration was successful, false otherwise
void QosArbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    std::string res = "";

    // Check for empty results
    if (discoveryEntries.size() == 0)
        return;

    std::int64_t highestPriority = -1;
    for (const joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        types::ProviderQos providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger, "Looping over capabilitiesEntry: {}", discoveryEntry.toString());
        if (discoveryQos.getProviderMustSupportOnChange() &&
            !providerQos.getSupportsOnChangeSubscriptions()) {
            continue;
        }
        if (providerQos.getPriority() > highestPriority) {
            res = discoveryEntry.getParticipantId();
            JOYNR_LOG_TRACE(logger, "setting res to {}", res);
            highestPriority = providerQos.getPriority();
        }
    }
    if (res == "") {
        JOYNR_LOG_WARN(logger,
                       "There was more than one entries in capabilitiesEntries, but none "
                       "had a Priority > -1");
        return;
    }

    updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationSuccessful, res);
}

} // namespace joynr

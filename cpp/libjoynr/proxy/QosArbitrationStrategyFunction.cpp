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
#include "joynr/QosArbitrationStrategyFunction.h"

#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

INIT_LOGGER(QosArbitrationStrategyFunction);

types::DiscoveryEntryWithMetaInfo QosArbitrationStrategyFunction::select(
        const std::map<std::string, types::CustomParameter> customParameters,
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const
{
    std::ignore = customParameters;
    auto selectedDiscoveryEntryIt = discoveryEntries.cend();
    std::int64_t highestPriority = -1;

    for (auto it = discoveryEntries.cbegin(); it != discoveryEntries.cend(); ++it) {
        types::ProviderQos providerQos = it->getQos();
        JOYNR_LOG_TRACE(logger, "Looping over discoveryEntry: {}", it->toString());

        if (providerQos.getPriority() > highestPriority) {
            selectedDiscoveryEntryIt = it;
            JOYNR_LOG_TRACE(logger,
                            "setting selectedParticipantId to {}",
                            selectedDiscoveryEntryIt->getParticipantId());
            highestPriority = providerQos.getPriority();
        }
    }

    if (selectedDiscoveryEntryIt == discoveryEntries.cend()) {
        std::string errorMsg;
        errorMsg = "There was more than one entries in capabilitiesEntries, but none of the "
                   "compatible entries had a priority > -1";
        JOYNR_LOG_WARN(logger, errorMsg);
        throw exceptions::DiscoveryException(errorMsg);
    }

    return *selectedDiscoveryEntryIt;
}
} // namespace joynr

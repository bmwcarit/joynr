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

#include "joynr/types/DiscoveryEntry.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/TypeUtil.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

INIT_LOGGER(QosArbitrationStrategyFunction);

std::string QosArbitrationStrategyFunction::select(
        const std::map<std::string, types::CustomParameter> customParameters,
        const std::vector<types::DiscoveryEntry>& discoveryEntries) const
{
    std::ignore = customParameters;
    std::string selectedParticipantId;
    std::int64_t highestPriority = -1;

    for (const auto& discoveryEntry : discoveryEntries) {
        types::ProviderQos providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger, "Looping over discoveryEntry: {}", discoveryEntry.toString());

        if (providerQos.getPriority() > highestPriority) {
            selectedParticipantId = discoveryEntry.getParticipantId();
            JOYNR_LOG_TRACE(logger, "setting selectedParticipantId to {}", selectedParticipantId);
            highestPriority = providerQos.getPriority();
        }
    }

    if (selectedParticipantId.empty()) {
        std::string errorMsg;
        errorMsg = "There was more than one entries in capabilitiesEntries, but none of the "
                   "compatible entries had a priority > -1";
        JOYNR_LOG_WARN(logger, errorMsg);
        throw exceptions::DiscoveryException(errorMsg);
    }
    return selectedParticipantId;
}
} // namespace joynr

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
#include "joynr/LastSeenArbitrationStrategyFunction.h"

#include "joynr/types/DiscoveryEntry.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

INIT_LOGGER(LastSeenArbitrationStrategyFunction);

std::string LastSeenArbitrationStrategyFunction::select(
        const std::map<std::string, types::CustomParameter> customParameters,
        const std::vector<types::DiscoveryEntry>& discoveryEntries) const
{
    std::ignore = customParameters;
    std::string selectedParticipantId;
    int64_t latestLastSeenDateMs = -1;

    for (const auto& discoveryEntry : discoveryEntries) {
        JOYNR_LOG_TRACE(logger, "Looping over discoveryEntry: {}", discoveryEntry.toString());
        int64_t lastSeenDateMs = discoveryEntry.getLastSeenDateMs();

        if (lastSeenDateMs > latestLastSeenDateMs) {
            latestLastSeenDateMs = lastSeenDateMs;
            selectedParticipantId = discoveryEntry.getParticipantId();
        }
    }

    if (selectedParticipantId.empty()) {
        std::string errorMsg;
        errorMsg = "There was one or more entries in capabilitiesEntries, but the LastSeenDateMs "
                   "wasn't set.";
        JOYNR_LOG_WARN(logger, errorMsg);
        throw exceptions::DiscoveryException(errorMsg);
    }
    return selectedParticipantId;
}
} // namespace joynr

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include "joynr/DiscoveryQos.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/ProviderQos.h"

namespace joynr
{

types::DiscoveryEntryWithMetaInfo LastSeenArbitrationStrategyFunction::select(
        const std::map<std::string, types::CustomParameter> customParameters,
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const
{
    std::ignore = customParameters;
    auto selectedEntryIt = discoveryEntries.cend();
    int64_t latestLastSeenDateMs = -1;

    for (auto it = discoveryEntries.cbegin(); it != discoveryEntries.cend(); ++it) {
        JOYNR_LOG_TRACE(logger(), "Looping over discoveryEntry: {}", it->toString());
        int64_t lastSeenDateMs = it->getLastSeenDateMs();

        if (lastSeenDateMs > latestLastSeenDateMs) {
            latestLastSeenDateMs = lastSeenDateMs;
            selectedEntryIt = it;
        }
    }

    if (selectedEntryIt == discoveryEntries.cend()) {
        std::string errorMsg;
        errorMsg = "There was one or more entries in capabilitiesEntries, but the LastSeenDateMs "
                   "wasn't set.";
        JOYNR_LOG_WARN(logger(), errorMsg);
        throw exceptions::DiscoveryException(errorMsg);
    }

    return *selectedEntryIt;
}
} // namespace joynr

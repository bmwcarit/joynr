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

#include "joynr/KeywordArbitrationStrategyFunction.h"

#include <stdexcept>

#include "joynr/DiscoveryQos.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

types::DiscoveryEntryWithMetaInfo KeywordArbitrationStrategyFunction::select(
        const std::map<std::string, types::CustomParameter> customParameters,
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const
{
    std::string keyword;
    try {
        keyword = customParameters.at(DiscoveryQos::KEYWORD_PARAMETER()).getValue();
    } catch (const std::out_of_range& e) {
        throw exceptions::DiscoveryException("No keyword found in customParameters.");
    }
    if (keyword.empty()) {
        std::string errorMsg;
        errorMsg = "The Keyword is not set.";
        JOYNR_LOG_WARN(logger(), errorMsg);
        throw exceptions::DiscoveryException(errorMsg);
    }
    for (const auto& discoveryEntry : discoveryEntries) {
        const types::ProviderQos& providerQos = discoveryEntry.getQos();
        JOYNR_LOG_TRACE(logger(), "Looping over capabilitiesEntry: {}", discoveryEntry.toString());

        // Search the providerQos.getCustomParameters() for the keyword field
        std::vector<types::CustomParameter> qosParameters = providerQos.getCustomParameters();
        for (types::CustomParameter& parameter : qosParameters) {
            if (parameter.getName() == DiscoveryQos::KEYWORD_PARAMETER() &&
                keyword == parameter.getValue()) {
                JOYNR_LOG_TRACE(logger(),
                                "setting selectedParticipantId to {}",
                                discoveryEntry.getParticipantId());
                return discoveryEntry;
            }
        }
    }

    std::string errorMsg;
    errorMsg = "There was more than one entries in capabilitiesEntries, but none of the "
               "compatible entries had the correct keyword.";
    JOYNR_LOG_WARN(logger(), errorMsg);
    throw exceptions::DiscoveryException(errorMsg);
}

} // namespace joynr

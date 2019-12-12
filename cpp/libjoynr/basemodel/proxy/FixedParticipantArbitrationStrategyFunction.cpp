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
#include "joynr/FixedParticipantArbitrationStrategyFunction.h"

#include <tuple> // for std::ignore

#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

types::DiscoveryEntryWithMetaInfo FixedParticipantArbitrationStrategyFunction::select(
        const std::map<std::string, types::CustomParameter> customParameters,
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const
{
    std::ignore = customParameters;
    if (discoveryEntries.empty()) {
        throw exceptions::DiscoveryException("No provider found for given ParticipantId");
    }

    if (discoveryEntries.size() > 1) {
        throw exceptions::DiscoveryException("No provider found for given ParticipantId");
    }

    auto selectedDiscovery = discoveryEntries.front();
    JOYNR_LOG_TRACE(
            logger(), "setting selectedParticipantId to {}", selectedDiscovery.getParticipantId());
    return selectedDiscovery;
}
} // namespace joynr

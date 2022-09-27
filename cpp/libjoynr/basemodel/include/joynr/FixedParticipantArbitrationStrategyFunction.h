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
#ifndef FIXEDPARTICIPANTARBITRATIONSTRATEGYFUNCTION_H
#define FIXEDPARTICIPANTARBITRATIONSTRATEGYFUNCTION_H
#include <map>
#include <string>
#include <vector>

#include "joynr/ArbitrationStrategyFunction.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

namespace types
{
class DiscoveryEntry;
class CustomParameter;
} // namespace types

/**
 * The FixedParticipant Arbitration Strategy Function arbitrates according to a specific
 * ParticipantId of a provider.
 */
class FixedParticipantArbitrationStrategyFunction final : public ArbitrationStrategyFunction
{

public:
    ~FixedParticipantArbitrationStrategyFunction() = default;
    FixedParticipantArbitrationStrategyFunction() = default;
    types::DiscoveryEntryWithMetaInfo select(
            const std::map<std::string, types::CustomParameter> customParameters,
            const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const;

private:
    DISALLOW_COPY_AND_ASSIGN(FixedParticipantArbitrationStrategyFunction);
    ADD_LOGGER(FixedParticipantArbitrationStrategyFunction)
};

} // namespace joynr
#endif // FIXEDPARTICIPANTARBITRATIONSTRATEGYFUNCTION_H

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
#ifndef KEYWORDARBITRATIONSTRATEGYFUNCTION_H
#define KEYWORDARBITRATIONSTRATEGYFUNCTION_H
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
class DiscoveryEntryWithMetaInfo;
} // namespace types

/**
  * The Keyword Arbitration Strategy requests CapabilityEntries for a give interfaceName and domain
 * from
  * the LocalCapabilitiesDirectory and filters them by a keyword in the qos. The first address
  * of the first entry with the correct keyword is returned.
  */
class KeywordArbitrationStrategyFunction : public ArbitrationStrategyFunction
{

public:
    ~KeywordArbitrationStrategyFunction() final = default;
    KeywordArbitrationStrategyFunction() = default;

    types::DiscoveryEntryWithMetaInfo select(
            const std::map<std::string, types::CustomParameter> customParameters,
            const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const final;

private:
    DISALLOW_COPY_AND_ASSIGN(KeywordArbitrationStrategyFunction);
    ADD_LOGGER(KeywordArbitrationStrategyFunction)
};

} // namespace joynr
#endif // KEYWORDARBITRATIONSTRATEGYFUNCTION_H

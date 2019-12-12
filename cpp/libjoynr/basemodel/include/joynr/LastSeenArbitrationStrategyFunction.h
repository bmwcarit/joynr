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
#ifndef LASTSEENARBITRATIONSTRATEGYFUNCTION_H
#define LASTSEENARBITRATIONSTRATEGYFUNCTION_H
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
class CustomParameter;
} // namespace types

/*
  * The Last Seen Arbitration Strategy arbitrates according to the LastSeenDateMs of the provider.
  * It arbitrates to the provider with the latest LastSeenDateMs.
  * This is the default arbitration strategy.
  */
class LastSeenArbitrationStrategyFunction : public ArbitrationStrategyFunction
{

public:
    ~LastSeenArbitrationStrategyFunction() final = default;
    LastSeenArbitrationStrategyFunction() = default;

    types::DiscoveryEntryWithMetaInfo select(
            std::map<std::string, types::CustomParameter> customParameters,
            const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const final;

private:
    DISALLOW_COPY_AND_ASSIGN(LastSeenArbitrationStrategyFunction);
    ADD_LOGGER(LastSeenArbitrationStrategyFunction)
};

} // namespace joynr
#endif // LASTSEENARBITRATIONSTRATEGYFUNCTION_H

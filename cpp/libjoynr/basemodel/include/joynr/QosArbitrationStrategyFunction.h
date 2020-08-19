
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
#ifndef QOSARBITRATIONSTRATEGYFUNCTION_H
#define QOSARBITRATIONSTRATEGYFUNCTION_H

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
  * The QoS Arbitration Strategy Function arbitrates according to the QoS of the provider.
  * It arbitrates to the provider with the highest priority.
  */
class QosArbitrationStrategyFunction final : public ArbitrationStrategyFunction
{

public:
    ~QosArbitrationStrategyFunction() = default;
    QosArbitrationStrategyFunction() = default;
    types::DiscoveryEntryWithMetaInfo select(
            const std::map<std::string, types::CustomParameter> customParameters,
            const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries) const;

private:
    DISALLOW_COPY_AND_ASSIGN(QosArbitrationStrategyFunction);
    ADD_LOGGER(QosArbitrationStrategyFunction)
};

} // namespace joynr
#endif // QOSARBITRATIONSTRATEGYFUNCTION_H

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

#include "joynr/ArbitratorFactory.h"
#include "joynr/ArbitrationStrategyFunction.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/FixedParticipantArbitrationStrategyFunction.h"
#include "joynr/KeywordArbitrationStrategyFunction.h"
#include "joynr/LastSeenArbitrationStrategyFunction.h"
#include "joynr/QosArbitrationStrategyFunction.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

std::shared_ptr<Arbitrator> ArbitratorFactory::createArbitrator(
        const std::string& domain,
        const std::string& interfaceName,
        const joynr::types::Version& interfaceVersion,
        std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
        const DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids)
{
    std::unique_ptr<ArbitrationStrategyFunction> arbitrationStrategyFunction;
    DiscoveryQos::ArbitrationStrategy strategy = discoveryQos.getArbitrationStrategy();
    switch (strategy) {
    case DiscoveryQos::ArbitrationStrategy::LAST_SEEN:
        arbitrationStrategyFunction = std::make_unique<LastSeenArbitrationStrategyFunction>();
        break;
    case DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT:
        if (discoveryQos.getCustomParameters().count("fixedParticipantId") == 0) {
            throw exceptions::DiscoveryException(
                    "Arbitrator creation failed: CustomParameter \"fixedParticipantId\" not set");
        }
        arbitrationStrategyFunction =
                std::make_unique<FixedParticipantArbitrationStrategyFunction>();
        break;
    case DiscoveryQos::ArbitrationStrategy::LOCAL_ONLY:
        throw exceptions::DiscoveryException("Arbitration: Local-only not implemented yet.");
    case DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY:
        arbitrationStrategyFunction = std::make_unique<QosArbitrationStrategyFunction>();
        break;
    case DiscoveryQos::ArbitrationStrategy::KEYWORD:
        if (discoveryQos.getCustomParameters().count("keyword") == 0) {
            throw exceptions::DiscoveryException(
                    "Arbitrator creation failed: CustomParameter \"keyword\" not set");
        }
        arbitrationStrategyFunction = std::make_unique<KeywordArbitrationStrategyFunction>();
        break;
    default:
        throw exceptions::DiscoveryException("Arbitrator creation failed: Invalid strategy!");
    }
    return std::make_shared<Arbitrator>(domain,
                                        interfaceName,
                                        interfaceVersion,
                                        discoveryProxy,
                                        discoveryQos,
                                        gbids,
                                        std::move(arbitrationStrategyFunction));
}

} // namespace joynr

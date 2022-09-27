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

#include "joynr/WebSocketMulticastAddressCalculator.h"

#include <memory>
#include <tuple>

#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

namespace joynr
{
WebSocketMulticastAddressCalculator::WebSocketMulticastAddressCalculator(
        std::shared_ptr<const system::RoutingTypes::WebSocketAddress> clusterControllerAddress)
        : _clusterControllerAddress(clusterControllerAddress)
{
}

std::vector<std::shared_ptr<const system::RoutingTypes::Address>>
WebSocketMulticastAddressCalculator::compute(const ImmutableMessage& message)
{
    std::ignore = message;
    return std::vector<std::shared_ptr<const system::RoutingTypes::Address>>{
            _clusterControllerAddress};
}
} // namespace joynr

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

#include "joynr/HttpMulticastAddressCalculator.h"

#include <memory>
#include <tuple>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"

namespace joynr
{
HttpMulticastAddressCalculator::HttpMulticastAddressCalculator(
        std::shared_ptr<const system::RoutingTypes::ChannelAddress> globalAddress)
        : _globalAddress(globalAddress)
{
}

std::vector<std::shared_ptr<const system::RoutingTypes::Address>> HttpMulticastAddressCalculator::
        compute(const ImmutableMessage& message)
{
    std::ignore = message;
    throw exceptions::JoynrRuntimeException("Not implemented...yet!");
}
}

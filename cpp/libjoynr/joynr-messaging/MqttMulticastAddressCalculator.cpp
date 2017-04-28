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

#include "joynr/MqttMulticastAddressCalculator.h"

#include <memory>
#include <tuple>

#include <joynr/ImmutableMessage.h>
#include <joynr/exceptions/JoynrException.h>
#include <joynr/system/RoutingTypes/Address.h>
#include <joynr/system/RoutingTypes/MqttAddress.h>

namespace joynr
{
MqttMulticastAddressCalculator::MqttMulticastAddressCalculator(
        std::shared_ptr<const system::RoutingTypes::MqttAddress> globalAddress,
        const std::string& mqttMulticastTopicPrefix)
        : globalAddress(globalAddress), mqttMulticastTopicPrefix(mqttMulticastTopicPrefix)
{
}

std::shared_ptr<const system::RoutingTypes::Address> MqttMulticastAddressCalculator::compute(
        const ImmutableMessage& message)
{
    if (!globalAddress) {
        return std::shared_ptr<const system::RoutingTypes::MqttAddress>();
    }
    return std::make_shared<const system::RoutingTypes::MqttAddress>(
            globalAddress->getBrokerUri(), mqttMulticastTopicPrefix + message.getRecipient());
}
}

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

#include "joynr/ImmutableMessage.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

namespace joynr
{
MqttMulticastAddressCalculator::MqttMulticastAddressCalculator(
        const std::string& mqttMulticastTopicPrefix,
        std::vector<std::string> availableGbids)
        : _mqttMulticastTopicPrefix(mqttMulticastTopicPrefix), _availableGbids(availableGbids)
{
}

std::vector<std::shared_ptr<const system::RoutingTypes::Address>> MqttMulticastAddressCalculator::
        compute(const ImmutableMessage& message)
{
    std::vector<std::shared_ptr<const system::RoutingTypes::Address>> globalAddressesVector;

    for (const auto& gbid : _availableGbids) {
        globalAddressesVector.push_back(std::make_shared<const system::RoutingTypes::MqttAddress>(
                gbid, _mqttMulticastTopicPrefix + message.getRecipient()));
    }
    return globalAddressesVector;
}
}

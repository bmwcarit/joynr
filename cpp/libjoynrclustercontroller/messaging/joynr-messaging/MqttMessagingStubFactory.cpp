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

#include "MqttMessagingStubFactory.h"

#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "MqttMessagingStub.h"

namespace joynr
{

MqttMessagingStubFactory::MqttMessagingStubFactory(
        std::shared_ptr<ITransportMessageSender> messageSender,
        const std::string& gbid)
        : _messageSender(messageSender), _gbid(gbid)
{
}

bool MqttMessagingStubFactory::canCreate(const joynr::system::RoutingTypes::Address& destAddress)
{
    const auto mqttAddress = dynamic_cast<const system::RoutingTypes::MqttAddress*>(&destAddress);

    if (!mqttAddress) {
        JOYNR_LOG_TRACE(logger(), "The provided address is not of type MqttAddress.");
        return false;
    }

    if (_gbid != (mqttAddress->getBrokerUri())) {
        JOYNR_LOG_TRACE(logger(),
                        "GBID: >{}< is unknown in MqttMessagingStubFactory for GBID: >{}<",
                        mqttAddress->getBrokerUri(),
                        _gbid);
        return false;
    }

    return true;
}

std::shared_ptr<IMessagingStub> MqttMessagingStubFactory::create(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    const auto mqttAddress = dynamic_cast<const system::RoutingTypes::MqttAddress*>(&destAddress);

    if (!mqttAddress) {
        JOYNR_LOG_FATAL(logger(), "The provided address is not of type MqttAddress.");
        return nullptr;
    }

    if (_gbid != (mqttAddress->getBrokerUri())) {
        JOYNR_LOG_FATAL(logger(),
                        "GBID: >{}< is unknown in MqttMessagingStubFactory for GBID: >{}<",
                        mqttAddress->getBrokerUri(),
                        _gbid);
        return nullptr;
    }
    return std::make_shared<MqttMessagingStub>(_messageSender, *mqttAddress);
}

void MqttMessagingStubFactory::registerOnMessagingStubClosedCallback(
        std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                   destinationAddress)> onMessagingStubClosedCallback)
{
    std::ignore = onMessagingStubClosedCallback;
}

} // namespace joynr

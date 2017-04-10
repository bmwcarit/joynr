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
#include "libjoynrclustercontroller/mqtt/MqttReceiver.h"

#include <chrono>

#include "joynr/MessagingSettings.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

namespace joynr
{

INIT_LOGGER(MqttReceiver);

MqttReceiver::MqttReceiver(std::shared_ptr<MosquittoConnection> mosquittoConnection,
                           const MessagingSettings& settings,
                           const std::string& channelIdForMqttTopic,
                           const std::string& unicastTopicPrefix)
        : channelIdForMqttTopic(channelIdForMqttTopic),
          globalClusterControllerAddress(),
          mosquittoConnection(mosquittoConnection)
{
    std::string brokerUri =
            "tcp://" + settings.getBrokerUrl().getBrokerChannelsBaseUrl().getHost() + ":" +
            std::to_string(settings.getBrokerUrl().getBrokerChannelsBaseUrl().getPort());

    std::string unicastChannelIdForMqttTopic = unicastTopicPrefix + channelIdForMqttTopic;
    system::RoutingTypes::MqttAddress receiveMqttAddress(brokerUri, unicastChannelIdForMqttTopic);
    globalClusterControllerAddress = joynr::serializer::serializeToJson(receiveMqttAddress);
    mosquittoConnection->registerChannelId(unicastChannelIdForMqttTopic);
}

void MqttReceiver::updateSettings()
{
}

void MqttReceiver::startReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger, "startReceiveQueue");
}

void MqttReceiver::stopReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger, "stopReceiveQueue");
}

const std::string& MqttReceiver::getGlobalClusterControllerAddress() const
{
    return globalClusterControllerAddress;
}

bool MqttReceiver::tryToDeleteChannel()
{
    return true;
}

bool MqttReceiver::isConnected()
{
    return mosquittoConnection->isSubscribedToChannelTopic();
}

void MqttReceiver::registerReceiveCallback(
        std::function<void(const std::string&)> onTextMessageReceived)
{
    mosquittoConnection->registerReceiveCallback(onTextMessageReceived);
}

void MqttReceiver::subscribeToTopic(const std::string& topic)
{
    mosquittoConnection->subscribeToTopic(topic);
}

void MqttReceiver::unsubscribeFromTopic(const std::string& topic)
{
    mosquittoConnection->unsubscribeFromTopic(topic);
}

} // namespace joynr

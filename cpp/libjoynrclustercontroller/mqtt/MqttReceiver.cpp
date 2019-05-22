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
#include "joynr/MqttReceiver.h"

#include "joynr/MessagingSettings.h"
#include "joynr/serializer/Serializer.h"

#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

namespace joynr
{

MqttReceiver::MqttReceiver(std::shared_ptr<MosquittoConnection> mosquittoConnection,
                           const MessagingSettings& settings,
                           const std::string& channelIdForMqttTopic,
                           const std::string& unicastTopicPrefix)
        : mosquittoConnection(std::move(mosquittoConnection))
{
    const std::string unicastChannelIdForMqttTopic = unicastTopicPrefix + channelIdForMqttTopic;
    std::ignore = settings; // TODO configure GBIDs
    globalClusterControllerAddress =
            system::RoutingTypes::MqttAddress("joynrdefaultgbid", unicastChannelIdForMqttTopic);
    this->mosquittoConnection->registerChannelId(unicastChannelIdForMqttTopic);
}

void MqttReceiver::updateSettings()
{
}

void MqttReceiver::startReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger(), "startReceiveQueue");
}

void MqttReceiver::stopReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger(), "stopReceiveQueue");
}

const std::string MqttReceiver::getSerializedGlobalClusterControllerAddress() const
{
    return joynr::serializer::serializeToJson(globalClusterControllerAddress);
}
const system::RoutingTypes::MqttAddress& MqttReceiver::getGlobalClusterControllerAddress() const
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
        std::function<void(smrf::ByteVector&&)> onMessageReceived)
{
    mosquittoConnection->registerReceiveCallback(std::move(onMessageReceived));
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

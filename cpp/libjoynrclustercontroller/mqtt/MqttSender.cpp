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
#include "libjoynrclustercontroller/mqtt/MqttSender.h"

#include "joynr/ITransportMessageReceiver.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/MessagingQosEffort.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Util.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

namespace joynr
{

MqttSender::MqttSender(std::shared_ptr<MosquittoConnection> mosquittoConnection,
                       const MessagingSettings& settings)
        : _mosquittoConnection(mosquittoConnection),
          _receiver(),
          _mqttMaxMessageSizeBytes(settings.getMqttMaxMessageSizeBytes())
{
}

void MqttSender::sendMessage(
        const system::RoutingTypes::Address& destinationAddress,
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    JOYNR_LOG_TRACE(logger(), "sendMessage: {}", message->toLogMessage());

    auto mqttAddress = dynamic_cast<const system::RoutingTypes::MqttAddress*>(&destinationAddress);
    if (mqttAddress == nullptr) {
        JOYNR_LOG_DEBUG(logger(), "Invalid destination address type provided");
        onFailure(exceptions::JoynrRuntimeException("Invalid destination address type provided"));
        return;
    }

    if (!_mosquittoConnection->isSubscribedToChannelTopic()) {
        const std::string msg = "MqttSender is not connected, delaying message";
        JOYNR_LOG_DEBUG(logger(), msg);
        onFailure(exceptions::JoynrDelayMessageException(std::chrono::seconds(2), msg));
        return;
    }
    std::string topic;
    if (message->getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        topic = mqttAddress->getTopic();
    } else {
        topic = mqttAddress->getTopic() + "/" + _mosquittoConnection->getMqttPrio();
    }

    int qosLevel = _mosquittoConnection->getMqttQos();

    boost::optional<std::string> optionalEffort = message->getEffort();
    if (optionalEffort &&
        *optionalEffort == MessagingQosEffort::getLiteral(MessagingQosEffort::Enum::BEST_EFFORT)) {
        qosLevel = 0;
    }

    const smrf::ByteVector& rawMessage = message->getSerializedMessage();

    if (_mqttMaxMessageSizeBytes != MessagingSettings::NO_MQTT_MAX_MESSAGE_SIZE_BYTES() &&
        ((rawMessage.size() > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) ||
         (static_cast<std::int64_t>(rawMessage.size()) > _mqttMaxMessageSizeBytes))) {
        std::stringstream errorMsg;
        errorMsg << "Message size MQTT Publish failed: maximum allowed message size of "
                 << _mqttMaxMessageSizeBytes << " bytes exceeded, actual size is "
                 << rawMessage.size() << " bytes";
        JOYNR_LOG_DEBUG(logger(), errorMsg.str());
        onFailure(exceptions::JoynrMessageNotSentException(errorMsg.str()));
        return;
    }

    _mosquittoConnection->publishMessage(
            topic, qosLevel, onFailure, rawMessage.size(), rawMessage.data());
}

} // namespace joynr

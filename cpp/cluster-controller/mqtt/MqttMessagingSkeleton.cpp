/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "MqttMessagingSkeleton.h"

#include "MqttReceiver.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MessageRouter.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/Util.h"

namespace joynr
{

INIT_LOGGER(MqttMessagingSkeleton);

const std::string MqttMessagingSkeleton::MQTT_MULTI_LEVEL_WILDCARD("#");

std::string MqttMessagingSkeleton::translateMulticastWildcard(std::string topic)
{
    if (topic.length() > 0 && topic.back() == util::MULTI_LEVEL_WILDCARD[0]) {
        topic.back() = MQTT_MULTI_LEVEL_WILDCARD[0];
    }
    return topic;
}

MqttMessagingSkeleton::MqttMessagingSkeleton(MessageRouter& messageRouter,
                                             std::shared_ptr<MqttReceiver> mqttReceiver,
                                             uint64_t ttlUplift)
        : messageRouter(messageRouter),
          mqttReceiver(mqttReceiver),
          ttlUplift(ttlUplift),
          multicastSubscriptionCount(),
          multicastSubscriptionCountMutex()
{
}

void MqttMessagingSkeleton::registerMulticastSubscription(const std::string& multicastId)
{
    std::string mqttTopic = translateMulticastWildcard(multicastId);
    std::lock_guard<std::mutex> lock(multicastSubscriptionCountMutex);
    if (multicastSubscriptionCount.find(mqttTopic) == multicastSubscriptionCount.cend()) {
        mqttReceiver->subscribeToTopic(mqttTopic);
        multicastSubscriptionCount[mqttTopic] = 1;
    } else {
        multicastSubscriptionCount[mqttTopic]++;
    }
}

void MqttMessagingSkeleton::unregisterMulticastSubscription(const std::string& multicastId)
{
    std::string mqttTopic = translateMulticastWildcard(multicastId);
    std::lock_guard<std::mutex> lock(multicastSubscriptionCountMutex);
    auto countIterator = multicastSubscriptionCount.find(mqttTopic);
    if (countIterator == multicastSubscriptionCount.cend()) {
        JOYNR_LOG_ERROR(
                logger, "unregister multicast subscription called for non existing subscription");
    } else if (countIterator->second == 1) {
        multicastSubscriptionCount.erase(mqttTopic);
        mqttReceiver->unsubscribeFromTopic(mqttTopic);
    } else {
        countIterator->second--;
    }
}

void MqttMessagingSkeleton::transmit(
        JoynrMessage& message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST) {
        std::string serializedReplyAddress = message.getHeaderReplyAddress();

        try {
            using system::RoutingTypes::MqttAddress;
            MqttAddress address;
            joynr::serializer::deserializeFromJson(address, serializedReplyAddress);
            messageRouter.addNextHop(
                    message.getHeaderFrom(), std::make_shared<const MqttAddress>(address));
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "could not deserialize MqttAddress from {} - error: {}",
                            serializedReplyAddress,
                            e.what());
            // do not try to route the message if address is not valid
            return;
        }
    } else if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST) {
        message.setReceivedFromGlobal(true);
    }

    try {
        messageRouter.route(message);
    } catch (const exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void MqttMessagingSkeleton::onTextMessageReceived(const std::string& message)
{
    try {
        JoynrMessage msg;
        joynr::serializer::deserializeFromJson(msg, message);

        if (msg.getType().empty()) {
            JOYNR_LOG_ERROR(logger, "received empty message - dropping Messages");
            return;
        }
        if (msg.getPayload().empty()) {
            JOYNR_LOG_ERROR(logger, "joynr message payload is empty: {}", message);
            return;
        }
        if (!msg.containsHeaderExpiryDate()) {
            JOYNR_LOG_ERROR(logger,
                            "received message [msgId=[{}] without decay time - dropping message",
                            msg.getHeaderMessageId());
            return;
        }
        JOYNR_LOG_TRACE(logger, "<<< INCOMING <<< {}", message);

        const JoynrTimePoint maxAbsoluteTime = DispatcherUtils::getMaxAbsoluteTime();
        JoynrTimePoint msgExpiryDate = msg.getHeaderExpiryDate();
        std::int64_t maxDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       maxAbsoluteTime - msgExpiryDate).count();
        if (static_cast<std::int64_t>(ttlUplift) > maxDiff) {
            msg.setHeaderExpiryDate(maxAbsoluteTime);
        } else {
            JoynrTimePoint newExpiryDate = msgExpiryDate + std::chrono::milliseconds(ttlUplift);
            msg.setHeaderExpiryDate(newExpiryDate);
        }

        auto onFailure = [msg](const exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Incoming Message with ID {} could not be sent! reason: {}",
                            msg.getHeaderMessageId(),
                            e.getMessage());
        };
        transmit(msg, onFailure);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize message. Raw message: {} - error: {}",
                        message,
                        e.what());
    }
}

} // namespace joynr

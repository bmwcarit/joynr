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
#include "libjoynrclustercontroller/mqtt/MqttMessagingSkeleton.h"

#include <smrf/exceptions.h>

#include "joynr/DispatcherUtils.h"
#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "libjoynrclustercontroller/mqtt/MqttReceiver.h"

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

MqttMessagingSkeleton::MqttMessagingSkeleton(IMessageRouter& messageRouter,
                                             std::shared_ptr<MqttReceiver> mqttReceiver,
                                             const std::string& multicastTopicPrefix,
                                             uint64_t ttlUplift)
        : messageRouter(messageRouter),
          mqttReceiver(mqttReceiver),
          ttlUplift(ttlUplift),
          multicastSubscriptionCount(),
          multicastSubscriptionCountMutex(),
          multicastTopicPrefix(multicastTopicPrefix)
{
}

void MqttMessagingSkeleton::registerMulticastSubscription(const std::string& multicastId)
{
    std::string mqttTopic = translateMulticastWildcard(multicastId);
    std::lock_guard<std::mutex> lock(multicastSubscriptionCountMutex);
    if (multicastSubscriptionCount.find(mqttTopic) == multicastSubscriptionCount.cend()) {
        mqttReceiver->subscribeToTopic(multicastTopicPrefix + mqttTopic);
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
        mqttReceiver->unsubscribeFromTopic(multicastTopicPrefix + mqttTopic);
    } else {
        countIterator->second--;
    }
}

void MqttMessagingSkeleton::transmit(
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    const std::string& messageType = message->getType();

    if (messageType == Message::VALUE_MESSAGE_TYPE_REQUEST() ||
        messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST() ||
        messageType == Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST() ||
        messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST()) {

        boost::optional<std::string> optionalReplyTo = message->getReplyTo();

        if (!optionalReplyTo.is_initialized()) {
            JOYNR_LOG_ERROR(logger,
                            "message {} did not contain replyTo header, discarding",
                            message->getId());
            return;
        }
        const std::string& replyTo = *optionalReplyTo;
        try {
            using system::RoutingTypes::MqttAddress;
            MqttAddress address;
            joynr::serializer::deserializeFromJson(address, replyTo);
            messageRouter.addNextHop(
                    message->getSender(), std::make_shared<const MqttAddress>(address));
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "could not deserialize MqttAddress from {} - error: {}",
                            replyTo,
                            e.what());
            // do not try to route the message if address is not valid
            return;
        }
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        message->setReceivedFromGlobal(true);
    }

    try {
        messageRouter.route(std::move(message));
    } catch (const exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void MqttMessagingSkeleton::onMessageReceived(smrf::ByteVector&& rawMessage)
{
    std::shared_ptr<ImmutableMessage> immutableMessage;
    try {
        immutableMessage = std::make_shared<ImmutableMessage>(std::move(rawMessage));
    } catch (const smrf::EncodingException& e) {
        JOYNR_LOG_ERROR(logger, "Unable to deserialize message - error: {}", e.what());
        return;
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger, "deserialized message is not valid - error: {}", e.what());
        return;
    }

    JOYNR_LOG_DEBUG(logger, "<<< INCOMING <<< {}", immutableMessage->toLogMessage());

    /*
    // TODO remove uplift ???? cannot modify msg here!
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
    */
    auto onFailure = [messageId = immutableMessage->getId()](
            const exceptions::JoynrRuntimeException& e)
    {
        JOYNR_LOG_ERROR(logger,
                        "Incoming Message with ID {} could not be sent! reason: {}",
                        messageId,
                        e.getMessage());
    };

    transmit(std::move(immutableMessage), onFailure);
}

} // namespace joynr

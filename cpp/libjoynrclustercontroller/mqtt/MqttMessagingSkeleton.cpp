/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
#include "joynr/MqttMessagingSkeleton.h"

#include <stdexcept>
#include <utility>

#include <smrf/exceptions.h>

#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MqttReceiver.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

std::string MqttMessagingSkeleton::translateMulticastWildcard(std::string topic)
{
    static constexpr char MQTT_MULTI_LEVEL_WILDCARD = '#';
    if (topic.length() > 0 && topic.back() == util::MULTI_LEVEL_WILDCARD[0]) {
        topic.back() = MQTT_MULTI_LEVEL_WILDCARD;
    }
    return topic;
}

MqttMessagingSkeleton::MqttMessagingSkeleton(std::weak_ptr<IMessageRouter> messageRouter,
                                             std::shared_ptr<MqttReceiver> mqttReceiver,
                                             const std::string& multicastTopicPrefix,
                                             const std::string& ownGbid,
                                             uint64_t /*ttlUplift*/)
        : _ownGbid(ownGbid),
          _messageRouter(std::move(messageRouter)),
          _mqttReceiver(std::move(mqttReceiver)),
          _multicastSubscriptionCount(),
          _multicastSubscriptionCountMutex(),
          _multicastTopicPrefix(multicastTopicPrefix)
{
}

void MqttMessagingSkeleton::registerMulticastSubscription(const std::string& multicastId)
{
    std::string mqttTopic = translateMulticastWildcard(multicastId);
    std::lock_guard<std::mutex> lock(_multicastSubscriptionCountMutex);
    if (_multicastSubscriptionCount.find(mqttTopic) == _multicastSubscriptionCount.cend()) {
        _mqttReceiver->subscribeToTopic(_multicastTopicPrefix + mqttTopic);
        _multicastSubscriptionCount[mqttTopic] = 1;
    } else {
        _multicastSubscriptionCount[mqttTopic]++;
    }
}

void MqttMessagingSkeleton::unregisterMulticastSubscription(const std::string& multicastId)
{
    std::string mqttTopic = translateMulticastWildcard(multicastId);
    std::lock_guard<std::mutex> lock(_multicastSubscriptionCountMutex);
    auto countIterator = _multicastSubscriptionCount.find(mqttTopic);
    if (countIterator == _multicastSubscriptionCount.cend()) {
        JOYNR_LOG_ERROR(
                logger(), "unregister multicast subscription called for non existing subscription");
    } else if (countIterator->second == 1) {
        _multicastSubscriptionCount.erase(mqttTopic);
        _mqttReceiver->unsubscribeFromTopic(_multicastTopicPrefix + mqttTopic);
    } else {
        countIterator->second--;
    }
}

void MqttMessagingSkeleton::transmit(
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    message->setReceivedFromGlobal(true);

    try {
        if (auto messageRouterSharedPtr = _messageRouter.lock()) {
            registerGlobalRoutingEntryIfRequired(*message, messageRouterSharedPtr, _ownGbid);
            messageRouterSharedPtr->route(std::move(message));
        } else {
            std::string errorMessage(
                    "unable to transmit message because messageRouter unavailable: " +
                    message->toLogMessage());
            onFailure(exceptions::JoynrMessageNotSentException(std::move(errorMessage)));
        }
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
        JOYNR_LOG_ERROR(logger(), "Unable to deserialize message - error: {}", e.what());
        return;
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(), "deserialized message is not valid - error: {}", e.what());
        return;
    }

    if (logger().getLogLevel() == LogLevel::Debug) {
        JOYNR_LOG_DEBUG(logger(),
                        "<<< INCOMING FROM >{}< <<< {}",
                        _ownGbid,
                        immutableMessage->getTrackingInfo());
    } else {
        JOYNR_LOG_TRACE(logger(),
                        "<<< INCOMING FROM >{}< <<< {}",
                        _ownGbid,
                        immutableMessage->toLogMessage());
    }

    auto onFailure = [ messageId = immutableMessage->getId(), _ownGbid = _ownGbid ](
            const exceptions::JoynrRuntimeException& e)
    {
        JOYNR_LOG_ERROR(logger(),
                        "Incoming Message from >{}< with ID {} could not be sent! reason: {}",
                        _ownGbid,
                        messageId,
                        e.getMessage());
    };

    transmit(std::move(immutableMessage), onFailure);
}

} // namespace joynr

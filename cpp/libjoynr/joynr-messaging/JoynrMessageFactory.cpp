/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/JoynrMessageFactory.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/JsonSerializer.h"
#include "joynr/OneWayRequest.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionStop.h"
#include "joynr-messaging/DummyPlatformSecurityManager.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

INIT_LOGGER(JoynrMessageFactory);

JoynrMessageFactory::JoynrMessageFactory()
        : securityManager(std::make_unique<DummyPlatformSecurityManager>())
{
}

JoynrMessage JoynrMessageFactory::createRequest(const std::string& senderId,
                                                const std::string& receiverId,
                                                const MessagingQos& qos,
                                                const Request& payload) const
{
    // create message and set type
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST);
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createReply(const std::string& senderId,
                                              const std::string& receiverId,
                                              const MessagingQos& qos,
                                              const Reply& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_REPLY);
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createOneWayRequest(const std::string& senderId,
                                                      const std::string& receiverId,
                                                      const MessagingQos& qos,
                                                      const OneWayRequest& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createSubscriptionPublication(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const SubscriptionPublication& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION);
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createSubscriptionRequest(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const SubscriptionRequest& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST);
    initMsg(msg,
            senderId,
            receiverId,
            qos,
            JsonSerializer::serialize<SubscriptionRequest>(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createBroadcastSubscriptionRequest(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const BroadcastSubscriptionRequest& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
    initMsg(msg,
            senderId,
            receiverId,
            qos,
            JsonSerializer::serialize<BroadcastSubscriptionRequest>(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createSubscriptionReply(const std::string& senderId,
                                                          const std::string& receiverId,
                                                          const MessagingQos& qos,
                                                          const SubscriptionReply& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);
    initMsg(msg, senderId, receiverId, qos, JsonSerializer::serialize<SubscriptionReply>(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createSubscriptionStop(const std::string& senderId,
                                                         const std::string& receiverId,
                                                         const MessagingQos& qos,
                                                         const SubscriptionStop& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP);
    initMsg(msg, senderId, receiverId, qos, JsonSerializer::serialize<SubscriptionStop>(payload));
    return msg;
}

void JoynrMessageFactory::initMsg(JoynrMessage& msg,
                                  const std::string& senderParticipantId,
                                  const std::string& receiverParticipantId,
                                  const MessagingQos& qos,
                                  std::string&& payload) const
{
    std::int64_t ttl = qos.getTtl();
    msg.setHeaderCreatorUserId(securityManager->getCurrentProcessUserId());
    msg.setHeaderFrom(senderParticipantId);
    msg.setHeaderTo(receiverParticipantId);

    for (const auto& it : qos.getCustomMessageHeaders()) {
        msg.setCustomHeader(it.first, it.second);
    }

    // calculate expiry date
    JoynrTimePoint expiryDate = DispatcherUtils::convertTtlToAbsoluteTime(ttl);
    msg.setHeaderExpiryDate(expiryDate);

    // add content type and class
    msg.setHeaderContentType(JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON);

    // set payload
    msg.setPayload(std::move(payload));
}

} // namespace joynr

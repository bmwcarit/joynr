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

#include "joynr/JoynrMessageFactory.h"

#include <limits>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/OneWayRequest.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/serializer/Serializer.h"
#include "libjoynr/joynr-messaging/DummyPlatformSecurityManager.h"

namespace joynr
{

INIT_LOGGER(JoynrMessageFactory);

JoynrMessageFactory::JoynrMessageFactory(std::uint64_t ttlUpliftMs)
        : securityManager(std::make_unique<DummyPlatformSecurityManager>()),
          ttlUpliftMs(ttlUpliftMs)
{
}

JoynrMessage JoynrMessageFactory::createRequest(const std::string& senderId,
                                                const std::string& receiverId,
                                                const MessagingQos& qos,
                                                const Request& payload,
                                                bool isLocalMessage) const
{
    // create message and set type
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST);
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getRequestReplyId());
    msg.setLocalMessage(isLocalMessage);
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
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getRequestReplyId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload), false);
    return msg;
}

JoynrMessage JoynrMessageFactory::createOneWayRequest(const std::string& senderId,
                                                      const std::string& receiverId,
                                                      const MessagingQos& qos,
                                                      const OneWayRequest& payload,
                                                      bool isLocalMessage) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    msg.setLocalMessage(isLocalMessage);
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createMulticastPublication(
        const std::string& senderId,
        const MessagingQos& qos,
        const MulticastPublication& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    initMsg(msg,
            senderId,
            payload.getMulticastId(),
            qos,
            joynr::serializer::serializeToJson(payload));
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
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createSubscriptionRequest(const std::string& senderId,
                                                            const std::string& receiverId,
                                                            const MessagingQos& qos,
                                                            const SubscriptionRequest& payload,
                                                            bool isLocalMessage) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST);
    msg.setLocalMessage(isLocalMessage);
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createMulticastSubscriptionRequest(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const MulticastSubscriptionRequest& payload,
        bool isLocalMessage) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST);
    msg.setLocalMessage(isLocalMessage);
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createBroadcastSubscriptionRequest(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const BroadcastSubscriptionRequest& payload,
        bool isLocalMessage) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
    msg.setLocalMessage(isLocalMessage);
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

JoynrMessage JoynrMessageFactory::createSubscriptionReply(const std::string& senderId,
                                                          const std::string& receiverId,
                                                          const MessagingQos& qos,
                                                          const SubscriptionReply& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload), false);
    return msg;
}

JoynrMessage JoynrMessageFactory::createSubscriptionStop(const std::string& senderId,
                                                         const std::string& receiverId,
                                                         const MessagingQos& qos,
                                                         const SubscriptionStop& payload) const
{
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP);
    msg.setCustomHeader(JoynrMessage::CUSTOM_HEADER_REQUEST_REPLY_ID, payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

void JoynrMessageFactory::initMsg(JoynrMessage& msg,
                                  const std::string& senderParticipantId,
                                  const std::string& receiverParticipantId,
                                  const MessagingQos& qos,
                                  std::string&& payload,
                                  bool upliftTtl) const
{
    std::int64_t ttl = qos.getTtl();
    if (upliftTtl &&
        ttl < (std::numeric_limits<std::int64_t>::max() - static_cast<std::int64_t>(ttlUpliftMs))) {
        ttl += ttlUpliftMs;
    }
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

    // if the effort has been set to best effort, then activate that in the headers
    if (qos.getEffort() != MessagingQosEffort::Enum::NORMAL) {
        msg.setHeaderEffort(MessagingQosEffort::getLiteral(qos.getEffort()));
    }

    // set payload
    msg.setPayload(std::move(payload));
}

} // namespace joynr

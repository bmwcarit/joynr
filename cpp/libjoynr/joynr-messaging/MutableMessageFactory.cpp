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

#include "joynr/MutableMessageFactory.h"

#include <limits>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/IKeychain.h"
#include "joynr/IPlatformSecurityManager.h"
#include "joynr/Message.h"
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

MutableMessageFactory::MutableMessageFactory(std::uint64_t ttlUpliftMs,
                                             std::shared_ptr<IKeychain> keyChain)
        : _securityManager(std::make_unique<DummyPlatformSecurityManager>()),
          _ttlUpliftMs(ttlUpliftMs),
          _keyChain(std::move(keyChain))
{
}

// needs to be implemented here because of IPlatformSecurityManager being forward declared
MutableMessageFactory::~MutableMessageFactory() = default;

MutableMessage MutableMessageFactory::createRequest(const std::string& senderId,
                                                    const std::string& receiverId,
                                                    const MessagingQos& qos,
                                                    const Request& payload,
                                                    bool isLocalMessage) const
{
    // create message and set type
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_REQUEST());
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getRequestReplyId());
    msg.setLocalMessage(isLocalMessage);
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

MutableMessage MutableMessageFactory::createReply(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        std::unordered_map<std::string, std::string>&& prefixedCustomHeaders,
        const Reply& payload) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_REPLY());
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getRequestReplyId());
    msg.setPrefixedCustomHeaders(std::move(prefixedCustomHeaders));
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload), false);
    return msg;
}

MutableMessage MutableMessageFactory::createOneWayRequest(const std::string& senderId,
                                                          const std::string& receiverId,
                                                          const MessagingQos& qos,
                                                          const OneWayRequest& payload,
                                                          bool isLocalMessage) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_ONE_WAY());
    msg.setLocalMessage(isLocalMessage);
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

MutableMessage MutableMessageFactory::createMulticastPublication(
        const std::string& senderId,
        const MessagingQos& qos,
        const MulticastPublication& payload) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_MULTICAST());
    initMsg(msg,
            senderId,
            payload.getMulticastId(),
            qos,
            joynr::serializer::serializeToJson(payload));
    return msg;
}

MutableMessage MutableMessageFactory::createSubscriptionPublication(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const SubscriptionPublication& payload) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_PUBLICATION());
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

MutableMessage MutableMessageFactory::createSubscriptionRequest(const std::string& senderId,
                                                                const std::string& receiverId,
                                                                const MessagingQos& qos,
                                                                const SubscriptionRequest& payload,
                                                                bool isLocalMessage) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST());
    msg.setLocalMessage(isLocalMessage);
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

MutableMessage MutableMessageFactory::createMulticastSubscriptionRequest(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const MulticastSubscriptionRequest& payload,
        bool isLocalMessage) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST());
    msg.setLocalMessage(isLocalMessage);
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

MutableMessage MutableMessageFactory::createBroadcastSubscriptionRequest(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const BroadcastSubscriptionRequest& payload,
        bool isLocalMessage) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST());
    msg.setLocalMessage(isLocalMessage);
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

MutableMessage MutableMessageFactory::createSubscriptionReply(
        const std::string& senderId,
        const std::string& receiverId,
        const MessagingQos& qos,
        const SubscriptionReply& payload) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY());
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload), false);
    return msg;
}

MutableMessage MutableMessageFactory::createSubscriptionStop(const std::string& senderId,
                                                             const std::string& receiverId,
                                                             const MessagingQos& qos,
                                                             const SubscriptionStop& payload) const
{
    MutableMessage msg;
    msg.setType(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP());
    msg.setCustomHeader(Message::CUSTOM_HEADER_REQUEST_REPLY_ID(), payload.getSubscriptionId());
    initMsg(msg, senderId, receiverId, qos, joynr::serializer::serializeToJson(payload));
    return msg;
}

void MutableMessageFactory::initMsg(MutableMessage& msg,
                                    const std::string& senderParticipantId,
                                    const std::string& receiverParticipantId,
                                    const MessagingQos& qos,
                                    std::string&& payload,
                                    bool upliftTtl) const
{
    std::int64_t ttl = static_cast<std::int64_t>(qos.getTtl());
    if (upliftTtl &&
        ttl < (std::numeric_limits<std::int64_t>::max() -
               static_cast<std::int64_t>(_ttlUpliftMs))) {
        ttl += static_cast<std::int64_t>(_ttlUpliftMs);
    }
    msg.setSender(senderParticipantId);
    msg.setRecipient(receiverParticipantId);
    msg.setKeychain(_keyChain);

    for (const auto& it : qos.getCustomMessageHeaders()) {
        msg.setCustomHeader(it.first, it.second);
    }

    msg.setExpiryDate(TimePoint::fromRelativeMs(ttl));

    // if the effort has been set to best effort, then activate that in the headers
    if (qos.getEffort() != MessagingQosEffort::Enum::NORMAL) {
        msg.setEffort(MessagingQosEffort::getLiteral(qos.getEffort()));
    }

    // set payload
    msg.setPayload(std::move(payload));

    // set flags
    msg.setEncrypt(qos.getEncrypt());
    msg.setCompress(qos.getCompress());
}

} // namespace joynr

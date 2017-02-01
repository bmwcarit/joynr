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
#include "HttpMessagingSkeleton.h"

#include "joynr/MessageRouter.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

INIT_LOGGER(HttpMessagingSkeleton);

HttpMessagingSkeleton::HttpMessagingSkeleton(MessageRouter& messageRouter)
        : messageRouter(messageRouter)
{
}

void HttpMessagingSkeleton::transmit(
        JoynrMessage& message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {
        // TODO ca: check if replyTo header info is available?
        try {
            using system::RoutingTypes::ChannelAddress;

            ChannelAddress channelAddress;
            joynr::serializer::deserializeFromJson(channelAddress, message.getHeaderReplyAddress());

            auto address = std::make_shared<const ChannelAddress>(channelAddress);
            messageRouter.addNextHop(message.getHeaderFrom(), address);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "could not deserialize ChannelAddress from {} - error: {}",
                            message.getHeaderReplyAddress(),
                            e.what());
            // do not try to route the message if address is not valid
            return;
        }
    }

    try {
        messageRouter.route(message);
    } catch (exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void HttpMessagingSkeleton::onTextMessageReceived(const std::string& message)
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
        JOYNR_LOG_DEBUG(logger, "<<< INCOMING <<< {}", message);

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

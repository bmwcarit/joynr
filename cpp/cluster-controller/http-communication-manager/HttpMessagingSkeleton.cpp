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

#include "joynr/JsonSerializer.h"
#include "joynr/MessageRouter.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

namespace joynr
{

INIT_LOGGER(HttpMessagingSkeleton);

HttpMessagingSkeleton::HttpMessagingSkeleton(MessageRouter& messageRouter)
        : messageRouter(messageRouter)
{
}

void HttpMessagingSkeleton::transmit(JoynrMessage& message)
{
    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {
        // TODO ca: check if replyTo header info is available?
        std::string replyChannelId = message.getHeaderReplyChannelId();
        std::shared_ptr<system::RoutingTypes::ChannelAddress> address(
                new system::RoutingTypes::ChannelAddress(replyChannelId));
        messageRouter.addNextHop(message.getHeaderFrom(), address);
    }

    messageRouter.route(message);
}

void HttpMessagingSkeleton::onTextMessageReceived(const std::string& message)
{
    try {
        JoynrMessage msg = JsonSerializer::deserialize<JoynrMessage>(message);
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
        transmit(msg);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize message. Raw message: {} - error: {}",
                        message,
                        e.what());
    }
}

} // namespace joynr

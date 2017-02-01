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
#include "WebSocketLibJoynrMessagingSkeleton.h"

#include "joynr/JoynrMessage.h"
#include "joynr/MessageRouter.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

INIT_LOGGER(WebSocketLibJoynrMessagingSkeleton);

WebSocketLibJoynrMessagingSkeleton::WebSocketLibJoynrMessagingSkeleton(
        std::shared_ptr<MessageRouter> messageRouter)
        : messageRouter(messageRouter)
{
}

void WebSocketLibJoynrMessagingSkeleton::transmit(
        JoynrMessage& message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    try {
        messageRouter->route(message);
    } catch (const exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void WebSocketLibJoynrMessagingSkeleton::onTextMessageReceived(const std::string& message)
{
    // deserialize message and transmit
    try {
        JoynrMessage joynrMsg;
        joynr::serializer::deserializeFromJson(joynrMsg, message);
        if (joynrMsg.getType().empty()) {
            JOYNR_LOG_ERROR(logger, "Message type is empty : {}", message);
            return;
        }
        if (joynrMsg.getPayload().empty()) {
            JOYNR_LOG_ERROR(logger, "joynr message payload is empty: {}", message);
            return;
        }
        if (!joynrMsg.containsHeaderExpiryDate()) {
            JOYNR_LOG_ERROR(logger,
                            "received message [msgId=[{}] without decay time - dropping message",
                            joynrMsg.getHeaderMessageId());
            return;
        }
        JOYNR_LOG_DEBUG(logger, "<<< INCOMING <<< {}", message);

        if (joynrMsg.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST) {
            joynrMsg.setReceivedFromGlobal(true);
        }

        auto onFailure = [joynrMsg](const exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Incoming Message with ID {} could not be sent! reason: {}",
                            joynrMsg.getHeaderMessageId(),
                            e.getMessage());
        };
        transmit(joynrMsg, onFailure);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize joynr message object from: {} - error: {}",
                        message,
                        e.what());
    }
}

} // namespace joynr

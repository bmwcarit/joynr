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

#include "joynr/JsonSerializer.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{

INIT_LOGGER(WebSocketLibJoynrMessagingSkeleton);

WebSocketLibJoynrMessagingSkeleton::WebSocketLibJoynrMessagingSkeleton(MessageRouter& messageRouter)
        : messageRouter(messageRouter)
{
}

void WebSocketLibJoynrMessagingSkeleton::transmit(JoynrMessage& message)
{
    messageRouter.route(message);
}

void WebSocketLibJoynrMessagingSkeleton::onTextMessageReceived(const std::string& message)
{
    // deserialize message and transmit
    try {
        JoynrMessage joynrMsg = JsonSerializer::deserialize<JoynrMessage>(message);
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
        JOYNR_LOG_TRACE(logger, "<<< INCOMING <<< {}", message);
        transmit(joynrMsg);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize joynr message object from: {} - error: {}",
                        message,
                        e.what());
    }
}

} // namespace joynr

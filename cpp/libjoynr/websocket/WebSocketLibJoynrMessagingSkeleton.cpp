/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

joynr_logging::Logger* WebSocketLibJoynrMessagingSkeleton::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG",
                                                         "WebSocketLibJoynrMessagingSkeleton");

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
    joynr::JoynrMessage* joynrMsg = JsonSerializer::deserialize<joynr::JoynrMessage>(message);
    if (joynrMsg == nullptr) {
        LOG_ERROR(logger,
                  FormatString("Unable to deserialize joynr message object from: %1")
                          .arg(message)
                          .str());
        return;
    }
    LOG_TRACE(logger, FormatString("INCOMING\nmessage: %1").arg(message).str());
    // message router copies joynr message when scheduling thread that handles
    // message delivery
    transmit(*joynrMsg);
    delete joynrMsg;
}

} // namespace joynr

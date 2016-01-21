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
#include "WebSocketMessagingStub.h"
#include "WebSocketMessagingStubFactory.h"

#include "joynr/JsonSerializer.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

INIT_LOGGER(WebSocketMessagingStub);

WebSocketMessagingStub::WebSocketMessagingStub(IWebSocketSendInterface* webSocket,
                                               std::function<void()> onStubClosed)
        : webSocket(webSocket)
{
    webSocket->registerDisconnectCallback(onStubClosed);
}

void WebSocketMessagingStub::transmit(JoynrMessage& message)
{
    if (!webSocket->isInitialized()) {
        JOYNR_LOG_ERROR(logger,
                        "WebSocket not ready. Unable to send message {}",
                        JsonSerializer::serialize(message));
        return;
    }

    std::string serializedMessage = JsonSerializer::serialize(message);
    JOYNR_LOG_TRACE(logger, ">>>> OUTGOING >>>> {}", serializedMessage);
    webSocket->send(serializedMessage);
}

} // namespace joynr

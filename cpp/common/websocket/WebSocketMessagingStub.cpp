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
#include "common/websocket/WebSocketMessagingStub.h"

#include <QtWebSockets/QWebSocket>

#include "include/joynr/JsonSerializer.h"
#include "include/joynr/JoynrMessage.h"

namespace joynr {

using namespace joynr_logging;

WebSocketMessagingStub::WebSocketMessagingStub(QWebSocket* webSocket) :
    webSocket(webSocket)
{
}

WebSocketMessagingStub::~WebSocketMessagingStub() {
    webSocket->close();
    webSocket->deleteLater();
}

void WebSocketMessagingStub::transmit(JoynrMessage& message, const MessagingQos &qos) {
    // QoS is not needed on transmit. Message already contains expiry date.
    // Messaging interface needs to be refactored.
    Q_UNUSED(qos);
    QByteArray serializedMessage(JsonSerializer::serialize(message));
    webSocket->sendBinaryMessage(serializedMessage);
}

} // namespace joynr

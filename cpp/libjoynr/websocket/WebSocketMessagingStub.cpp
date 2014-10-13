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

#include <QtWebSockets/QWebSocket>
#include <QtCore/QMetaObject>

#include "joynr/JsonSerializer.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/Address.h"

namespace joynr {

joynr_logging::Logger* WebSocketMessagingStub::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "WebSocketMessagingStub");

WebSocketMessagingStub::WebSocketMessagingStub(
        system::Address* address,
        QWebSocket* webSocket,
        QObject *parent
) :
    QObject(parent),
    address(address),
    webSocket(webSocket)
{
    connect(
            webSocket, &QWebSocket::disconnected,
            this, &WebSocketMessagingStub::onSocketDisconnected
    );
}

WebSocketMessagingStub::~WebSocketMessagingStub() {
    webSocket->close();
    webSocket->deleteLater();
    address->deleteLater();
}

void WebSocketMessagingStub::onSocketDisconnected() {
    LOG_DEBUG(logger, QString("Web Socket disconnected: %0").arg(address->toString()));
    emit closed(*address);
}

void WebSocketMessagingStub::sendTextMessage(const QString &message)
{
    LOG_TRACE(logger, QString("OUTGOING\nmessage: %0\nto: %1")
              .arg(message)
              .arg(address->toString())
    );
    qint64 bytesSent = webSocket->sendTextMessage(message);
    LOG_TRACE(logger, QString("bytes actually sent: %0 of %1")
              .arg(bytesSent)
              .arg(message.size())
    );

}

void WebSocketMessagingStub::transmit(JoynrMessage& message, const MessagingQos &qos) {
    // QoS is not needed on transmit. Message already contains expiry date.
    // Messaging interface needs to be refactored.
    Q_UNUSED(qos);

    if(!webSocket->isValid()) {
        LOG_ERROR(logger, QString("WebSocket not ready %0. Unable to send message %1.")
                  .arg(address->toString())
                  .arg(QString(JsonSerializer::serialize(message))));
        return;
    }

    QByteArray serializedMessage(JsonSerializer::serialize(message));
    QMetaObject::invokeMethod(
                this,
                "sendTextMessage",
                Qt::AutoConnection,
                Q_ARG(QString, QString(serializedMessage))
    );
}

} // namespace joynr

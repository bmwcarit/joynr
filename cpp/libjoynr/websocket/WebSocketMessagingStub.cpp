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
#include "joynr/system/RoutingTypes_QtAddress.h"

namespace joynr
{

joynr_logging::Logger* WebSocketMessagingStub::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "WebSocketMessagingStub");

WebSocketMessagingStub::WebSocketMessagingStub(system::RoutingTypes::QtAddress* address,
                                               QWebSocket* webSocket,
                                               QObject* parent)
        : QObject(parent), address(address), webSocket(webSocket)
{
    connect(webSocket,
            &QWebSocket::disconnected,
            this,
            &WebSocketMessagingStub::onSocketDisconnected,
            Qt::QueuedConnection);
    connect(this,
            &WebSocketMessagingStub::queueTextMessage,
            this,
            &WebSocketMessagingStub::sendTextMessage,
            Qt::QueuedConnection);
}

WebSocketMessagingStub::~WebSocketMessagingStub()
{
    // QWebSocket.close() is a slot - call from the event loop
    QMetaObject::invokeMethod(webSocket, "close", Qt::QueuedConnection);
    webSocket->deleteLater();
    address->deleteLater();
}

void WebSocketMessagingStub::onSocketDisconnected()
{
    LOG_DEBUG(logger,
              FormatString("Web Socket disconnected: %1")
                      .arg(address->toString().toStdString())
                      .str());
    emit closed(*address);
}

void WebSocketMessagingStub::sendTextMessage(const QString& message)
{
    LOG_TRACE(logger,
              FormatString("OUTGOING\nmessage: %1\nto: %2")
                      .arg(message.toStdString())
                      .arg(address->toString().toStdString())
                      .str());
    qint64 bytesSent = webSocket->sendTextMessage(message);
    bool flushed = webSocket->flush();
    LOG_TRACE(logger,
              FormatString("bytes actually sent (%1): %2 of %3")
                      .arg(flushed)
                      .arg(bytesSent)
                      .arg(message.size())
                      .str());
}

void WebSocketMessagingStub::transmit(JoynrMessage& message)
{
    if (!webSocket->isValid()) {
        LOG_ERROR(logger,
                  FormatString("WebSocket not ready %1. Unable to send message %2.")
                          .arg(address->toString().toStdString())
                          .arg(JsonSerializer::serialize(message))
                          .str());
        return;
    }

    QByteArray serializedMessage =
            QString::fromStdString(JsonSerializer::serialize(message)).toUtf8();
    emit queueTextMessage(serializedMessage);
}

} // namespace joynr

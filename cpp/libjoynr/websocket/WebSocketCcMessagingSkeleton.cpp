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
#include "joynr/WebSocketCcMessagingSkeleton.h"

#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>

#include "joynr/JsonSerializer.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{

joynr_logging::Logger* WebSocketCcMessagingSkeleton::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "WebSocketCcMessagingSkeleton");

WebSocketCcMessagingSkeleton::WebSocketCcMessagingSkeleton(
        MessageRouter& messageRouter,
        WebSocketMessagingStubFactory& messagingStubFactory,
        const system::RoutingTypes::WebSocketAddress& serverAddress)
        : webSocketServer(nullptr),
          clients(),
          messageRouter(messageRouter),
          messagingStubFactory(messagingStubFactory)
{
    QWebSocketServer::SslMode sslMode(QWebSocketServer::NonSecureMode);
    if (serverAddress.getProtocol() == joynr::system::RoutingTypes::WebSocketProtocol::WSS) {
        sslMode = QWebSocketServer::SecureMode;
    }

    webSocketServer = new QWebSocketServer(QStringLiteral("joynr CC"), sslMode, this);

    if (webSocketServer->listen(QHostAddress::Any, serverAddress.getPort())) {
        LOG_INFO(logger,
                 FormatString("joynr CC WebSocket server listening on port %1.")
                         .arg(serverAddress.getPort())
                         .str());
        connect(webSocketServer,
                &QWebSocketServer::newConnection,
                this,
                &WebSocketCcMessagingSkeleton::onNewConnection);
    } else {
        LOG_FATAL(logger,
                  FormatString("Error: WebSocket server could not listen on port %1.")
                          .arg(serverAddress.getPort())
                          .str());
    }
}

WebSocketCcMessagingSkeleton::~WebSocketCcMessagingSkeleton()
{
    webSocketServer->close();
    webSocketServer->deleteLater();
    qDeleteAll(clients.begin(), clients.end());
}

void WebSocketCcMessagingSkeleton::transmit(JoynrMessage& message)
{
    messageRouter.route(message);
}

void WebSocketCcMessagingSkeleton::onNewConnection()
{
    QWebSocket* client = webSocketServer->nextPendingConnection();

    connect(client,
            &QWebSocket::textMessageReceived,
            this,
            &WebSocketCcMessagingSkeleton::onTextMessageReceived);
    connect(client,
            &QWebSocket::disconnected,
            this,
            &WebSocketCcMessagingSkeleton::onSocketDisconnected);

    clients.push_back(client);
}

void WebSocketCcMessagingSkeleton::onTextMessageReceived(const QString& message)
{
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());

    if (isInitializationMessage(message)) {
        LOG_DEBUG(logger,
                  FormatString("received initialization message from websocket client: %1")
                          .arg(message.toStdString())
                          .str());
        // register client with messaging stub factory
        joynr::system::RoutingTypes::WebSocketClientAddress* clientAddress =
                JsonSerializer::deserialize<joynr::system::RoutingTypes::WebSocketClientAddress>(
                        message.toStdString());
        // client address must be valid, or libjoynr and CC are deployed in different versions
        assert(clientAddress);

        messagingStubFactory.addClient(clientAddress, client);

        // cleanup
        disconnect(client,
                   &QWebSocket::disconnected,
                   this,
                   &WebSocketCcMessagingSkeleton::onSocketDisconnected);
        removeAll(clients, client);
        return;
    }

    // deserialize message and transmit
    joynr::JoynrMessage* joynrMsg =
            JsonSerializer::deserialize<joynr::JoynrMessage>(message.toStdString());
    if (joynrMsg == nullptr || joynrMsg->getPayload().empty()) {
        LOG_ERROR(logger,
                  FormatString("Unable to deserialize joynr message object from: %1")
                          .arg(message.toStdString())
                          .str());
        return;
    }
    LOG_TRACE(logger, FormatString("INCOMING\nmessage: %1").arg(message.toStdString()).str());
    // message router copies joynr message when scheduling thread that handles
    // message delivery
    transmit(*joynrMsg);
    delete joynrMsg;
}

void WebSocketCcMessagingSkeleton::onSocketDisconnected()
{
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (vectorContains(clients, client)) {
        removeAll(clients, client);
        client->deleteLater();
    }
}

void WebSocketCcMessagingSkeleton::onAcceptError(QAbstractSocket::SocketError socketError)
{
    QString code;
    QDebug(&code) << socketError;
    LOG_ERROR(logger,
              FormatString("Unable to accept new socket connection. Error: %1.")
                      .arg(code.toStdString())
                      .str());
}

void WebSocketCcMessagingSkeleton::onServerError(QWebSocketProtocol::CloseCode closeCode)
{
    QString code;
    QDebug(&code) << closeCode;
    LOG_ERROR(logger,
              FormatString("Error occured on WebSocket connection: %1. Description: %2.")
                      .arg(code.toStdString())
                      .arg(webSocketServer->errorString().toStdString())
                      .str());
}

bool WebSocketCcMessagingSkeleton::isInitializationMessage(const QString& message)
{
    return message.startsWith(
            "{\"_typeName\":\"joynr.system.RoutingTypes.WebSocketClientAddress\"");
}

} // namespace joynr

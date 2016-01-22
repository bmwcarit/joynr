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
#include "WebSocketCcMessagingSkeleton.h"

#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>

#include "joynr/JoynrMessage.h"
#include "joynr/Util.h"
#include "joynr/MessageRouter.h"
#include "joynr/JsonSerializer.h"
#include "joynr/IWebSocketSendInterface.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/WebSocketProtocol.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "QWebSocketSendWrapper.h"

namespace joynr
{

INIT_LOGGER(WebSocketCcMessagingSkeleton);

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
        JOYNR_LOG_INFO(
                logger, "joynr CC WebSocket server listening on port {}", serverAddress.getPort());
        connect(webSocketServer,
                &QWebSocketServer::newConnection,
                this,
                &WebSocketCcMessagingSkeleton::onNewConnection);
    } else {
        JOYNR_LOG_FATAL(logger,
                        "Error: WebSocket server could not listen on port {}",
                        serverAddress.getPort());
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
        JOYNR_LOG_DEBUG(logger,
                        "received initialization message from websocket client: {}",
                        message.toStdString());
        // register client with messaging stub factory
        joynr::system::RoutingTypes::WebSocketClientAddress* clientAddress =
                JsonSerializer::deserialize<joynr::system::RoutingTypes::WebSocketClientAddress>(
                        message.toStdString());
        // client address must be valid, or libjoynr and CC are deployed in different versions
        assert(clientAddress);

        IWebSocketSendInterface* clientWrapper = new QWebSocketSendWrapper(client);
        messagingStubFactory.addClient(clientAddress, clientWrapper);

        // cleanup
        disconnect(client,
                   &QWebSocket::disconnected,
                   this,
                   &WebSocketCcMessagingSkeleton::onSocketDisconnected);
        removeAll(clients, client);
        return;
    }

    // deserialize message and transmit
    JoynrMessage* joynrMsg = JsonSerializer::deserialize<JoynrMessage>(message.toStdString());
    if (joynrMsg == nullptr || joynrMsg->getPayload().empty()) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize joynr message object from: {}",
                        message.toStdString());
        return;
    }
    JOYNR_LOG_TRACE(logger, "<<<< INCOMING <<<< {}", message.toStdString());
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
    JOYNR_LOG_ERROR(
            logger, "Unable to accept new socket connection. Error: {}", code.toStdString());
}

void WebSocketCcMessagingSkeleton::onServerError(QWebSocketProtocol::CloseCode closeCode)
{
    QString code;
    QDebug(&code) << closeCode;
    JOYNR_LOG_ERROR(logger,
                    "Error occured on WebSocket connection: {}. Description: {}",
                    code.toStdString(),
                    webSocketServer->errorString().toStdString());
}

bool WebSocketCcMessagingSkeleton::isInitializationMessage(const QString& message)
{
    return message.startsWith(
            "{\"_typeName\":\"joynr.system.RoutingTypes.WebSocketClientAddress\"");
}

} // namespace joynr

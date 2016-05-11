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
#include "WebSocketCcMessagingSkeleton.h"

#include <memory>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>

#include "joynr/serializer/Serializer.h"
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
        std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
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

void WebSocketCcMessagingSkeleton::transmit(
        JoynrMessage& message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    try {
        messageRouter.route(message);
    } catch (exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
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
        using joynr::system::RoutingTypes::WebSocketClientAddress;
        JOYNR_LOG_DEBUG(logger,
                        "received initialization message from websocket client: {}",
                        message.toStdString());
        // register client with messaging stub factory
        try {
            WebSocketClientAddress clientAddress =
                    JsonSerializer::deserialize<WebSocketClientAddress>(message.toStdString());
            std::shared_ptr<IWebSocketSendInterface> clientWrapper =
                    std::make_shared<QWebSocketSendWrapper>(client);
            messagingStubFactory->addClient(clientAddress, clientWrapper);

            std::weak_ptr<WebSocketMessagingStubFactory> weakFactoryRef(messagingStubFactory);
            clientWrapper->registerDisconnectCallback([weakFactoryRef, clientAddress]() {
                if (auto factory = weakFactoryRef.lock()) {
                    factory->onMessagingStubClosed(clientAddress);
                }
            });

            // cleanup
            disconnect(client,
                       &QWebSocket::disconnected,
                       this,
                       &WebSocketCcMessagingSkeleton::onSocketDisconnected);
            util::removeAll(clients, client);

        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "client address must be valid, otherwise libjoynr and CC are deployed "
                            "in different versions - raw: {} - error: {}",
                            message.toStdString(),
                            e.what());
        }
        return;
    }

    // deserialize message and transmit
    try {
        JoynrMessage joynrMsg;
        using Stream = muesli::StringIStream;
        Stream stream(message.toStdString());
        muesli::JsonInputArchive<Stream> archive(stream);
        archive(joynrMsg);
        if (joynrMsg.getType().empty()) {
            JOYNR_LOG_ERROR(logger, "Message type is empty : {}", message.toStdString());
            return;
        }
        if (joynrMsg.getPayload().empty()) {
            JOYNR_LOG_ERROR(logger, "joynr message payload is empty: {}", message.toStdString());
            return;
        }
        if (!joynrMsg.containsHeaderExpiryDate()) {
            JOYNR_LOG_ERROR(logger,
                            "received message [msgId=[{}] without decay time - dropping message",
                            joynrMsg.getHeaderMessageId());
            return;
        }

        JOYNR_LOG_TRACE(logger, "<<<< INCOMING <<<< {}", message.toStdString());

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
                        message.toStdString(),
                        e.what());
        return;
    }
}

void WebSocketCcMessagingSkeleton::onSocketDisconnected()
{
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (util::vectorContains(clients, client)) {
        util::removeAll(clients, client);
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

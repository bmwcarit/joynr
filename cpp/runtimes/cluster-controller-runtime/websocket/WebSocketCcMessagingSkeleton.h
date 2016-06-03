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
#ifndef WEBSOCKETCCMESSAGINGSKELETON_H
#define WEBSOCKETCCMESSAGINGSKELETON_H

#include <vector>

#include <QtCore/QObject>
#include <QtNetwork/QAbstractSocket>
#include <QtWebSockets/qwebsocketprotocol.h>

#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"

#include "joynr/IMessaging.h"

class QWebSocketServer;
class QWebSocket;

namespace joynr
{

class JoynrMessage;
class MessageRouter;
class WebSocketMessagingStubFactory;

namespace system
{
namespace RoutingTypes
{
class WebSocketAddress;
} // namespace RoutingTypes
} // namespace system

/**
 * @class WebSocketCcMessagingSkeleton
 * @brief Messaging skeleton for the cluster controller
 */
class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT WebSocketCcMessagingSkeleton : public QObject,
                                                                          public IMessaging
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param messageRouter Router
     * @param messagingStubFactory Factory
     * @param serverAddress Address of the server
     */
    WebSocketCcMessagingSkeleton(
            MessageRouter& messageRouter,
            std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
            const system::RoutingTypes::WebSocketAddress& serverAddress);

    /**
     * @brief Destructor
     */
    ~WebSocketCcMessagingSkeleton() override;

    void transmit(JoynrMessage& message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

private Q_SLOTS:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onSocketDisconnected();
    void onAcceptError(QAbstractSocket::SocketError socketError);
    void onServerError(QWebSocketProtocol::CloseCode closeCode);

private:
    DISALLOW_COPY_AND_ASSIGN(WebSocketCcMessagingSkeleton);
    bool isInitializationMessage(const QString& message);
    ADD_LOGGER(WebSocketCcMessagingSkeleton);
    /*! Websocket server listening for incoming connections */
    QWebSocketServer* webSocketServer;
    /*! List of client connections */
    std::vector<QWebSocket*> clients;
    /*! Router for incoming messages */
    MessageRouter& messageRouter;
    /*! Factory to build outgoing messaging stubs */
    std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory;
};

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H

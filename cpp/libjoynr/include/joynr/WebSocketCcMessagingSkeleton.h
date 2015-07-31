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
#ifndef WEBSOCKETCCMESSAGINGSKELETON_H
#define WEBSOCKETCCMESSAGINGSKELETON_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtNetwork/QAbstractSocket>
#include <QtWebSockets/qwebsocketprotocol.h>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/joynrlogging.h"

#include "joynr/MessageRouter.h"
#include "joynr/IMessaging.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/QtWebSocketAddress.h"

#include "joynr/JoynrExport.h"

class QWebSocketServer;
class QWebSocket;

namespace joynr
{

class JOYNR_EXPORT WebSocketCcMessagingSkeleton : public QObject, public IMessaging
{
    Q_OBJECT
public:
    WebSocketCcMessagingSkeleton(MessageRouter& messageRouter,
                                 WebSocketMessagingStubFactory& messagingStubFactory,
                                 const system::RoutingTypes::QtWebSocketAddress& serverAddress);

    ~WebSocketCcMessagingSkeleton();

    virtual void transmit(JoynrMessage& message);

private Q_SLOTS:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onSocketDisconnected();
    void onAcceptError(QAbstractSocket::SocketError socketError);
    void onServerError(QWebSocketProtocol::CloseCode closeCode);

private:
    DISALLOW_COPY_AND_ASSIGN(WebSocketCcMessagingSkeleton);
    bool isInitializationMessage(const QString& message);
    static joynr_logging::Logger* logger;
    QWebSocketServer* webSocketServer;
    QList<QWebSocket*> clients;
    MessageRouter& messageRouter;
    WebSocketMessagingStubFactory& messagingStubFactory;
};

} // namespace joynr
#endif // WEBSOCKETCCMESSAGINGSKELETON_H

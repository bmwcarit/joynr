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
#include "WebSocketMessagingStubFactory.h"

#include <QtCore/QMutexLocker>
#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtWebSockets/QWebSocket>
#include <assert.h>

#include "websocket/WebSocketMessagingStub.h"
#include "joynr/system/Address.h"
#include "joynr/system/WebSocketAddress.h"
#include "joynr/system/WebSocketClientAddress.h"

namespace  joynr {

joynr_logging::Logger* WebSocketMessagingStubFactory::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "WebSocketMessagingStubFactory");

WebSocketMessagingStubFactory::WebSocketMessagingStubFactory(QObject *parent) :
    QObject(parent),
    serverStubMap(),
    clientStubMap(),
    mutex()
{
}

bool WebSocketMessagingStubFactory::canCreate(const joynr::system::Address& destAddress) {
    return destAddress.inherits(system::WebSocketAddress::staticMetaObject.className())
            || destAddress.inherits(system::WebSocketClientAddress::staticMetaObject.className());
}

QSharedPointer<IMessaging> WebSocketMessagingStubFactory::create(
        const joynr::system::Address& destAddress
) {
    // if destination is a WS client address
    if(destAddress.inherits(system::WebSocketClientAddress::staticMetaObject.className())) {
        const system::WebSocketClientAddress* webSocketClientAddress =
                qobject_cast<const system::WebSocketClientAddress*>(&destAddress);
        // lookup address
        {
            QMutexLocker locker(&mutex);
            if(!clientStubMap.contains(*webSocketClientAddress)) {
                LOG_ERROR(logger, QString("No websocket found for address %0")
                          .arg(webSocketClientAddress->toString())
                );
            }
        }
        return clientStubMap.value(*webSocketClientAddress, QSharedPointer<IMessaging>());
    }
    // if destination is a WS server address
    if(destAddress.inherits(system::WebSocketAddress::staticMetaObject.className())) {
        const system::WebSocketAddress* webSocketServerAddress =
                qobject_cast<const system::WebSocketAddress*>(&destAddress);
        // lookup address
        {
            QMutexLocker locker(&mutex);
            if(!serverStubMap.contains(*webSocketServerAddress)) {
                LOG_ERROR(logger, QString("No websocket found for address %0")
                          .arg(webSocketServerAddress->toString())
                );
            }
        }
        return serverStubMap.value(*webSocketServerAddress, QSharedPointer<IMessaging>());
    }

    return QSharedPointer<IMessaging>();
}

void WebSocketMessagingStubFactory::addClient(
        const joynr::system::WebSocketClientAddress& clientAddress,
        QWebSocket* webSocket
) {
    WebSocketMessagingStub* wsClientStub = new WebSocketMessagingStub(
                new joynr::system::WebSocketClientAddress(clientAddress),
                webSocket
    );
    connect(
            wsClientStub, &WebSocketMessagingStub::closed,
            this, &WebSocketMessagingStubFactory::onMessagingStubClosed
    );
    QSharedPointer<IMessaging> clientStub(wsClientStub);
    clientStubMap.insert(clientAddress, clientStub);
}

void WebSocketMessagingStubFactory::removeClient(
        const joynr::system::WebSocketClientAddress& clientAddress
) {
    clientStubMap.remove(clientAddress);
}

void WebSocketMessagingStubFactory::addServer(
        const system::WebSocketAddress &serverAddress,
        QWebSocket *webSocket
) {
    WebSocketMessagingStub* wsServerStub = new WebSocketMessagingStub(
                new joynr::system::WebSocketAddress(serverAddress),
                webSocket
    );
    connect(
            wsServerStub, &WebSocketMessagingStub::closed,
            this, &WebSocketMessagingStubFactory::onMessagingStubClosed
    );
    QSharedPointer<IMessaging> serverStub(wsServerStub);
    serverStubMap.insert(serverAddress, serverStub);
}

void WebSocketMessagingStubFactory::onMessagingStubClosed(const system::Address &address)
{
    LOG_DEBUG(logger, QString("removing messaging stub for addres: %0").arg(address.toString()));
    if(address.inherits(system::WebSocketClientAddress::staticMetaObject.className())) {
        const system::WebSocketClientAddress* wsClientAddress =
                qobject_cast<const system::WebSocketClientAddress*>(&address);
        clientStubMap.remove(*wsClientAddress);
    }
    if(address.inherits(system::WebSocketAddress::staticMetaObject.className())) {
        const system::WebSocketAddress* wsServerAddress =
                qobject_cast<const system::WebSocketAddress*>(&address);
        serverStubMap.remove(*wsServerAddress);
    }
}

QUrl WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(
        const system::WebSocketAddress &address
) {
    return QUrl(QString("%0://%1:%2/%3")
                .arg(address.getProtocolInternal().toLower())
                .arg(address.getHost())
                .arg(address.getPort())
                .arg(address.getPath())
    );
}

} // namespace joynr

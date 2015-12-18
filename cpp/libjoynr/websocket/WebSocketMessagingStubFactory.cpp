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

#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtWebSockets/QWebSocket>
#include <assert.h>

#include "websocket/WebSocketMessagingStub.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/TypeUtil.h"

namespace joynr
{

joynr_logging::Logger* WebSocketMessagingStubFactory::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "WebSocketMessagingStubFactory");

WebSocketMessagingStubFactory::WebSocketMessagingStubFactory(QObject* parent)
        : QObject(parent), serverStubMap(), clientStubMap(), mutex()
{
}

bool WebSocketMessagingStubFactory::canCreate(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    return dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&destAddress) ||
           dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(&destAddress);
}

std::shared_ptr<IMessaging> WebSocketMessagingStubFactory::create(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    // if destination is a WS client address
    if (auto webSocketClientAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(&destAddress)) {
        // lookup address
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (clientStubMap.find(*webSocketClientAddress) == clientStubMap.cend()) {
                LOG_ERROR(logger,
                          FormatString("No websocket found for address %1")
                                  .arg(webSocketClientAddress->toString())
                                  .str());
                return std::shared_ptr<IMessaging>();
            }
        }
        return clientStubMap[*webSocketClientAddress];
    }
    // if destination is a WS server address
    if (const system::RoutingTypes::WebSocketAddress* webSocketServerAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&destAddress)) {
        // lookup address
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (serverStubMap.find(*webSocketServerAddress) == serverStubMap.cend()) {
                LOG_ERROR(logger,
                          FormatString("No websocket found for address %1")
                                  .arg(webSocketServerAddress->toString())
                                  .str());
                return std::shared_ptr<IMessaging>();
            }
        }
        return serverStubMap[*webSocketServerAddress];
    }

    return std::shared_ptr<IMessaging>();
}

void WebSocketMessagingStubFactory::addClient(
        const joynr::system::RoutingTypes::WebSocketClientAddress* clientAddress,
        QWebSocket* webSocket)
{

    if (clientStubMap.count(*clientAddress) == 0) {
        WebSocketMessagingStub* wsClientStub = new WebSocketMessagingStub(clientAddress, webSocket);
        connect(wsClientStub,
                &WebSocketMessagingStub::closed,
                this,
                &WebSocketMessagingStubFactory::onMessagingStubClosed);
        std::shared_ptr<IMessaging> clientStub(wsClientStub);
        clientStubMap[*clientAddress] = clientStub;
    } else {
        LOG_ERROR(logger,
                  FormatString("Client with address %1 already exists in the clientStubMap")
                          .arg(clientAddress->toString())
                          .str());
    }
}

void WebSocketMessagingStubFactory::removeClient(
        const joynr::system::RoutingTypes::WebSocketClientAddress& clientAddress)
{
    clientStubMap.erase(clientAddress);
}

void WebSocketMessagingStubFactory::addServer(
        const joynr::system::RoutingTypes::WebSocketAddress& serverAddress,
        QWebSocket* webSocket)
{

    WebSocketMessagingStub* wsServerStub = new WebSocketMessagingStub(
            new system::RoutingTypes::WebSocketAddress(serverAddress), webSocket);
    connect(wsServerStub,
            &WebSocketMessagingStub::closed,
            this,
            &WebSocketMessagingStubFactory::onMessagingStubClosed);
    std::shared_ptr<IMessaging> serverStub(wsServerStub);
    serverStubMap[serverAddress] = serverStub;
}

void WebSocketMessagingStubFactory::onMessagingStubClosed(
        const system::RoutingTypes::Address& address)
{
    LOG_DEBUG(
            logger,
            FormatString("removing messaging stub for address: %1").arg(address.toString()).str());
    // if destination is a WS client address
    if (auto webSocketClientAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(&address)) {
        clientStubMap.erase(*webSocketClientAddress);
    } else if (auto webSocketServerAddress =
                       dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&address)) {
        serverStubMap.erase(*webSocketServerAddress);
    }
}

QUrl WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(
        const system::RoutingTypes::WebSocketAddress& address)
{
    return QUrl(QString("%0://%1:%2%3")
                        .arg(QString::fromStdString(
                                     joynr::system::RoutingTypes::WebSocketProtocol::getLiteral(
                                             address.getProtocol())).toLower())
                        .arg(QString::fromStdString(address.getHost()))
                        .arg(address.getPort())
                        .arg(QString::fromStdString(address.getPath())));
}

} // namespace joynr

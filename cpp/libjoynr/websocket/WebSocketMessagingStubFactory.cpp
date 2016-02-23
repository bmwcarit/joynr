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

#include <cassert>
#include <typeinfo>
#include <functional>
#include <cctype>
#include <algorithm>
#include <string>

#include "websocket/WebSocketMessagingStub.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/TypeUtil.h"
#include "joynr/FormatString.h"

namespace joynr
{

INIT_LOGGER(WebSocketMessagingStubFactory);

WebSocketMessagingStubFactory::WebSocketMessagingStubFactory()
        : serverStubMap(), clientStubMap(), mutex()
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
                JOYNR_LOG_ERROR(logger,
                                "No websocket found for address {}",
                                webSocketClientAddress->toString());
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
                JOYNR_LOG_ERROR(logger,
                                "No websocket found for address {}",
                                webSocketServerAddress->toString());
                return std::shared_ptr<IMessaging>();
            }
        }
        return serverStubMap[*webSocketServerAddress];
    }

    return std::shared_ptr<IMessaging>();
}

void WebSocketMessagingStubFactory::addClient(
        const system::RoutingTypes::WebSocketClientAddress& clientAddress,
        IWebSocketSendInterface* webSocket)
{

    if (clientStubMap.count(clientAddress) == 0) {
        WebSocketMessagingStub* wsClientStub = new WebSocketMessagingStub(
                webSocket, [this, clientAddress]() { this->onMessagingStubClosed(clientAddress); });
        std::shared_ptr<IMessaging> clientStub(wsClientStub);
        clientStubMap[clientAddress] = clientStub;
    } else {
        JOYNR_LOG_ERROR(logger,
                        "Client with address {} already exists in the clientStubMap",
                        clientAddress.toString());
    }
}

void WebSocketMessagingStubFactory::removeClient(
        const joynr::system::RoutingTypes::WebSocketClientAddress& clientAddress)
{
    clientStubMap.erase(clientAddress);
}

void WebSocketMessagingStubFactory::addServer(
        const joynr::system::RoutingTypes::WebSocketAddress& serverAddress,
        IWebSocketSendInterface* webSocket)
{

    WebSocketMessagingStub* wsServerStub = new WebSocketMessagingStub(
            webSocket, [this, serverAddress]() { this->onMessagingStubClosed(serverAddress); });
    std::shared_ptr<IMessaging> serverStub(wsServerStub);
    serverStubMap[serverAddress] = serverStub;
}

void WebSocketMessagingStubFactory::onMessagingStubClosed(
        const system::RoutingTypes::Address& address)
{
    JOYNR_LOG_DEBUG(logger, "removing messaging stub for address: {}", address.toString());
    // if destination is a WS client address
    if (auto webSocketClientAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(&address)) {
        clientStubMap.erase(*webSocketClientAddress);
    } else if (auto webSocketServerAddress =
                       dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&address)) {
        serverStubMap.erase(*webSocketServerAddress);
    }
}

Url WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(
        const system::RoutingTypes::WebSocketAddress& address)
{
    std::string protocol =
            joynr::system::RoutingTypes::WebSocketProtocol::getLiteral(address.getProtocol());
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);

    return Url(FormatString("%1://%2:%3%4")
                       .arg(protocol)
                       .arg(address.getHost())
                       .arg(address.getPort())
                       .arg(address.getPath())
                       .str());
}

} // namespace joynr

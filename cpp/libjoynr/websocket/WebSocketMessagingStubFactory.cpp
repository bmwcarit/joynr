/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include <cctype>
#include <algorithm>
#include <string>

#include <boost/format.hpp>

#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"

namespace joynr
{

INIT_LOGGER(WebSocketMessagingStubFactory);

WebSocketMessagingStubFactory::WebSocketMessagingStubFactory()
        : serverStubMap(),
          serverStubMapMutex(),
          clientStubMap(),
          clientStubMapMutex(),
          onMessagingStubClosedCallback(nullptr)
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
        std::lock_guard<std::mutex> lock(clientStubMapMutex);
        const std::unordered_map<joynr::system::RoutingTypes::WebSocketClientAddress,
                                 std::shared_ptr<IMessaging>>::const_iterator stub =
                clientStubMap.find(*webSocketClientAddress);
        if (stub == clientStubMap.cend()) {
            JOYNR_LOG_ERROR(logger,
                            "No websocket found for address {}",
                            webSocketClientAddress->toString());
            return std::shared_ptr<IMessaging>();
        }
        return stub->second;
    }
    // if destination is a WS server address
    if (const system::RoutingTypes::WebSocketAddress* webSocketServerAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&destAddress)) {
        std::lock_guard<std::mutex> lock(serverStubMapMutex);
        const std::unordered_map<joynr::system::RoutingTypes::WebSocketAddress,
                                 std::shared_ptr<IMessaging>>::const_iterator stub =
                serverStubMap.find(*webSocketServerAddress);
        if (stub == serverStubMap.cend()) {
            JOYNR_LOG_ERROR(logger,
                            "No websocket found for address {}",
                            webSocketServerAddress->toString());
            return std::shared_ptr<IMessaging>();
        }
        return stub->second;
    }

    return std::shared_ptr<IMessaging>();
}

void WebSocketMessagingStubFactory::addClient(
        const system::RoutingTypes::WebSocketClientAddress& clientAddress,
        const std::shared_ptr<IWebSocketSendInterface>& webSocket)
{
    if (clientStubMap.count(clientAddress) == 0) {
        WebSocketMessagingStub* wsClientStub = new WebSocketMessagingStub(webSocket);
        std::shared_ptr<IMessaging> clientStub(wsClientStub);
        {
            std::lock_guard<std::mutex> lock(clientStubMapMutex);
            clientStubMap[clientAddress] = clientStub;
        }
    } else {
        JOYNR_LOG_ERROR(logger,
                        "Client with address {} already exists in the clientStubMap",
                        clientAddress.toString());
    }
}

void WebSocketMessagingStubFactory::removeClient(
        const joynr::system::RoutingTypes::WebSocketClientAddress& clientAddress)
{
    std::lock_guard<std::mutex> lock(clientStubMapMutex);
    clientStubMap.erase(clientAddress);
}

void WebSocketMessagingStubFactory::addServer(
        const joynr::system::RoutingTypes::WebSocketAddress& serverAddress,
        const std::shared_ptr<IWebSocketSendInterface>& webSocket)
{
    auto serverStub = std::make_shared<WebSocketMessagingStub>(webSocket);
    {
        std::lock_guard<std::mutex> lock(serverStubMapMutex);
        serverStubMap[serverAddress] = serverStub;
    }
}

void WebSocketMessagingStubFactory::onMessagingStubClosed(
        const system::RoutingTypes::Address& address)
{
    JOYNR_LOG_DEBUG(logger, "removing messaging stub for address: {}", address.toString());
    std::shared_ptr<const system::RoutingTypes::Address> addressPtr = nullptr;
    // if destination is a WS client address
    if (auto webSocketClientAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(&address)) {
        std::lock_guard<std::mutex> lock(clientStubMapMutex);
        addressPtr = std::make_shared<const system::RoutingTypes::WebSocketClientAddress>(
                *webSocketClientAddress);
        clientStubMap.erase(*webSocketClientAddress);
    } else if (auto webSocketServerAddress =
                       dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&address)) {
        std::lock_guard<std::mutex> lock(serverStubMapMutex);
        addressPtr = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
                *webSocketServerAddress);
        serverStubMap.erase(*webSocketServerAddress);
    }
    if (onMessagingStubClosedCallback) {
        onMessagingStubClosedCallback(addressPtr);
    }
}

void WebSocketMessagingStubFactory::registerOnMessagingStubClosedCallback(
        std::function<void(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&
                                   destinationAddress)> onMessagingStubClosedCallback)
{
    this->onMessagingStubClosedCallback = std::move(onMessagingStubClosedCallback);
}

Url WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(
        const system::RoutingTypes::WebSocketAddress& address)
{
    std::string protocol =
            joynr::system::RoutingTypes::WebSocketProtocol::getLiteral(address.getProtocol());
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);

    return Url((boost::format("%1%://%2%:%3%%4%") % protocol % address.getHost() %
                address.getPort() % address.getPath()).str());
}

} // namespace joynr

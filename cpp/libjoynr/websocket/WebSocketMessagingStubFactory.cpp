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
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"

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
    return dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&destAddress) != nullptr ||
           dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(&destAddress) !=
                   nullptr;
}

std::shared_ptr<IMessagingStub> WebSocketMessagingStubFactory::create(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    // if destination is a WS client address
    if (auto webSocketClientAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(&destAddress)) {
        std::lock_guard<std::mutex> lock(clientStubMapMutex);
        const std::unordered_map<joynr::system::RoutingTypes::WebSocketClientAddress,
                                 std::shared_ptr<IMessagingStub>>::const_iterator stub =
                clientStubMap.find(*webSocketClientAddress);
        if (stub == clientStubMap.cend()) {
            JOYNR_LOG_ERROR(logger,
                            "No websocket found for address {}",
                            webSocketClientAddress->toString());
            return std::shared_ptr<IMessagingStub>();
        }
        return stub->second;
    }
    // if destination is a WS server address
    if (const system::RoutingTypes::WebSocketAddress* webSocketServerAddress =
                dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(&destAddress)) {
        std::lock_guard<std::mutex> lock(serverStubMapMutex);
        const std::unordered_map<joynr::system::RoutingTypes::WebSocketAddress,
                                 std::shared_ptr<IMessagingStub>>::const_iterator stub =
                serverStubMap.find(*webSocketServerAddress);
        if (stub == serverStubMap.cend()) {
            JOYNR_LOG_ERROR(logger,
                            "No websocket found for address {}",
                            webSocketServerAddress->toString());
            return std::shared_ptr<IMessagingStub>();
        }
        return stub->second;
    }

    return std::shared_ptr<IMessagingStub>();
}

void WebSocketMessagingStubFactory::addClient(
        const system::RoutingTypes::WebSocketClientAddress& clientAddress,
        std::shared_ptr<IWebSocketSendInterface> webSocket)
{
    if (clientStubMap.count(clientAddress) == 0) {
        auto wsClientStub = std::make_shared<WebSocketMessagingStub>(std::move(webSocket));
        {
            std::lock_guard<std::mutex> lock(clientStubMapMutex);
            clientStubMap[clientAddress] = std::move(wsClientStub);
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
        std::shared_ptr<IWebSocketSendInterface> webSocket)
{
    auto serverStub = std::make_shared<WebSocketMessagingStub>(std::move(webSocket));
    {
        std::lock_guard<std::mutex> lock(serverStubMapMutex);
        serverStubMap[serverAddress] = std::move(serverStub);
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
        std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                   destinationAddress)> onMessagingStubClosedCallback)
{
    this->onMessagingStubClosedCallback = std::move(onMessagingStubClosedCallback);
}

} // namespace joynr

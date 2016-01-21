/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include "joynr/ConcurrentQueue.h"
#include "joynr/Logger.h"
#include "WebSocket.h"
#include "WebSocketContext.h"

#include <string>
#include <memory>

namespace joynr
{

/**
 * @class WebSocketClient
 * @brief Representing a client web socket based on libwebsocket
 */
class WebSocketClient : public joynr::WebSocket
{
public:
    /**
     * @brief Constructor
     * @param onErrorOccurred On error callback
     * @param onWebSocketConnected On connection successful callback
     */
    WebSocketClient(std::function<void(const std::string& error)> onErrorOccurred,
                    std::function<void(WebSocket* webSocket)> onWebSocketConnected);
    /**
     * @brief Destructor
     */
    ~WebSocketClient();

    /**
     * @brief Connects to WebSocket server on the given address
     * @note This call will block until the connection is established
     * @param address Address of the server
     */
    virtual void connect(const system::RoutingTypes::WebSocketAddress& address);
    /**
     * @brief Send text message to the server
     * @param message Message to be sent
     */
    virtual void send(const std::string& message);

    virtual void terminate();

    virtual void onMessageReceived(const std::string& host,
                                   const std::string& name,
                                   const std::string& message);

    virtual void onConnectionClosed();

    virtual void onConnectionEstablished();

    virtual void onErrorOccured(WebSocketError error);

    virtual void onWebSocketWriteable(WebSocketContext::WebSocketConnectionHandle handle,
                                      std::function<int(const std::string&)> write);

    virtual void onNewConnection(WebSocketContext::WebSocketConnectionHandle handle,
                                 const std::string& host,
                                 const std::string& name);

private:
    /*! Pointer to the context initialized by @ref connect */
    std::unique_ptr<WebSocketContext> context;
    /*! Queue of outgoing messages */
    ConcurrentQueue<std::string> messageQueue;
    /*! Handle of the current connection */
    WebSocketContext::WebSocketConnectionHandle handle;
    /*! Address of the server */
    system::RoutingTypes::WebSocketAddress address;

    /*! On error callback */
    std::function<void(const std::string& error)> onErrorOccurred;
    /*! On connection successful callback */
    std::function<void(WebSocket* webSocket)> onWebSocketConnected;

    ADD_LOGGER(WebSocketClient);
};

} // namespace joynr
#endif // WEBSOCKETCLIENT_H

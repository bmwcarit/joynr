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
#ifndef IWEBSOCKETCONTEXTCALLBACK_H_
#define IWEBSOCKETCONTEXTCALLBACK_H_

#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "WebSocketContext.h"

#include <string>
#include <functional>

namespace joynr
{

/**
 * @class WebSocketContextCallback
 * @brief Callback interface to be used by @ref WebSocketContext
 */
class IWebSocketContextCallback
{
public:
    virtual ~IWebSocketContextCallback() = default;

    /**
     * @brief Error definitions
     */
    enum WebSocketError {
        WebSocketError_GenericError,   ///< A generic error
        WebSocketError_ConnectionError ///< Connection error
    };

    /**
     * @brief Callback on message received
     * @param host Hostname or address of the sender
     * @param name Name of the sender
     * @param message Received message
     */
    virtual void onMessageReceived(const std::string& host,
                                   const std::string& name,
                                   const std::string& message) = 0;

    /**
     * @brief Notification on connection closed
     */
    virtual void onConnectionClosed() = 0;

    /**
     * @brief Notification on connection established
     */
    virtual void onConnectionEstablished() = 0;

    /**
     * @brief Notification on error occured
     * @param error Error type
     */
    virtual void onErrorOccured(WebSocketError error) = 0;

    /**
     * @brief Notification on WebSocket writeable
     * @param handle Handle of connection that is writeable at the moment
     * @param write Method to be used to send the message. This method must be
     *      called from caller context.
     */
    virtual void onWebSocketWriteable(WebSocketContext::WebSocketConnectionHandle handle,
                                      std::function<int(const std::string&)> write) = 0;

    /**
     * @brief Notification on new client connection
     * @param handle Handle of the new connection
     * @param host Hostname or address of the client
     * @param name Name of the client
     */
    virtual void onNewConnection(WebSocketContext::WebSocketConnectionHandle handle,
                                 const std::string& host,
                                 const std::string& name) = 0;
};

} // namespace joynr

#endif // IWEBSOCKETCONTEXTCALLBACK_H_

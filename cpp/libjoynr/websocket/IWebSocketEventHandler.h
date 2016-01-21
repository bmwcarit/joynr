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
#ifndef WEBSOCKETEVENTHANDLER_H
#define WEBSOCKETEVENTHANDLER_H

#include <string>

namespace joynr
{
class WebSocket;

/**
 * @brief The IWebSocketEventHandler class declares event handlers for set of websocket events.
 * WebSocket holds a reference to IWebSocketEventHandler and use it to inform application context
 * about websocket events. This concept allows various implementations of IWebSocketEventHandler
 * (e.g. Sync, or Async variant, different implementation for testing purposes, etc.).
 */
class IWebSocketEventHandler
{
public:
    /**
     * @brief Destructor
     */
    virtual ~IWebSocketEventHandler() = default;
    /**
     * @brief onConnected
     * @param endpointId
     */
    virtual void onConnected(WebSocket* endpointId) = 0;
    /**
     * @brief onDisconnected
     * @param endpointId
     */
    virtual void onDisconnected(WebSocket* endpointId) = 0;
    /**
     * @brief onError
     * @param errorMessage
     */
    virtual void onError(const std::string& errorMessage) = 0;
    /**
     * @brief onTextMessageReceived
     * @param message
     */
    virtual void onTextMessageReceived(const std::string& message) = 0;
};

} // namespace joynr
#endif // WEBSOCKETEVENTHANDLER_H

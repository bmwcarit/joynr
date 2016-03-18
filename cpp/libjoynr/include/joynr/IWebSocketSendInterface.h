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
#ifndef IWEBSOCKETSENDINTERFACE_H
#define IWEBSOCKETSENDINTERFACE_H

#include <functional>
#include <string>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/JoynrExport.h"

namespace joynr
{

/**
 * @class WebSocketSendInterface
 * @brief A generic send interface for different WebSocket implementation
 */
class JOYNR_EXPORT IWebSocketSendInterface
{
public:
    /**
     * @brief Destructor
     */
    virtual ~IWebSocketSendInterface() = default;

    /**
     * @brief Send a message asynchronously via WebSocket
     * @param message Message to be sent
     */
    virtual void send(
            const std::string& message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) = 0;

    /**
     * @brief Register method called on disconnect
     * @param onWebSocketDisconnected Callback method
     * @note This is needed because of the missing signal / slot mechanism of
     *      Qt. The ownership of objects based on @ref WebSocketSendInterface
     *      is given to WebSocketMessagingStub. So WebSocketMessagingStub and
     *      it needs to be informed about a disconnect
     */
    virtual void registerDisconnectCallback(std::function<void()> onWebSocketDisconnected) = 0;

    /**
     * @brief Register method called on message received
     * @param onTextMessageReceived Callback method with message as parameter
     * @note This is needed because of the missing signal / slot mechanism of
     *      Qt. All messages will be received by the runtime.
     */
    virtual void registerReceiveCallback(
            std::function<void(const std::string&)> onTextMessageReceived) = 0;

    /**
     * @brief Returns whether the socket is initialized or not
     * @return Initialization flag
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Returns whether the socket is connected or not
     * @return Connection flag
     */
    virtual bool isConnected() const = 0;
};

} // namespace joynr

#endif // IWEBSOCKETSENDINTERFACE_H

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
#include "WebSocket.h"
#include "WebSocketContext.h"

namespace joynr
{

WebSocket::WebSocket()
        : IWebSocketSendInterface(),
          state(WebSocketState_Closed),
          sslCertPath(),
          sslKeyPath(),
          onTextMessageReceived(),
          onWebSocketDisconnected()
{
}

void WebSocket::configureSSL(const std::string& certPath, const std::string& keyPath)
{
    sslCertPath = certPath;
    sslKeyPath = keyPath;
}

void WebSocket::registerDisconnectCallback(std::function<void()> onWebSocketDisconnected)
{
    this->onWebSocketDisconnected = onWebSocketDisconnected;
}

void WebSocket::registerReceiveCallback(
        std::function<void(const std::string&)> onTextMessageReceived)
{
    this->onTextMessageReceived = onTextMessageReceived;
}

bool WebSocket::isInitialized() const
{
    return state >= WebSocketState_Initialized;
}

bool WebSocket::isConnected() const
{
    return state >= WebSocketState_Connected;
}

} // namespace joynr

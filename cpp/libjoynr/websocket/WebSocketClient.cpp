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
#include "IWebSocketEventHandler.h"
#include "websocket/WebSocketClient.h"
#include "joynr/exceptions/JoynrException.h"

#include <thread>
#include <iostream>
#include <cassert>
#include <sstream>

namespace joynr
{

using namespace system::RoutingTypes;

INIT_LOGGER(WebSocketClient);

WebSocketClient::WebSocketClient(std::function<void(const std::string& error)> onErrorOccurred,
                                 std::function<void(WebSocket* webSocket)> onWebSocketConnected)
        : WebSocket(),
          context(),
          messageQueue(),
          handle(-1),
          address(),
          onErrorOccurred(onErrorOccurred),
          onWebSocketConnected(onWebSocketConnected)
{
}

WebSocketClient::~WebSocketClient()
{
    terminate();
    state = WebSocketState_Closed;
}

void WebSocketClient::connect(const WebSocketAddress& address)
{
    WebSocketContext::WebSocketSsl ssl = WebSocketContext::WebSocketSsl_NoSsl;

    JOYNR_LOG_TRACE(logger, "Called connect");

    if (address.getProtocol() == WebSocketProtocol::WSS) {
        assert(!sslCertPath.empty());
        assert(!sslKeyPath.empty());
        ssl = WebSocketContext::WebSocketSsl_SelfSigned;
        JOYNR_LOG_DEBUG(logger, "Configured for use SSL");
    }

    context = std::make_unique<WebSocketContext>(this, "", "", -1, sslCertPath, sslKeyPath);
    state = WebSocketState_Initializing;
    context->start();
    state = WebSocketState_Initialized;
    handle = context->connectToServer(address, ssl);
    state = WebSocketState_Connecting;
    assert(handle >= 0);

    JOYNR_LOG_TRACE(logger, "Called finished");

    this->address = address;
}

void WebSocketClient::send(const std::string& message)
{
    messageQueue.push(message);
    if (context == nullptr || handle < 0) {
        JOYNR_LOG_WARN(logger, "Not yet connected, message was queued");
    } else {
        context->onOutgoingDataAvailable(handle);
    }
}

void WebSocketClient::terminate()
{
    state = WebSocketState_Terminating;
    JOYNR_LOG_DEBUG(logger, "Signaling to stop WebSocketContext");
    if (context.get() != nullptr) {
        context->stop();
    }
}

void WebSocketClient::onMessageReceived(const std::string&,
                                        const std::string&,
                                        const std::string& message)
{
    if (onTextMessageReceived) {
        onTextMessageReceived(message);
    } else {
        JOYNR_LOG_ERROR(
                logger,
                "Discarding received message, since onTextMessageReceived callback is empty.");
    }
}

void WebSocketClient::onConnectionClosed()
{
    state = WebSocketState_Closed;
    // Notification must done asynchronously to avoid blocking
    std::thread(onWebSocketDisconnected).detach();
}

void WebSocketClient::onConnectionEstablished()
{
    state = WebSocketState_Connected;
    JOYNR_LOG_DEBUG(logger, "Connection got established");
    if (!messageQueue.empty()) {
        JOYNR_LOG_DEBUG(logger,
                        "Will try to send messages already queued (queue: ){}",
                        messageQueue.size());
        context->onOutgoingDataAvailable(handle);
    }
    onWebSocketConnected(this);
}

void WebSocketClient::onErrorOccured(WebSocketError err)
{
    JOYNR_LOG_ERROR(logger, "onErrorOccured with value {}", err);
}

void WebSocketClient::onWebSocketWriteable(WebSocketContext::WebSocketConnectionHandle handle,
                                           std::function<int32_t(const std::string&)> write)
{
    assert(this->handle == handle);

    std::string& message = messageQueue.front();

    if (!message.empty()) {
        JOYNR_LOG_TRACE(logger, "Sending message from queue");
        int32_t bytesSent = write(message);
        if (bytesSent < 0) {
            JOYNR_LOG_ERROR(logger, "Writing to socket returned with an error");
        }
        if (static_cast<uint32_t>(bytesSent) != message.length()) {
            JOYNR_LOG_WARN(logger, "Was not able to send exact message, will retry...");
        } else {
            messageQueue.pop();
            JOYNR_LOG_TRACE(
                    logger, "Message sent successfully. {} messages in queue", messageQueue.size());
        }
    }

    // On message sent, but maybe there are still pending outgoing messages.
    if (!messageQueue.empty() && handle >= 0) {
        context->onOutgoingDataAvailable(handle);
    }
}

void WebSocketClient::onNewConnection(WebSocketContext::WebSocketConnectionHandle,
                                      const std::string&,
                                      const std::string&)
{
    JOYNR_LOG_TRACE(logger, "New incoming connection on a client should not happen.");
}

} // namespace joynr

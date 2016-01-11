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
#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "IWebSocketContextCallback.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/IWebSocketSendInterface.h"

#include <string>
#include <functional>

namespace joynr
{

/**
 * @brief The WebSocket class is used as base class for Client and Server implementations.
 */
class WebSocket : public IWebSocketSendInterface, public IWebSocketContextCallback
{
public:
    /**
     * @brief Constructor
     */
    WebSocket();

    /**
     * @brief Destructor
     */
    virtual ~WebSocket() = default;

    /**
     * @brief Enables secured connection over SSL.
     * @note It requires libwebsockets to be build with SSL support which
     *      significantly increases memory footprint!
     * @param certPath Path to private certificate
     * @param keyPath Path to public key file
     */
    void configureSSL(const std::string& certPath, const std::string& keyPath);

    void registerDisconnectCallback(std::function<void()> onWebSocketDisconnected) override;

    void registerReceiveCallback(
            std::function<void(const std::string&)> onTextMessageReceived) override;

    bool isInitialized() const override;

    bool isConnected() const override;

    /**
     * @brief terminate closes connetion and terminates websocket thread
     */
    virtual void terminate() = 0;

protected:
    /**
     * @brief Definition of the web socket states
     */
    enum WebSocketState {
        WebSocketState_Closed,       ///< Closed
        WebSocketState_Terminating,  ///< During termination
        WebSocketState_Initializing, ///< During initialization
        WebSocketState_Initialized,  ///< Initialized
        WebSocketState_Connecting,   ///< During connecting
        WebSocketState_Connected     ///< Connected
    };

    /*! Current state of the web socket */
    WebSocketState state;

    /*! Path to private certificate */
    std::string sslCertPath;
    /*! Path to public key file */
    std::string sslKeyPath;

    /*! On text message received callback */
    std::function<void(const std::string&)> onTextMessageReceived;
    /*! On disconnected callback */
    std::function<void()> onWebSocketDisconnected;

private:
    DISALLOW_COPY_AND_ASSIGN(WebSocket);
};

} // namespace joynr
#endif // WEBSOCKET_H

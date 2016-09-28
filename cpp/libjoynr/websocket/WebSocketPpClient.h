/*
 * #%L
 * %%
 * Copyright (C) 2016 - 2016 BMW Car IT GmbH
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
#ifndef WEBSOCKETPPCLIENT_H
#define WEBSOCKETPPCLIENT_H

#include <atomic>
#include <chrono>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_service.hpp>

#include "joynr/Logger.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "libjoynr/websocket/WebSocketSettings.h"
#include "libjoynr/websocket/WebSocketPpReceiver.h"
#include "libjoynr/websocket/WebSocketPpSender.h"

namespace joynr
{

class WebSocketPpClient
{
public:
    using ConnectionHandle = websocketpp::connection_hdl;
    using Config = websocketpp::config::asio_client;
    using Client = websocketpp::client<Config>;

    WebSocketPpClient(const WebSocketSettings& wsSettings, boost::asio::io_service& ioService);
    virtual ~WebSocketPpClient();

    void registerConnectCallback(std::function<void()> callback);

    /**
     * @brief Register method called on disconnect
     * @param onWebSocketDisconnected Callback method
     * @note This is needed because of the missing signal / slot mechanism of
     *      Qt. The ownership of objects based on @ref WebSocketSendInterface
     *      is given to WebSocketMessagingStub. So WebSocketMessagingStub and
     *      it needs to be informed about a disconnect
     */
    virtual void registerDisconnectCallback(std::function<void()> onWebSocketDisconnected);

    /**
     * @brief Register method called on message received
     * @param onTextMessageReceived Callback method with message as parameter
     * @note This is needed because of the missing signal / slot mechanism of
     *      Qt. All messages will be received by the runtime.
     */
    void registerReceiveCallback(std::function<void(const std::string&)> onTextMessageReceived);

    void connect(system::RoutingTypes::WebSocketAddress address);
    void close();

    /**
     * @brief Returns whether the socket is initialized or not
     * @return Initialization flag
     */
    bool isInitialized() const;

    /**
     * @brief Returns whether the socket is connected or not
     * @return Connection flag
     */
    bool isConnected() const;

    void send(const std::string& msg,
              const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

    void sendTextMessage(
            const std::string& msg,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

    void sendBinaryMessage(
            const std::string& msg,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

    std::shared_ptr<WebSocketPpSender<Client>> getSender() const
    {
        return sender;
    }

protected:
    enum class State { Disconnected, Disconnecting, Connecting, Connected };

private:
    void reconnect(
            const boost::system::error_code& reconnectTimerError = boost::system::error_code());
    void delayedReconnect();
    void disconnect();
    void onConnectionOpened(ConnectionHandle hdl);
    void onConnectionClosed(ConnectionHandle hdl);
    void onConnectionFailed(ConnectionHandle hdl);

    Client endpoint;
    ConnectionHandle connection;
    std::atomic<bool> isRunning;
    boost::asio::steady_timer reconnectTimer;
    std::atomic<State> state;

    // store address for reconnect
    system::RoutingTypes::WebSocketAddress address;
    std::chrono::milliseconds reconnectSleepTimeMs;

    std::function<void()> onConnectionOpenedCallback;
    std::function<void()> onConnectionClosedCallback;

    std::shared_ptr<WebSocketPpSender<Client>> sender;
    WebSocketPpReceiver<Client> receiver;

    ADD_LOGGER(WebSocketPpClient);
};

} // namespace joynr

#endif // WEBSOCKETPPCLIENT_H

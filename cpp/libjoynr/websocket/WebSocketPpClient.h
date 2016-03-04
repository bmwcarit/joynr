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

#include <thread>
#include <atomic>
#include <chrono>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_service.hpp>

#include "joynr/Logger.h"
#include "joynr/IWebSocketSendInterface.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "libjoynr/websocket/WebSocketSettings.h"

namespace joynr
{

class WebSocketPpClient : public IWebSocketSendInterface
{
    using ConnectionHandle = websocketpp::connection_hdl;
    using Config = websocketpp::config::asio_client;
    using MessagePtr = Config::message_type::ptr;
    using Client = websocketpp::client<Config>;

    enum class State { Disconnected, Disconnecting, Connecting, Connected };

public:
    WebSocketPpClient(const WebSocketSettings& wsSettings);
    ~WebSocketPpClient();

    void connect(system::RoutingTypes::WebSocketAddress address);
    void close();

    void registerConnectCallback(std::function<void()> callback);
    void registerDisconnectCallback(std::function<void()> callback) override;
    void registerReceiveCallback(std::function<void(const std::string&)> callback) override;

    bool isInitialized() const override;
    bool isConnected() const override;

    void send(const std::string& msg,
              const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;
    void sendTextMessage(
            const std::string& msg,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);
    void sendBinaryMessage(
            const std::string& msg,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

private:
    void reconnect(
            const boost::system::error_code& reconnectTimerError = boost::system::error_code());
    void delayedReconnect();
    void disconnect();
    void onConnectionOpened(ConnectionHandle hdl);
    void onConnectionClosed(ConnectionHandle hdl);
    void onMessageReceived(ConnectionHandle hdl, MessagePtr message);
    void onConnectionFailed(ConnectionHandle hdl);

    Client endpoint;
    std::thread thread;
    ConnectionHandle connection;
    std::atomic<bool> isRunning;

    boost::asio::io_service ioService;
    boost::asio::steady_timer reconnectTimer;

    std::atomic<State> state;
    std::function<void(const std::string&)> onTextMessageReceivedCallback;
    std::function<void(const std::string&)> onBinaryMessageReceived;
    std::function<void()> onConnectionOpenedCallback;
    std::function<void()> onConnectionClosedCallback;

    // store address for reconnect
    system::RoutingTypes::WebSocketAddress address;
    std::chrono::milliseconds reconnectSleepTimeMs;

    ADD_LOGGER(WebSocketPpClient);
};

} // namespace joynr

#endif // WEBSOCKETPPCLIENT_H

/*
 * #%L
 * %%
 * Copyright (C) 2016 - 2017 BMW Car IT GmbH
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
#include <cassert>
#include <chrono>
#include <functional>

#include <boost/asio/steady_timer.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/error.hpp>
#include <websocketpp/uri.hpp>

#include "IWebSocketPpClient.h"
#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/WebSocketSettings.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketProtocol.h"
#include "libjoynr/websocket/WebSocketPpReceiver.h"
#include "libjoynr/websocket/WebSocketPpSender.h"

namespace joynr
{

template <typename Config>
class WebSocketPpClient : public IWebSocketPpClient
{
public:
    using ConnectionHandle = websocketpp::connection_hdl;
    using Client = websocketpp::client<Config>;
    using ConnectionPtr = typename Client::connection_ptr;

    WebSocketPpClient(const WebSocketSettings& wsSettings, boost::asio::io_service& ioService)
            : webSocketPpSingleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              endpoint(),
              connection(),
              isRunning(true),
              reconnectTimer(ioService),
              state(State::Disconnected),
              performingInitialConnect(true),
              address(),
              reconnectSleepTimeMs(wsSettings.getReconnectSleepTimeMs()),
              sender(nullptr),
              receiver(),
              isShuttingDown(false),
              isShuttingDownLock()
    {
        webSocketPpSingleThreadedIOService->start();
        boost::asio::io_service& endpointIoService =
                webSocketPpSingleThreadedIOService->getIOService();

        websocketpp::lib::error_code initializationError;

        endpoint.clear_access_channels(websocketpp::log::alevel::all);
        endpoint.clear_error_channels(websocketpp::log::alevel::all);
        endpoint.init_asio(&endpointIoService, initializationError);
        if (initializationError) {
            JOYNR_LOG_FATAL(logger(),
                            "error during WebSocketPp initialization: ",
                            initializationError.message());
            return;
        }

        endpoint.start_perpetual();

        // register handlers
        endpoint.set_open_handler(
                std::bind(&WebSocketPpClient::onConnectionOpened, this, std::placeholders::_1));
        endpoint.set_fail_handler(
                std::bind(&WebSocketPpClient::onConnectionFailed, this, std::placeholders::_1));
        endpoint.set_close_handler(
                std::bind(&WebSocketPpClient::onConnectionClosed, this, std::placeholders::_1));
        endpoint.set_message_handler(std::bind(&WebSocketPpReceiver<Client>::onMessageReceived,
                                               &receiver,
                                               std::placeholders::_1,
                                               std::placeholders::_2));

        sender = std::make_shared<WebSocketPpSender<Client>>(endpoint);
    }

    ~WebSocketPpClient() override
    {
        // make sure stop() has been invoked earlier
        assert(isShuttingDown);
    }

    void stop() final
    {
        // make sure stop() is not called multiple times
        std::lock_guard<std::mutex> lock(isShuttingDownLock);
        assert(!isShuttingDown);
        isShuttingDown = true;

        endpoint.stop_perpetual();
        close();
        // prior to destruction of the endpoint, the background thread
        // under direct control of the webSocketPpSingleThreadedIOService
        // must have finished its work, thus wait for it here;
        // however do not destruct the ioService since it is still
        // referenced within the endpoint by an internally created
        // thread from tcp::resolver which is joined by the endpoint
        // destructor
        webSocketPpSingleThreadedIOService->stop();
    }

    void registerConnectCallback(std::function<void()> callback) final
    {
        onConnectionOpenedCallback = std::move(callback);
    }

    void registerReconnectCallback(std::function<void()> callback) final
    {
        onConnectionReestablishedCallback = std::move(callback);
    }

    /**
     * @brief Register method called on final disconnect (shutdown)
     * @param onWebSocketDisconnected Callback method
     * @note The WebSocketMessagingStubFactory holds references of objects based
     *      on @ref IWebSocketSendInterface which must be destroyed.
     *      So WebSocketMessagingStubFactory needs to be informed about a
     *      disconnect.
     */
    void registerDisconnectCallback(std::function<void()> onWebSocketDisconnected) final
    {
        onConnectionClosedCallback = std::move(onWebSocketDisconnected);
    }

    /**
     * @brief Register method called on message received
     * @param onMessageReceived Callback method with message as parameter
     * @note All received messages will be forwarded to this receive callback.
     */
    void registerReceiveCallback(
            std::function<void(ConnectionHandle&&, smrf::ByteVector&&)> onMessageReceived) final
    {
        receiver.registerReceiveCallback(onMessageReceived);
    }

    void connect(const system::RoutingTypes::WebSocketAddress& address) final
    {
        this->address = address;

        performingInitialConnect = true;
        reconnect();
    }

    void close() final
    {
        if (isRunning) {
            isRunning = false;
            boost::system::error_code timerError;
            // ignore errors
            reconnectTimer.cancel(timerError);
            if (state == State::Connected) {
                disconnect();
            }
        }
    }

    /**
     * @brief Returns whether the socket is connected or not
     * @return Connection flag
     */
    bool isConnected() const final
    {
        return state == State::Connected;
    }

    void send(const smrf::ByteArrayView& msg,
              const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) final
    {
        sender->send(msg, onFailure);
    }

    std::shared_ptr<IWebSocketSendInterface> getSender() const final
    {
        return sender;
    }

protected:
    enum class State { Disconnected, Disconnecting, Connecting, Connected };

    std::shared_ptr<SingleThreadedIOService> webSocketPpSingleThreadedIOService;
    Client endpoint;
    ADD_LOGGER(WebSocketPpClient)

private:
    void reconnect(
            const boost::system::error_code& reconnectTimerError = boost::system::error_code())
    {
        if (reconnectTimerError == boost::asio::error::operation_aborted) {
            // Assume WebSocketPp.close() has been called
            JOYNR_LOG_INFO(logger(),
                           "reconnect aborted after shutdown, error code from reconnect timer: {}",
                           reconnectTimerError.message());
            return;
        } else if (reconnectTimerError) {
            JOYNR_LOG_ERROR(logger(),
                            "reconnect called with error code from reconnect timer: {}",
                            reconnectTimerError.message());
        }
        bool secure = address.getProtocol() == system::RoutingTypes::WebSocketProtocol::WSS;

        assert((!secure && std::is_same<Config, websocketpp::config::asio_client>::value) ||
               (secure && std::is_same<Config, websocketpp::config::asio_tls_client>::value));

        websocketpp::uri uri(secure, address.getHost(), address.getPort(), address.getPath());
        JOYNR_LOG_INFO(logger(), "Connecting to websocket server {}", uri.str());

        websocketpp::lib::error_code websocketError;
        ConnectionPtr connectionPtr = endpoint.get_connection(uri.str(), websocketError);

        if (websocketError) {
            JOYNR_LOG_ERROR(logger(),
                            "could not try to connect to {} - error: {}",
                            uri.str(),
                            websocketError.message());
            return;
        }

        state = State::Connecting;
        sender->resetConnectionHandle();
        endpoint.connect(connectionPtr);
    }

    void delayedReconnect()
    {
        boost::system::error_code reconnectTimerError;
        reconnectTimer.expires_from_now(reconnectSleepTimeMs, reconnectTimerError);
        if (reconnectTimerError) {
            JOYNR_LOG_FATAL(logger(),
                            "Error from reconnect timer: {}: {}",
                            reconnectTimerError.value(),
                            reconnectTimerError.message());
        } else {
            reconnectTimer.async_wait(
                    std::bind(&WebSocketPpClient::reconnect, this, std::placeholders::_1));
        }
    }

    void disconnect()
    {
        websocketpp::lib::error_code websocketError;
        endpoint.close(connection, websocketpp::close::status::normal, "", websocketError);
        if (websocketError) {
            if (websocketError != websocketpp::error::bad_connection) {
                JOYNR_LOG_ERROR(logger(),
                                "Unable to close websocket connection. Error: {}",
                                websocketError.message());
            }
        }
    }

    void onConnectionOpened(ConnectionHandle hdl)
    {
        connection = hdl;
        sender->setConnectionHandle(connection);
        state = State::Connected;
        JOYNR_LOG_INFO(logger(), "connection established");

        if (performingInitialConnect) {
            if (onConnectionOpenedCallback) {
                onConnectionOpenedCallback();
            }
        } else {
            if (onConnectionReestablishedCallback) {
                onConnectionReestablishedCallback();
            }
        }

        performingInitialConnect = false;

        if (!isRunning) {
            disconnect();
        }
    }

    void onConnectionClosed(ConnectionHandle hdl)
    {
        std::ignore = hdl;
        state = State::Disconnected;
        sender->resetConnectionHandle();
        if (!isRunning) {
            JOYNR_LOG_INFO(logger(), "connection closed");
            if (onConnectionClosedCallback) {
                onConnectionClosedCallback();
            }
        } else {
            JOYNR_LOG_WARN(logger(), "connection closed unexpectedly. Trying to reconnect...");
            delayedReconnect();
        }
    }

    void onConnectionFailed(ConnectionHandle hdl)
    {
        state = State::Disconnected;
        sender->resetConnectionHandle();
        if (!isRunning) {
            JOYNR_LOG_INFO(logger(), "connection closed");
        } else {
            ConnectionPtr con = endpoint.get_con_from_hdl(hdl);
            JOYNR_LOG_WARN(logger(),
                           "websocket connection failed - error: {}. Trying to reconnect...",
                           con->get_ec().message());
            delayedReconnect();
        }
    }

    ConnectionHandle connection;
    std::atomic<bool> isRunning;
    boost::asio::steady_timer reconnectTimer;
    std::atomic<State> state;
    bool performingInitialConnect;

    // store address for reconnect
    system::RoutingTypes::WebSocketAddress address;
    std::chrono::milliseconds reconnectSleepTimeMs;

    std::function<void()> onConnectionOpenedCallback;
    std::function<void()> onConnectionClosedCallback;
    std::function<void()> onConnectionReestablishedCallback;

    std::shared_ptr<WebSocketPpSender<Client>> sender;
    WebSocketPpReceiver<Client> receiver;
    std::atomic<bool> isShuttingDown;
    std::mutex isShuttingDownLock;
};

} // namespace joynr

#endif // WEBSOCKETPPCLIENT_H

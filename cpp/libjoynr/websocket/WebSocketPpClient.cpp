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
#include "WebSocketPpClient.h"

#include <cassert>
#include <functional>

#include <websocketpp/uri.hpp>
#include <websocketpp/error.hpp>

#include "joynr/system/RoutingTypes/WebSocketProtocol.h"

namespace joynr
{

INIT_LOGGER(WebSocketPpClient);

WebSocketPpClient::WebSocketPpClient(const WebSocketSettings& wsSettings,
                                     boost::asio::io_service& ioService)
        : endpoint(),
          connection(),
          isRunning(true),
          reconnectTimer(ioService),
          state(State::Disconnected),
          performingInitialConnect(true),
          address(),
          reconnectSleepTimeMs(wsSettings.getReconnectSleepTimeMs()),
          sender(nullptr),
          receiver()
{
    websocketpp::lib::error_code initializationError;
    endpoint.init_asio(&ioService, initializationError);
    if (initializationError) {
        JOYNR_LOG_FATAL(
                logger, "error during WebSocketPp initialization: ", initializationError.message());
        return;
    }

    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.clear_error_channels(websocketpp::log::alevel::all);
    // register handlers
    using namespace std::placeholders;
    endpoint.set_open_handler(std::bind(&WebSocketPpClient::onConnectionOpened, this, _1));
    endpoint.set_fail_handler(std::bind(&WebSocketPpClient::onConnectionFailed, this, _1));
    endpoint.set_close_handler(std::bind(&WebSocketPpClient::onConnectionClosed, this, _1));
    endpoint.set_message_handler(
            std::bind(&WebSocketPpReceiver<Client>::onMessageReceived, &receiver, _1, _2));

    sender = std::make_shared<WebSocketPpSender<Client>>(endpoint);
}

WebSocketPpClient::~WebSocketPpClient()
{
    close();
}

bool WebSocketPpClient::isInitialized() const
{
    return isConnected();
}

bool WebSocketPpClient::isConnected() const
{
    return state == State::Connected;
}

void WebSocketPpClient::connect(system::RoutingTypes::WebSocketAddress address)
{
    this->address = std::move(address);

    performingInitialConnect = true;
    reconnect();
}

void WebSocketPpClient::reconnect(const boost::system::error_code& reconnectTimerError)
{
    if (reconnectTimerError == boost::asio::error::operation_aborted) {
        // Assume WebSocketPp.close() has been called
        JOYNR_LOG_INFO(logger,
                       "reconnect aborted after shutdown, error code from reconnect timer: {}",
                       reconnectTimerError.message());
        return;
    } else if (reconnectTimerError) {
        JOYNR_LOG_ERROR(logger,
                        "reconnect called with error code from reconnect timer: {}",
                        reconnectTimerError.message());
    }
    bool secure = address.getProtocol() == system::RoutingTypes::WebSocketProtocol::WSS;
    assert(!secure && "SSL is not yet supported");
    websocketpp::uri uri(secure, address.getHost(), address.getPort(), address.getPath());
    JOYNR_LOG_INFO(logger, "Connecting to websocket server {}", uri.str());

    websocketpp::lib::error_code websocketError;
    Client::connection_ptr connectionPtr = endpoint.get_connection(uri.str(), websocketError);

    if (websocketError) {
        JOYNR_LOG_ERROR(logger,
                        "could not try to connect to {} - error: {}",
                        uri.str(),
                        websocketError.message());
        return;
    }

    state = State::Connecting;
    sender->resetConnectionHandle();
    endpoint.connect(connectionPtr);
}

void WebSocketPpClient::delayedReconnect()
{
    boost::system::error_code reconnectTimerError;
    reconnectTimer.expires_from_now(reconnectSleepTimeMs, reconnectTimerError);
    if (reconnectTimerError) {
        JOYNR_LOG_FATAL(logger,
                        "Error from reconnect timer: {}: {}",
                        reconnectTimerError.value(),
                        reconnectTimerError.message());
    } else {
        reconnectTimer.async_wait(
                std::bind(&WebSocketPpClient::reconnect, this, std::placeholders::_1));
    }
}

void WebSocketPpClient::close()
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

void WebSocketPpClient::disconnect()
{
    websocketpp::lib::error_code websocketError;
    endpoint.close(connection, websocketpp::close::status::normal, "", websocketError);
    if (websocketError) {
        if (websocketError != websocketpp::error::bad_connection) {
            JOYNR_LOG_ERROR(logger,
                            "Unable to close websocket connection. Error: {}",
                            websocketError.message());
        }
    }
}

void WebSocketPpClient::onConnectionOpened(ConnectionHandle hdl)
{
    connection = hdl;
    sender->setConnectionHandle(connection);
    state = State::Connected;
    JOYNR_LOG_INFO(logger, "connection established");

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

void WebSocketPpClient::onConnectionClosed(ConnectionHandle hdl)
{
    std::ignore = hdl;
    state = State::Disconnected;
    sender->resetConnectionHandle();
    if (!isRunning) {
        JOYNR_LOG_INFO(logger, "connection closed");
        if (onConnectionClosedCallback) {
            onConnectionClosedCallback();
        }
    } else {
        JOYNR_LOG_WARN(logger, "connection closed unexpectedly. Trying to reconnect...");
        delayedReconnect();
    }
}

void WebSocketPpClient::onConnectionFailed(ConnectionHandle hdl)
{
    state = State::Disconnected;
    sender->resetConnectionHandle();
    if (!isRunning) {
        JOYNR_LOG_INFO(logger, "connection closed");
    } else {
        Client::connection_ptr con = endpoint.get_con_from_hdl(hdl);
        JOYNR_LOG_ERROR(logger,
                        "websocket connection failed - error: {}. Trying to reconnect...",
                        con->get_ec().message());
        delayedReconnect();
    }
}

void WebSocketPpClient::registerConnectCallback(std::function<void()> callback)
{
    onConnectionOpenedCallback = std::move(callback);
}

void WebSocketPpClient::registerReconnectCallback(std::function<void()> callback)
{
    onConnectionReestablishedCallback = std::move(callback);
}

void WebSocketPpClient::registerDisconnectCallback(std::function<void()> callback)
{
    onConnectionClosedCallback = std::move(callback);
}

// wrapper for WebSocketPpReceiver

void WebSocketPpClient::registerReceiveCallback(
        std::function<void(const std::string&)> onTextMessageReceived)
{
    receiver.registerReceiveCallback(onTextMessageReceived);
}

// wrapper for WebSocketPpSender

void WebSocketPpClient::send(
        const std::string& msg,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    sender->send(msg, onFailure);
}

void WebSocketPpClient::sendTextMessage(
        const std::string& msg,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    sender->sendTextMessage(msg, onFailure);
}

void WebSocketPpClient::sendBinaryMessage(
        const std::string& msg,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    sender->sendTextMessage(msg, onFailure);
}

} // namespace joynr

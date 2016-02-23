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

WebSocketPpClient::WebSocketPpClient(const WebSocketSettings& wsSettings)
        : endpoint(),
          thread(),
          connection(),
          isRunning(true),
          state(State::Disconnected),
          onTextMessageReceivedCallback(),
          onBinaryMessageReceived(),
          onConnectionOpenedCallback(),
          onConnectionClosedCallback(),
          address(),
          reconnectSleepTimeMs(wsSettings.getReconnectSleepTimeMs())
{
    endpoint.init_asio();
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.clear_error_channels(websocketpp::log::alevel::all);

    // register handlers
    using namespace std::placeholders;
    endpoint.set_open_handler(std::bind(&WebSocketPpClient::onConnectionOpened, this, _1));
    endpoint.set_fail_handler(std::bind(&WebSocketPpClient::onConnectionFailed, this, _1));
    endpoint.set_close_handler(std::bind(&WebSocketPpClient::onConnectionClosed, this, _1));
    endpoint.set_message_handler(std::bind(&WebSocketPpClient::onMessageReceived, this, _1, _2));
}

WebSocketPpClient::~WebSocketPpClient()
{
    close();
    if (thread.joinable()) {
        thread.join();
    }
}

void WebSocketPpClient::registerReceiveCallback(std::function<void(const std::string&)> callback)
{
    onTextMessageReceivedCallback = std::move(callback);
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

    reconnect();

    // start the worker thread
    // this thread will finish when the connection is closed
    thread = std::thread(&Client::run, &endpoint);
}

void WebSocketPpClient::reconnect()
{
    bool secure = address.getProtocol() == system::RoutingTypes::WebSocketProtocol::WSS;
    assert(!secure && "SSL is not yet supported");
    websocketpp::uri uri(secure, address.getHost(), address.getPort(), address.getPath());
    JOYNR_LOG_DEBUG(logger, "Connecting to websocket server {}", uri.str());

    websocketpp::lib::error_code errorCode;
    Client::connection_ptr connectionPtr = endpoint.get_connection(uri.str(), errorCode);

    if (errorCode) {
        JOYNR_LOG_ERROR(logger,
                        "could not try to connect to {} - error: {}",
                        uri.str(),
                        errorCode.message());
        return;
    }

    state = State::Connecting;
    endpoint.connect(connectionPtr);
}

void WebSocketPpClient::sendTextMessage(const std::string& msg)
{
    JOYNR_LOG_TRACE(logger, "outgoing text message \"{}\"", msg);
    endpoint.send(connection, msg, websocketpp::frame::opcode::text);
}

void WebSocketPpClient::sendBinaryMessage(const std::string& msg)
{
    JOYNR_LOG_TRACE(logger, "outgoing binary message of size {}", msg.size());
    endpoint.send(connection, msg, websocketpp::frame::opcode::binary);
}

void WebSocketPpClient::close()
{
    isRunning = false;
    if (state == State::Connected) {
        disconnect();
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

void WebSocketPpClient::send(const std::string& msg)
{
    sendTextMessage(msg);
}

void WebSocketPpClient::registerConnectCallback(std::function<void()> callback)
{
    onConnectionOpenedCallback = std::move(callback);
}

void WebSocketPpClient::registerDisconnectCallback(std::function<void()> callback)
{
    onConnectionClosedCallback = std::move(callback);
}

void WebSocketPpClient::onConnectionOpened(ConnectionHandle hdl)
{
    connection = hdl;
    state = State::Connected;
    JOYNR_LOG_DEBUG(logger, "connection established");
    if (onConnectionOpenedCallback) {
        onConnectionOpenedCallback();
    }
    if (!isRunning) {
        disconnect();
    }
}

void WebSocketPpClient::onConnectionClosed(ConnectionHandle hdl)
{
    std::ignore = hdl;
    state = State::Disconnected;
    if (!isRunning) {
        JOYNR_LOG_DEBUG(logger, "connection closed");
        if (onConnectionClosedCallback) {
            onConnectionClosedCallback();
        }
    } else {
        std::this_thread::sleep_for(reconnectSleepTimeMs);
        JOYNR_LOG_DEBUG(logger, "connection closed unexpectedly. Trying to reconnect...");
        reconnect();
    }
}

void WebSocketPpClient::onConnectionFailed(ConnectionHandle hdl)
{
    state = State::Disconnected;
    if (!isRunning) {
        JOYNR_LOG_DEBUG(logger, "connection closed");
    } else {
        Client::connection_ptr con = endpoint.get_con_from_hdl(hdl);
        std::this_thread::sleep_for(reconnectSleepTimeMs);
        JOYNR_LOG_ERROR(logger,
                        "websocket connection failed - error: {}. Trying to reconnect...",
                        con->get_ec().message());
        reconnect();
    }
}

void WebSocketPpClient::onMessageReceived(ConnectionHandle hdl, MessagePtr message)
{
    std::ignore = hdl;
    using websocketpp::frame::opcode::value;
    const value mode = message->get_opcode();
    if (mode == value::text) {
        JOYNR_LOG_TRACE(logger, "incoming text message \"{}\"", message->get_payload());
        if (onTextMessageReceivedCallback) {
            onTextMessageReceivedCallback(message->get_payload());
        }
    } else if (mode == value::binary) {
        JOYNR_LOG_TRACE(
                logger, "incoming binary message of size {}", message->get_payload().size());
        if (onBinaryMessageReceived) {
            onBinaryMessageReceived(message->get_payload());
        }
    } else {
        JOYNR_LOG_ERROR(logger, "received unsupported message type {}, dropping message", mode);
    }
}

} // namespace joynr

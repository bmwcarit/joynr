/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include <functional>
#include <memory>
#include <vector>

#include <smrf/ByteVector.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "joynr/ImmutableMessage.h"
#include "joynr/Logger.h"
#include "joynr/MutableMessage.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/WebSocketSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

#include "libjoynr/websocket/WebSocketMessagingStub.h"
#include "libjoynr/websocket/WebSocketPpClient.h"

#include "tests/JoynrTest.h"

using namespace ::testing;

class WebSocketServer : public std::enable_shared_from_this<WebSocketServer>
{
    using Server = websocketpp::server<websocketpp::config::asio>;
    using MessagePtr = Server::message_ptr;
    using ConnectionHandle = websocketpp::connection_hdl;

public:
    WebSocketServer()
            : std::enable_shared_from_this<WebSocketServer>(),
              _webSocketPpSingleThreadedIOService(
                      std::make_shared<joynr::SingleThreadedIOService>()),
              endpoint(),
              port(),
              messageReceivedCallback(),
              _clientsMutex(),
              _clients(),
              _shuttingDown(false)
    {
        endpoint.clear_access_channels(websocketpp::log::alevel::all);
        endpoint.clear_error_channels(websocketpp::log::alevel::all);
    }

    ~WebSocketServer()
    {
        std::lock_guard<std::mutex> lock1(_clientsMutex);
        {
            // std::lock_guard<std::mutex> lock1(_clientsMutex);
            _shuttingDown = true;
        }
        websocketpp::lib::error_code shutdownError;
        endpoint.stop_listening(shutdownError);
        if (shutdownError) {
            JOYNR_LOG_ERROR(logger(),
                            "error during WebSocketCcMessagingSkeleton shutdown: ",
                            shutdownError.message());
        }

        // std::lock_guard<std::mutex> lock1(_clientsMutex);
        for (const auto& elem : _clients) {
            websocketpp::lib::error_code websocketError;
            endpoint.close(elem.first, websocketpp::close::status::normal, "", websocketError);
            if (websocketError) {
                if (websocketError != websocketpp::error::bad_connection) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Unable to close websocket connection. Error: {}",
                                    websocketError.message());
                }
            }
        }
        _webSocketPpSingleThreadedIOService->stop();
    }

    std::uint16_t getPort() const
    {
        return port;
    }

    void registerMessageReceivedCallback(std::function<void(smrf::ByteVector&&)> callback)
    {
        messageReceivedCallback = std::move(callback);
    }

    void start()
    {
        try {
            using std::placeholders::_1;
            using std::placeholders::_2;

            _webSocketPpSingleThreadedIOService->start();
            boost::asio::io_service& endpointIoService =
                    _webSocketPpSingleThreadedIOService->getIOService();
            websocketpp::lib::error_code initializationError;

            // endpoint.set_message_handler(
            //         std::bind(&WebSocketServer::onMessageReceived, this, _1, _2));
            // endpoint.set_close_handler(
            //         std::bind(&WebSocketServer::onConnectionClosed, this, _1));

            endpoint.set_message_handler(
                    [thisWeakPtr = joynr::util::as_weak_ptr(this->shared_from_this())](
                            ConnectionHandle hdl, MessagePtr message) {
                        if (auto thisSharedPtr = thisWeakPtr.lock()) {
                            thisSharedPtr->onMessageReceived(hdl, message);
                        }
                    });

            endpoint.set_close_handler([thisWeakPtr = joynr::util::as_weak_ptr(
                                                this->shared_from_this())](ConnectionHandle hdl) {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    thisSharedPtr->onConnectionClosed(hdl);
                }
            });

            endpoint.init_asio(&endpointIoService, initializationError);
            // listen on random free port
            endpoint.listen(0);
            boost::system::error_code ec;
            port = endpoint.get_local_endpoint(ec).port();
            endpoint.start_accept();
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger(), "caught exception: {}", e.what());
        }
    }

private:
    void onMessageReceived(ConnectionHandle hdl, MessagePtr msg)
    {
        JOYNR_LOG_DEBUG(logger(), "received message of size {}", msg->get_payload().size());
        if (messageReceivedCallback) {
            const std::string& messageStr = msg->get_payload();
            smrf::ByteVector rawMessage(messageStr.begin(), messageStr.end());
            messageReceivedCallback(std::move(rawMessage));
        }

        std::lock_guard<std::mutex> lock(_clientsMutex);
        auto it = _clients.find(hdl);
        if (it != _clients.cend()) {
            JOYNR_LOG_DEBUG(logger(), "onMessageReceived: client found, updating");
            it->second = true;
        } else {
            _clients[hdl] = true;
            JOYNR_LOG_DEBUG(logger(), "onMessageReceived: client NOT found, adding");
        }
    }

    void onConnectionClosed(ConnectionHandle hdl)
    {
        JOYNR_LOG_DEBUG(logger(), "onConnectionClosed: invoked");
        std::lock_guard<std::mutex> lock2(_clientsMutex);
        if (_shuttingDown) {
            JOYNR_LOG_DEBUG(logger(), "onConnectionClosed: shuttingDown is set");
            return;
        }
        auto it = _clients.find(hdl);
        if (it != _clients.cend()) {
            JOYNR_LOG_DEBUG(logger(), "onConnectionClosed: client found, is getting erased");
            _clients.erase(it);
        } else {
            JOYNR_LOG_DEBUG(logger(), "onConnectionClosed: client NOT found");
        }
    }

    std::shared_ptr<joynr::SingleThreadedIOService> _webSocketPpSingleThreadedIOService;
    Server endpoint;
    std::uint32_t port;
    std::function<void(smrf::ByteVector&&)> messageReceivedCallback;
    std::mutex _clientsMutex;
    std::map<ConnectionHandle, bool, std::owner_less<ConnectionHandle>> _clients;
    std::atomic<bool> _shuttingDown;
    ADD_LOGGER(WebSocketServer)
};

class WebSocketMessagingStubTest : public testing::TestWithParam<std::size_t>
{

public:
    WebSocketMessagingStubTest()
            : settings(),
              wsSettings(settings),
              server(std::make_shared<WebSocketServer>()),
              serverAddress(),
              singleThreadedIOService(std::make_shared<joynr::SingleThreadedIOService>()),
              webSocket(nullptr)
    {
        singleThreadedIOService->start();
        server->start();
    }

    ~WebSocketMessagingStubTest()
    {
        webSocket->stop();
        singleThreadedIOService->stop();
    }

    MOCK_METHOD1(onWebSocketConnectionClosed, void(const joynr::system::RoutingTypes::Address&));

    virtual void SetUp()
    {
        serverAddress = joynr::system::RoutingTypes::WebSocketAddress(
                joynr::system::RoutingTypes::WebSocketProtocol::WS,
                "localhost",
                server->getPort(),
                "");
        JOYNR_LOG_DEBUG(logger(), "server URL: {}", serverAddress.toString());
        auto connected = std::make_shared<joynr::Semaphore>(0);
        webSocket = std::make_shared<joynr::WebSocketPpClient<websocketpp::config::asio_client>>(
                wsSettings, singleThreadedIOService->getIOService());
        webSocket->registerConnectCallback([connected]() { connected->notify(); });
        webSocket->connect(serverAddress);

        // wait until connected
        connected->wait();
        JOYNR_LOG_DEBUG(logger(), "WebSocket is connected: {}", webSocket->isConnected());
    }

protected:
    ADD_LOGGER(WebSocketMessagingStubTest)
    joynr::Settings settings;
    joynr::WebSocketSettings wsSettings;
    std::shared_ptr<WebSocketServer> server;
    joynr::system::RoutingTypes::WebSocketAddress serverAddress;
    std::shared_ptr<joynr::SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<joynr::IWebSocketPpClient> webSocket;
};

TEST_P(WebSocketMessagingStubTest, transmitMessageWithVaryingSize)
{
    JOYNR_LOG_TRACE(logger(), "transmit message");

    auto semaphore = std::make_shared<joynr::Semaphore>(0);
    smrf::ByteVector receivedMessage;

    auto callback = [semaphore, &receivedMessage](smrf::ByteVector&& msg) {
        receivedMessage = std::move(msg);
        semaphore->notify();
    };
    server->registerMessageReceivedCallback(callback);

    // send message using messaging stub
    joynr::WebSocketMessagingStub messagingStub(webSocket->getSender());
    joynr::MutableMessage mutableMessage;

    const std::size_t payloadSize = GetParam();
    std::string payload(payloadSize, 'x');
    mutableMessage.setPayload(payload);
    std::shared_ptr<joynr::ImmutableMessage> immutableMessage =
            mutableMessage.getImmutableMessage();
    smrf::ByteVector expectedMessage = immutableMessage->getSerializedMessage();

    auto onFailure = [](const joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "Unexpected call of onFailure function, exception: " + e.getMessage();
    };
    messagingStub.transmit(immutableMessage, onFailure);

    // wait until message is received
    semaphore->wait();
    ASSERT_EQ(0, semaphore->getStatus());

    // verify received message
    ASSERT_EQ(expectedMessage.size(), receivedMessage.size());
    EXPECT_EQ(expectedMessage, receivedMessage);
}

INSTANTIATE_TEST_SUITE_P(WebsocketTransmitMessagesWithIncreasingSize,
                         WebSocketMessagingStubTest,
                         ::testing::Values<std::size_t>(256 * 1024, 512 * 1024, 1024 * 1024));

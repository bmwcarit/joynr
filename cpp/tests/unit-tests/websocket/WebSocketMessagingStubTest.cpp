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
#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include <smrf/ByteVector.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MutableMessage.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Settings.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/WebSocketSettings.h"

#include "libjoynr/websocket/WebSocketMessagingStub.h"
#include "libjoynr/websocket/WebSocketPpClient.h"

#include "tests/JoynrTest.h"

using namespace ::testing;

class WebSocketServer
{
    using Server = websocketpp::server<websocketpp::config::asio>;
    using MessagePtr = Server::message_ptr;
    using ConnectionHandle = websocketpp::connection_hdl;

public:
    WebSocketServer() : port(), thread(), messageReceivedCallback()
    {
        endpoint.clear_access_channels(websocketpp::log::alevel::all);
        endpoint.clear_error_channels(websocketpp::log::alevel::all);
    }

    ~WebSocketServer()
    {
        endpoint.stop_listening();
        thread.join();
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
            endpoint.set_message_handler(
                    std::bind(&WebSocketServer::onMessageReceived, this, _1, _2));
            endpoint.init_asio();
            // listen on random free port
            endpoint.listen(0);
            boost::system::error_code ec;
            port = endpoint.get_local_endpoint(ec).port();
            endpoint.start_accept();
            thread = std::thread(&Server::run, &endpoint);
        } catch (const std::exception& e) {
            JOYNR_LOG_ERROR(logger(), "caught exception: {}", e.what());
        }
    }

private:
    void onMessageReceived(ConnectionHandle hdl, MessagePtr msg)
    {
        std::ignore = hdl;
        JOYNR_LOG_DEBUG(logger(), "received message of size {}", msg->get_payload().size());
        if (messageReceivedCallback) {
            const std::string& messageStr = msg->get_payload();
            smrf::ByteVector rawMessage(messageStr.begin(), messageStr.end());
            messageReceivedCallback(std::move(rawMessage));
        }
    }
    Server endpoint;
    std::uint32_t port;
    std::thread thread;
    std::function<void(smrf::ByteVector&&)> messageReceivedCallback;
    ADD_LOGGER(WebSocketServer)
};

class WebSocketMessagingStubTest : public testing::TestWithParam<std::size_t>
{

public:
    WebSocketMessagingStubTest()
            : settings(),
              wsSettings(settings),
              server(),
              serverAddress(),
              singleThreadedIOService(std::make_shared<joynr::SingleThreadedIOService>()),
              webSocket(nullptr)
    {
        singleThreadedIOService->start();
        server.start();
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
                server.getPort(),
                "");
        JOYNR_LOG_DEBUG(logger(), "server URL: {}", serverAddress.toString());
        joynr::Semaphore connected(0);
        webSocket = std::make_shared<joynr::WebSocketPpClient<websocketpp::config::asio_client>>(
                wsSettings, singleThreadedIOService->getIOService());
        webSocket->registerConnectCallback([&connected]() { connected.notify(); });
        webSocket->connect(serverAddress);

        // wait until connected
        connected.wait();
        JOYNR_LOG_DEBUG(logger(), "WebSocket is connected: {}", webSocket->isConnected());
    }

protected:
    ADD_LOGGER(WebSocketMessagingStubTest)
    joynr::Settings settings;
    joynr::WebSocketSettings wsSettings;
    WebSocketServer server;
    joynr::system::RoutingTypes::WebSocketAddress serverAddress;
    std::shared_ptr<joynr::SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<joynr::IWebSocketPpClient> webSocket;
};

TEST_P(WebSocketMessagingStubTest, transmitMessageWithVaryingSize)
{
    JOYNR_LOG_TRACE(logger(), "transmit message");

    joynr::Semaphore sem(0);
    smrf::ByteVector receivedMessage;

    auto callback = [&sem, &receivedMessage](smrf::ByteVector&& msg) {
        receivedMessage = std::move(msg);
        sem.notify();
    };
    server.registerMessageReceivedCallback(callback);

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
    sem.wait();
    ASSERT_EQ(0, sem.getStatus());

    // verify received message
    ASSERT_EQ(expectedMessage.size(), receivedMessage.size());
    EXPECT_EQ(expectedMessage, receivedMessage);
}

INSTANTIATE_TEST_SUITE_P(WebsocketTransmitMessagesWithIncreasingSize,
                        WebSocketMessagingStubTest,
                        ::testing::Values<std::size_t>(256 * 1024, 512 * 1024, 1024 * 1024));

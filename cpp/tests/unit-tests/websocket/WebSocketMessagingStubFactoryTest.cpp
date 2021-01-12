/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include <memory>

#include <gtest/gtest.h>

#include "joynr/IMessagingStub.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"

#include "tests/mock/MockWebSocketSendInterface.h"
#include "tests/mock/MockWebSocketClient.h"

using namespace ::testing;

namespace joynr
{

class WebSocketMessagingStubFactoryTest : public testing::Test
{
public:
    WebSocketMessagingStubFactoryTest()
            : udsAddress("dummyPath"),
              webSocketServerAddress(joynr::system::RoutingTypes::WebSocketProtocol::WS,
                                     "localhost",
                                     42,
                                     "path"),
              webSocketClientAddress("clientId")
    {
    }

protected:
    ADD_LOGGER(WebSocketMessagingStubFactoryTest)
    joynr::system::RoutingTypes::UdsAddress udsAddress;
    joynr::system::RoutingTypes::WebSocketAddress webSocketServerAddress;
    joynr::system::RoutingTypes::WebSocketClientAddress webSocketClientAddress;
};

TEST_F(WebSocketMessagingStubFactoryTest, canCreateWebSocketAddressses)
{
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE(factory.canCreate(webSocketClientAddress));
    EXPECT_TRUE(factory.canCreate(webSocketServerAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, canOnlyCreateWebSocketAddressses)
{
    WebSocketMessagingStubFactory factory;

    EXPECT_FALSE(factory.canCreate(udsAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsNullForUnknownClient)
{
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == 0);
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsMessagingStub)
{
    WebSocketMessagingStubFactory factory;
    auto clientWebsocket = std::make_shared<MockWebSocketClient>();
    auto wrapper = std::make_shared<MockWebSocketSendInterface>();

    factory.addClient(joynr::system::RoutingTypes::WebSocketClientAddress(webSocketClientAddress),
                      clientWebsocket->getSender());
    factory.addServer(webSocketServerAddress, wrapper);
    EXPECT_TRUE(factory.create(webSocketClientAddress).get() != nullptr);
    EXPECT_TRUE(factory.create(webSocketServerAddress).get() != nullptr);
}

TEST_F(WebSocketMessagingStubFactoryTest,
       closedMessagingStubsAreRemovedFromWebSocketMessagingStubFactory)
{
    WebSocketMessagingStubFactory factory;
    auto addressCopy = joynr::system::RoutingTypes::WebSocketClientAddress(webSocketClientAddress);
    auto websocket = std::make_shared<MockWebSocketClient>();
    websocket->registerDisconnectCallback(
            [&factory, &addressCopy]() { factory.onMessagingStubClosed(addressCopy); });

    factory.addClient(webSocketClientAddress, websocket->getSender());

    EXPECT_TRUE(factory.canCreate(webSocketClientAddress));
    std::shared_ptr<IMessagingStub> messagingStub(factory.create(webSocketClientAddress));
    EXPECT_TRUE(messagingStub.get() != nullptr);

    EXPECT_CALL(*websocket, dtorCalled());
    std::thread(&MockWebSocketClient::signalDisconnect, websocket).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == nullptr);
}

TEST_F(WebSocketMessagingStubFactoryTest, closedMessagingStubsAreRemovedFromMessagingStubFactory)
{
    auto factory = std::make_shared<WebSocketMessagingStubFactory>();
    auto address =
            std::make_shared<system::RoutingTypes::WebSocketClientAddress>(webSocketClientAddress);
    auto addressCopy =
            std::make_shared<system::RoutingTypes::WebSocketClientAddress>(webSocketClientAddress);
    auto websocket = std::make_shared<MockWebSocketClient>();
    websocket->registerDisconnectCallback(
            [factory, addressCopy]() { factory->onMessagingStubClosed(*addressCopy); });

    factory->addClient(
            joynr::system::RoutingTypes::WebSocketClientAddress(*address), websocket->getSender());

    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
    factory->registerOnMessagingStubClosedCallback([messagingStubFactory](
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destinationAddress) {
        messagingStubFactory->remove(destinationAddress);
    });
    messagingStubFactory->registerStubFactory(std::move(factory));

    EXPECT_NE(nullptr, messagingStubFactory->create(address).get());
    EXPECT_TRUE(messagingStubFactory->contains(address));
    EXPECT_TRUE(messagingStubFactory->contains(addressCopy));

    EXPECT_CALL(*websocket, dtorCalled());
    std::thread(&MockWebSocketClient::signalDisconnect, websocket).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(messagingStubFactory->contains(address));
    EXPECT_FALSE(messagingStubFactory->contains(addressCopy));
    EXPECT_TRUE(messagingStubFactory->create(address).get() == nullptr);
    messagingStubFactory->shutdown();
}

} // namespace joynr

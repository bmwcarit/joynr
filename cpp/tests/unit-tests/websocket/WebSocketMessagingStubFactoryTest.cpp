/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#include <gtest/gtest.h>

#include "PrettyPrint.h"

#include <QWebSocket>

#include "joynr/system/WebSocketAddress.h"
#include "joynr/system/ChannelAddress.h"
#include "joynr/system/CommonApiDbusAddress.h"
#include "joynr/system/BrowserAddress.h"

#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"

using namespace joynr;

class WebSocketMessagingStubFactoryTest : public testing::Test {
public:
    WebSocketMessagingStubFactoryTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "WebSocketMessagingStubFactoryTest")),
        webSocketAddress(joynr::system::WebSocketProtocol::WS, "localhost", 42, "path"),
        channelAddress("channelId"),
        commonApiDbusAddress("domain", "serviceName", "participantId"),
        browserAddress("windowId")
    {
    }

    virtual void TearDown() {
    }

protected:
    joynr_logging::Logger* logger;
    joynr::system::WebSocketAddress webSocketAddress;
    joynr::system::ChannelAddress channelAddress;
    joynr::system::CommonApiDbusAddress commonApiDbusAddress;
    joynr::system::BrowserAddress browserAddress;
};

TEST_F(WebSocketMessagingStubFactoryTest, canCreateWebSocketAddressses) {
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE(factory.canCreate(webSocketAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, canOnlyCreateWebSocketAddressses) {
    WebSocketMessagingStubFactory factory;

    EXPECT_FALSE(factory.canCreate(channelAddress));
    EXPECT_FALSE(factory.canCreate(commonApiDbusAddress));
    EXPECT_FALSE(factory.canCreate(browserAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsNullForUnknownClient) {
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE(factory.create(webSocketAddress).isNull());
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsMessagingStub) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* websocket = new QWebSocket();

    factory.addClient(webSocketAddress, websocket);
    EXPECT_FALSE(factory.create(webSocketAddress).isNull());
}

TEST_F(WebSocketMessagingStubFactoryTest, removeClientRemovesMessagingStub) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* websocket = new QWebSocket();

    factory.addClient(webSocketAddress, websocket);
    EXPECT_FALSE(factory.create(webSocketAddress).isNull());
    factory.removeClient(webSocketAddress);
    EXPECT_TRUE(factory.create(webSocketAddress).isNull());
}

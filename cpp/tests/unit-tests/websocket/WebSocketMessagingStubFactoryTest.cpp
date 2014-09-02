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

#include <QtTest/QSignalSpy>
#include <QtCore/QTimer>
#include <QtWebSockets/QWebSocket>

#include "joynr/IMessaging.h"
#include "joynr/system/WebSocketAddress.h"
#include "joynr/system/WebSocketClientAddress.h"
#include "joynr/system/ChannelAddress.h"
#include "joynr/system/CommonApiDbusAddress.h"
#include "joynr/system/BrowserAddress.h"

#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"

namespace joynr {

class WebSocketMessagingStubFactoryTest : public testing::Test {
public:
    WebSocketMessagingStubFactoryTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "WebSocketMessagingStubFactoryTest")),
        webSocketServerAddress(joynr::system::WebSocketProtocol::WS, "localhost", 42, "path"),
        webSocketClientAddress("clientId"),
        channelAddress("channelId"),
        commonApiDbusAddress("domain", "serviceName", "participantId"),
        browserAddress("windowId")
    {
    }

    virtual void TearDown() {
    }

protected:
    joynr_logging::Logger* logger;
    joynr::system::WebSocketAddress webSocketServerAddress;
    joynr::system::WebSocketClientAddress webSocketClientAddress;
    joynr::system::ChannelAddress channelAddress;
    joynr::system::CommonApiDbusAddress commonApiDbusAddress;
    joynr::system::BrowserAddress browserAddress;
};

TEST_F(WebSocketMessagingStubFactoryTest, canCreateWebSocketAddressses) {
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE(factory.canCreate(webSocketClientAddress));
    EXPECT_TRUE(factory.canCreate(webSocketServerAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, canOnlyCreateWebSocketAddressses) {
    WebSocketMessagingStubFactory factory;

    EXPECT_FALSE(factory.canCreate(channelAddress));
    EXPECT_FALSE(factory.canCreate(commonApiDbusAddress));
    EXPECT_FALSE(factory.canCreate(browserAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsNullForUnknownClient) {
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE(factory.create(webSocketClientAddress).isNull());
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsMessagingStub) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* websocket = new QWebSocket();

    factory.addClient(webSocketClientAddress, websocket);
    EXPECT_FALSE(factory.create(webSocketClientAddress).isNull());
    EXPECT_FALSE(factory.create(webSocketServerAddress).isNull());
}

TEST_F(WebSocketMessagingStubFactoryTest, closedMessagingStubsAreRemoved) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* websocket = new QWebSocket();

    factory.addClient(webSocketClientAddress, websocket);
    QSharedPointer<joynr::IMessaging> messagingStub(factory.create(webSocketClientAddress));
    QSharedPointer<joynr::WebSocketMessagingStub> wsMessagingStub(messagingStub.dynamicCast<joynr::WebSocketMessagingStub>());
    QSignalSpy wsMessagingStubClosedSpy(wsMessagingStub.data(), SIGNAL(closed(joynr::system::Address)));
    EXPECT_FALSE(messagingStub.isNull());

    QTimer::singleShot(0, wsMessagingStub.data(), SLOT(onSocketDisconnected()));
    EXPECT_TRUE(wsMessagingStubClosedSpy.wait());
    EXPECT_EQ(1, wsMessagingStubClosedSpy.count());
    EXPECT_TRUE(factory.create(webSocketClientAddress).isNull());
}

TEST_F(WebSocketMessagingStubFactoryTest, removeClientRemovesMessagingStub) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* websocket = new QWebSocket();

    factory.addClient(webSocketClientAddress, websocket);
    EXPECT_FALSE(factory.create(webSocketClientAddress).isNull());
    factory.removeClient(webSocketClientAddress);
    EXPECT_TRUE(factory.create(webSocketClientAddress).isNull());
}

TEST_F(WebSocketMessagingStubFactoryTest, convertWebSocketAddressToUrl) {
    joynr::system::WebSocketAddress wsAddress(
                joynr::system::WebSocketProtocol::WS,
                "localhost",
                42,
                "some/path/"
    );
    QUrl expectedWsUrl(QString("ws://localhost:42/some/path/"));

    QUrl wsUrl(WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(wsAddress));
    EXPECT_EQ(expectedWsUrl, wsUrl);

    joynr::system::WebSocketAddress wssAddress(
                joynr::system::WebSocketProtocol::WSS,
                "localhost",
                42,
                "some/path"
    );
    QUrl expectedWssUrl(QString("wss://localhost:42/some/path"));

    QUrl wssUrl(WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(wssAddress));
    EXPECT_EQ(expectedWssUrl, wssUrl);
}

} // namespace joynr

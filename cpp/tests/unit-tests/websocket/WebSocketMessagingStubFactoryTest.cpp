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
#include "joynr/system/RoutingTypes/QtWebSocketAddress.h"
#include "joynr/system/RoutingTypes/QtWebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/QtChannelAddress.h"
#include "joynr/system/RoutingTypes/QtCommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/QtBrowserAddress.h"

#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"

namespace joynr {

class WebSocketMessagingStubFactoryTest : public testing::Test {
public:
    WebSocketMessagingStubFactoryTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "WebSocketMessagingStubFactoryTest")),
        webSocketServerAddress(joynr::system::RoutingTypes::QtWebSocketProtocol::WS, "localhost", 42, "path"),
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
    joynr::system::RoutingTypes::QtWebSocketAddress webSocketServerAddress;
    joynr::system::RoutingTypes::QtWebSocketClientAddress webSocketClientAddress;
    joynr::system::RoutingTypes::QtChannelAddress channelAddress;
    joynr::system::RoutingTypes::QtCommonApiDbusAddress commonApiDbusAddress;
    joynr::system::RoutingTypes::QtBrowserAddress browserAddress;
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

    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == 0);
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsMessagingStub) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* clientWebsocket = new QWebSocket();
    QWebSocket* serverWebsocket = new QWebSocket();

    factory.addClient(webSocketClientAddress, clientWebsocket);
    factory.addServer(webSocketServerAddress, serverWebsocket);
    EXPECT_FALSE(factory.create(webSocketClientAddress).get() == 0);
    EXPECT_FALSE(factory.create(webSocketServerAddress).get() == 0);
}

TEST_F(WebSocketMessagingStubFactoryTest, closedMessagingStubsAreRemoved) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* websocket = new QWebSocket();

    factory.addClient(webSocketClientAddress, websocket);
    std::shared_ptr<joynr::IMessaging> messagingStub(factory.create(webSocketClientAddress));
    std::shared_ptr<joynr::WebSocketMessagingStub> wsMessagingStub(std::dynamic_pointer_cast<joynr::WebSocketMessagingStub>(messagingStub));
    QSignalSpy wsMessagingStubClosedSpy(wsMessagingStub.get(), SIGNAL(closed(joynr::system::RoutingTypes::QtAddress)));
    EXPECT_FALSE(messagingStub.get() == 0);

    QTimer::singleShot(0, wsMessagingStub.get(), SLOT(onSocketDisconnected()));
    EXPECT_TRUE(wsMessagingStubClosedSpy.wait());
    EXPECT_EQ(1, wsMessagingStubClosedSpy.count());
    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == 0);
}

TEST_F(WebSocketMessagingStubFactoryTest, removeClientRemovesMessagingStub) {
    WebSocketMessagingStubFactory factory;
    QWebSocket* websocket = new QWebSocket();

    factory.addClient(webSocketClientAddress, websocket);
    EXPECT_FALSE(factory.create(webSocketClientAddress).get() == 0);
    factory.removeClient(webSocketClientAddress);
    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == 0);
}

TEST_F(WebSocketMessagingStubFactoryTest, convertWebSocketAddressToUrl) {
    joynr::system::RoutingTypes::QtWebSocketAddress wsAddress(
                joynr::system::RoutingTypes::QtWebSocketProtocol::WS,
                "localhost",
                42,
                "/some/path/"
    );
    QUrl expectedWsUrl(QString("ws://localhost:42/some/path/"));

    QUrl wsUrl(WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(wsAddress));
    EXPECT_EQ(expectedWsUrl, wsUrl);

    joynr::system::RoutingTypes::QtWebSocketAddress wssAddress(
                joynr::system::RoutingTypes::QtWebSocketProtocol::WSS,
                "localhost",
                42,
                "/some/path"
    );
    QUrl expectedWssUrl(QString("wss://localhost:42/some/path"));

    QUrl wssUrl(WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(wssAddress));
    EXPECT_EQ(expectedWssUrl, wssUrl);
}

} // namespace joynr

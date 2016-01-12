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

#include "utils/MockObjects.h"

#include <QtTest/QtTest>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtWebSockets/qwebsocketserver.h>
#include <QtWebSockets/qwebsocket.h>

#include <functional>
#include <thread>
#include <vector>

#include "joynr/Logger.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/JsonSerializer.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "libjoynr/websocket/WebSocketClient.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"

#include "runtimes/cluster-controller-runtime/websocket/QWebSocketSendWrapper.h"

using namespace ::testing;

class WebSocketMessagingStubTest : public QObject, public testing::Test
{
    Q_OBJECT
public:
    explicit WebSocketMessagingStubTest(QObject* parent = nullptr) :
        QObject(parent),
        server(
            QStringLiteral("WebSocketMessagingStubTest Server"),
            QWebSocketServer::NonSecureMode,
            this
        ),
        serverAddress(nullptr),
        webSocket(nullptr)
    {
        if(server.listen(QHostAddress::Any)) {
            JOYNR_LOG_DEBUG(logger) << "server listening on " << server.serverAddress().toString().toStdString() << ":" << server.serverPort();
            connect(
                    &server, &QWebSocketServer::newConnection,
                    this, &WebSocketMessagingStubTest::onNewConnection
            );
        } else {
            JOYNR_LOG_ERROR(logger) << "unable to start WebSocket server";
        }
    }

    ~WebSocketMessagingStubTest() {
        QSignalSpy serverClosedSpy(&server, SIGNAL(closed()));
        server.close();
        serverClosedSpy.wait();
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    MOCK_METHOD1(onWebSocketError, void (const std::string&));
    MOCK_METHOD1(onWebSocketConnected, void (joynr::WebSocket*));
    MOCK_METHOD1(onWebSocketConnectionClosed, void (const joynr::system::RoutingTypes::Address&));
#pragma GCC diagnostic pop

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    virtual void SetUp() {
        // create and open client web socket
        // ownership of web socket is passed over to messaging stub
        webSocket = new joynr::WebSocketClient(
            std::bind(&WebSocketMessagingStubTest::onWebSocketError, this, std::placeholders::_1),
            std::bind(&WebSocketMessagingStubTest::onWebSocketConnected, this, std::placeholders::_1));
        // ownership of server address is passed over to messaging stub
        serverAddress = new joynr::system::RoutingTypes::WebSocketAddress(
                    joynr::system::RoutingTypes::WebSocketProtocol::WS,
                    "localhost",
                    server.serverPort(),
                    ""
        );
        JOYNR_LOG_DEBUG(logger) << "server URL: " << serverAddress->toString();
        webSocket->connect(*serverAddress);

        // waiting until the web socket is connected
        JOYNR_LOG_DEBUG(logger) << "WebSocket is connected: " << webSocket->isConnected();
    }
    virtual void TearDown() {}

Q_SIGNALS:
    void textMessageReceived(const QString& message);

public Q_SLOTS:
    void onNewConnection() {
        JOYNR_LOG_TRACE(logger) << "on new connection";
        QWebSocket* client = server.nextPendingConnection();

        connect(
                client, &QWebSocket::textMessageReceived,
                this, &WebSocketMessagingStubTest::textMessageReceived
        );
        connect(
                client, &QWebSocket::disconnected,
                this, &WebSocketMessagingStubTest::onDisconnected
        );
    }

    void onDisconnected() {
        JOYNR_LOG_TRACE(logger) << "on disconnected";
        QWebSocket* client = qobject_cast<QWebSocket*>(sender());
        if(client) {
            client->deleteLater();
        }
    }

protected:
    ADD_LOGGER(WebSocketMessagingStubTest);
    QWebSocketServer server;
    joynr::system::RoutingTypes::WebSocketAddress* serverAddress;
    joynr::WebSocketClient* webSocket;
};

INIT_LOGGER(WebSocketMessagingStubTest);

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

TEST_F(WebSocketMessagingStubTest, emitsClosedSignal) {
    JOYNR_LOG_TRACE(logger) << "emits closed signal";

    // create messaging stub
    joynr::WebSocketMessagingStub messagingStub(
        webSocket,
        [this](){ this->onWebSocketConnectionClosed(*(this->serverAddress)); });

    // close websocket
    joynr::Semaphore sem(0);
    ON_CALL(*this, onWebSocketConnectionClosed(Ref(*serverAddress))).WillByDefault(ReleaseSemaphore(&sem));
    EXPECT_CALL(*this, onWebSocketConnectionClosed(Ref(*serverAddress))).Times(1);
    webSocket->terminate();

    sem.wait();
}

TEST_F(WebSocketMessagingStubTest, transmitMessage) {
    JOYNR_LOG_TRACE(logger) << "transmit message";
    QSignalSpy textMessageReceivedSignalSpy(this, SIGNAL(textMessageReceived(QString)));

    // send message using messaging stub
    joynr::WebSocketMessagingStub messagingStub(
        webSocket,
        [](){});
    joynr::JoynrMessage joynrMsg;
    std::string expectedMessage = joynr::JsonSerializer::serialize(joynrMsg);
    messagingStub.transmit(joynrMsg);

    // wait until message is received
    EXPECT_TRUE(textMessageReceivedSignalSpy.wait());
    ASSERT_EQ(1, textMessageReceivedSignalSpy.count());

    // verify received message
    std::vector<QVariant> args = textMessageReceivedSignalSpy.takeFirst().toVector().toStdVector();
    ASSERT_EQ(1, args.size());
    EXPECT_EQ(QVariant::String, args.begin()->type());
    EXPECT_EQ(expectedMessage, args.begin()->toString().toStdString());
}

#include "WebSocketMessagingStubTest.moc"

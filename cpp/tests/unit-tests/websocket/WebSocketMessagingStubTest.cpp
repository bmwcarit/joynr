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

#include <QtTest/QtTest>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include "joynr/JoynrMessage.h"
#include "joynr/JsonSerializer.h"
#include "joynr/system/RoutingTypes_QtWebSocketAddress.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include <vector>

class WebSocketMessagingStubTest : public QObject, public testing::Test
{
    Q_OBJECT
public:
    explicit WebSocketMessagingStubTest(QObject* parent = nullptr) :
        QObject(parent),
        logger(joynr::joynr_logging::Logging::getInstance()->getLogger("TEST", "WebSocketMessagingStubTest")),
        server(
            QStringLiteral("WebSocketMessagingStubTest Server"),
            QWebSocketServer::NonSecureMode,
            this
        ),
        serverAddress(nullptr),
        webSocket(nullptr)
    {
        if(server.listen(QHostAddress::Any)) {
            LOG_DEBUG(
                        logger,
                        joynr::FormatString("server listening on %1:%2")
                            .arg(server.serverAddress().toString().toStdString())
                            .arg(server.serverPort()).str()
            );
            connect(
                    &server, &QWebSocketServer::newConnection,
                    this, &WebSocketMessagingStubTest::onNewConnection
            );
        } else {
            LOG_ERROR(logger, "unable to start WebSocket server");
        }
    }

    ~WebSocketMessagingStubTest() {
        QSignalSpy serverClosedSpy(&server, SIGNAL(closed()));
        server.close();
        serverClosedSpy.wait();
    }

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    virtual void SetUp() {
        // create and open client web socket
        // ownership of web socket is passed over to messaging stub
        webSocket = new QWebSocket();
        QSignalSpy webSocketConnectedSignalSpy(webSocket, SIGNAL(connected()));
        // ownership of server address is passed over to messaging stub
        serverAddress = new joynr::system::RoutingTypes::QtWebSocketAddress(
                    joynr::system::RoutingTypes::QtWebSocketProtocol::WS,
                    QStringLiteral("localhost"),
                    server.serverPort(),
                    QString()
        );
        QUrl url(joynr::WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(*serverAddress));
        LOG_DEBUG(logger, joynr::FormatString("server URL: %1").arg(url.toString().toStdString()).str());
        webSocket->open(url);

        // waiting until the web socket is connected
        QString webSocketState;
        QDebug(&webSocketState) << "WebSocket state: " << webSocket->state();
        LOG_DEBUG(logger, webSocketState.toStdString());
        webSocketConnectedSignalSpy.wait();
        webSocketState.clear();
        QDebug(&webSocketState) << "WebSocket state: " << webSocket->state();
        LOG_DEBUG(logger, webSocketState.toStdString());
    }
    virtual void TearDown() {}

Q_SIGNALS:
    void textMessageReceived(const QString& message);

public Q_SLOTS:
    void onNewConnection() {
        LOG_TRACE(logger, "on new connection");
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
        LOG_TRACE(logger, "on disconnected");
        QWebSocket* client = qobject_cast<QWebSocket*>(sender());
        if(client) {
            client->deleteLater();
        }
    }

protected:
    joynr::joynr_logging::Logger* logger;
    QWebSocketServer server;
    joynr::system::RoutingTypes::QtWebSocketAddress* serverAddress;
    QWebSocket* webSocket;
};

TEST_F(WebSocketMessagingStubTest, emitsClosedSignal) {
    LOG_TRACE(logger, "emits closed signal");

    // create messaging stub
    joynr::WebSocketMessagingStub messagingStub(serverAddress, webSocket);
    QSignalSpy messagingStubClosedSpy(&messagingStub, SIGNAL(closed(joynr::system::RoutingTypes::QtAddress)));

    // close websocket
    QTimer::singleShot(0, webSocket, SLOT(close()));

    // wait for closed signal
    EXPECT_TRUE(messagingStubClosedSpy.wait());
    ASSERT_EQ(1, messagingStubClosedSpy.count());

    // verify signal's address parameter
    std::vector<QVariant> args = messagingStubClosedSpy.takeFirst().toVector().toStdVector();
    ASSERT_EQ(1, args.size());
    EXPECT_EQ(QVariant::UserType, args.begin()->type());
    EXPECT_TRUE(args.begin()->canConvert<joynr::system::RoutingTypes::QtAddress>());
}

TEST_F(WebSocketMessagingStubTest, transmitMessage) {
    LOG_TRACE(logger, "transmit message");
    QSignalSpy textMessageReceivedSignalSpy(this, SIGNAL(textMessageReceived(QString)));

    // send message using messaging stub
    joynr::WebSocketMessagingStub messagingStub(serverAddress, webSocket);
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

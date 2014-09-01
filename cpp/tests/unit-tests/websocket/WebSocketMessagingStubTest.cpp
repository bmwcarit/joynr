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

#include <QtTest/QtTest>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include "joynr/joynrlogging.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/JsonSerializer.h"
#include "joynr/system/WebSocketAddress.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"

class WebSocketMessagingStubTest : public QObject, public testing::Test
{
    Q_OBJECT
public:
    WebSocketMessagingStubTest(QObject* parent = Q_NULLPTR) :
        QObject(parent),
        logger(joynr::joynr_logging::Logging::getInstance()->getLogger("TEST", "WebSocketMessagingStubTest")),
        server(
            QStringLiteral("WebSocketMessagingStubTest Server"),
            QWebSocketServer::NonSecureMode,
            this
        ),
        clients(),
        serverAddress(Q_NULLPTR),
        webSocket(Q_NULLPTR)
    {
        qRegisterMetaType<joynr::JoynrMessage>("joynr::JoynrMessage");
        if(server.listen(QHostAddress::Any)) {
            LOG_DEBUG(
                        logger,
                        QString("server listening on %0:%1")
                            .arg(server.serverAddress().toString())
                            .arg(server.serverPort())
            );
            connect(
                    &server, &QWebSocketServer::newConnection,
                    this, &WebSocketMessagingStubTest::onNewConnection
            );
        } else {
            LOG_ERROR(logger, QStringLiteral("unable to start WebSocket server"));
        }
    }

    ~WebSocketMessagingStubTest() {
        server.close();
        qDeleteAll(clients.begin(), clients.end());
    }

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    virtual void SetUp() {
        // create and open client web socket
        // ownership of web socket is passed over to messaging stub
        webSocket = new QWebSocket();
        QSignalSpy webSocketConnectedSignalSpy(webSocket, SIGNAL(connected()));
        // ownership of server address is passed over to messaging stub
        serverAddress = new joynr::system::WebSocketAddress(
                    joynr::system::WebSocketProtocol::WS,
                    QStringLiteral("localhost"),
                    server.serverPort(),
                    QString()
        );
        QUrl url(QString("%0://%1:%2/%3")
                    .arg((serverAddress->getProtocol() == joynr::system::WebSocketProtocol::WS) ? QStringLiteral("ws") : QStringLiteral("wss"))
                    .arg(serverAddress->getHost())
                    .arg(serverAddress->getPort())
                    .arg(serverAddress->getPath())
        );
        LOG_DEBUG(logger, QString("server URL: %0").arg(url.toString()));
        webSocket->open(url);

        // waiting until the web socket is connected
        QString webSocketState;
        QDebug(&webSocketState) << "WebSocket state: " << webSocket->state();
        LOG_DEBUG(logger, webSocketState);
        webSocketConnectedSignalSpy.wait();
        webSocketState.clear();
        QDebug(&webSocketState) << "WebSocket state: " << webSocket->state();
        LOG_DEBUG(logger, webSocketState);
    }
    virtual void TearDown() {}

Q_SIGNALS:
    void binaryMessageReceived(const QByteArray& message);

public Q_SLOTS:
    void onNewConnection() {
        LOG_TRACE(logger, QStringLiteral("on new connection"));
        QWebSocket* client = server.nextPendingConnection();

        connect(
                client, &QWebSocket::binaryMessageReceived,
                this, &WebSocketMessagingStubTest::binaryMessageReceived
        );
        connect(
                client, &QWebSocket::disconnected,
                this, &WebSocketMessagingStubTest::onDisconnected
        );
    }

    void onDisconnected() {
        LOG_TRACE(logger, QStringLiteral("on disconnected"));
        QWebSocket* client = qobject_cast<QWebSocket*>(sender());
        if(client) {
            clients.removeAll(client);
            client->deleteLater();
        }
    }

    void onMessagingStubClosed(const joynr::system::Address& address) {
        LOG_TRACE(logger, QString("on messaging stub closed: %0").arg(address.toString()));
    }

protected:
    joynr::joynr_logging::Logger* logger;
    QWebSocketServer server;
    QList<QWebSocket*> clients;
    joynr::system::WebSocketAddress* serverAddress;
    QWebSocket* webSocket;
};

TEST_F(WebSocketMessagingStubTest, emitsClosedSignal) {
    LOG_TRACE(logger, QStringLiteral("emits closed signal"));

    // create messaging stub
    joynr::WebSocketMessagingStub messagingStub(serverAddress, webSocket);
    QSignalSpy messagingStubClosedSpy(&messagingStub, SIGNAL(closed(joynr::system::Address)));

    // close websocket
    QTimer::singleShot(0, webSocket, SLOT(close()));

    // wait for closed signal
    EXPECT_TRUE(messagingStubClosedSpy.wait());
    EXPECT_EQ(1, messagingStubClosedSpy.count());

    // verify signal's address parameter
    QList<QVariant> args = messagingStubClosedSpy.takeFirst();
    EXPECT_EQ(1, args.size());
    EXPECT_EQ(QVariant::UserType, args.first().type());
    EXPECT_TRUE(args.first().canConvert<joynr::system::Address>());
}

TEST_F(WebSocketMessagingStubTest, transmitMessage) {
    LOG_TRACE(logger, QStringLiteral("transmit message"));
    QSignalSpy binaryMessageReceivedSignalSpy(this, SIGNAL(binaryMessageReceived(QByteArray)));

    // send message using messaging stub
    joynr::WebSocketMessagingStub messagingStub(serverAddress, webSocket);
    joynr::JoynrMessage joynrMsg;
    QByteArray expectedMessage(joynr::JsonSerializer::serialize(joynrMsg));
    joynr::MessagingQos qos;
    messagingStub.transmit(joynrMsg, qos);

    // wait until message is received
    EXPECT_TRUE(binaryMessageReceivedSignalSpy.wait());
    EXPECT_EQ(1, binaryMessageReceivedSignalSpy.count());

    // verify received message
    QList<QVariant> args = binaryMessageReceivedSignalSpy.takeFirst();
    EXPECT_EQ(1, args.size());
    EXPECT_EQ(QVariant::ByteArray, args.first().type());
    EXPECT_EQ(expectedMessage, args.first().toByteArray());
}

#include "WebSocketMessagingStubTest.moc"

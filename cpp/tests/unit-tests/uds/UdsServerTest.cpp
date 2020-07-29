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
#include "UdsServerTest.h"

#include "libjoynr/uds/UdsFrameBufferV1.h"

#include "joynr/Semaphore.h"

#include "tests/PrettyPrint.h"

using namespace joynr;
using namespace testing;

constexpr char UdsServerTest::_settingsFile[];
// Global settings for timeout, when communication between client-server is checked
const std::chrono::seconds UdsServerTest::_waitPeriodForClientServerCommunication(5);
const std::chrono::milliseconds UdsServerTest::_retryIntervalDuringClientServerCommunication(200);

TEST_F(UdsServerTest, multipleServerReusingSocket)
{
    auto server1 = createServer();
    server1->start();
    ASSERT_EQ(waitClientConnected(true), true) << "Client is not connected to initial server.";
    auto server2 = createServer();
    server2->start();
    ASSERT_EQ(waitClientConnected(true), true)
            << "Client lost connection after second server reuses socket.";
    server1.reset();
    ASSERT_EQ(waitClientConnected(false), false)
            << "Client is not disconnected after server server stopped.";
    restartClient();
    ASSERT_EQ(waitClientConnected(true), true)
            << "Client is not connected to second server after restart.";
}

TEST_F(UdsServerTest, serverCallbacks)
{
    Semaphore semaphore;
    Sequence sequence;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    joynr::system::RoutingTypes::UdsClientAddress clientAddress;
    EXPECT_CALL(mockUdsServerCallbacks, connected(_, _)).InSequence(sequence).WillOnce(
            DoAll(SaveArg<0>(&clientAddress), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";
    Mock::VerifyAndClearExpectations(&semaphore);

    const smrf::Byte message1 = 42;
    const smrf::Byte message2 = 43;
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(clientAddress, ElementsAre(message1), _))
            .InSequence(sequence);
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(clientAddress, ElementsAre(message2), _))
            .InSequence(sequence)
            .WillOnce(InvokeWithoutArgs(&semaphore, &Semaphore::notify));
    sendFromClient(message1);
    sendFromClient(message2);
    EXPECT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive second frame.";
    Mock::VerifyAndClearExpectations(&semaphore);

    EXPECT_CALL(mockUdsServerCallbacks, disconnected(clientAddress)).Times(1).WillOnce(
            InvokeWithoutArgs(&semaphore, &Semaphore::notify));
    stopClient();
    EXPECT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive disconnection call.";
}

TEST_F(UdsServerTest, sendToClient)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    EXPECT_CALL(mockUdsServerCallbacks, connected(_, _)).WillOnce(
            DoAll(SaveArg<1>(&sender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, _, _)).Times(0);
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_))
            .Times(0); // Server disconnects before client
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";
    const smrf::ByteVector messageWithContent(1, 1);
    sendToClient(sender, messageWithContent);
    const smrf::ByteVector messageEmpty;
    sendToClient(sender, messageEmpty);
    ASSERT_EQ(waitFor(_messagesReceivedByClient, 2), 2);
    EXPECT_EQ(_messagesReceivedByClient[0], messageWithContent);
    EXPECT_EQ(_messagesReceivedByClient[1], messageEmpty);
}

TEST_F(UdsServerTest, streamInterfaceRobustness)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> goodClientSender;
    EXPECT_CALL(mockUdsServerCallbacks, connected(_, _)).Times(2).WillOnce(DoAll(
            SaveArg<1>(&goodClientSender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, _, _)).Times(0);
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_)).Times(2).WillRepeatedly(
            InvokeWithoutArgs(&semaphore, &Semaphore::notify));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";

    ErroneousClient erroneousClientCookie(_udsSettings);
    EXPECT_TRUE(erroneousClientCookie.write(smrf::ByteVector(100, 0x01)));
    EXPECT_TRUE(erroneousClientCookie.waitTillClose())
            << "Erroneous client still connected to server after providing invalid cookie.";

    ErroneousClient erroneousClientMessage(_udsSettings);
    joynr::system::RoutingTypes::UdsClientAddress addr("evilMessage");
    joynr::UdsFrameBufferV1 initFrame(addr);
    EXPECT_TRUE(erroneousClientMessage.write(initFrame.raw()));
    EXPECT_TRUE(erroneousClientMessage.write(smrf::ByteVector(100, 0x01)));
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive disconnection callback for erroneous message.";

    const smrf::ByteVector message(1, 1);
    sendToClient(goodClientSender, message);
    ASSERT_EQ(waitFor(_messagesReceivedByClient, 1), 1)
            << "Erroneous clients affected good client connection.";
    EXPECT_EQ(_messagesReceivedByClient[0], message);
    stopClient();
    EXPECT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive disconnection call.";
}

TEST_F(UdsServerTest, sendWhileClientDisconnection)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    EXPECT_CALL(mockUdsServerCallbacks, connected(_, _)).Times(1).WillOnce(
            DoAll(SaveArg<1>(&sender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, _, _)).Times(0);
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_)).Times(AnyNumber());
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";
    const unsigned int sendRequests = 100000;
    // Check that the stopping of the client occured while processing the send calls.
    EXPECT_CALL(mockUdsServerCallbacks, sendFailed(_)).Times(Between(1, sendRequests - 1));
    auto doDisconnectAsync = std::async([this]() { stopClient(); });
    const smrf::ByteVector message;
    for (unsigned int i = 0; i < sendRequests; i++) {
        sendToClient(sender, message, mockUdsServerCallbacks);
    }
    doDisconnectAsync.get();
}

TEST_F(UdsServerTest, stopServerWhileSending)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    auto server = createServer(mockUdsServerCallbacks);
    EXPECT_CALL(mockUdsServerCallbacks, connected(_, _)).Times(1).WillOnce(
            DoAll(SaveArg<1>(&sender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, _, _)).Times(0); // Not registered
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_))
            .Times(0); // Only called when client connects, but not when server stops before
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";
    constexpr std::size_t sendRequests = 100000;
    auto doStopAsync = std::async([this, &server]() {
        waitFor(_messagesReceivedByClient, 1);
        std::this_thread::yield(); // Continue sending before doing the real stop
        server.reset();
    });
    const smrf::ByteVector empty;
    for (unsigned int i = 0; i < sendRequests; i++) {
        sendToClient(sender, empty);
    }
    doStopAsync.get();
    stopClient();
    // Note that on the CI the yield does not always lead to a further reception. Hence sometimes
    // only 1 message gets through.
    EXPECT_TRUE(_messagesReceivedByClient.size() > 0)
            << "Test timing problem. All messages received before server shutdown.";
    EXPECT_TRUE(_messagesReceivedByClient.size() < sendRequests)
            << "Test timing problem. All messages received before server shutdown.";
}

TEST_F(UdsServerTest, sendAfterClientDisconnection)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    EXPECT_CALL(mockUdsServerCallbacks, connected(_, _)).Times(1).WillOnce(
            DoAll(SaveArg<1>(&sender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, _, _)).Times(0);
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_)).Times(1).WillOnce(
            InvokeWithoutArgs(&semaphore, &Semaphore::notify));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";
    stopClient();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive disconnection callback.";
    const unsigned int sendRequests = 1000;
    EXPECT_CALL(mockUdsServerCallbacks, sendFailed(_)).Times(sendRequests);
    const smrf::ByteVector message;
    for (unsigned int i = 0; i < sendRequests; i++) {
        sendToClient(sender, message, mockUdsServerCallbacks);
    }
    std::this_thread::sleep_for(_waitPeriodForClientServerCommunication); // Wait for the AISO
                                                                          // action pool to process
                                                                          // complete sending
}

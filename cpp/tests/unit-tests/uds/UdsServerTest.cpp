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
const std::chrono::seconds UdsServerTest::_waitPeriodForClientServerCommunication(90);
const std::chrono::milliseconds UdsServerTest::_retryIntervalDuringClientServerCommunication(200);

std::function<void(const joynr::exceptions::JoynrRuntimeException&)>
        UdsServerTest::_ignoreClientFatalRuntimeErrors =
                [](const joynr::exceptions::JoynrRuntimeException&) {};

TEST_F(UdsServerTest, multipleServerReusingSocket)
{
    std::shared_ptr<joynr::IUdsSender> sender;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(2).WillRepeatedly(
            SaveArg<1>(&sender));
    auto server1 = createServer(mockUdsServerCallbacks);
    server1->start();
    ASSERT_EQ(waitClientConnected(true), true) << "Client is not connected to initial server.";
    auto server2 = createServer(mockUdsServerCallbacks);
    server2->start();
    std::this_thread::yield();
    ASSERT_EQ(waitClientConnected(true), true)
            << "Client lost connection after second server reuses socket.";
    server1.reset();
    ASSERT_EQ(waitClientConnected(false), false)
            << "Client is not disconnected after initial server stopped.";
    restartClient();
    ASSERT_EQ(waitClientConnected(true), true)
            << "Client is not connected to second server after restart.";
}

TEST_F(UdsServerTest, connectReceiveAndDisconnectFromClient_serverCallbacksCalled)
{
    Semaphore semaphore;
    Sequence sequence;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    joynr::system::RoutingTypes::UdsClientAddress clientAddress;
    std::shared_ptr<joynr::IUdsSender> sender;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).InSequence(sequence).WillOnce(
            DoAll(SaveArg<0>(&clientAddress),
                  SaveArg<1>(&sender),
                  InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";

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
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).WillOnce(
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

TEST_F(UdsServerTest, robustness_sendException_otherClientsNotAffected)
{
    Semaphore connectionSemaphore, disconnectionSemaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> tmpSender, nominalSender, erroneousSender;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(2).WillRepeatedly(DoAll(
            SaveArg<1>(&tmpSender), InvokeWithoutArgs(&connectionSemaphore, &Semaphore::notify)));
    // disconnected called for erroneous sender, not for nominal one
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_))
            .WillOnce(InvokeWithoutArgs(&disconnectionSemaphore, &Semaphore::notify));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(connectionSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for nominal client.";
    nominalSender = tmpSender;

    _udsSettings.setClientId("ErrorCausedBySenderNotByClient");
    joynr::UdsClient otherClient(_udsSettings, _ignoreClientFatalRuntimeErrors);
    otherClient.start();
    ASSERT_TRUE(connectionSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for other client.";
    erroneousSender = tmpSender;

    constexpr std::size_t sizeViolatingLimit =
            1UL + std::numeric_limits<UdsFrameBufferV1::BodyLength>::max();
    smrf::ByteArrayView viewCausingException(nullptr, sizeViolatingLimit);
    erroneousSender->send(viewCausingException, [](const exceptions::JoynrRuntimeException&) {});
    ASSERT_TRUE(disconnectionSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive disconnection callback for other client.";

    // Connections / Senders are independent. Error in one does not affect the other-
    const smrf::ByteVector message(1, 1);
    sendToClient(nominalSender, message);
    ASSERT_EQ(waitFor(_messagesReceivedByClient, 1), 1)
            << "Erroneous clients affected good client connection.";
    EXPECT_EQ(_messagesReceivedByClient[0], message);

    // Assure that only the expected disconnection has been called
    Mock::VerifyAndClearExpectations(&mockUdsServerCallbacks);
}

TEST_F(UdsServerTest, robustness_disconnectsErroneousClients_goodClientsNotAffected)
{
    Semaphore connectSemaphore, disconnectSemaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> tmpSender, goodClientSender, badClientSender;
    /*
     * Connected callback called for good client and erroneous client which causes error after init
     * frame. All errors are introduced in the cookie of the corresponding frame.
     */
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(2).WillRepeatedly(DoAll(
            SaveArg<1>(&tmpSender), InvokeWithoutArgs(&connectSemaphore, &Semaphore::notify)));

    // disconnected callback called for erroneous client, not for good client
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_)).Times(1).WillRepeatedly(
            InvokeWithoutArgs(&disconnectSemaphore, &Semaphore::notify));

    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(connectSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for good client.";

    goodClientSender = tmpSender;

    // connected and disconnected callbacks are not called if init message is invalid
    ErroneousClient errorInInitFrame(_udsSettings);

    EXPECT_TRUE(errorInInitFrame.write(smrf::ByteVector(100, 0x01)));
    EXPECT_TRUE(errorInInitFrame.waitTillClose())
            << "Erroneous client still connected to server after providing invalid cookie.";

    ErroneousClient errorInFrameAfterInit(_udsSettings);
    joynr::system::RoutingTypes::UdsClientAddress addr("errorInFrameAfterInit");
    joynr::UdsFrameBufferV1 initFrame(addr);
    EXPECT_TRUE(errorInFrameAfterInit.write(initFrame.raw()));
    EXPECT_TRUE(errorInFrameAfterInit.write(smrf::ByteVector(100, 0x01)));
    ASSERT_TRUE(connectSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for bad client.";
    badClientSender = tmpSender;
    ASSERT_TRUE(disconnectSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive disconnection callback for erroneous message.";

    const smrf::ByteVector message(1, 1);
    sendToClient(goodClientSender, message);
    ASSERT_EQ(waitFor(_messagesReceivedByClient, 1), 1)
            << "Erroneous clients affected good client connection.";
    EXPECT_EQ(_messagesReceivedByClient[0], message);

    // Disconnection currently only called for errorneous client
    Mock::VerifyAndClearExpectations(&mockUdsServerCallbacks);
}

TEST_F(UdsServerTest, robustness_nonblockingRead)
{
    const smrf::Byte goodMessage = 42;
    Semaphore connectSemaphore, messageSemaphore;
    std::shared_ptr<joynr::IUdsSender> tmpSender, goodClientSender, badClientSender;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(2).WillRepeatedly(DoAll(
            SaveArg<1>(&tmpSender), InvokeWithoutArgs(&connectSemaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, ElementsAre(goodMessage), _))
            .WillOnce(InvokeWithoutArgs(&messageSemaphore, &Semaphore::notify));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(connectSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for good client.";
    goodClientSender = tmpSender;

    ErroneousClient erroneousClient(_udsSettings);
    joynr::system::RoutingTypes::UdsClientAddress addr("invalidLengthMessage");
    joynr::UdsFrameBufferV1 initFrame(addr);
    EXPECT_TRUE(erroneousClient.write(initFrame.raw()));
    ASSERT_TRUE(connectSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for bad client.";
    badClientSender = tmpSender;

    const smrf::Byte incompletedMessage = 0xFF;
    const smrf::ByteVector incompletePayload(42, incompletedMessage);
    const smrf::ByteArrayView incompletePayloadView(incompletePayload);
    joynr::UdsFrameBufferV1 frameWithInvalidSize(incompletePayloadView);
    UdsFrameBufferV1::BodyLength sizeGreaterThanTransmittedBytes = incompletePayload.size() + 100;
    auto* frameData = static_cast<smrf::Byte*>(frameWithInvalidSize.header().data());
    std::memcpy(frameData + sizeof(UdsFrameBufferV1::Cookie),
                &sizeGreaterThanTransmittedBytes,
                sizeof(UdsFrameBufferV1::BodyLength));
    /*
     * The write operation is acomplished without any error, as soon as the 42 payload bytes
     * are stored in the underlying OS UDS buffer,
     */
    EXPECT_TRUE(erroneousClient.write(frameWithInvalidSize.raw()));

    // Just ensure that the erroneous client triggers the write before the good client
    std::this_thread::yield();

    /*
     * The UdsServer uses ASIO with one thread for all connections. If the async-read OPs
     * would block, till the expected bytes are received, the previous incomplete
     * payload transmission would block peristently since UDS sockets do not
     * have timeouts per default.
     * boost::asio::detail::read_op however uses "async_read_some" and continues with
     * the next OP, if no bytes are in the underying OS UDS buffer.
     */
    sendFromClient(goodMessage);
    ASSERT_TRUE(messageSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive message from good client.";

    /*
     * Assure that no disconnection has been called yet and the content of the good-message
     * has been received.
     */
    Mock::VerifyAndClearExpectations(&mockUdsServerCallbacks);
}

TEST_F(UdsServerTest, robustness_nonblockingWrite)
{
    Semaphore connectionSemaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> tmpClientSender, goodClientSender, blockingClientSender;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(2).WillRepeatedly(
            DoAll(SaveArg<1>(&tmpClientSender),
                  InvokeWithoutArgs(&connectionSemaphore, &Semaphore::notify)));
    // disconnected not called, since no connection loss (UDS socket has per default no timeout)
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_)).Times(0);
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(connectionSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for good client.";
    goodClientSender = tmpClientSender;

    _udsSettings.setClientId("blockMessageProcessing");
    BlockReceptionClient blockingClient(_udsSettings);
    blockingClient.start();
    ASSERT_TRUE(connectionSemaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback for blocking client.";
    blockingClientSender = tmpClientSender;

    // Cause OS UDS buffer reaching limit (the size of OS UDS buffer is e.g. round about 1kB)
    for (unsigned int i = 0; i < 1024; i++) {
        sendToClient(blockingClientSender, smrf::ByteVector(1024, 1));
        std::this_thread::yield();
    }

    /*
     * The UdsServer uses ASIO with one thread for all connections. If the async-write OP
     * for transimission to blocking-client would block till the OS UDS buffer has sufficient
     * bytes to store, transmissions to other clients would block as well.
     * boost::asio::detail::write_op however uses "async_write_some" and continues with
     * the next OP, if not all bytes can be stored in the underying OS UDS buffer.
     */
    const smrf::ByteVector message(1, 1);
    sendToClient(goodClientSender, message);
    ASSERT_EQ(waitFor(_messagesReceivedByClient, 1), 1)
            << "Erroneous clients affected good client connection.";
    EXPECT_EQ(_messagesReceivedByClient[0], message);

    ASSERT_TRUE(blockingClient.stopBlockingAndWaitForReceivedBytes(1024 * 1024));

    // Assure that no disconnection has been called yet
    Mock::VerifyAndClearExpectations(&mockUdsServerCallbacks);
}

TEST_F(UdsServerTest, sendToClientWhileClientDisconnection)
{
    Semaphore semaphore;
    Semaphore semaphoreStop;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(1).WillOnce(
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
    auto doDisconnectAsync = std::async([this, &semaphoreStop]() {
        semaphoreStop.wait();
        stopClient();
    });
    const smrf::ByteVector message;
    waitClientConnected(true);
    for (unsigned int i = 0; i < sendRequests; i++) {
        sendToClient(sender, message, mockUdsServerCallbacks);
        if (i == sendRequests / 2) {
            semaphoreStop.notify();
        }
    }
    doDisconnectAsync.get();
}

TEST_F(UdsServerTest, stopServerWhileSending)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    auto server = createServer(mockUdsServerCallbacks);
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(1).WillOnce(
            DoAll(SaveArg<1>(&sender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";
    constexpr std::size_t numberOfMessages = 10000;
    auto doStopAsync = std::async([this, &server]() {
        waitFor(_messagesReceivedByClient, 1);
        std::this_thread::yield();
        server.reset();
    });
    const smrf::ByteVector message(32, 1);
    for (unsigned int i = 0; i < numberOfMessages; i++) {
        sendToClient(sender, message);
        std::this_thread::yield();
    }
    doStopAsync.get();
    stopClient();
    EXPECT_TRUE(_messagesReceivedByClient.size() > 0)
            << "Test timing problem. No messages received before server shutdown.";
    EXPECT_TRUE(_messagesReceivedByClient.size() < numberOfMessages)
            << "Test timing problem. All messages received before server shutdown.";
}

TEST_F(UdsServerTest, sendAfterClientDisconnection)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(1).WillOnce(
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
    AtomicCounter sendFailures;
    EXPECT_CALL(mockUdsServerCallbacks, sendFailed(_))
            .WillRepeatedly(InvokeWithoutArgs(&sendFailures, &AtomicCounter::increment));
    const smrf::ByteVector message;
    for (unsigned int i = 0; i < sendRequests; i++) {
        sendToClient(sender, message, mockUdsServerCallbacks);
    }
    ASSERT_EQ(waitFor(sendFailures, sendRequests), sendRequests);
}

TEST_F(UdsServerTest, sendFailedCallbackException)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    const std::logic_error userException("Test exception");
    std::shared_ptr<joynr::IUdsSender> sender;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(1).WillOnce(
            DoAll(SaveArg<1>(&sender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, sendFailed(_)).Times(AtLeast(1)).WillRepeatedly(
            Throw(userException));

    _udsSettings.setSendingQueueSize(0);
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";

    ASSERT_EQ(waitClientConnected(true), true);

    const smrf::ByteVector message;
    for (unsigned int i = 0; i < 1024; i++) {
        sendToClient(sender, message, mockUdsServerCallbacks);
    }

    ASSERT_EQ(waitClientConnected(false), false);
}

TEST_F(UdsServerTest, stopServerWhileReceiving)
{
    Semaphore semaphore;
    std::atomic_ullong numberOfMessagesReceived(0);
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    auto server = createServer(mockUdsServerCallbacks);
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).Times(1).WillOnce(
            DoAll(SaveArg<1>(&sender), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, _, _)).WillRepeatedly(
            InvokeWithoutArgs([&numberOfMessagesReceived] { numberOfMessagesReceived++; }));
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";
    constexpr std::size_t numberOfMessages = 10000;
    auto doStopAsync = std::async([this, &server]() {
        waitClientConnected(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        server.reset();
    });
    const smrf::ByteVector message(100, 1);
    for (unsigned int i = 0; i < numberOfMessages; i++) {
        sendFromClient(message);
    }
    doStopAsync.get();
    stopClient();
    EXPECT_TRUE(numberOfMessagesReceived.load() > 0)
            << "Test timing problem. No messages received before server shutdown.";
    EXPECT_TRUE(numberOfMessagesReceived.load() < numberOfMessages)
            << "Test timing problem. All messages received before server shutdown.";
}

TEST_F(UdsServerTest, connectedCallbackException)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    const std::logic_error userException("Test exception");
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).WillOnce(
            DoAll(InvokeWithoutArgs(&semaphore, &Semaphore::notify), Throw(userException)));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "connection callback not invoked";
    ASSERT_EQ(waitClientConnected(false), false);
}

TEST_F(UdsServerTest, disconnectedCallbackException)
{
    Semaphore semaphore;
    std::shared_ptr<joynr::IUdsSender> sender;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    const std::logic_error userException("Test exception");
    EXPECT_CALL(mockUdsServerCallbacks, disconnected(_)).WillOnce(
            DoAll(InvokeWithoutArgs(&semaphore, &Semaphore::notify), Throw(userException)));
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).WillOnce(SaveArg<1>(&sender));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_EQ(waitClientConnected(true), true) << "Failed to receive connection callback.";
    stopClient();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "disconnection callback not invoked";
    ASSERT_EQ(waitClientConnected(false), false);
}

TEST_F(UdsServerTest, receivedCallbackException)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    std::shared_ptr<joynr::IUdsSender> sender;
    const std::logic_error userException("Test exception");
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(_, _, _)).WillOnce(Throw(userException));
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).WillOnce(SaveArg<1>(&sender));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_EQ(waitClientConnected(true), true);
    sendFromClient(1);
    ASSERT_EQ(waitClientConnected(false), false);
}

TEST_F(UdsServerTest, getUserName)
{
    std::string username = UdsServerUtil::getUserNameByUid(0);
    ASSERT_EQ(username, "root");
    username = UdsServerUtil::getUserNameByUid(3000);
    ASSERT_EQ(username, "3000");
}

TEST_F(UdsServerTest, getCorrectUsernameAfterConnection)
{
    Semaphore semaphore;
    MockUdsServerCallbacks mockUdsServerCallbacks;
    joynr::system::RoutingTypes::UdsClientAddress clientAddress;
    std::shared_ptr<joynr::IUdsSender> sender;
    std::string capturedUsername;
    EXPECT_CALL(mockUdsServerCallbacks, connectedMock(_, _)).WillOnce(
            DoAll(SaveArg<0>(&clientAddress),
                  SaveArg<1>(&sender),
                  InvokeWithoutArgs(&semaphore, &Semaphore::notify)));
    auto server = createServer(mockUdsServerCallbacks);
    server->start();
    ASSERT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive connection callback.";

    const smrf::Byte message = 42;
    EXPECT_CALL(mockUdsServerCallbacks, receivedMock(clientAddress, ElementsAre(message), _))
            .WillOnce(DoAll(SaveArg<2>(&capturedUsername), InvokeWithoutArgs(&semaphore, &Semaphore::notify)));

    sendFromClient(message);
    EXPECT_TRUE(semaphore.waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive frame from client.";

    std::string expectedUsername = getUserName();
    ASSERT_EQ(capturedUsername, expectedUsername);
}

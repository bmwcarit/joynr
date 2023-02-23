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
#include "UdsClientTest.h"

#include <stdlib.h>

#include "libjoynr/uds/UdsFrameBufferV1.h"

#include "tests/JoynrTest.h"
#include "tests/PrettyPrint.h"

using namespace joynr;
using namespace testing;
namespace fs = boost::filesystem;

constexpr char UdsClientTest::_settingsFile[];
// Global settings for timeout, when communication between client-server is checked
const std::chrono::seconds UdsClientTest::_waitPeriodForClientServerCommunication(5);
const std::chrono::milliseconds UdsClientTest::_retryIntervalDuringClientServerCommunication(200);

TEST_F(UdsClientTest, connectAndDisconnectMultipleClients)
{
    auto client1 = createClient();
    client1->start();
    ASSERT_EQ(countServerConnections(1), 1) << "First client not connected.";
    auto client2 = createClient();
    client2->start();
    ASSERT_EQ(countServerConnections(2), 2) << "Second client not connected.";
    client1.reset();
    ASSERT_EQ(countServerConnections(1), 1) << "First client not disconnected.";
    client2.reset();
    ASSERT_EQ(countServerConnections(0), 0) << "Second client not disconnected.";
}

TEST_F(UdsClientTest, receiveFromServer_clientCallbacksCalled)
{
    Sequence sequence;
    auto semaphore = std::make_shared<Semaphore>();
    MockUdsClientCallbacks mockUdsClientCallbacks;
    EXPECT_CALL(mockUdsClientCallbacks, connected()).Times(1).InSequence(sequence);
    EXPECT_CALL(mockUdsClientCallbacks, receivedMock(ElementsAre(1))).InSequence(sequence);
    EXPECT_CALL(mockUdsClientCallbacks, receivedMock(ElementsAre(2)))
            .InSequence(sequence)
            .WillOnce(ReleaseSemaphore(semaphore));
    EXPECT_CALL(mockUdsClientCallbacks, disconnected()).Times(1).InSequence(sequence);
    // On an expected connection loss, no fatal runtime error is signaled
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).Times(0);
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);

    sendFromServer(1);
    sendFromServer(2);
    EXPECT_TRUE(semaphore->waitFor(_waitPeriodForClientServerCommunication))
            << "Failed to receive second frame.";

    client.reset();
    ASSERT_EQ(countServerConnections(0), 0);
}

TEST_F(UdsClientTest, disconnectFromServer_clientCallbacksCalled)
{
    auto semaphore1 = std::make_shared<Semaphore>();
    auto semaphore2 = std::make_shared<Semaphore>();
    MockUdsClientCallbacks mockUdsClientCallbacks;
    Sequence sequence;
    EXPECT_CALL(mockUdsClientCallbacks, connected())
            .Times(1)
            .InSequence(sequence)
            .WillOnce(ReleaseSemaphore(semaphore1));
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).Times(1).InSequence(sequence);
    EXPECT_CALL(mockUdsClientCallbacks, disconnected())
            .Times(1)
            .InSequence(sequence)
            .WillOnce(ReleaseSemaphore(semaphore2));
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    EXPECT_TRUE(semaphore1->waitFor(_waitPeriodForClientServerCommunication))
            << "Connected not reported when client started";
    ASSERT_EQ(countServerConnections(1), 1);

    stopServer();
    EXPECT_TRUE(semaphore2->waitFor(_waitPeriodForClientServerCommunication))
            << "Disconnected not reported when server closes";
}

TEST_F(UdsClientTest, sendFrom2Clients)
{
    auto client1 = createClient();
    const smrf::ByteVector messageBeforeStartClient1(1, 1);
    sendFromClient(client1, messageBeforeStartClient1);
    client1->start();
    ASSERT_EQ(countServerConnections(1), 1) << "First client not connected.";
    const smrf::ByteVector messageAfterStartClient1(1, 2);
    sendFromClient(client1, messageAfterStartClient1);

    auto client2 = createClient();
    const smrf::ByteVector messageWhileStartClient2(1, 11);
    client2->start();
    sendFromClient(client2, messageWhileStartClient2);
    ASSERT_EQ(countServerConnections(2), 2) << "Second client not connected.";
    const smrf::ByteVector messageWhileDisconnectingClient2(1, 12);
    const unsigned int numberOfMessagesWhileDisconnectingClient2 = 200;
    for (unsigned int i = 0; i < numberOfMessagesWhileDisconnectingClient2; i++) {
        sendFromClient(client2, messageWhileDisconnectingClient2);
    }

    const smrf::ByteVector messageWhileDisconnectingOtherClient1(1, 3);
    client2.reset();
    sendFromClient(client1, messageWhileDisconnectingOtherClient1);
    ASSERT_EQ(countServerConnections(1), 1) << "Second client not disconnected.";
    const smrf::ByteVector messageAfterDisconnectingClient1(1, 4);
    sendFromClient(client1, messageAfterDisconnectingClient1);

    const smrf::ByteVector messageEmptyClient1;
    sendFromClient(client1, messageEmptyClient1);

    const auto& messagesReceivedFromClient1 = _connectedClients[0]._receivedMessages;
    ASSERT_EQ(waitFor(messagesReceivedFromClient1, 5), 5)
            << "Unexpected number of messages received for client 1.";
    EXPECT_EQ(messagesReceivedFromClient1[0], messageBeforeStartClient1);
    EXPECT_EQ(messagesReceivedFromClient1[1], messageAfterStartClient1);
    EXPECT_EQ(messagesReceivedFromClient1[2], messageWhileDisconnectingOtherClient1);
    EXPECT_EQ(messagesReceivedFromClient1[3], messageAfterDisconnectingClient1);
    EXPECT_EQ(messagesReceivedFromClient1[4], messageEmptyClient1);

    const auto& messagesReceivedFromClient2 = _connectedClients[1]._receivedMessages;
    EXPECT_TRUE(messagesReceivedFromClient2.size() <=
                        numberOfMessagesWhileDisconnectingClient2 + 1 &&
                messagesReceivedFromClient2.size() >= 1)
            << "Unexpected number of messages received for client 2 (which had been closed while "
               "sending): "
            << messagesReceivedFromClient2.size();
}

TEST_F(UdsClientTest, sendFromClientBufferOverflow)
{
    constexpr unsigned int sendQueueSize = 1000;
    std::atomic_uint32_t countSendFailures{0};
    MockUdsClientCallbacks mockUdsClientCallbacks;
    EXPECT_CALL(mockUdsClientCallbacks, sendFailed(_))
            .Times(sendQueueSize)
            .WillRepeatedly(InvokeWithoutArgs([&countSendFailures] { countSendFailures++; }));
    const smrf::ByteVector messageEmpty;
    _udsSettings.setSendingQueueSize(sendQueueSize);
    auto client = createClient();
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    std::lock_guard<std::mutex> lockNextServerSideRead(_connectedClientsMutex);
    // The client writes an unkown number to the UDS write buffer.
    // That number depends on the thread switches and OS dependent UDS buffer size (which is e.g.
    // just around KB).
    // These send attempts are blocking (no timeout implemented by client), hence they do not appear
    // in the failures.
    for (unsigned int i = 0; i < 2 * sendQueueSize; i++) {
        sendFromClient(client, messageEmpty, mockUdsClientCallbacks);
    }
    EXPECT_EQ(waitForGreaterThan(countSendFailures, sendQueueSize), sendQueueSize);
}

TEST_F(UdsClientTest, sendFromClientAfterDisconnection)
{
    MockUdsClientCallbacks mockUdsClientCallbacks;
    auto client = createClient();
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    const smrf::ByteVector messageEmpty;
    sendFromClient(client, messageEmpty);
    const auto& messagesReceivedFromClient = _connectedClients[0]._receivedMessages;
    EXPECT_EQ(waitFor(messagesReceivedFromClient, 1), 1);
    stopServer();

    // When the server is down, no send failure is triggered since the client does not reconnect
    EXPECT_CALL(mockUdsClientCallbacks, sendFailed(_)).Times(0);
    sendFromClient(client, messageEmpty, mockUdsClientCallbacks);
    std::this_thread::sleep_for(_waitPeriodForClientServerCommunication);
    EXPECT_EQ(messagesReceivedFromClient.size(), 1)
            << "A message has been received though the server was down.";
}

TEST_F(UdsClientTest, sendFromClientAsynchronous)
{
    auto client = createClient();
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    const auto& messagesReceivedFromClient = _connectedClients[0]._receivedMessages;
    std::vector<std::future<void>> concurrentTasks;
    constexpr unsigned int numberOfSendCalls = 1000;
    for (unsigned int i = 0; i < numberOfSendCalls; i++) {
        concurrentTasks.push_back(std::async([&client]() {
            const smrf::ByteVector messageEmpty;
            UdsClientTest::sendFromClient(client, messageEmpty);
        }));
    }
    ASSERT_EQ(waitFor(messagesReceivedFromClient, numberOfSendCalls), numberOfSendCalls);
}

TEST_F(UdsClientTest, sendException)
{
    Sequence sequence;
    MockUdsClientCallbacks mockUdsClientCallbacks;
    EXPECT_CALL(mockUdsClientCallbacks, connected()).Times(1).InSequence(sequence);
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).Times(1).InSequence(sequence);
    EXPECT_CALL(mockUdsClientCallbacks, disconnected()).Times(AtMost(1));
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    constexpr std::size_t sizeViolatingLimit =
            1UL + std::numeric_limits<UdsFrameBufferV1::BodyLength>::max();
    smrf::ByteArrayView viewCausingException(nullptr, sizeViolatingLimit);
    client->send(viewCausingException, [](const exceptions::JoynrRuntimeException&) {});
    ASSERT_EQ(countServerConnections(0), 0);
}

TEST_F(UdsClientTest, sendFailedCallbackException)
{
    MockUdsClientCallbacks mockUdsClientCallbacks;
    const std::logic_error userException("Test exception");
    EXPECT_CALL(mockUdsClientCallbacks, sendFailed(_))
            .Times(AtLeast(1))
            .WillRepeatedly(Throw(userException));
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).Times(1);
    EXPECT_CALL(mockUdsClientCallbacks, connected()).Times(1);
    EXPECT_CALL(mockUdsClientCallbacks, disconnected()).Times(AtMost(1));

    _udsSettings.setSendingQueueSize(0);
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    const smrf::ByteVector message;
    for (unsigned int i = 0; i < 1024; i++) {
        sendFromClient(client, message, mockUdsClientCallbacks);
    }
    ASSERT_EQ(countServerConnections(0), 0);
}

TEST_F(UdsClientTest, connectedCallbackException)
{
    auto semaphore = std::make_shared<Semaphore>();
    MockUdsClientCallbacks mockUdsClientCallbacks;
    const std::logic_error userException("Test exception");
    EXPECT_CALL(mockUdsClientCallbacks, connected()).WillOnce(Throw(userException));
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).WillOnce(ReleaseSemaphore(semaphore));
    EXPECT_CALL(mockUdsClientCallbacks, disconnected()).Times(AtMost(1));
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    ASSERT_TRUE(semaphore->waitFor(_waitPeriodForClientServerCommunication))
            << "fatal error callback not invoked";
    ASSERT_EQ(countServerConnections(0), 0);
}

TEST_F(UdsClientTest, disconnectedCallbackException)
{
    auto semaphore = std::make_shared<Semaphore>();
    MockUdsClientCallbacks mockUdsClientCallbacks;
    const std::logic_error userException("Test exception");
    EXPECT_CALL(mockUdsClientCallbacks, disconnected()).WillOnce(Throw(userException));
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).WillOnce(ReleaseSemaphore(semaphore));
    EXPECT_CALL(mockUdsClientCallbacks, connected()).Times(1);
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    stopServer();
    ASSERT_TRUE(semaphore->waitFor(_waitPeriodForClientServerCommunication))
            << "fatal error callback not invoked";
}

TEST_F(UdsClientTest, receivedCallbackException)
{
    auto semaphore = std::make_shared<Semaphore>();
    MockUdsClientCallbacks mockUdsClientCallbacks;
    const std::logic_error userException("Test exception");
    EXPECT_CALL(mockUdsClientCallbacks, receivedMock(_)).WillOnce(Throw(userException));
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).WillOnce(ReleaseSemaphore(semaphore));
    EXPECT_CALL(mockUdsClientCallbacks, connected()).Times(1);
    EXPECT_CALL(mockUdsClientCallbacks, disconnected()).Times(1);
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    sendFromServer(1);
    ASSERT_TRUE(semaphore->waitFor(_waitPeriodForClientServerCommunication))
            << "fatal error callback not invoked";
    ASSERT_EQ(countServerConnections(0), 0);
}

TEST_F(UdsClientTest, fatalErrorCallbackException)
{
    MockUdsClientCallbacks mockUdsClientCallbacks;
    const std::logic_error triggerFatalErrorCallback("Test exception 1");
    EXPECT_CALL(mockUdsClientCallbacks, receivedMock(_)).WillOnce(Throw(triggerFatalErrorCallback));
    const std::logic_error userException("Test exception 2");
    EXPECT_CALL(mockUdsClientCallbacks, fatalRuntimeError(_)).WillOnce(Throw(userException));
    EXPECT_CALL(mockUdsClientCallbacks, connected()).Times(1);
    EXPECT_CALL(mockUdsClientCallbacks, disconnected()).Times(1);
    auto client = createClient(mockUdsClientCallbacks);
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
    sendFromServer(1);
    // fatal runtime error, stopping all communication permanently
    ASSERT_EQ(countServerConnections(0), 0);
}

TEST_F(UdsClientTest, fatalErrorSocketDirDoesNotExist)
{
    // Stop server to prevent race condition where server
    // has temporarily set umask to exclude execute
    // permissions while fs::create_directories(...)
    // runs in parallel attempting to create directory
    // tree causing Permission denied error leading to
    // test failure.
    // The server is not required for initial test.
    stopServer();
    auto socketDir = _tmpDirectory / "does/not/exist";
    auto socketPath = socketDir / "someSocket";
    _udsSettings.setSocketPath(socketPath.string());
    std::string errorMessage;
    createClientAndGetRuntimeErrorMessage(errorMessage);
    EXPECT_THAT(errorMessage, HasSubstr("Socket path directory"));
    EXPECT_THAT(errorMessage, HasSubstr("does not exist"));
    EXPECT_THAT(errorMessage, EndsWith(socketDir.string()));

    // fatal runtime error, stopping all communication permanently
    fs::create_directories(socketDir);
    restartServer();
    std::this_thread::sleep_for(_waitPeriodForClientServerCommunication);
    ASSERT_EQ(countServerConnections(0), 0);

    // Check validity of test-setup
    auto client = createClient();
    client->start();
    ASSERT_EQ(countServerConnections(1), 1);
}

TEST_F(UdsClientTest, fatalErrorSocketDirNotReadable)
{
    // make sure server is not starting up late and then gets
    // impacted by changes to shared socket path and dir / file permissions
    stopServer();
    auto socketDir = _tmpDirectory / "noReadAccess";
    auto socketPath = socketDir / "someSocket";
    _udsSettings.setSocketPath(socketPath.string());
    fs::create_directory(socketDir);
    fs::permissions(socketDir,
                    fs::perms::remove_perms | fs::perms::owner_read | fs::perms::group_read |
                            fs::perms::others_read);
    std::string errorMessage;
    createClientAndGetRuntimeErrorMessage(errorMessage);
    EXPECT_THAT(errorMessage, HasSubstr("Socket path directory"));
    EXPECT_THAT(errorMessage, HasSubstr("not readable"));
    EXPECT_THAT(errorMessage, EndsWith(socketDir.string()));
    fs::permissions(socketDir,
                    fs::perms::add_perms | fs::perms::owner_read | fs::perms::group_read |
                            fs::perms::others_read);
}

TEST_F(UdsClientTest, fatalErrorSocketPathNotReadWriteable)
{
    // make sure server is not starting up late and then gets
    // impacted by changes to shared socket path and dir / file permissions
    stopServer();
    auto socketDir = _tmpDirectory / "socketDir";
    auto socketPath = socketDir / "someSocket";
    _udsSettings.setSocketPath(socketPath.string());
    fs::create_directory(socketDir);
    fs::ofstream createSocketFile(socketPath);
    createSocketFile.close();
    for (auto permsToRemove :
         {fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read,
          fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write}) {
        fs::permissions(socketPath, fs::perms::remove_perms | permsToRemove);
        std::string errorMessage;
        createClientAndGetRuntimeErrorMessage(errorMessage);
        EXPECT_THAT(errorMessage, HasSubstr("Socket path exist"));
        EXPECT_THAT(errorMessage, HasSubstr("not readable or not writable"));
        EXPECT_THAT(errorMessage, EndsWith(socketPath.string()));
        fs::permissions(socketPath, fs::perms::add_perms | permsToRemove);
    }
}

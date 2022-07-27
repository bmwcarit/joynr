/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2019 BMW Car IT GmbH
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
#include <string>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Future.h"
#include "joynr/Message.h"
#include "joynr/MessagingSettings.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/Settings.h"
#include "joynr/StatusCode.h"
#include "libjoynrclustercontroller/capabilities-directory/GlobalCapabilitiesDirectoryClient.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockGlobalCapabilitiesDirectoryProxy.h"
#include "tests/mock/MockJoynrRuntime.h"
#include "tests/mock/MockLocalCapabilitiesDirectoryStore.h"
#include "tests/mock/MockMessageSender.h"
#include "tests/mock/MockTaskSequencer.h"

using namespace ::testing;
using namespace joynr;

class GlobalCapabilitiesDirectoryClientTest : public TestWithParam<std::string>
{

public:
    GlobalCapabilitiesDirectoryClientTest()
            : settings(std::make_unique<Settings>()),
              messagingSettings(*settings),
              clusterControllerSettings(*settings),
              taskSequencer(
                      std::make_unique<TaskSequencer<void>>(
                              std::chrono::milliseconds(MessagingQos().getTtl()))),
              mockJoynrRuntime(std::make_shared<MockJoynrRuntime>(*settings)),
              mockMessageSender(std::make_shared<MockMessageSender>()),
              joynrMessagingConnectorFactory(
                      std::make_shared<JoynrMessagingConnectorFactory>(mockMessageSender, nullptr)),
              mockGlobalCapabilitiesDirectoryProxy(
                      std::make_shared<MockGlobalCapabilitiesDirectoryProxy>(
                              mockJoynrRuntime,
                              joynrMessagingConnectorFactory)),
              mockLCDStore(std::make_shared<MockLocalCapabilitiesDirectoryStore>()),
              globalCapabilitiesDirectoryClient(std::make_shared<GlobalCapabilitiesDirectoryClient>(
                      clusterControllerSettings, std::move(taskSequencer))),
              mockLocalCapabilitiesDirectoryStore(std::make_shared<MockLocalCapabilitiesDirectoryStore>()),
              capDomain("testDomain"),
              capInterface("testInterface"),
              capParticipantId("testParticipantId"),
              capPublicKeyId("publicKeyId"),
              capLastSeenMs(0),
              capExpiryDateMs(1000),
              capSerializedMqttAddress("{\"_typeName\":\"joynr.system.RoutingTypes.MqttAddress\",\"brokerUri\":\"testGbid\",\"topic\":\"testTopic}"),
              gbids({"gbid1", "gbid2"}),
              messagingTtl(10000),
              capProviderQos(),
              providerVersion(47, 11),
              globalDiscoveryEntry(providerVersion,
                                   capDomain,
                                   capInterface,
                                   capParticipantId,
                                   capProviderQos,
                                   capLastSeenMs,
                                   capExpiryDateMs,
                                   capPublicKeyId,
                                   capSerializedMqttAddress),
              mockFuture(std::make_shared<joynr::Future<void>>()),
              onSuccess([]() {}),
              onError([](const types::DiscoveryError::Enum& /*error*/) {}),
              onRuntimeError([](const exceptions::JoynrRuntimeException& /*error*/) {})
    {
    }

    std::unique_ptr<Settings> settings;
    MessagingSettings messagingSettings;
    ClusterControllerSettings clusterControllerSettings;
    std::unique_ptr<TaskSequencer<void>> taskSequencer;
    std::shared_ptr<MockJoynrRuntime> mockJoynrRuntime;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    std::shared_ptr<JoynrMessagingConnectorFactory> joynrMessagingConnectorFactory;
    std::shared_ptr<MockGlobalCapabilitiesDirectoryProxy> mockGlobalCapabilitiesDirectoryProxy;
    std::shared_ptr<MockLocalCapabilitiesDirectoryStore> mockLCDStore;
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient;
    std::shared_ptr<MockLocalCapabilitiesDirectoryStore> mockLocalCapabilitiesDirectoryStore;

    void SetUp() override
    {
        globalCapabilitiesDirectoryClient->setProxy(
                mockGlobalCapabilitiesDirectoryProxy);
    }

    void TearDown() override
    {
        if (globalCapabilitiesDirectoryClient) {
            /*
             * Assure that a shutdown is accomplished before calls used by the
             * callbacks are deleted.
             */
            globalCapabilitiesDirectoryClient->shutdown();
        }
    }

protected:
    void testAddUsesCorrectRemainingTtl(const bool awaitGlobalRegistration);
    void testAdd(bool awaitGlobalRegistration);
    void testAddTaskExpiryDateHasCorrectValue(bool awaitGlobalRegistration, const joynr::TimePoint& expiryDate);

    const std::string capDomain;
    const std::string capInterface;
    const std::string capParticipantId;
    const std::string capPublicKeyId;
    const std::int64_t capLastSeenMs;
    const std::int64_t capExpiryDateMs;
    const std::string capSerializedMqttAddress;
    const std::vector<std::string> gbids;
    const std::int64_t messagingTtl;
    types::ProviderQos capProviderQos;
    joynr::types::Version providerVersion;
    types::GlobalDiscoveryEntry globalDiscoveryEntry;
    std::shared_ptr<joynr::Future<void>> mockFuture;
    std::function<void()> onSuccess;
    std::function<void(const types::DiscoveryError::Enum& error)> onError;
    std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError;
};

void testMessagingQosForCustomHeaderGbidKey(std::string gbid,
                                            std::shared_ptr<joynr::MessagingQos> messagingQos)
{
    ASSERT_NE(messagingQos, nullptr);
    auto customMessageHeaders = messagingQos->getCustomMessageHeaders();
    ASSERT_EQ(gbid, customMessageHeaders.find(joynr::Message::CUSTOM_HEADER_GBID_KEY())->second);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddTaskTimeoutFunctionCallsOnRuntimeErrorCorrectly)
{
    std::unique_ptr<MockTaskSequencer<void>> mockTaskSequencer = std::make_unique<MockTaskSequencer<void>>(std::chrono::milliseconds(60000));
    auto mockTaskSequencerRef = mockTaskSequencer.get();

    std::shared_ptr<GlobalCapabilitiesDirectoryClient> gcdClient = std::make_shared<GlobalCapabilitiesDirectoryClient>(
                                                  clusterControllerSettings, std::move(mockTaskSequencer));
    Semaphore semaphore;
    MockTaskSequencer<void>::MockTaskWithExpiryDate capturedTask;

    EXPECT_CALL(*mockTaskSequencerRef, add(_)).Times(1)
            .WillOnce(DoAll(SaveArg<0>(&capturedTask),
                            ReleaseSemaphore(&semaphore)));

    bool onRuntimeErrorCalled = false;
    bool exceptionMessageFound = false;
    auto onRuntimeError = [&onRuntimeErrorCalled, &exceptionMessageFound](
            const exceptions::JoynrRuntimeException& exception) {
        std::string exceptionMessage = exception.getMessage();
        std::string expectedMessage = "Failed to process global registration in time, please try again";
        onRuntimeErrorCalled = true;
        if (exceptionMessage.find(expectedMessage) != std::string::npos) {
            exceptionMessageFound = true;
        }
    };
    gcdClient->add(
            globalDiscoveryEntry, true, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "TaskSequencer.add() not called.";
    capturedTask._timeout();
    ASSERT_TRUE(onRuntimeErrorCalled);
    ASSERT_TRUE(exceptionMessageFound);
}

void GlobalCapabilitiesDirectoryClientTest::testAddTaskExpiryDateHasCorrectValue(
        bool awaitGlobalRegistration,
        const TimePoint &expectedTaskExpiryDate)
{
    std::unique_ptr<MockTaskSequencer<void>> mockTaskSequencer = std::make_unique<MockTaskSequencer<void>>(std::chrono::milliseconds(60000));
    auto mockTaskSequencerRef = mockTaskSequencer.get();
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> gcdClient = std::make_shared<GlobalCapabilitiesDirectoryClient>(
                                                  clusterControllerSettings, std::move(mockTaskSequencer));
    Semaphore semaphore;
    MockTaskSequencer<void>::MockTaskWithExpiryDate capturedTask;

    EXPECT_CALL(*mockTaskSequencerRef, add(_)).Times(1)
            .WillOnce(DoAll(SaveArg<0>(&capturedTask),
                            ReleaseSemaphore(&semaphore)));
    gcdClient->add(
            globalDiscoveryEntry, awaitGlobalRegistration, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "TaskSequencer.add() not called.";
    auto actualTaskExpiryDate = capturedTask._expiryDate;
    if (awaitGlobalRegistration) {
        ASSERT_TRUE(actualTaskExpiryDate - expectedTaskExpiryDate < std::chrono::milliseconds(1000));
        ASSERT_TRUE(actualTaskExpiryDate.toMilliseconds() >= expectedTaskExpiryDate.toMilliseconds());
    } else {
        ASSERT_EQ(expectedTaskExpiryDate.toMilliseconds(), actualTaskExpiryDate.toMilliseconds());
    }
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddTaskExpiryDateHasCorrectValueWithAwaitGlobalRegistration)
{
    bool awaitGlobalRegistration = true;
    TimePoint taskExpiryDate = TimePoint::fromRelativeMs(
                static_cast<std::int64_t>(MessagingQos().getTtl()));
    testAddTaskExpiryDateHasCorrectValue(awaitGlobalRegistration, taskExpiryDate);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddTaskExpiryDateHasCorrectValueWithoutAwaitGlobalRegistration)
{
    bool awaitGlobalRegistration = false;
    TimePoint taskExpiryDate = TimePoint::max();
    testAddTaskExpiryDateHasCorrectValue(awaitGlobalRegistration, taskExpiryDate);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest,
       testReAddTask_onSuccessForAllEntries_proxyCalledAndResultFutureResolved) {
    std::unique_ptr<MockTaskSequencer<void>> mockTaskSequencer =
            std::make_unique<MockTaskSequencer<void>>(std::chrono::milliseconds(60000));
    auto mockTaskSequencerRef = mockTaskSequencer.get();
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> gcdClient =
            std::make_shared<GlobalCapabilitiesDirectoryClient>(
                                                  clusterControllerSettings, std::move(mockTaskSequencer));
    gcdClient->setProxy(
            mockGlobalCapabilitiesDirectoryProxy);

    Semaphore semaphore;
    MockTaskSequencer<void>::MockTaskWithExpiryDate capturedTask;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture1;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture2;
    EXPECT_CALL(*mockTaskSequencerRef, add(_)).Times(1)
            .WillOnce(DoAll(SaveArg<0>(&capturedTask),
                            ReleaseSemaphore(&semaphore)));

    gcdClient->reAdd(mockLCDStore, capSerializedMqttAddress);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "TaskSequencer.add() not called.";

    std::string participantId1 = "participantId1";
    std::string participantId2 = "participantId2";

    types::GlobalDiscoveryEntry globalDiscoveryEntry1 = globalDiscoveryEntry;
    globalDiscoveryEntry1.setParticipantId(participantId1);

    types::GlobalDiscoveryEntry globalDiscoveryEntry2 = globalDiscoveryEntry1;
    globalDiscoveryEntry2.setParticipantId(participantId2);

    std::vector<types::DiscoveryEntry> allGlobalEntries {globalDiscoveryEntry1, globalDiscoveryEntry2};

    EXPECT_CALL(*mockLCDStore,
                getAllGlobalCapabilities())
               .WillOnce(Return(allGlobalEntries));

    EXPECT_CALL(*mockLCDStore,
                getGbidsForParticipantId(Eq(globalDiscoveryEntry1.getParticipantId()), _)).Times(1)
            .WillOnce(Return(gbids));

    EXPECT_CALL(*mockLCDStore,
                getGbidsForParticipantId(Eq(globalDiscoveryEntry2.getParticipantId()), _)).Times(1)
            .WillOnce(Return(gbids));

    std::function<void()> onSuccessOfAddGlobalEntry1;
    std::function<void()> onSuccessOfAddGlobalEntry2;
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry1),
                             Eq(gbids),
                             _, // onSuccess
                             _, // onError
                             _, // onRuntimeError
                             _) // qos
                )
            .WillOnce(DoAll(SaveArg<2>(&onSuccessOfAddGlobalEntry1),
                            SaveArg<5>(&messagingQosCapture1),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry2),
                             Eq(gbids),
                             _, // onSuccess
                             _, // onError
                             _, // onRuntimeError
                             _) // qos
                )
            .WillOnce(DoAll(SaveArg<2>(&onSuccessOfAddGlobalEntry2),
                            SaveArg<5>(&messagingQosCapture2),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    std::shared_ptr<Future<void>> reAddResultFuture = capturedTask._task();

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";

    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture1);
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture2);

    EXPECT_EQ(StatusCodeEnum::IN_PROGRESS, reAddResultFuture->getStatus());
    onSuccessOfAddGlobalEntry1();
    EXPECT_EQ(StatusCodeEnum::IN_PROGRESS, reAddResultFuture->getStatus());
    onSuccessOfAddGlobalEntry2();

    EXPECT_EQ(StatusCodeEnum::SUCCESS, reAddResultFuture->getStatus());
}

TEST_F(GlobalCapabilitiesDirectoryClientTest,
       testReAddTask_mixOfOnSuccessOnError_proxyCalledAndResultFutureResolved) {
    std::unique_ptr<MockTaskSequencer<void>> mockTaskSequencer =
            std::make_unique<MockTaskSequencer<void>>(std::chrono::milliseconds(60000));
    auto mockTaskSequencerRef = mockTaskSequencer.get();
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> gcdClient =
            std::make_shared<GlobalCapabilitiesDirectoryClient>(
                                                  clusterControllerSettings, std::move(mockTaskSequencer));
    gcdClient->setProxy(
            mockGlobalCapabilitiesDirectoryProxy);

    Semaphore semaphore;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture1;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture2;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture3;
    MockTaskSequencer<void>::MockTaskWithExpiryDate capturedTask;
    EXPECT_CALL(*mockTaskSequencerRef, add(_)).Times(1)
            .WillOnce(DoAll(SaveArg<0>(&capturedTask),
                            ReleaseSemaphore(&semaphore)));

    gcdClient->reAdd(mockLCDStore, capSerializedMqttAddress);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "TaskSequencer.add() not called.";

    std::string participantId1 = "participantId1";
    std::string participantId2 = "participantId2";
    std::string participantId3 = "participantId3";

    types::GlobalDiscoveryEntry globalDiscoveryEntry1 = globalDiscoveryEntry;
    globalDiscoveryEntry1.setParticipantId(participantId1);

    types::GlobalDiscoveryEntry globalDiscoveryEntry2 = globalDiscoveryEntry1;
    globalDiscoveryEntry2.setParticipantId(participantId2);

    types::GlobalDiscoveryEntry globalDiscoveryEntry3 = globalDiscoveryEntry2;
    globalDiscoveryEntry3.setParticipantId(participantId3);

    std::vector<types::DiscoveryEntry> allGlobalEntries { globalDiscoveryEntry1,
                                                          globalDiscoveryEntry2,
                                                          globalDiscoveryEntry3 };

    EXPECT_CALL(*mockLCDStore,
                getAllGlobalCapabilities())
               .WillOnce(Return(allGlobalEntries));

    EXPECT_CALL(*mockLCDStore,
                getGbidsForParticipantId(Eq(globalDiscoveryEntry1.getParticipantId()), _)).Times(1)
            .WillOnce(Return(gbids));

    EXPECT_CALL(*mockLCDStore,
                getGbidsForParticipantId(Eq(globalDiscoveryEntry2.getParticipantId()), _)).Times(1)
            .WillOnce(Return(gbids));

    EXPECT_CALL(*mockLCDStore,
                getGbidsForParticipantId(Eq(globalDiscoveryEntry3.getParticipantId()), _)).Times(1)
            .WillOnce(Return(gbids));

    std::function<void()> onSuccessOfAddGlobalEntry1;
    std::function<void(const types::DiscoveryError::Enum&)> onErrorOfAddGlobalEntry2;
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorOfAddGlobalEntry3;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry1),
                             Eq(gbids),
                             _, // onSuccess
                             _, // onError
                             _, // onRuntimeError
                             _) // qos
                )
            .WillOnce(DoAll(SaveArg<2>(&onSuccessOfAddGlobalEntry1),
                            SaveArg<5>(&messagingQosCapture1),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry2),
                             Eq(gbids),
                             _, // onSuccess
                             _, // onError
                             _, // onRuntimeError
                             _) // qos
                )
            .WillOnce(DoAll(SaveArg<3>(&onErrorOfAddGlobalEntry2),
                            SaveArg<5>(&messagingQosCapture2),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry3),
                             Eq(gbids),
                             _, // onSuccess
                             _, // onError
                             _, // onRuntimeError
                             _) // qos
                )
            .WillOnce(DoAll(SaveArg<4>(&onRuntimeErrorOfAddGlobalEntry3),
                            SaveArg<5>(&messagingQosCapture3),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));

    std::shared_ptr<Future<void>> reAddResultFuture = capturedTask._task();

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(15))) << "GCD Proxy not called.";
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(15))) << "GCD Proxy not called.";
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(15))) << "GCD Proxy not called.";

    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture1);
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture2);
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture3);

    EXPECT_EQ(StatusCodeEnum::IN_PROGRESS, reAddResultFuture->getStatus());

    const exceptions::JoynrRuntimeException joynrRuntimeError("Test exception.");
    const types::DiscoveryError::Enum errorEnum(types::DiscoveryError::Enum::INTERNAL_ERROR);

    onSuccessOfAddGlobalEntry1();
    EXPECT_EQ(StatusCodeEnum::IN_PROGRESS, reAddResultFuture->getStatus());
    onErrorOfAddGlobalEntry2(errorEnum);
    EXPECT_EQ(StatusCodeEnum::IN_PROGRESS, reAddResultFuture->getStatus());
    onRuntimeErrorOfAddGlobalEntry3(joynrRuntimeError);

    EXPECT_EQ(StatusCodeEnum::SUCCESS, reAddResultFuture->getStatus());
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testReAddTask_noEntries_resultFutureResolvedAndProxyNotCalled) {
    std::unique_ptr<MockTaskSequencer<void>> mockTaskSequencer =
            std::make_unique<MockTaskSequencer<void>>(std::chrono::milliseconds(60000));
    auto mockTaskSequencerRef = mockTaskSequencer.get();
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> gcdClient =
            std::make_shared<GlobalCapabilitiesDirectoryClient>(
                                                  clusterControllerSettings,
                                                  std::move(mockTaskSequencer));

    gcdClient->setProxy(
            mockGlobalCapabilitiesDirectoryProxy);

    Semaphore semaphore;
    MockTaskSequencer<void>::MockTaskWithExpiryDate capturedTask;
    EXPECT_CALL(*mockTaskSequencerRef, add(_)).Times(1)
            .WillOnce(DoAll(SaveArg<0>(&capturedTask),
                            ReleaseSemaphore(&semaphore)));

    gcdClient->reAdd(mockLCDStore, capSerializedMqttAddress);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "TaskSequencer.add() not called.";

    std::vector<types::DiscoveryEntry> allGlobalEntries {};

    EXPECT_CALL(*mockLCDStore,
                getAllGlobalCapabilities())
               .WillOnce(Return(allGlobalEntries));

    EXPECT_CALL(*mockLCDStore, getGbidsForParticipantId(_, _)).Times(0);

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(_, _, _, _, _, _))
                .Times(0);

    std::shared_ptr<Future<void>> reAddResultFuture = capturedTask._task();

    EXPECT_EQ(StatusCodeEnum::SUCCESS, reAddResultFuture->getStatus());
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testReAddTask_entryWithoutGbids_resultFutureResolved) {
    std::unique_ptr<MockTaskSequencer<void>> mockTaskSequencer =
            std::make_unique<MockTaskSequencer<void>>(std::chrono::milliseconds(60000));
    auto mockTaskSequencerRef = mockTaskSequencer.get();
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> gcdClient =
            std::make_shared<GlobalCapabilitiesDirectoryClient>(
                                                  clusterControllerSettings, std::move(mockTaskSequencer));
    gcdClient->setProxy(
            mockGlobalCapabilitiesDirectoryProxy);

    Semaphore semaphore;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    MockTaskSequencer<void>::MockTaskWithExpiryDate capturedTask;
    EXPECT_CALL(*mockTaskSequencerRef, add(_)).Times(1)
            .WillOnce(DoAll(SaveArg<0>(&capturedTask),
                            ReleaseSemaphore(&semaphore)));

    gcdClient->reAdd(mockLCDStore, capSerializedMqttAddress);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "TaskSequencer.add() not called.";

    std::string participantId1 = "participantId1";
    std::string participantId2 = "participantId2";

    types::GlobalDiscoveryEntry globalDiscoveryEntry1 = globalDiscoveryEntry;
    globalDiscoveryEntry1.setParticipantId(participantId1);

    types::GlobalDiscoveryEntry globalDiscoveryEntry2 = globalDiscoveryEntry1;
    globalDiscoveryEntry2.setParticipantId(participantId2);

    std::vector<types::DiscoveryEntry> allGlobalEntries {globalDiscoveryEntry1, globalDiscoveryEntry2};

    EXPECT_CALL(*mockLCDStore,
                getAllGlobalCapabilities())
               .WillOnce(Return(allGlobalEntries));

    std::vector<std::string> emptyGbids {};
    EXPECT_CALL(*mockLCDStore,
                getGbidsForParticipantId(Eq(globalDiscoveryEntry1.getParticipantId()), _)).Times(1)
            .WillOnce(Return(emptyGbids));

    EXPECT_CALL(*mockLCDStore,
                getGbidsForParticipantId(Eq(globalDiscoveryEntry2.getParticipantId()), _)).Times(1)
            .WillOnce(Return(gbids));

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(_, _, _, _, _, _))
            .Times(0);

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry2),
                             Eq(gbids),
                             _, // onSuccess
                             _, // onError
                             _, // onRuntimeError
                             _) // qos
                )
            .WillOnce(DoAll(SaveArg<2>(&onSuccess),
                            SaveArg<5>(&messagingQosCapture),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));

    std::shared_ptr<Future<void>> reAddResultFuture = capturedTask._task();

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";

    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture);

    onSuccess();

    EXPECT_EQ(StatusCodeEnum::SUCCESS, reAddResultFuture->getStatus());
}

void GlobalCapabilitiesDirectoryClientTest::testAdd(bool awaitGlobalRegistration) {
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, awaitGlobalRegistration, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddWithAwaitGlobalRegistration)
{
    bool awaitGlobalRegistration = true;
    testAdd(awaitGlobalRegistration);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddWithoutAwaitGlobalRegistration)
{
    bool awaitGlobalRegistration = false;
    testAdd(awaitGlobalRegistration);
}

void GlobalCapabilitiesDirectoryClientTest::testAddUsesCorrectRemainingTtl(const bool awaitGlobalRegistration) {
    std::uint64_t expectedTtl;

    expectedTtl = MessagingQos().getTtl();
    std::function<void()> onSuccessCallback;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    Semaphore semaphore;

    types::GlobalDiscoveryEntry globalDiscoveryEntry2(globalDiscoveryEntry);
    globalDiscoveryEntry2.setParticipantId("ParticipantId2");

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<2>(&onSuccessCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry2), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, awaitGlobalRegistration, gbids, onSuccess, onError, onRuntimeError);
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry2, awaitGlobalRegistration, gbids, onSuccess, onError, onRuntimeError);
    std::int64_t firstNow = TimePoint::now().toMilliseconds();
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::int64_t secondNow = TimePoint::now().toMilliseconds();
    std::uint64_t delta = static_cast<std::uint64_t>(secondNow - firstNow);
    onSuccessCallback();
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    auto capturedTtl = messagingQosCapture->getTtl();
    if (awaitGlobalRegistration) {
        EXPECT_TRUE(capturedTtl <= (expectedTtl - delta));
        EXPECT_TRUE(capturedTtl > (expectedTtl - delta - 1000));
    } else {
        EXPECT_EQ(expectedTtl, capturedTtl);
    }
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddUsesCorrectRemainingTtlWithAwaitGlobalRegistration)
{
    const bool awaitGlobalRegistration = true;
    testAddUsesCorrectRemainingTtl(awaitGlobalRegistration);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddUsesCorrectRemainingTtlWithoutAwaitGlobalRegistration)
{
    const bool awaitGlobalRegistration = false;
    testAddUsesCorrectRemainingTtl(awaitGlobalRegistration);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAdd_WithoutAwaitGlobalRegistration_noRetryAfterSuccess)
{
    std::function<void()> onSuccessCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<2>(&onSuccessCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, false, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    onSuccessCallback();
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAdd_WithoutAwaitGlobalRegistration_RetryAfterTimeout)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;
    constexpr unsigned int numberOfTimeouts = 10;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .Times(numberOfTimeouts + 1)
            .WillRepeatedly(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, false, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    const exceptions::JoynrTimeOutException timeoutException("Test timeout");
    for (unsigned int i = 0; i < numberOfTimeouts; i++) {
        onRuntimeErrorCallback(timeoutException);
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "No retry after timeout number "
                                                                 << i + 1 << ".";
    }
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAdd_WithAwaitGlobalRegistration_noRetryAfterTimeout)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, true, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    const exceptions::JoynrTimeOutException timeoutException("Test timeout");
    onRuntimeErrorCallback(timeoutException);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAdd_WithoutAwaitGlobalRegistration_noRetryAfterRuntimeException)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, false, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    onRuntimeErrorCallback(exceptions::JoynrRuntimeException("Some runtime exception"));
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAdd_WithoutAwaitGlobalRegistration_noRetryAfterApplicationException)
{
    std::function<void(const types::DiscoveryError::Enum&)> onApplicationErrorCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<3>(&onApplicationErrorCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, false, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    onApplicationErrorCallback(types::DiscoveryError::Enum::UNKNOWN_GBID);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testLookupDomainInterface)
{
    std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccessForLookup =
            [](const std::vector<types::GlobalDiscoveryEntry>& /*result*/) {};
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    const std::vector<std::string> domains = {capDomain};
    auto mockLookupFuture =
            std::make_shared<joynr::Future<std::vector<joynr::types::GlobalDiscoveryEntry>>>();

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                lookupAsyncMock(Eq(domains), Eq(capInterface), Eq(gbids), _, _, _, _))
            .WillRepeatedly(DoAll(SaveArg<6>(&messagingQosCapture), Return(mockLookupFuture)));

    globalCapabilitiesDirectoryClient->lookup(domains,
                                              capInterface,
                                              gbids,
                                              messagingTtl,
                                              onSuccessForLookup,
                                              onError,
                                              onRuntimeError);
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture);
    ASSERT_EQ(messagingTtl, messagingQosCapture->getTtl());
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testLookupParticipantId)
{
    std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccessForLookup =
            [](const std::vector<types::GlobalDiscoveryEntry>& /*result*/) {};
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    auto mockLookupFuture = std::make_shared<joynr::Future<joynr::types::GlobalDiscoveryEntry>>();

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                lookupAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillRepeatedly(DoAll(SaveArg<5>(&messagingQosCapture), Return(mockLookupFuture)));
    globalCapabilitiesDirectoryClient->lookup(
            capParticipantId, gbids, messagingTtl, onSuccessForLookup, onError, onRuntimeError);
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture);
    ASSERT_EQ(messagingTtl, messagingQosCapture->getTtl());
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemove)
{
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    Semaphore semaphore;

    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(1)
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Return(gbids)));

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));

    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveTaskExpiryDateHasCorrectValue)
{
    std::unique_ptr<MockTaskSequencer<void>> mockTaskSequencer =
            std::make_unique<MockTaskSequencer<void>>(std::chrono::milliseconds(60000));
    auto mockTaskSequencerRef = mockTaskSequencer.get();
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> gcdClient =
            std::make_shared<GlobalCapabilitiesDirectoryClient>(
                                                  clusterControllerSettings, std::move(mockTaskSequencer));
    Semaphore semaphore;
    MockTaskSequencer<void>::MockTaskWithExpiryDate capturedTask;
    TimePoint expectedTaskExpiryDate = TimePoint::max();

    EXPECT_CALL(*mockTaskSequencerRef, add(_)).Times(1)
            .WillOnce(DoAll(SaveArg<0>(&capturedTask),
                            ReleaseSemaphore(&semaphore)));
    gcdClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "TaskSequencer.add() not called.";
    auto actualTaskExpiryDate = capturedTask._expiryDate;
    ASSERT_EQ(expectedTaskExpiryDate, actualTaskExpiryDate);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveUsesCorrectTtl)
{
    std::uint64_t defaultTtl = MessagingQos().getTtl();
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    Semaphore semaphore;

    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(1)
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Return(gbids)));

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    EXPECT_EQ(defaultTtl, messagingQosCapture->getTtl());
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveNoRetryAfterSuccess)
{
    std::function<void()> onSuccessCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(1)
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Return(gbids)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<2>(&onSuccessCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onSuccessCallback();
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveRetryAfterTimeout)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;
    constexpr unsigned int numberOfTimeouts = 10;

    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(numberOfTimeouts + 1)
            .WillRepeatedly(DoAll(ReleaseSemaphore(&semaphore),
                            Return(gbids)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .Times(numberOfTimeouts + 1)
            .WillRepeatedly(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                                  ReleaseSemaphore(&semaphore),
                                  Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    const exceptions::JoynrTimeOutException timeoutException("Test timeout");
    for (unsigned int i = 0; i < numberOfTimeouts; i++) {
        onRuntimeErrorCallback(timeoutException);
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "No retry after timeout number "
                                                                 << i + 1 << ".";
    }
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveNoRetryAfterRuntimeException)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(1)
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Return(gbids)));

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onRuntimeErrorCallback(exceptions::JoynrRuntimeException("Some runtime exception"));
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveNoRetryAfterApplicationException)
{
    std::function<void(const types::DiscoveryError::Enum&)> onApplicationErrorCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(1)
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Return(gbids)));

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<3>(&onApplicationErrorCallback),
                            ReleaseSemaphore(&semaphore),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onApplicationErrorCallback(types::DiscoveryError::Enum::UNKNOWN_GBID);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveDeletionWhileRetry)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;
    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(2)
            .WillRepeatedly(DoAll(ReleaseSemaphore(&semaphore),
                            Return(gbids)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .Times(2)
            .WillRepeatedly(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                                  ReleaseSemaphore(&semaphore),
                                  Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onRuntimeErrorCallback(exceptions::JoynrTimeOutException("Test timeout"));
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveParticipantNotRegisteredNoGbidsGcdNotCalled)
{
    // Init empty list of gbids
    std::vector<std::string> expectedGbids;
    Semaphore lcdStoreSemaphore;

    EXPECT_CALL(*mockLocalCapabilitiesDirectoryStore,
                    getGbidsForParticipantId(Eq(capParticipantId), _)).Times(1)
            .WillOnce(DoAll(ReleaseSemaphore(&lcdStoreSemaphore),
                            Return(expectedGbids)));

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _)).Times(0);

    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, mockLocalCapabilitiesDirectoryStore, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(lcdStoreSemaphore.waitFor(std::chrono::seconds(10)));
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testTouch)
{
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    const std::string clusterControllerId = "dummyClustercontrollerId";
    const std::string gbid = "dummyGbid";
    const std::vector<std::string> participantIds = {"dummyParticipantId"};

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                touchAsyncMock(Eq(clusterControllerId), Eq(participantIds), _, _, _))
            .WillRepeatedly(DoAll(SaveArg<4>(&messagingQosCapture), Return(mockFuture)));

    globalCapabilitiesDirectoryClient->touch(
            clusterControllerId, participantIds, gbid, onSuccess, onRuntimeError);
    ASSERT_EQ(clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs().count(),
              messagingQosCapture->getTtl());

    const auto foundGbidMessageHeader = messagingQosCapture->getCustomMessageHeaders().find(
            joynr::Message::CUSTOM_HEADER_GBID_KEY());
    ASSERT_TRUE(foundGbidMessageHeader != messagingQosCapture->getCustomMessageHeaders().cend());
    ASSERT_EQ(gbid, foundGbidMessageHeader->second);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveStale)
{
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    std::string clusterControllerId = "dummyClustercontrollerId";
    const std::int64_t maxLastSeenMs = 100000.0;
    const std::int64_t expectedTtl = 60000.0;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeStaleAsyncMock(Eq(clusterControllerId), Eq(maxLastSeenMs), _, _, _))
            .Times(1)
            .WillOnce(DoAll(SaveArg<4>(&messagingQosCapture), Return(mockFuture)));

    globalCapabilitiesDirectoryClient->removeStale(
            clusterControllerId, maxLastSeenMs, gbids[0], onSuccess, onRuntimeError);
    ASSERT_EQ(expectedTtl, messagingQosCapture->getTtl());

    auto messageHeaders = messagingQosCapture->getCustomMessageHeaders();
    auto gbidEntry = messageHeaders.find(joynr::Message::CUSTOM_HEADER_GBID_KEY());
    ASSERT_NE(gbidEntry, messageHeaders.end());
    ASSERT_EQ(gbids[0], gbidEntry->second);
}

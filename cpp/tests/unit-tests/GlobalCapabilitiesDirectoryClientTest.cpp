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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Future.h"
#include "joynr/Message.h"
#include "joynr/MessagingSettings.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/Settings.h"
#include "libjoynrclustercontroller/capabilities-directory/GlobalCapabilitiesDirectoryClient.h"
#include "tests/mock/MockGlobalCapabilitiesDirectoryProxy.h"
#include "tests/mock/MockJoynrRuntime.h"
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
              globalCapabilitiesDirectoryClient(std::make_shared<GlobalCapabilitiesDirectoryClient>(
                      clusterControllerSettings, std::move(taskSequencer))),
              capDomain("testDomain"),
              capInterface("testInterface"),
              capParticipantId("testParticipantId"),
              capPublicKeyId("publicKeyId"),
              capLastSeenMs(0),
              capExpiryDateMs(1000),
              capSerializedChannelAddress("testChannelId"),
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
                                   capSerializedChannelAddress),
              mockFuture(std::make_shared<joynr::Future<void>>()),
              onSuccess([]() {}),
              onError([](const types::DiscoveryError::Enum& /*error*/) {}),
              onRuntimeError([](const exceptions::JoynrRuntimeException& /*error*/) {})
    {
    }

    ADD_LOGGER(GlobalCapabilitiesDirectoryClientTest)
    std::unique_ptr<Settings> settings;
    MessagingSettings messagingSettings;
    ClusterControllerSettings clusterControllerSettings;
    std::unique_ptr<TaskSequencer<void>> taskSequencer;
    std::shared_ptr<MockJoynrRuntime> mockJoynrRuntime;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    std::shared_ptr<JoynrMessagingConnectorFactory> joynrMessagingConnectorFactory;
    std::shared_ptr<MockGlobalCapabilitiesDirectoryProxy> mockGlobalCapabilitiesDirectoryProxy;
    std::shared_ptr<GlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient;

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
    const std::string capDomain;
    const std::string capInterface;
    const std::string capParticipantId;
    const std::string capPublicKeyId;
    const std::int64_t capLastSeenMs;
    const std::int64_t capExpiryDateMs;
    const std::string capSerializedChannelAddress;
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

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAdd)
{
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testAddUsesCorrectRemainingTtl)
{
    std::uint64_t defaultTtl = MessagingQos().getTtl();
    std::function<void()> onSuccessCallback;
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    Semaphore semaphore;

    types::GlobalDiscoveryEntry globalDiscoveryEntry2(globalDiscoveryEntry);
    globalDiscoveryEntry2.setParticipantId("ParticipantId2");

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<2>(&onSuccessCallback),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                addAsyncMock(Eq(globalDiscoveryEntry2), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry, gbids, onSuccess, onError, onRuntimeError);
    globalCapabilitiesDirectoryClient->add(
            globalDiscoveryEntry2, gbids, onSuccess, onError, onRuntimeError);
    std::int64_t firstNow = TimePoint::now().toMilliseconds();
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::int64_t secondNow = TimePoint::now().toMilliseconds();
    std::uint64_t delta = static_cast<std::uint64_t>(secondNow - firstNow);
    onSuccessCallback();
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "GCD Proxy not called.";
    auto capturedTtl = messagingQosCapture->getTtl();
    EXPECT_TRUE(capturedTtl <= (defaultTtl - delta));
    EXPECT_TRUE(capturedTtl > (defaultTtl - delta - 1000));
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

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    testMessagingQosForCustomHeaderGbidKey(gbids[0], messagingQosCapture);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveUsesCorrectTtl)
{
    std::uint64_t defaultTtl = MessagingQos().getTtl();
    std::shared_ptr<joynr::MessagingQos> messagingQosCapture;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<5>(&messagingQosCapture),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    EXPECT_EQ(defaultTtl, messagingQosCapture->getTtl());
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveNoRetryAfterSuccess)
{
    std::function<void()> onSuccessCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<2>(&onSuccessCallback),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onSuccessCallback();
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveRetryAfterTimeout)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;
    constexpr unsigned int numberOfTimeouts = 10;
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .Times(numberOfTimeouts + 1)
            .WillRepeatedly(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                                  InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                                  Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    const exceptions::JoynrTimeOutException timeoutException("Test timeout");
    for (unsigned int i = 0; i < numberOfTimeouts; i++) {
        onRuntimeErrorCallback(timeoutException);
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10))) << "No retry after timeout number "
                                                                 << i + 1 << ".";
    }
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveNoRetryAfterRuntimeException)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onRuntimeErrorCallback(exceptions::JoynrRuntimeException("Some runtime exception"));
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveNoRetryAfterApplicationException)
{
    std::function<void(const types::DiscoveryError::Enum&)> onApplicationErrorCallback;
    Semaphore semaphore;

    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .WillOnce(DoAll(SaveArg<3>(&onApplicationErrorCallback),
                            InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                            Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onApplicationErrorCallback(types::DiscoveryError::Enum::UNKNOWN_GBID);
}

TEST_F(GlobalCapabilitiesDirectoryClientTest, testRemoveDeletionWhileRetry)
{
    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeErrorCallback;
    Semaphore semaphore;
    EXPECT_CALL(*mockGlobalCapabilitiesDirectoryProxy,
                removeAsyncMock(Eq(capParticipantId), Eq(gbids), _, _, _, _))
            .Times(2)
            .WillRepeatedly(DoAll(SaveArg<4>(&onRuntimeErrorCallback),
                                  InvokeWithoutArgs(&semaphore, &Semaphore::notify),
                                  Return(mockFuture)));
    globalCapabilitiesDirectoryClient->remove(
            capParticipantId, gbids, onSuccess, onError, onRuntimeError);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(10)));
    onRuntimeErrorCallback(exceptions::JoynrTimeOutException("Test timeout"));
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
    const std::int64_t expectedTtl = 3600000.0;

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

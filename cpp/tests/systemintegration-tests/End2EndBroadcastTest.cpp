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

#include "End2EndBroadcastTestBase.cpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>

#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Future.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/tests/testProxy.h"
#include "joynr/Settings.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockGpsFloatSubscriptionListener.h"

using namespace ::testing;
using namespace joynr;

namespace joynr
{

class End2EndBroadcastTest : public End2EndBroadcastTestBase
{
public:
    End2EndBroadcastTest() : End2EndBroadcastTestBase()
    {
    }

protected:
    void testMulticastWithWildcards(std::shared_ptr<MyTestProvider> testProvider,
                                    std::shared_ptr<tests::testProxy> testProxy)
    {
        std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener =
                std::make_shared<MockGpsSubscriptionListener>();

        std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>> partitionsToTest{
                {{"partition0", "+", "partition2"}, {"partition0", "partition1", "partition2"}},
                {{"+", "partition1"}, {"partition0", "partition1"}},
                {{"+", "partition1", "+", "partition3"},
                 {"partition0", "partition1", "partition2", "partition3"}},
                {{"+", "*"}, {"partition0", "partition1", "partition2", "partition3"}},
                {{"+", "partition1"}, {"partition0", "partition1"}},
                {{"+", "+", "+", "*"}, {"partition0", "partition1", "partition2", "partition3"}},
        };

        EXPECT_CALL(*mockSubscriptionListener, onReceive(_))
                .Times(partitionsToTest.size())
                .WillRepeatedly(ReleaseSemaphore(&semaphore));

        auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
        subscriptionQos->setValidityMs(500000);

        for (const auto& partitions : partitionsToTest) {
            auto subscriptionIdFuture = testProxy->subscribeToLocationUpdateBroadcast(
                    mockSubscriptionListener, subscriptionQos, partitions.first);

            std::string subscriptionId;
            JOYNR_ASSERT_NO_THROW(subscriptionIdFuture->get(5000, subscriptionId));

            delayForMqttSubscribeOrUnsubscribe();

            testProvider->fireLocationUpdate(gpsLocation2, partitions.second);
            std::stringstream subscriptionPartitions;
            std::copy(partitions.first.begin(),
                      partitions.first.end(),
                      std::ostream_iterator<std::string>(subscriptionPartitions, " "));
            std::stringstream firePartitions;
            std::copy(partitions.second.begin(),
                      partitions.second.end(),
                      std::ostream_iterator<std::string>(firePartitions, " "));
            EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(5)))
                    << "Partitions used for subscription: " + subscriptionPartitions.str()
                    << std::endl << "Partitions used for fire broadcast" + firePartitions.str();

            testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId);

            delayForMqttSubscribeOrUnsubscribe();
        }
    }

    void testMulticastWithWildcardsAndTwoProxies(std::shared_ptr<MyTestProvider> testProvider,
                                                 std::shared_ptr<tests::testProxy> testProxy1,
                                                 std::shared_ptr<tests::testProxy> testProxy2)
    {
        std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener1 =
                std::make_shared<MockGpsSubscriptionListener>();

        std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener2 =
                std::make_shared<MockGpsSubscriptionListener>();

        // Will not receive any publication
        EXPECT_CALL(*mockSubscriptionListener1, onReceive(_)).Times(0);

        // Shall receive a publication
        EXPECT_CALL(*mockSubscriptionListener2, onReceive(_)).Times(1).WillOnce(
                ReleaseSemaphore(&semaphore));

        auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
        subscriptionQos->setValidityMs(500000);

        auto subscriptionIdFuture1 = testProxy1->subscribeToLocationUpdateBroadcast(
                mockSubscriptionListener1, subscriptionQos, {"partition0", "+", "partition2"});

        auto subscriptionIdFuture2 = testProxy2->subscribeToLocationUpdateBroadcast(
                mockSubscriptionListener2, subscriptionQos, {"partition0", "*"});

        std::string subscriptionId1, subscriptionId2;

        subscriptionIdFuture1->get(5000, subscriptionId1);
        subscriptionIdFuture2->get(5000, subscriptionId2);

        delayForMqttSubscribeOrUnsubscribe();

        testProvider->fireLocationUpdate(gpsLocation2, {"partition0", "partition1", "partitionX"});

        EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(5)));

        testProxy1->unsubscribeFromLocationUpdateBroadcast(subscriptionId1);
        testProxy2->unsubscribeFromLocationUpdateBroadcast(subscriptionId2);

        delayForMqttSubscribeOrUnsubscribe();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndBroadcastTest);
};

} // namespace joynr

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithEnumOutput)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }
    tests::testTypes::TestEnum::Enum expectedTestEnum = tests::testTypes::TestEnum::TWO;

    testOneShotBroadcastSubscription(
            expectedTestEnum,
            [this](tests::testProxy* testProxy,
                   std::shared_ptr<ISubscriptionListener<tests::testTypes::TestEnum::Enum>>
                           subscriptionListener,
                   std::shared_ptr<MulticastSubscriptionQos> subscriptionQos,
                   std::string& subscriptionId) {
                std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                        testProxy->subscribeToBroadcastWithEnumOutputBroadcast(
                                subscriptionListener, subscriptionQos);
                JOYNR_EXPECT_NO_THROW(
                        subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));
            },
            [this](tests::testProxy* testProxy, std::string& subscriptionId) {
                testProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
            },
            &tests::testProvider::fireBroadcastWithEnumOutput);
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithByteBufferParameter)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }
    joynr::ByteBuffer expectedByteBuffer{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    testOneShotBroadcastSubscription(
            expectedByteBuffer,
            [this](tests::testProxy* testProxy,
                   std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer>> subscriptionListener,
                   std::shared_ptr<MulticastSubscriptionQos> subscriptionQos,
                   std::string& subscriptionId) {
                std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                        testProxy->subscribeToBroadcastWithByteBufferParameterBroadcast(
                                subscriptionListener, subscriptionQos);
                JOYNR_EXPECT_NO_THROW(
                        subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));
            },
            [this](tests::testProxy* testProxy, std::string& subscriptionId) {
                testProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
            },
            &tests::testProvider::fireBroadcastWithByteBufferParameter);
}

TEST_P(End2EndBroadcastTest, updateBroadcastSubscription)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener =
            std::make_shared<MockGpsSubscriptionListener>();
    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener2 =
            std::make_shared<MockGpsSubscriptionListener>();

    EXPECT_CALL(*mockSubscriptionListener, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockSubscriptionListener2, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&altSemaphore));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    std::shared_ptr<Future<std::string>> future = testProxy->subscribeToLocationUpdateBroadcast(
            mockSubscriptionListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW(future->get(5000, subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();

    testProvider->fireLocationUpdate(gpsLocation2);

    // make sure the fireLocationUpdate is received by the first listener
    // before updating the subscription
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // update subscription, new listener
    future = testProxy->subscribeToLocationUpdateBroadcast(
            subscriptionId, mockSubscriptionListener2, subscriptionQos);
    JOYNR_ASSERT_NO_THROW(future->get(5000, subscriptionId));

    testProvider->fireLocationUpdate(gpsLocation2);

    // make sure the fireLocationUpdate is received by the second listener
    ASSERT_TRUE(altSemaphore.waitFor(std::chrono::seconds(3)));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, subscribeToSameBroadcastTwice)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    auto mockSubscriptionListener = std::make_shared<MockGpsSubscriptionListener>();
    auto mockSubscriptionListener2 = std::make_shared<MockGpsSubscriptionListener>();

    EXPECT_CALL(*mockSubscriptionListener, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockSubscriptionListener2, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&altSemaphore));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    std::shared_ptr<Future<std::string>> future = testProxy->subscribeToLocationUpdateBroadcast(
            mockSubscriptionListener, subscriptionQos);

    // second subscription
    std::shared_ptr<Future<std::string>> future2 = testProxy->subscribeToLocationUpdateBroadcast(
            mockSubscriptionListener2, subscriptionQos);

    std::string subscriptionId1, subscriptionId2;
    JOYNR_ASSERT_NO_THROW(future->get(5000, subscriptionId1));
    JOYNR_ASSERT_NO_THROW(future2->get(5000, subscriptionId2));

    delayForMqttSubscribeOrUnsubscribe();

    testProvider->fireLocationUpdate(gpsLocation2);

    // make sure the fireLocationUpdate is received by the first listener
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    // make sure the fireLocationUpdate is received by the second listener
    ASSERT_TRUE(altSemaphore.waitFor(std::chrono::seconds(3)));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId1));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId2));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, subscribeAndUnsubscribeFromBroadcast_OneOutput)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> subscriptionListener(
            std::make_shared<MockGpsSubscriptionListener>());

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    auto future =
            testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW(future->get(5000, subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();

    EXPECT_CALL(*subscriptionListener, onReceive(Eq(gpsLocation2))).Times(1).WillOnce(
            ReleaseSemaphore(&semaphore));

    testProvider->fireLocationUpdate(gpsLocation2);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId);

    EXPECT_CALL(*subscriptionListener, onReceive(Eq(gpsLocation3))).Times(0);
    testProvider->fireLocationUpdate(gpsLocation3);

    // ensure to wait long enough before ending
    std::this_thread::sleep_for(std::chrono::seconds(3));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_OneOutput)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> mockListener =
            std::make_shared<MockGpsSubscriptionListener>();

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToLocationUpdateBroadcast(mockListener, subscriptionQos);
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2))).Times(1);
    testProvider->fireLocationUpdate(gpsLocation2);

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3))).Times(1);
    testProvider->fireLocationUpdate(gpsLocation3);

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4))).Times(1).WillOnce(
            ReleaseSemaphore(&semaphore));
    testProvider->fireLocationUpdate(gpsLocation4);

    // ensure to wait long enough before ending
    std::this_thread::sleep_for(std::chrono::seconds(3));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, waitForSuccessfulSubscriptionRegistration)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    std::string subscriptionIdFromListener;
    std::string subscriptionIdFromFuture;

    std::shared_ptr<MockGpsSubscriptionListener> mockListener =
            std::make_shared<MockGpsSubscriptionListener>();

    EXPECT_CALL(*mockListener, onSubscribed(_))
            .WillOnce(DoAll(SaveArg<0>(&subscriptionIdFromListener), ReleaseSemaphore(&semaphore)));

    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToLocationUpdateBroadcast(mockListener, subscriptionQos);

    JOYNR_EXPECT_NO_THROW(
            subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionIdFromFuture));

    delayForMqttSubscribeOrUnsubscribe();

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);
    JOYNR_ASSERT_NO_THROW(
            testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionIdFromFuture));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, waitForSuccessfulSubscriptionUpdate)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> mockListener =
            std::make_shared<MockGpsSubscriptionListener>();

    // Use a semaphore to count and wait on calls to the mock listener
    std::string initialSubscriptionIdFromListener;
    std::string updateSubscriptionIdFromListener;
    std::string initialSubscriptionIdFromFuture;
    std::string updateSubscriptionIdFromFuture;
    EXPECT_CALL(*mockListener, onSubscribed(_))
            .WillOnce(DoAll(
                    SaveArg<0>(&initialSubscriptionIdFromListener), ReleaseSemaphore(&semaphore)))
            .WillOnce(DoAll(
                    SaveArg<0>(&updateSubscriptionIdFromListener), ReleaseSemaphore(&semaphore)));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToLocationUpdateBroadcast(mockListener, subscriptionQos);
    // the sequence of calling the onReceive listener and the future resolve is not guaranteed
    JOYNR_EXPECT_NO_THROW(
            subscriptionIdFuture->get(subscribeToBroadcastWait, initialSubscriptionIdFromFuture));

    delayForMqttSubscribeOrUnsubscribe();

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    EXPECT_EQ(initialSubscriptionIdFromListener, initialSubscriptionIdFromFuture);

    // update subscription
    subscriptionIdFuture = nullptr;
    subscriptionIdFuture = testProxy->subscribeToLocationUpdateBroadcast(
            initialSubscriptionIdFromListener, mockListener, subscriptionQos);

    // the sequence of calling the onReceive listener and the future resolve is not guaranteed
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(5000, updateSubscriptionIdFromFuture));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    EXPECT_EQ(updateSubscriptionIdFromListener, updateSubscriptionIdFromFuture);
    // subscription id from update is the same as the original subscription id
    EXPECT_EQ(initialSubscriptionIdFromListener, updateSubscriptionIdFromListener);
    JOYNR_ASSERT_NO_THROW(
            testProxy->unsubscribeFromLocationUpdateBroadcast(initialSubscriptionIdFromListener));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_EmptyOutput)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockSubscriptionListenerZeroTypes> mockListener =
            std::make_shared<MockSubscriptionListenerZeroTypes>();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive()).WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToEmptyBroadcastBroadcast(mockListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(
            subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();

    testProvider->fireEmptyBroadcast();

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    testProvider->fireEmptyBroadcast();
    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    testProvider->fireEmptyBroadcast();

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromEmptyBroadcastBroadcast(subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_MultipleOutput)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    MockGpsFloatSubscriptionListener* mockListener = new MockGpsFloatSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2), Eq(100)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3), Eq(200)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4), Eq(300)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation, float>>
            subscriptionListener(mockListener);

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToLocationUpdateWithSpeedBroadcast(
                    subscriptionListener, subscriptionQos);
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(
            subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();

    // Change the location 3 times

    testProvider->fireLocationUpdateWithSpeed(gpsLocation2, 100);

    //     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    testProvider->fireLocationUpdateWithSpeed(gpsLocation3, 200);
    //     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    testProvider->fireLocationUpdateWithSpeed(gpsLocation4, 300);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    JOYNR_ASSERT_NO_THROW(
            testProxy->unsubscribeFromLocationUpdateWithSpeedBroadcast(subscriptionId));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, subscribeToAttributeWithoutProvider)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }
    std::int64_t receiveBroadcastWait = 2000;

    std::shared_ptr<MockGpsSubscriptionListener> mockListenerAttribute =
            std::make_shared<MockGpsSubscriptionListener>();

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    unregisterProvider();

    std::int64_t minInterval_ms = 50;
    std::int64_t publicationTtl = 50;
    auto subscriptionQosAttribute =
            std::make_shared<OnChangeSubscriptionQos>(500000, // validity_ms
                                                      publicationTtl,
                                                      minInterval_ms); // minInterval_ms

    // Initial attribute publication on subscription
    EXPECT_CALL(*mockListenerAttribute, onReceive(Eq(gpsLocation))).Times(1);

    std::shared_ptr<joynr::Future<std::string>> subscriptionAttributeResult =
            testProxy->subscribeToLocation(mockListenerAttribute, subscriptionQosAttribute);
    std::string subscriptionIdAttribute;
    EXPECT_THROW(
            subscriptionAttributeResult->get(subscribeToAttributeWait, subscriptionIdAttribute),
            joynr::exceptions::JoynrTimeOutException);

    // Register same provider again
    testProvider = registerProvider();

    JOYNR_EXPECT_NO_THROW(
            subscriptionAttributeResult->get(subscribeToAttributeWait, subscriptionIdAttribute));

    // Expect to get notified if attribute changed
    EXPECT_CALL(*mockListenerAttribute, onReceive(Eq(gpsLocation2)))
            .WillOnce(ReleaseSemaphore(&semaphore));
    testProvider->locationChanged(gpsLocation2);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));

    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocation(subscriptionIdAttribute));
}

TEST_P(End2EndBroadcastTest, subscribeToSameBroadcastWithDifferentPartitions)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }
    std::int64_t receiveBroadcastWait = 2000;

    std::vector<std::string> partitions1{"partition1", "partition2"};
    std::vector<std::string> partitions2{"partition1", "partition2", "partition3"};

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener1 =
            std::make_shared<MockGpsSubscriptionListener>();
    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener2 =
            std::make_shared<MockGpsSubscriptionListener>();

    // Use a semaphore to count and wait on calls to the mock listener
    // we expect to notifications before updating the subscription
    // on the second call we release the sync semaphore
    EXPECT_CALL(*mockSubscriptionListener1, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockSubscriptionListener2, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&altSemaphore));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    auto subscriptionIdFuture1 = testProxy->subscribeToLocationUpdateBroadcast(
            mockSubscriptionListener1, subscriptionQos, partitions1);
    auto subscriptionIdFuture2 = testProxy->subscribeToLocationUpdateBroadcast(
            mockSubscriptionListener2, subscriptionQos, partitions2);

    std::string subscriptionId1, subscriptionId2;
    JOYNR_ASSERT_NO_THROW(subscriptionIdFuture1->get(subscribeToBroadcastWait, subscriptionId1));
    JOYNR_ASSERT_NO_THROW(subscriptionIdFuture2->get(subscribeToBroadcastWait, subscriptionId2));

    delayForMqttSubscribeOrUnsubscribe();

    // fire broadcast without partitions (should not be received by any subscriber)
    testProvider->fireLocationUpdate(gpsLocation2);
    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));
    EXPECT_FALSE(altSemaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // fire broadcast for subscriber 1 (should not be received by subscriber 2)
    testProvider->fireLocationUpdate(gpsLocation2, partitions1);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));
    EXPECT_FALSE(altSemaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));

    // Waiting between broadcast is neccessary because otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // fire broadcast for subscriber 2 (should not be received by subscriber 1)
    testProvider->fireLocationUpdate(gpsLocation2, partitions2);

    EXPECT_TRUE(altSemaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));
    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId1));
    JOYNR_ASSERT_NO_THROW(testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId2));

    delayForMqttSubscribeOrUnsubscribe();
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithWildcards)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider(runtime1);
    std::shared_ptr<tests::testProxy> testProxy = buildProxy(runtime2);

    testMulticastWithWildcards(testProvider, testProxy);
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithWildcards_InProcess)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider(runtime1);
    std::shared_ptr<tests::testProxy> testProxy = buildProxy(runtime1);

    testMulticastWithWildcards(testProvider, testProxy);
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithWildcards_TwoProxies)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider(runtime1);
    std::shared_ptr<tests::testProxy> testProxy1 = buildProxy(runtime2);
    std::shared_ptr<tests::testProxy> testProxy2 = buildProxy(runtime2);

    testMulticastWithWildcardsAndTwoProxies(testProvider, testProxy1, testProxy2);
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithWildcards_TwoProxiesInProcess)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider(runtime1);
    std::shared_ptr<tests::testProxy> testProxy1 = buildProxy(runtime1);
    std::shared_ptr<tests::testProxy> testProxy2 = buildProxy(runtime1);

    testMulticastWithWildcardsAndTwoProxies(testProvider, testProxy1, testProxy2);
}

TEST_P(End2EndBroadcastTest, publishBroadcastWithInvalidPartitions)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    EXPECT_THROW(testProvider->fireLocationUpdate(gpsLocation2, {"partition0", "partition1", "*"}),
                 std::invalid_argument);
}

TEST_P(End2EndBroadcastTest, sendBroadcastMessageOnlyOnceIfMultipleProxiesAreOnSameRuntime)
{
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy1 = buildProxy();
    std::shared_ptr<tests::testProxy> testProxy2 = buildProxy();

    const std::vector<std::string> partitions{"partition1", "partition2"};

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener1 =
            std::make_shared<MockGpsSubscriptionListener>();
    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener2 =
            std::make_shared<MockGpsSubscriptionListener>();

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(500000);

    auto subscriptionIdFuture1 = testProxy1->subscribeToLocationUpdateBroadcast(
            mockSubscriptionListener1, subscriptionQos, partitions);

    auto subscriptionIdFuture2 = testProxy2->subscribeToLocationUpdateBroadcast(
            mockSubscriptionListener2, subscriptionQos, partitions);

    std::string subscriptionId1, subscriptionId2;
    JOYNR_ASSERT_NO_THROW(subscriptionIdFuture1->get(subscribeToBroadcastWait, subscriptionId1));
    JOYNR_ASSERT_NO_THROW(subscriptionIdFuture2->get(subscribeToBroadcastWait, subscriptionId2));

    delayForMqttSubscribeOrUnsubscribe();

    // Each proxy will receive a message exactly one time
    EXPECT_CALL(*mockSubscriptionListener1, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockSubscriptionListener2, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&altSemaphore));

    testProvider->fireLocationUpdate(gpsLocation2, partitions);

    const std::int64_t receiveBroadcastWait = 2000;
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));
    EXPECT_TRUE(altSemaphore.waitFor(std::chrono::milliseconds(receiveBroadcastWait)));
    JOYNR_ASSERT_NO_THROW(testProxy1->unsubscribeFromLocationUpdateBroadcast(subscriptionId1));
    JOYNR_ASSERT_NO_THROW(testProxy2->unsubscribeFromLocationUpdateBroadcast(subscriptionId2));

    delayForMqttSubscribeOrUnsubscribe();
}

using namespace std::literals;

INSTANTIATE_TEST_CASE_P(
        DISABLED_Http,
        End2EndBroadcastTest,
        testing::Values(std::make_tuple("test-resources/HttpSystemIntegrationTest1.settings"s,
                                        "test-resources/HttpSystemIntegrationTest2.settings"s)));

INSTANTIATE_TEST_CASE_P(
        Mqtt,
        End2EndBroadcastTest,
        testing::Values(std::make_tuple("test-resources/MqttSystemIntegrationTest1.settings"s,
                                        "test-resources/MqttSystemIntegrationTest2.settings"s)));

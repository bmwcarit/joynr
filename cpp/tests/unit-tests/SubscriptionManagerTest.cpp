/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <gmock/gmock.h>
#include <memory>
#include "joynr/SubscriptionManager.h"
#include "joynr/ISubscriptionCallback.h"
#include "tests/utils/MockObjects.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/SingleThreadedDelayedScheduler.h"
#include "joynr/Runnable.h"
#include "tests/utils/TimeUtils.h"
#include "joynr/Logger.h"
#include "joynr/Directory.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Util.h"
#include <chrono>
#include <cstdint>
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Future.h"
#include "joynr/MulticastSubscriptionRequest.h"

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;

MATCHER_P(publicationMissedException, subscriptionId, "") {
    if (arg.getTypeName() == joynr::exceptions::PublicationMissedException::TYPE_NAME()) {
        joynr::exceptions::PublicationMissedException *errorArg = dynamic_cast<joynr::exceptions::PublicationMissedException*>(arg.clone());
        bool success = errorArg->getSubscriptionId() == subscriptionId && errorArg->getMessage() == subscriptionId;
        delete errorArg;
        return success;
    }
    return false;
}

using namespace joynr;

class SubscriptionManagerTest : public testing::Test
{
public:
    SubscriptionManagerTest() : singleThreadedIOService()
    {
        singleThreadedIOService.start();
    }
protected:
    SingleThreadedIOService singleThreadedIOService;
};

class SubscriptionManagerMulticastTest : public testing::Test
{
public:
    SubscriptionManagerMulticastTest() :
        subscribeToName("subscribeToName"),
        subscriberParticipantId("subscriberParticipantId"),
        providerParticipantId1("providerParticipantId"),
        partitions({ "partition1", "partition2" }),
        multicastId1("providerParticipantId/subscribeToName/partition1/partition2"),
        mockMessageRouter(std::make_shared<MockMessageRouter>(singleThreadedIOService.getIOService())),
        mockGpsSubscriptionListener(std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>()),
        qos(std::make_shared<OnChangeSubscriptionQos>()),
        future(std::make_shared<Future<std::string>>()),
        subscriptionManager(singleThreadedIOService.getIOService(), mockMessageRouter),
        subscriptionCallback(std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(
            mockGpsSubscriptionListener, "testSubscriptionId", future, &subscriptionManager))
    {
    }

protected:
    SingleThreadedIOService singleThreadedIOService;

    const std::string subscribeToName;
    const std::string subscriberParticipantId;
    const std::string providerParticipantId1;
    const std::vector<std::string> partitions;
    const std::string multicastId1;

    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>> mockGpsSubscriptionListener;
    std::shared_ptr<OnChangeSubscriptionQos> qos;
    std::shared_ptr<Future<std::string>> future;

    SubscriptionManager subscriptionManager;
    std::shared_ptr<ISubscriptionCallback> subscriptionCallback;
};

TEST_F(SubscriptionManagerTest, registerSubscription_subscriptionRequestIsCorrect) {
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    SubscriptionRequest subscriptionRequest;
    auto future = std::make_shared<Future<std::string>>();
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<joynr::OnChangeSubscriptionQos>();
    std::int64_t now = TimeUtils::getCurrentMillisSinceEpoch();
    qos->setExpiryDateMs(now + 10000);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest);

    EXPECT_EQ("methodName", subscriptionRequest.getSubscribeToName());
    EXPECT_EQ(qos, subscriptionRequest.getQos());
}

TEST_F(SubscriptionManagerTest, registerSubscription_missedPublicationRunnableWorks) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtLeast(4));
    auto future = std::make_shared<Future<std::string>>();
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<joynr::PeriodicSubscriptionQos>(1100, 100, 200);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
}

TEST_F(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_missedPublicationRunnableWorks) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtMost(6));
    auto future = std::make_shared<Future<std::string>>();
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<PeriodicSubscriptionQos>(1100, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    auto mockGpsSubscriptionListener2 = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    EXPECT_CALL(*mockGpsSubscriptionListener2,
                onError(_))
            .Times(0);
    future = std::make_shared<Future<std::string>>();
    auto gpslocationCallback2 = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener2, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos2 = std::make_shared<OnChangeSubscriptionQos>(700, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback2,
                mockGpsSubscriptionListener2,
                qos2,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    std::this_thread::sleep_for(std::chrono::milliseconds(900));
}

TEST_F(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithEnlargedExpiryDate) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtLeast(6));
    auto future = std::make_shared<Future<std::string>>();
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<PeriodicSubscriptionQos>(300, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );

    qos = std::make_shared<PeriodicSubscriptionQos>(1000, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST_F(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithReducedExpiryDate) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtMost(6));
    auto future = std::make_shared<Future<std::string>>();
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<PeriodicSubscriptionQos>(1000, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );

    qos = std::make_shared<PeriodicSubscriptionQos>(300, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST_F(SubscriptionManagerTest, registerSubscription_withoutExpiryDate) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    auto future = std::make_shared<Future<std::string>>();
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler(singleThreadedIOService.getIOService());
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(_,_))
            .Times(0);
    SubscriptionManager subscriptionManager(mockDelayedScheduler, nullptr);
    SubscriptionRequest subscriptionRequest;
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<OnChangeSubscriptionQos>(-1, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );
}

DelayedScheduler::RunnableHandle internalRunnableHandle = 1;

DelayedScheduler::RunnableHandle runnableHandle()
{
    return internalRunnableHandle++;
}

TEST_F(SubscriptionManagerTest, registerSubscription_withExpiryDate) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    auto future = std::make_shared<Future<std::string>>();
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler(singleThreadedIOService.getIOService());
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(A<Runnable*>(),_))
            .Times(1).WillRepeatedly(::testing::Return(runnableHandle()));
    SubscriptionManager subscriptionManager(mockDelayedScheduler, nullptr);
    SubscriptionRequest subscriptionRequest;
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<OnChangeSubscriptionQos>(1000, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );
}

TEST_F(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsToStoppingMissedPublicationRunnables) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(Between(2,3));
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    auto future = std::make_shared<Future<std::string>>();
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>
            >(mockGpsSubscriptionListener, subscriptionRequest.getSubscriptionId(), future, &subscriptionManager);
    auto qos = std::make_shared<PeriodicSubscriptionQos>(
                2000, // validity
                100,  // period
                400   // alert after interval
    );
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest);
     std::this_thread::sleep_for(std::chrono::milliseconds(900));
     subscriptionManager.unregisterSubscription(subscriptionRequest.getSubscriptionId());
     std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

TEST_F(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsOnNonExistantSubscription) {
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    subscriptionManager.unregisterSubscription("superId");
}

TEST_F(SubscriptionManagerTest, getSubscriptionListener) {
    auto mockGpsSubscriptionListener =
            std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    SubscriptionRequest subscriptionRequest;
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService(), nullptr);
    auto future = std::make_shared<Future<std::string>>();
    auto gpslocationCallback =
            std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(
                mockGpsSubscriptionListener, future, &subscriptionManager);
    auto qos = std::make_shared<PeriodicSubscriptionQos>(
                2000, // validity
                100,  // period
                400   // alert after interval
    );
    subscriptionManager.registerSubscription(
                "broadcastName",
                gpslocationCallback,
                mockGpsSubscriptionListener,
                qos,
                subscriptionRequest
    );
    EXPECT_EQ(
                mockGpsSubscriptionListener,
                subscriptionManager.getSubscriptionListener(subscriptionRequest.getSubscriptionId())
    );
}

TEST_F(SubscriptionManagerMulticastTest, registerMulticastSubscription_registrationSucceeds) {
    MulticastSubscriptionRequest subscriptionRequest;

    EXPECT_CALL(*mockMessageRouter, addMulticastReceiver(
        multicastId1, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions,
        subscriptionCallback,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    auto registeredSubscriptionCallbacks = subscriptionManager.getMulticastSubscriptionCallbacks(multicastId1);

    ASSERT_EQ(std::distance(registeredSubscriptionCallbacks.begin(), registeredSubscriptionCallbacks.end()), 1);
    ASSERT_EQ(*registeredSubscriptionCallbacks.begin(), subscriptionCallback);
}

TEST_F(SubscriptionManagerMulticastTest, unregisterMulticastSubscription_unregisterSucceeds) {
    MulticastSubscriptionRequest subscriptionRequest;

    EXPECT_CALL(*mockMessageRouter, removeMulticastReceiver(
        multicastId1, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions,
        subscriptionCallback,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    subscriptionManager.unregisterSubscription(subscriptionRequest.getSubscriptionId());

    auto registeredSubscriptionCallbacks = subscriptionManager.getMulticastSubscriptionCallbacks(multicastId1);

    ASSERT_EQ(std::distance(registeredSubscriptionCallbacks.begin(), registeredSubscriptionCallbacks.end()), 0);
}

TEST_F(SubscriptionManagerMulticastTest, registerMultipleMulticastSubscription_correctCallbacksAreReturned) {
    const std::string providerParticipantId2("providerParticipantId2");
    const std::string providerParticipantId3("providerParticipantId3");

    const std::string multicastId2("providerParticipantId2/subscribeToName/partition1/partition2");
    const std::string multicastId3("providerParticipantId3/subscribeToName/partition1/partition2");

    MulticastSubscriptionRequest subscriptionRequest_Provider1_1;
    MulticastSubscriptionRequest subscriptionRequest_Provider1_2;
    MulticastSubscriptionRequest subscriptionRequest_Provider2;
    MulticastSubscriptionRequest subscriptionRequest_Provider3;

    auto subscriptionCallback1_1 = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(
        mockGpsSubscriptionListener, subscriptionRequest_Provider1_1.getSubscriptionId(), future, &subscriptionManager);

    auto subscriptionCallback1_2 = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(
        mockGpsSubscriptionListener, subscriptionRequest_Provider1_2.getSubscriptionId(), future, &subscriptionManager);

    auto subscriptionCallback2 = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(
        mockGpsSubscriptionListener, subscriptionRequest_Provider2.getSubscriptionId(),  future, &subscriptionManager);

    auto subscriptionCallback3 = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(
        mockGpsSubscriptionListener, subscriptionRequest_Provider3.getSubscriptionId(), future, &subscriptionManager);

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions,
        subscriptionCallback1_1,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest_Provider1_1,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions,
        subscriptionCallback1_2,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest_Provider1_2,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId2,
        partitions,
        subscriptionCallback2,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest_Provider2,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId3,
        partitions,
        subscriptionCallback3,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest_Provider3,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    auto registeredSubscriptionCallbacks_multicast1 =
        subscriptionManager.getMulticastSubscriptionCallbacks(multicastId1);
    auto registeredSubscriptionCallbacks_multicast2 =
        subscriptionManager.getMulticastSubscriptionCallbacks(multicastId2);
    auto registeredSubscriptionCallbacks_multicast3 =
        subscriptionManager.getMulticastSubscriptionCallbacks(multicastId3);

    ASSERT_EQ(std::distance(registeredSubscriptionCallbacks_multicast1.begin(),
                            registeredSubscriptionCallbacks_multicast1.end()), 2);
    ASSERT_TRUE(std::find(registeredSubscriptionCallbacks_multicast1.begin(),
                          registeredSubscriptionCallbacks_multicast1.end(),
                          subscriptionCallback1_1) != registeredSubscriptionCallbacks_multicast1.cend());
    ASSERT_TRUE(std::find(registeredSubscriptionCallbacks_multicast1.begin(),
                          registeredSubscriptionCallbacks_multicast1.end(),
                          subscriptionCallback1_2) != registeredSubscriptionCallbacks_multicast1.cend());
    ASSERT_EQ(std::distance(registeredSubscriptionCallbacks_multicast2.begin(),
                            registeredSubscriptionCallbacks_multicast2.end()), 1);
    ASSERT_EQ(*registeredSubscriptionCallbacks_multicast2.begin(), subscriptionCallback2);

    ASSERT_EQ(std::distance(registeredSubscriptionCallbacks_multicast3.begin(),
                            registeredSubscriptionCallbacks_multicast3.end()), 1);
    ASSERT_EQ(*registeredSubscriptionCallbacks_multicast3.begin(), subscriptionCallback3);
}

TEST_F(SubscriptionManagerMulticastTest, updateMulticastSubscription_changedPartitions_callsMessageRouter)
{
    std::string partition1 = "partition1";
    std::string partition2 = "partition2";
    std::vector<std::string> partitions1 = {partition1};
    std::string multicastId1 = providerParticipantId1 + "/" + subscribeToName + "/" + partition1;
    MulticastSubscriptionRequest subscriptionRequest1;

    EXPECT_CALL(*mockMessageRouter, addMulticastReceiver(
        multicastId1, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions1,
        subscriptionCallback,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest1,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    testing::Mock::VerifyAndClearExpectations(mockMessageRouter.get());

    std::vector<std::string> partitions2 = {partition2};
    std::string multicastId2 = providerParticipantId1 + "/" + subscribeToName + "/" + partition2;
    MulticastSubscriptionRequest subscriptionRequest2;
    subscriptionRequest2.setSubscriptionId(subscriptionRequest1.getSubscriptionId());

    EXPECT_CALL(*mockMessageRouter, removeMulticastReceiver(
        multicastId1, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);
    EXPECT_CALL(*mockMessageRouter, addMulticastReceiver(
        multicastId2, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions2,
        subscriptionCallback,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest2,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});
}

TEST_F(SubscriptionManagerMulticastTest, updateMulticastSubscription_samePartitions_doesNotCallMessageRouter)
{
    MulticastSubscriptionRequest subscriptionRequest1;

    EXPECT_CALL(*mockMessageRouter, addMulticastReceiver(
        multicastId1, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions,
        subscriptionCallback,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest1,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});

    testing::Mock::VerifyAndClearExpectations(mockMessageRouter.get());

    MulticastSubscriptionRequest subscriptionRequest2;
    subscriptionRequest2.setSubscriptionId(subscriptionRequest1.getSubscriptionId());

    EXPECT_CALL(*mockMessageRouter, removeMulticastReceiver(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*mockMessageRouter, addMulticastReceiver(_, _, _, _, _)).Times(0);

    subscriptionManager.registerSubscription(
        subscribeToName,
        subscriberParticipantId,
        providerParticipantId1,
        partitions,
        subscriptionCallback,
        mockGpsSubscriptionListener,
        qos,
        subscriptionRequest2,
        [](){},
        [](const joynr::exceptions::ProviderRuntimeException&){});
}

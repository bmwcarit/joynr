/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include <chrono>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/DispatcherUtils.h"
#include "joynr/Future.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/MulticastSubscriptionCallback.h"
#include "joynr/Util.h"

#include "tests/mock/MockObjects.h"
#include "tests/mock/MockMessageRouter.h"

using ::testing::_;

using namespace joynr;

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
        qos(std::make_shared<MulticastSubscriptionQos>()),
        future(std::make_shared<Future<std::string>>()),
        subscriptionManager(std::make_shared<SubscriptionManager>(singleThreadedIOService.getIOService(), mockMessageRouter)),
        subscriptionCallback(std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
            "testSubscriptionId", future, subscriptionManager))
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
    std::shared_ptr<SubscriptionQos> qos;
    std::shared_ptr<Future<std::string>> future;

    std::shared_ptr<SubscriptionManager> subscriptionManager;
    std::shared_ptr<ISubscriptionCallback> subscriptionCallback;
};

TEST_F(SubscriptionManagerMulticastTest, registerMulticastSubscription_registrationSucceeds) {
    MulticastSubscriptionRequest subscriptionRequest;

    EXPECT_CALL(*mockMessageRouter, addMulticastReceiver(
        multicastId1, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);

    subscriptionManager->registerSubscription(
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

    auto registeredSubscriptionCallback = subscriptionManager->getMulticastSubscriptionCallback(multicastId1);

    ASSERT_EQ(subscriptionCallback, registeredSubscriptionCallback);
}

TEST_F(SubscriptionManagerMulticastTest, unregisterMulticastSubscription_unregisterSucceeds) {
    MulticastSubscriptionRequest subscriptionRequest;

    EXPECT_CALL(*mockMessageRouter, removeMulticastReceiver(
        multicastId1, subscriberParticipantId, providerParticipantId1, _, _)).Times(1);

    subscriptionManager->registerSubscription(
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

    subscriptionManager->unregisterSubscription(subscriptionRequest.getSubscriptionId());

    auto registeredSubscriptionCallback = subscriptionManager->getMulticastSubscriptionCallback(multicastId1);

    ASSERT_EQ(nullptr, registeredSubscriptionCallback);
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

    auto subscriptionCallback1_1 = std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
        subscriptionRequest_Provider1_1.getSubscriptionId(), future, subscriptionManager);

    auto subscriptionCallback1_2 = std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
        subscriptionRequest_Provider1_2.getSubscriptionId(), future, subscriptionManager);

    auto subscriptionCallback2 = std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
        subscriptionRequest_Provider2.getSubscriptionId(),  future, subscriptionManager);

    auto subscriptionCallback3 = std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
        subscriptionRequest_Provider3.getSubscriptionId(), future, subscriptionManager);

    subscriptionManager->registerSubscription(
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

    subscriptionManager->registerSubscription(
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

    subscriptionManager->registerSubscription(
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

    subscriptionManager->registerSubscription(
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

    auto registeredSubscriptionCallback_multicast1 =
        subscriptionManager->getMulticastSubscriptionCallback(multicastId1);
    auto registeredSubscriptionCallback_multicast2 =
        subscriptionManager->getMulticastSubscriptionCallback(multicastId2);
    auto registeredSubscriptionCallback_multicast3 =
        subscriptionManager->getMulticastSubscriptionCallback(multicastId3);

    ASSERT_TRUE(
                registeredSubscriptionCallback_multicast1 == subscriptionCallback1_1
                || registeredSubscriptionCallback_multicast1 == subscriptionCallback1_2
    );
    ASSERT_EQ(subscriptionCallback2, registeredSubscriptionCallback_multicast2);

    ASSERT_EQ(subscriptionCallback3, registeredSubscriptionCallback_multicast3);
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

    subscriptionManager->registerSubscription(
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

    subscriptionManager->registerSubscription(
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

    subscriptionManager->registerSubscription(
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

    subscriptionManager->registerSubscription(
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

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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/Future.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/MulticastSubscriptionCallback.h"
#include "joynr/Util.h"

#include "tests/mock/MockMessageRouter.h"
#include "tests/mock/MockSubscriptionListener.h"

using ::testing::_;

using namespace joynr;

class SubscriptionManagerMulticastTest : public testing::Test
{
public:
    SubscriptionManagerMulticastTest()
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _subscribeToName("subscribeToName"),
              _subscriberParticipantId("subscriberParticipantId"),
              _providerParticipantId1("providerParticipantId"),
              _partitions({"partition1", "partition2"}),
              _multicastId1("providerParticipantId/subscribeToName/partition1/partition2"),
              _mockMessageRouter(
                      std::make_shared<MockMessageRouter>(_singleThreadedIOService->getIOService())),
              _mockGpsSubscriptionListener(std::make_shared<
                      MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>()),
              _qos(std::make_shared<MulticastSubscriptionQos>()),
              _future(std::make_shared<Future<std::string>>()),
              _subscriptionManager(
                      std::make_shared<SubscriptionManager>(_singleThreadedIOService->getIOService(),
                                                            _mockMessageRouter)),
              _subscriptionCallback(std::make_shared<
                      MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
                      "testSubscriptionId",
                      _future,
                      _subscriptionManager))
    {
    }

protected:
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;

    const std::string _subscribeToName;
    const std::string _subscriberParticipantId;
    const std::string _providerParticipantId1;
    const std::vector<std::string> _partitions;
    const std::string _multicastId1;

    std::shared_ptr<MockMessageRouter> _mockMessageRouter;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>
            _mockGpsSubscriptionListener;
    std::shared_ptr<SubscriptionQos> _qos;
    std::shared_ptr<Future<std::string>> _future;

    std::shared_ptr<SubscriptionManager> _subscriptionManager;
    std::shared_ptr<ISubscriptionCallback> _subscriptionCallback;
};

TEST_F(SubscriptionManagerMulticastTest, registerMulticastSubscription_registrationSucceeds)
{
    MulticastSubscriptionRequest subscriptionRequest;

    EXPECT_CALL(*_mockMessageRouter,
                addMulticastReceiver(
                        _multicastId1, _subscriberParticipantId, _providerParticipantId1, _, _))
            .Times(1);

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            _partitions,
            _subscriptionCallback,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    auto registeredSubscriptionCallback =
            _subscriptionManager->getMulticastSubscriptionCallback(_multicastId1);

    ASSERT_EQ(_subscriptionCallback, registeredSubscriptionCallback);
}

TEST_F(SubscriptionManagerMulticastTest, unregisterMulticastSubscription_unregisterSucceeds)
{
    MulticastSubscriptionRequest subscriptionRequest;

    EXPECT_CALL(*_mockMessageRouter,
                removeMulticastReceiver(
                        _multicastId1, _subscriberParticipantId, _providerParticipantId1, _, _))
            .Times(1);

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            _partitions,
            _subscriptionCallback,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    _subscriptionManager->unregisterSubscription(subscriptionRequest.getSubscriptionId());

    auto registeredSubscriptionCallback =
            _subscriptionManager->getMulticastSubscriptionCallback(_multicastId1);

    ASSERT_EQ(nullptr, registeredSubscriptionCallback);
}

TEST_F(SubscriptionManagerMulticastTest,
       registerMultipleMulticastSubscription_correctCallbacksAreReturned)
{
    const std::string providerParticipantId2("providerParticipantId2");
    const std::string providerParticipantId3("providerParticipantId3");

    const std::string multicastId2("providerParticipantId2/subscribeToName/partition1/partition2");
    const std::string multicastId3("providerParticipantId3/subscribeToName/partition1/partition2");

    MulticastSubscriptionRequest subscriptionRequest_Provider1_1;
    MulticastSubscriptionRequest subscriptionRequest_Provider1_2;
    MulticastSubscriptionRequest subscriptionRequest_Provider2;
    MulticastSubscriptionRequest subscriptionRequest_Provider3;

    auto subscriptionCallback1_1 =
            std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
                    subscriptionRequest_Provider1_1.getSubscriptionId(),
                    _future,
                    _subscriptionManager);

    auto subscriptionCallback1_2 =
            std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
                    subscriptionRequest_Provider1_2.getSubscriptionId(),
                    _future,
                    _subscriptionManager);

    auto subscriptionCallback2 =
            std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
                    subscriptionRequest_Provider2.getSubscriptionId(), _future, _subscriptionManager);

    auto subscriptionCallback3 =
            std::make_shared<MulticastSubscriptionCallback<types::Localisation::GpsLocation>>(
                    subscriptionRequest_Provider3.getSubscriptionId(), _future, _subscriptionManager);

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            _partitions,
            subscriptionCallback1_1,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest_Provider1_1,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            _partitions,
            subscriptionCallback1_2,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest_Provider1_2,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            providerParticipantId2,
            _partitions,
            subscriptionCallback2,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest_Provider2,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            providerParticipantId3,
            _partitions,
            subscriptionCallback3,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest_Provider3,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    auto registeredSubscriptionCallback_multicast1 =
            _subscriptionManager->getMulticastSubscriptionCallback(_multicastId1);
    auto registeredSubscriptionCallback_multicast2 =
            _subscriptionManager->getMulticastSubscriptionCallback(multicastId2);
    auto registeredSubscriptionCallback_multicast3 =
            _subscriptionManager->getMulticastSubscriptionCallback(multicastId3);

    ASSERT_TRUE(registeredSubscriptionCallback_multicast1 == subscriptionCallback1_1 ||
                registeredSubscriptionCallback_multicast1 == subscriptionCallback1_2);
    ASSERT_EQ(subscriptionCallback2, registeredSubscriptionCallback_multicast2);

    ASSERT_EQ(subscriptionCallback3, registeredSubscriptionCallback_multicast3);
}

TEST_F(SubscriptionManagerMulticastTest,
       updateMulticastSubscription_changedPartitions_callsMessageRouter)
{
    std::string partition1 = "partition1";
    std::string partition2 = "partition2";
    std::vector<std::string> partitions1 = {partition1};
    std::string localMulticastId1 = _providerParticipantId1 + "/" + _subscribeToName + "/" + partition1;
    MulticastSubscriptionRequest subscriptionRequest1;

    EXPECT_CALL(*_mockMessageRouter,
                addMulticastReceiver(
                        localMulticastId1, _subscriberParticipantId, _providerParticipantId1, _, _))
            .Times(1);

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            partitions1,
            _subscriptionCallback,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest1,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    testing::Mock::VerifyAndClearExpectations(_mockMessageRouter.get());

    std::vector<std::string> partitions2 = {partition2};
    std::string multicastId2 = _providerParticipantId1 + "/" + _subscribeToName + "/" + partition2;
    MulticastSubscriptionRequest subscriptionRequest2;
    subscriptionRequest2.setSubscriptionId(subscriptionRequest1.getSubscriptionId());

    EXPECT_CALL(*_mockMessageRouter,
                removeMulticastReceiver(
                        localMulticastId1, _subscriberParticipantId, _providerParticipantId1, _, _))
            .Times(1);
    EXPECT_CALL(*_mockMessageRouter,
                addMulticastReceiver(
                        multicastId2, _subscriberParticipantId, _providerParticipantId1, _, _))
            .Times(1);

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            partitions2,
            _subscriptionCallback,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest2,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});
}

TEST_F(SubscriptionManagerMulticastTest,
       updateMulticastSubscription_samePartitions_doesNotCallMessageRouter)
{
    MulticastSubscriptionRequest subscriptionRequest1;

    EXPECT_CALL(*_mockMessageRouter,
                addMulticastReceiver(
                        _multicastId1, _subscriberParticipantId, _providerParticipantId1, _, _))
            .Times(1);

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            _partitions,
            _subscriptionCallback,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest1,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});

    testing::Mock::VerifyAndClearExpectations(_mockMessageRouter.get());

    MulticastSubscriptionRequest subscriptionRequest2;
    subscriptionRequest2.setSubscriptionId(subscriptionRequest1.getSubscriptionId());

    EXPECT_CALL(*_mockMessageRouter, removeMulticastReceiver(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*_mockMessageRouter, addMulticastReceiver(_, _, _, _, _)).Times(0);

    _subscriptionManager->registerSubscription(
            _subscribeToName,
            _subscriberParticipantId,
            _providerParticipantId1,
            _partitions,
            _subscriptionCallback,
            _mockGpsSubscriptionListener,
            _qos,
            subscriptionRequest2,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) {});
}

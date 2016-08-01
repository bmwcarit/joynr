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
#include "joynr/Util.h"
#include <chrono>
#include <cstdint>
#include "joynr/SingleThreadedIOService.h"

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
protected:
    SingleThreadedIOService singleThreadedIOService;
};

TEST_F(SubscriptionManagerTest, registerSubscription_subscriptionRequestIsCorrect) {
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService());
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    auto qos = std::make_shared<joynr::OnChangeSubscriptionQos>();
    std::int64_t now = TimeUtils::getCurrentMillisSinceEpoch();
    qos->setExpiryDateMs(now + 10000);
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
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
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService());
    auto qos = std::make_shared<joynr::PeriodicSubscriptionQos>(1100, 100, 200);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
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
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService());
    auto qos = std::make_shared<PeriodicSubscriptionQos>(1100, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    auto mockGpsSubscriptionListener2 = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    EXPECT_CALL(*mockGpsSubscriptionListener2,
                onError(_))
            .Times(0);
    auto gpslocationCallback2 = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener2);
    auto qos2 = std::make_shared<OnChangeSubscriptionQos>(700, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback2,
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
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService());
    auto qos = std::make_shared<PeriodicSubscriptionQos>(300, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = std::make_shared<PeriodicSubscriptionQos>(1000, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
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
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService());
    auto qos = std::make_shared<PeriodicSubscriptionQos>(1000, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = std::make_shared<PeriodicSubscriptionQos>(300, 100, 100);
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST_F(SubscriptionManagerTest, registerSubscription_withoutExpiryDate) {
    auto mockGpsSubscriptionListener = std::make_shared<MockSubscriptionListenerOneType<types::Localisation::GpsLocation>>();
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler(singleThreadedIOService.getIOService());
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(_,_))
            .Times(0);
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    auto qos = std::make_shared<OnChangeSubscriptionQos>(-1, 100);
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
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
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler(singleThreadedIOService.getIOService());
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(A<Runnable*>(),_))
            .Times(1).WillRepeatedly(::testing::Return(runnableHandle()));
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    auto qos = std::make_shared<OnChangeSubscriptionQos>(1000, 100);
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
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
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService());
    auto gpslocationCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsSubscriptionListener);
    auto qos = std::make_shared<PeriodicSubscriptionQos>(
                2000, // validity
                100,  // period
                400   // alert after interval
    );
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest);
     std::this_thread::sleep_for(std::chrono::milliseconds(900));
     subscriptionManager.unregisterSubscription(subscriptionRequest.getSubscriptionId());
     std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

TEST_F(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsOnNonExistantSubscription) {
    SubscriptionManager subscriptionManager(singleThreadedIOService.getIOService());
    subscriptionManager.unregisterSubscription("superId");
}


/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/TimeUtils.h"
#include "joynr/Logger.h"
#include "joynr/Directory.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/Util.h"
#include <chrono>
#include <cstdint>

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;

MATCHER_P(publicationMissedException, subscriptionId, "") {
    if (arg.getTypeName() == joynr::exceptions::PublicationMissedException::TYPE_NAME) {
        joynr::exceptions::PublicationMissedException *errorArg = dynamic_cast<joynr::exceptions::PublicationMissedException*>(arg.clone());
        bool success = errorArg->getSubscriptionId() == subscriptionId && errorArg->getMessage() == subscriptionId;
        delete errorArg;
        return success;
    }
    return false;
}

using namespace joynr;

TEST(SubscriptionManagerTest, registerSubscription_subscriptionRequestIsCorrect) {
    SubscriptionManager subscriptionManager;
    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation> > gpslocationCallback(
                new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    OnChangeSubscriptionQos qos{};
    std::int64_t now = TimeUtils::getCurrentMillisSinceEpoch();
    qos.setExpiryDateMs(now + 10000);
    Variant qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qosVariant,
                subscriptionRequest);

    EXPECT_EQ("methodName", subscriptionRequest.getSubscribeToName());
    EXPECT_EQ(qos, subscriptionRequest.getQos().get<OnChangeSubscriptionQos>());
}

TEST(SubscriptionManagerTest, registerSubscription_missedPublicationRunnableWorks) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtLeast(4));
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1100, 100, 200));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_missedPublicationRunnableWorks) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtMost(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1100, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsSubscriptionListener2(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    EXPECT_CALL(*mockGpsSubscriptionListener2,
                onError(_))
            .Times(0);
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation> > gpslocationCallback2(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener2));
    Variant qos2 = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(700, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback2,
                qos2,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    std::this_thread::sleep_for(std::chrono::milliseconds(900));
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithEnlargedExpiryDate) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtLeast(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(300, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1000, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithReducedExpiryDate) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtMost(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1000, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(300, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(SubscriptionManagerTest, registerSubscription_withoutExpiryDate) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation>> gpslocationCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler();
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(_,_))
            .Times(0);
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(-1, 100));
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

TEST(SubscriptionManagerTest, registerSubscription_withExpiryDate) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation>> gpslocationCallback(
            new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler();
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(A<Runnable*>(),_))
            .Times(1).WillRepeatedly(::testing::Return(runnableHandle()));
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(1000, 100));
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
}

TEST(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsToStoppingMissedPublicationRunnables) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(Between(2,3));
    SubscriptionManager subscriptionManager;
    std::shared_ptr<SubscriptionCallback<types::Localisation::GpsLocation>> gpslocationCallback(
                new SubscriptionCallback<types::Localisation::GpsLocation>(mockGpsSubscriptionListener));
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                2000, // validity
                100,  // period
                400   // alert after interval
    ));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest);
     std::this_thread::sleep_for(std::chrono::milliseconds(900));
     subscriptionManager.unregisterSubscription(subscriptionRequest.getSubscriptionId());
     std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

TEST(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsOnNonExistantSubscription) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()
    );
    SubscriptionManager subscriptionManager;
    subscriptionManager.unregisterSubscription("superId");
}

class TestRunnable : public Runnable {
public:
    virtual ~TestRunnable() = default;
    TestRunnable()
        : Runnable(true)
    {

    }
    void shutdown() {
        JOYNR_LOG_TRACE(logger, "shutdown called...");
    }
    void run() {
        JOYNR_LOG_TRACE(logger, "run: entering...");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        JOYNR_LOG_TRACE(logger, "run: leaving...");
    }
private:
    ADD_LOGGER(TestRunnable);
};

INIT_LOGGER(TestRunnable);

TEST(SingleThreadedDelayedSchedulerTest, schedule_deletingRunnablesCorrectly) {
    SingleThreadedDelayedScheduler scheduler("SingleThread");
    TestRunnable* runnable = new TestRunnable();
    scheduler.schedule(runnable, std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.shutdown();
}

TEST(ThreadPoolDelayedSchedulerTest, schedule_deletingRunnablesCorrectly) {
    ThreadPoolDelayedScheduler scheduler(3, "ThreadPool");
    TestRunnable* runnable = new TestRunnable();
    scheduler.schedule(runnable, std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.shutdown();
}

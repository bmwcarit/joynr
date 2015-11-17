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
#include <QRunnable>
#include <QThreadPool>
#include "tests/utils/MockObjects.h"
#include "utils/TestQString.h"
#include "utils/QThreadSleep.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/joynrlogging.h"
#include "joynr/Directory.h"
#include "joynr/QtPeriodicSubscriptionQos.h"
#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/Util.h"
#include <chrono>
#include <stdint.h>

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

using namespace std::chrono;

TEST(SubscriptionManagerTest, registerSubscription_subscriptionRequestIsCorrect) {
    SubscriptionManager subscriptionManager;
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
                new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    std::shared_ptr<QtSubscriptionQos> qos(new QtOnChangeSubscriptionQos());
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    qos->setExpiryDate(now + 10000);
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest);

    EXPECT_EQ("methodName", subscriptionRequest.getSubscribeToName());
    EXPECT_EQ(qos, subscriptionRequest.getQos());
}

TEST(SubscriptionManagerTest, registerSubscription_missedPublicationRunnableWorks) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId().toStdString())))
            .Times(AtLeast(4));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    std::shared_ptr<QtPeriodicSubscriptionQos> qos(new QtPeriodicSubscriptionQos(1100, 100, 200));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
    QThreadSleep::msleep(1200);
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_missedPublicationRunnableWorks) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId().toStdString())))
            .Times(AtMost(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    std::shared_ptr<QtPeriodicSubscriptionQos> qos(new QtPeriodicSubscriptionQos(1100, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
    QThreadSleep::msleep(300);

    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener2(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    EXPECT_CALL(*mockGpsSubscriptionListener2,
                onError(_))
            .Times(0);
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback2(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener2));
    std::shared_ptr<QtOnChangeSubscriptionQos> qos2(new QtOnChangeSubscriptionQos(700, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback2,
                qos2,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    QThreadSleep::msleep(900);
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithEnlargedExpiryDate) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId().toStdString())))
            .Times(AtLeast(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    std::shared_ptr<QtPeriodicSubscriptionQos> qos(new QtPeriodicSubscriptionQos(300, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = std::shared_ptr<QtPeriodicSubscriptionQos>(new QtPeriodicSubscriptionQos(1000, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    QThreadSleep::msleep(1000);
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithReducedExpiryDate) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId().toStdString())))
            .Times(AtMost(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    std::shared_ptr<QtPeriodicSubscriptionQos> qos(new QtPeriodicSubscriptionQos(1000, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = std::shared_ptr<QtPeriodicSubscriptionQos>(new QtPeriodicSubscriptionQos(300, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    QThreadSleep::msleep(1000);
}

TEST(SubscriptionManagerTest, registerSubscription_withoutExpiryDate) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation>> gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler(QString("SubscriptionManager-MockScheduler"));
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(_,_))
            .Times(0);
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(-1, 100));
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
}

quint32 internalRunnableHandle = 1;

quint32 runnableHandle()
{
    return internalRunnableHandle++;
}

TEST(SubscriptionManagerTest, registerSubscription_withExpiryDate) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation>> gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler(QString("SubscriptionManager-MockScheduler"));
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(A<QRunnable*>(),_))
            .Times(1).WillRepeatedly(::testing::Return(runnableHandle()));
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    std::shared_ptr<QtOnChangeSubscriptionQos> qos(new QtOnChangeSubscriptionQos(1000, 100));
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
}

TEST(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsToStoppingMissedPublicationRunnables) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId().toStdString())))
            .Times(Between(2,3));
    SubscriptionManager subscriptionManager;
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation>> gpslocationCallback(
                new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    std::shared_ptr<QtPeriodicSubscriptionQos> qos(new QtPeriodicSubscriptionQos(
                2000, // validity
                100,  // period
                400   // alert after interval
    ));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest);
     QThreadSleep::msleep(900);
     subscriptionManager.unregisterSubscription(subscriptionRequest.getSubscriptionId());
     QThreadSleep::msleep(1100);
}

TEST(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsOnNonExistantSubscription) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionManager subscriptionManager;
    subscriptionManager.unregisterSubscription("superId");
}

class TestRunnable : public QRunnable {
public:
    virtual ~TestRunnable() {

    }
    TestRunnable() {

    }
    void run() {
        LOG_TRACE(logger, "run: entering...");
        QThreadSleep::msleep(200);
        LOG_TRACE(logger, "run: leaving...");
    }
private:
    static joynr_logging::Logger* logger;
};
joynr_logging::Logger* TestRunnable::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "TestRunnable");

TEST(SingleThreadedDelayedSchedulerTest, schedule_deletingRunnablesCorrectly) {
    SingleThreadedDelayedScheduler scheduler(QString("SingleThreadedDelayedScheduler"));
    TestRunnable* runnable = new TestRunnable();
    scheduler.schedule(runnable, 1);
    QThreadSleep::msleep(100);
}

TEST(ThreadPoolDelayedSchedulerTest, schedule_deletingRunnablesCorrectly) {
    QThreadPool threadPool;
    ThreadPoolDelayedScheduler scheduler(threadPool, QString("ThreadPoolDelayedScheduler"));
    TestRunnable* runnable = new TestRunnable();
    scheduler.schedule(runnable, 1);
    QThreadSleep::msleep(100);
}

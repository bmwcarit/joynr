/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/CapabilityInformation.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "utils/QThreadSleep.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/MessagingSettings.h"
#include "joynr/SettingsMerger.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/LocalChannelUrlDirectory.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr_logging;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->release(1);
}

static const QString messagingPropertiesPersistenceFileName1(
        "End2EndBroadcastTest-runtime1-joynr.settings");
static const QString messagingPropertiesPersistenceFileName2(
        "End2EndBroadcastTest-runtime2-joynr.settings");

class End2EndBroadcastTest : public Test {
public:
    types::ProviderQos qRegisterMetaTypeQos;
//    types::CapabilityInformation qRegisterMetaTypeCi;
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    QSettings settings1;
    QSettings settings2;
    MessagingSettings messagingSettings1;
    MessagingSettings messagingSettings2;
    QString baseUuid;
    QString uuid;
    QString domainName;
    QSemaphore semaphore;
    joynr::tests::TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    QSharedPointer<MockLocationUpdatedSelectiveFilter> filter;

    End2EndBroadcastTest() :
        qRegisterMetaTypeQos(),
//        qRegisterMetaTypeCi(),
        runtime1(NULL),
        runtime2(NULL),
        settings1("test-resources/SystemIntegrationTest1.settings", QSettings::IniFormat),
        settings2("test-resources/SystemIntegrationTest2.settings", QSettings::IniFormat),
        messagingSettings1(settings1),
        messagingSettings2(settings2),
        baseUuid(QUuid::createUuid().toString()),
        uuid( "_" + baseUuid.mid(1,baseUuid.length()-2 )),
        domainName(QString("cppCombinedEnd2EndTest_Domain") + uuid),
        semaphore(0),
        filter(new MockLocationUpdatedSelectiveFilter)

    {
        messagingSettings1.setMessagingPropertiesPersistenceFilename(
                    messagingPropertiesPersistenceFileName1);
        messagingSettings2.setMessagingPropertiesPersistenceFilename(
                    messagingPropertiesPersistenceFileName2);

        QSettings* settings_1 = SettingsMerger::mergeSettings(
                    QString("test-resources/SystemIntegrationTest1.settings"));
        SettingsMerger::mergeSettings(
                    QString("test-resources/libjoynrSystemIntegration1.settings"),
                    settings_1);
        runtime1 = new JoynrClusterControllerRuntime(NULL, settings_1);
        QSettings* settings_2 = SettingsMerger::mergeSettings(
                    QString("test-resources/SystemIntegrationTest2.settings"));
        SettingsMerger::mergeSettings(
                    QString("test-resources/libjoynrSystemIntegration2.settings"),
                    settings_2);
        runtime2 = new JoynrClusterControllerRuntime(NULL, settings_2);
    }

    void SetUp() {
        runtime1->startMessaging();
        runtime1->waitForChannelCreation();
        runtime2->startMessaging();
        runtime2->waitForChannelCreation();
    }

    void TearDown() {
        runtime1->deleteChannel(); //cleanup the channels so they dont remain on the bp
        runtime2->deleteChannel(); //cleanup the channels so they dont remain on the bp
        runtime1->stopMessaging();
        runtime2->stopMessaging();

        // Delete the persisted participant ids so that each test uses different participant ids
        QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
    }

    ~End2EndBroadcastTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndBroadcastTest);

};

TEST_F(End2EndBroadcastTest, subscribeToBroadcast_OneOutput) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           2))))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           3))))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           4))))
            .WillOnce(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    mockListener);

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setSupportsOnChangeSubscriptions(true);
    QSharedPointer<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    runtime1->registerCapability<tests::testProvider>(domainName,testProvider, QString());

    //This wait is necessary, because registerCapability is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->getProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 500;
    qlonglong qosCacheDataFreshnessMs = 400000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setRuntimeQos(MessagingQos(qosRoundTripTTL))
                                               ->setProxyQos(ProxyQos(qosCacheDataFreshnessMs))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    qint64 minInterval_ms = 50;
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms));  // minInterval_ms

    testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    // This wait is necessary, because subcriptions are async, and an event could occur
    // before the subscription has started.
    QThreadSleep::msleep(50);

    testProvider->locationUpdateEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    2));

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    3));
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    4));
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToBroadcast_MultipleOutput) {

    MockGpsDoubleSubscriptionListener* mockListener = new MockGpsDoubleSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           2)), Eq(100)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           3)), Eq(200)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           4)), Eq(300)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation, double> > subscriptionListener(
                    mockListener);

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setSupportsOnChangeSubscriptions(true);
    QSharedPointer<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    runtime1->registerCapability<tests::testProvider>(domainName,testProvider, QString());

    //This wait is necessary, because registerCapability is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->getProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 500;
    qlonglong qosCacheDataFreshnessMs = 400000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setRuntimeQos(MessagingQos(qosRoundTripTTL))
                                               ->setProxyQos(ProxyQos(qosCacheDataFreshnessMs))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    qint64 minInterval_ms = 50;
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms));  // minInterval_ms

    testProxy->subscribeToLocationUpdateWithSpeedBroadcast(subscriptionListener, subscriptionQos);

    //This wait is necessary, because subcriptions are async, and an event could occur could be
    // changed before before the subscription has started.
    QThreadSleep::msleep(5000);

    // Change the location 3 times

    testProvider->locationUpdateWithSpeedEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    2), 100);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateWithSpeedEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    3), 200);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateWithSpeedEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    4), 300);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToSelectiveBroadcast_FilterSuccess) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           2))))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           3))))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, receive(Eq(types::GpsLocation(
                                           9.0,
                                           51.0,
                                           508.0,
                                           types::GpsFixEnum::MODE2D,
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           444,
                                           444,
                                           4))))
            .WillOnce(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    mockListener);

    ON_CALL(*filter, filter(_,_)).WillByDefault(Return(true));

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setSupportsOnChangeSubscriptions(true);
    QSharedPointer<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    testProvider->addBroadcastFilter(filter);
    runtime1->registerCapability<tests::testProvider>(domainName,testProvider, QString());

    //This wait is necessary, because registerCapability is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->getProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 500;
    qlonglong qosCacheDataFreshnessMs = 400000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setRuntimeQos(MessagingQos(qosRoundTripTTL))
                                               ->setProxyQos(ProxyQos(qosCacheDataFreshnessMs))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    qint64 minInterval_ms = 50;
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms));  // minInterval_ms

    testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                filterParameters,
                subscriptionListener,
                subscriptionQos);

    // This wait is necessary, because subcriptions are async, and an event could occur
    // before the subscription has started.
    QThreadSleep::msleep(5000);

    // Change the location 3 times

    testProvider->locationUpdateSelectiveEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    2));

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateSelectiveEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    3));

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateSelectiveEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    4));

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToSelectiveBroadcast_FilterFail) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, receive(A<types::GpsLocation>())).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    mockListener);

    ON_CALL(*filter, filter(_,_)).WillByDefault(Return(false));

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setSupportsOnChangeSubscriptions(true);
    QSharedPointer<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    testProvider->addBroadcastFilter(filter);
    runtime1->registerCapability<tests::testProvider>(domainName,testProvider, QString());

    //This wait is necessary, because registerCapability is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->getProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 500;
    qlonglong qosCacheDataFreshnessMs = 400000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setRuntimeQos(MessagingQos(qosRoundTripTTL))
                                               ->setProxyQos(ProxyQos(qosCacheDataFreshnessMs))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    qint64 minInterval_ms = 50;
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms));  // minInterval_ms

    testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                filterParameters,
                subscriptionListener,
                subscriptionQos);

    // This wait is necessary, because subcriptions are async, and an event could occur
    // before the subscription has started.
    QThreadSleep::msleep(5000);

    // Change the location 3 times

    testProvider->locationUpdateSelectiveEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    2));

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.tryAcquire(1, 500));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateSelectiveEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    3));

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.tryAcquire(1, 500));

    // Waiting between event occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->locationUpdateSelectiveEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    4));

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.tryAcquire(1, 500));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToBroadcastWithSameNameAsAttribute) {

    MockGpsSubscriptionListener* mockListenerAttribute = new MockGpsSubscriptionListener();
    MockGpsSubscriptionListener* mockListenerBroadcast = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener

    // Expect initial attribute publication with default value
    EXPECT_CALL(*mockListenerAttribute, receive(Eq(types::GpsLocation()))).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListenerAttribute, receive(Eq(types::GpsLocation(
                                                                  9.0,
                                                                  51.0,
                                                                  508.0,
                                                                  types::GpsFixEnum::MODE2D,
                                                                  0.0,
                                                                  0.0,
                                                                  0.0,
                                                                  0.0,
                                                                  444,
                                                                  444,
                                                                  2)))).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListenerBroadcast, receive(Eq(types::GpsLocation(
                                                                  9.0,
                                                                  51.0,
                                                                  508.0,
                                                                  types::GpsFixEnum::MODE2D,
                                                                  0.0,
                                                                  0.0,
                                                                  0.0,
                                                                  0.0,
                                                                  444,
                                                                  444,
                                                                  3)))).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListenerAttribute(
                    mockListenerAttribute);

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListenerBroadcast(
                    mockListenerBroadcast);

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setSupportsOnChangeSubscriptions(true);
    QSharedPointer<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    runtime1->registerCapability<tests::testProvider>(domainName,testProvider, QString());

    //This wait is necessary, because registerCapability is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(100);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->getProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 500;
    qlonglong qosCacheDataFreshnessMs = 400000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setRuntimeQos(MessagingQos(qosRoundTripTTL))
                                               ->setProxyQos(ProxyQos(qosCacheDataFreshnessMs))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    qint64 minInterval_ms = 50;
    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms));  // minInterval_ms

    testProxy->subscribeToLocation(
                subscriptionListenerAttribute,
                subscriptionQos);

    testProxy->subscribeToLocationBroadcast(
                subscriptionListenerBroadcast,
                subscriptionQos);

    // This wait is necessary, because subcriptions are async, and an event could occur
    // before the subscription has started.
    QThreadSleep::msleep(500);

    // Initial attribute publication
    ASSERT_TRUE(semaphore.tryAcquire(1, 50));

    // Change attribute
    testProvider->locationChanged(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    2));

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 50));

    // Emit broadcast
    testProvider->locationEventOccurred(
                types::GpsLocation(
                    9.0,
                    51.0,
                    508.0,
                    types::GpsFixEnum::MODE2D,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    444,
                    444,
                    3));

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 50));

    delete testProxyBuilder;
}

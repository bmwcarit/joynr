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
#include <memory>
#include <string>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/localisation/QtGpsLocation.h"
#include "joynr/types/QtProviderQos.h"
#include "joynr/types/QtCapabilityInformation.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "utils/QThreadSleep.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/MessagingSettings.h"
#include "joynr/SettingsMerger.h"
#include "joynr/QtOnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/LocalChannelUrlDirectory.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"
#include "joynr/TypeUtil.h"
#include "joynr/tests/testAbstractProvider.h"

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

namespace joynr {

class MyTestProvider : public tests::DefaulttestProvider {
public:
    virtual void locationChanged(const joynr::types::localisation::GpsLocation& location) {
        tests::testAbstractProvider::locationChanged(location);
    }

    virtual void fireLocation(const joynr::types::localisation::GpsLocation& location) {
        tests::testAbstractProvider::fireLocation(location);
    }

    virtual void fireBroadcastWithEnumOutput(const joynr::tests::testtypes::TestEnum::Enum& testEnum) {
        tests::testAbstractProvider::fireBroadcastWithEnumOutput(testEnum);
    }

    virtual void fireLocationUpdate(const joynr::types::localisation::GpsLocation& location) {
        tests::testAbstractProvider::fireLocationUpdate(location);
    }

    virtual void fireLocationUpdateWithSpeed(
            const joynr::types::localisation::GpsLocation& location,
            const double& currentSpeed
    ) {
        tests::testAbstractProvider::fireLocationUpdateWithSpeed(location, currentSpeed);
    }

    virtual void fireLocationUpdateSelective(const joynr::types::localisation::GpsLocation& location) {
        tests::testAbstractProvider::fireLocationUpdateSelective(location);
    }
};

class End2EndBroadcastTest : public Test {
public:
    types::QtProviderQos qRegisterMetaTypeQos;
//    types::QtCapabilityInformation qRegisterMetaTypeCi;
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    QSettings settings1;
    QSettings settings2;
    MessagingSettings messagingSettings1;
    MessagingSettings messagingSettings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    QSemaphore semaphore;
    QSemaphore altSemaphore;
    joynr::tests::TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> filter;
    unsigned long registerProviderWait;
    unsigned long subscribeToAttributeWait;
    unsigned long subscribeToBroadcastWait;
    joynr::types::localisation::GpsLocation gpsLocation;
    joynr::types::localisation::GpsLocation gpsLocation2;
    joynr::types::localisation::GpsLocation gpsLocation3;
    joynr::types::localisation::GpsLocation gpsLocation4;

    End2EndBroadcastTest() :
        qRegisterMetaTypeQos(),
//        qRegisterMetaTypeCi(),
        runtime1(NULL),
        runtime2(NULL),
        settings1("test-resources/SystemIntegrationTest1.settings", QSettings::IniFormat),
        settings2("test-resources/SystemIntegrationTest2.settings", QSettings::IniFormat),
        messagingSettings1(settings1),
        messagingSettings2(settings2),
        baseUuid(TypeUtil::toStd(QUuid::createUuid().toString())),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domainName("cppEnd2EndBroadcastTest_Domain" + uuid),
        semaphore(0),
        altSemaphore(0),
        filter(new MockLocationUpdatedSelectiveFilter),
        registerProviderWait(1000),
        subscribeToAttributeWait(2000),
        subscribeToBroadcastWait(2000),
        gpsLocation(types::localisation::GpsLocation()),
        gpsLocation2(types::localisation::GpsLocation(
                         9.0,
                         51.0,
                         508.0,
                         types::localisation::GpsFixEnum::MODE2D,
                         0.0,
                         0.0,
                         0.0,
                         0.0,
                         444,
                         444,
                         2)),
        gpsLocation3(types::localisation::GpsLocation(
                         9.0,
                         51.0,
                         508.0,
                         types::localisation::GpsFixEnum::MODE2D,
                         0.0,
                         0.0,
                         0.0,
                         0.0,
                         444,
                         444,
                         3)),
        gpsLocation4(types::localisation::GpsLocation(
                         9.0,
                         51.0,
                         508.0,
                         types::localisation::GpsFixEnum::MODE2D,
                         0.0,
                         0.0,
                         0.0,
                         0.0,
                         444,
                         444,
                         4))

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

        filterParameters.setCountry("Germany");
        filterParameters.setStartTime("4.00 pm");
    }

    void SetUp() {
        runtime1->start();
        runtime2->start();
    }

    void TearDown() {
        bool deleteChannel = true;
        runtime1->stop(deleteChannel);
        runtime2->stop(deleteChannel);

        // Delete the persisted participant ids so that each test uses different participant ids
        QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
    }

    /*
     *  This wait is necessary, because subcriptions are async, and a broadcast could occur
     * before the subscription has started.
     */
    void waitForAttributeSubscriptionArrivedAtProvider(
            std::shared_ptr<tests::testAbstractProvider> testProvider,
            const std::string& attributeName)
    {
        unsigned long delay = 0;

        while (testProvider->attributeListeners.value(attributeName).isEmpty() && delay <= subscribeToAttributeWait) {
            QThreadSleep::msleep(50);
            delay+=50;
        }
        EXPECT_FALSE(testProvider->attributeListeners.value(attributeName).isEmpty());
    }

    /*
     *  This wait is necessary, because subcriptions are async, and a broadcast could occur
     * before the subscription has started.
     */
    void waitForBroadcastSubscriptionArrivedAtProvider(
            std::shared_ptr<tests::testAbstractProvider> testProvider,
            const std::string& broadcastName)
    {
        unsigned long delay = 0;

        while (testProvider->broadcastListeners.value(broadcastName).isEmpty() && delay <= subscribeToBroadcastWait) {
            QThreadSleep::msleep(50);
            delay+=50;
        }
        EXPECT_FALSE(testProvider->broadcastListeners.value(broadcastName).isEmpty());
    }

    ~End2EndBroadcastTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndBroadcastTest);

};

} // namespace joynr

TEST_F(End2EndBroadcastTest, subscribeToBroadcastWithEnumOutput) {
    tests::testtypes::TestEnum::Enum expectedTestEnum = tests::testtypes::TestEnum::TWO;
    MockSubscriptionListenerOneType<tests::testtypes::TestEnum::Enum>* mockListener =
            new MockSubscriptionListenerOneType<tests::testtypes::TestEnum::Enum>();

    // Use a semaphore to count and wait on calls to the mock listener
    ON_CALL(*mockListener, onReceive(Eq(expectedTestEnum)))
            .WillByDefault(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<tests::testtypes::TestEnum::Enum>> subscriptionListener(
                    mockListener);

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(qosRoundTripTTL))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToBroadcastWithEnumOutputBroadcast(subscriptionListener, subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "broadcastWithEnumOutput");

    testProvider->fireBroadcastWithEnumOutput(expectedTestEnum);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    delete testProxyBuilder;
    delete testProxy;
}

TEST_F(End2EndBroadcastTest, subscribeTwiceToSameBroadcast_OneOutput) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    MockGpsSubscriptionListener* mockListener2 = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(_))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));


    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener2, onReceive(_))
            .WillRepeatedly(ReleaseSemaphore(&altSemaphore));

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListener(
                    mockListener);


    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListener2(
                    mockListener2);

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::string subscriptionId = testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    // This wait is necessary, because subcriptions are async, and a broadcast could occur
    // before the subscription has started.
    QThreadSleep::msleep(subscribeToBroadcastWait);

    testProvider->fireLocationUpdate(
                gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between   occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // update subscription, much longer minInterval_ms
    subscriptionQos.setMinInterval(5000);
    testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener2, subscriptionQos, subscriptionId);

    QThreadSleep::msleep(subscribeToBroadcastWait);
    testProvider->fireLocationUpdate(gpsLocation2);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(altSemaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    //now, the next broadcast shall not be received, as the minInterval has been updated
    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_FALSE(altSemaphore.tryAcquire(1, 1000));
    //the "old" semaphore shall not be touced, as listener has been replaced with listener2 as callback
    ASSERT_FALSE(semaphore.tryAcquire(1, 1000));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeAndUnsubscribeFromBroadcast_OneOutput) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3)))
            .Times(0);

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::string subscriptionId = testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    // This wait is necessary, because subcriptions are async, and a broadcast could occur
    // before the subscription has started.
    QThreadSleep::msleep(subscribeToBroadcastWait);

    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);


    testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId);

    testProvider->fireLocationUpdate(gpsLocation3);
//     Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.tryAcquire(1, 2000));
}

TEST_F(End2EndBroadcastTest, subscribeToBroadcast_OneOutput) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "locationUpdate");

    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdate(gpsLocation3);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdate(gpsLocation4);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToBroadcast_MultipleOutput) {

    MockGpsFloatSubscriptionListener* mockListener = new MockGpsFloatSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2), Eq(100)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3), Eq(200)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4), Eq(300)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation, float> > subscriptionListener(
                    mockListener);

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToLocationUpdateWithSpeedBroadcast(subscriptionListener, subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "locationUpdateWithSpeed");

    // Change the location 3 times

    testProvider->fireLocationUpdateWithSpeed(gpsLocation2, 100);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdateWithSpeed(gpsLocation3, 200);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdateWithSpeed(gpsLocation4, 300);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToSelectiveBroadcast_FilterSuccess) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    ON_CALL(*filter, filter(_, Eq(filterParameters))).WillByDefault(Return(true));

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    testProvider->addBroadcastFilter(filter);
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                filterParameters,
                subscriptionListener,
                subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "locationUpdateSelective");

    // Change the location 3 times

    testProvider->fireLocationUpdateSelective(gpsLocation2);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdateSelective(gpsLocation3);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdateSelective(gpsLocation4);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 3000));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToSelectiveBroadcast_FilterFail) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::localisation::GpsLocation&>())).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    ON_CALL(*filter, filter(_, Eq(filterParameters))).WillByDefault(Return(false));

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    testProvider->addBroadcastFilter(filter);
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                filterParameters,
                subscriptionListener,
                subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "locationUpdateSelective");

    // Change the location 3 times

    testProvider->fireLocationUpdate(gpsLocation2);

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.tryAcquire(1, 500));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdateSelective(gpsLocation3);

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.tryAcquire(1, 500));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    QThreadSleep::msleep(minInterval_ms);

    testProvider->fireLocationUpdateSelective(gpsLocation4);

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.tryAcquire(1, 500));

    delete testProxyBuilder;
}

TEST_F(End2EndBroadcastTest, subscribeToBroadcastWithSameNameAsAttribute) {

    MockGpsSubscriptionListener* mockListenerAttribute = new MockGpsSubscriptionListener();
    MockGpsSubscriptionListener* mockListenerBroadcast = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener

    // Expect initial attribute publication with default value
    EXPECT_CALL(*mockListenerAttribute, onReceive(Eq(gpsLocation))).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListenerAttribute, onReceive(Eq(gpsLocation2))).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListenerBroadcast, onReceive(Eq(gpsLocation3))).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListenerAttribute(
                    mockListenerAttribute);

    std::shared_ptr<ISubscriptionListener<types::localisation::GpsLocation> > subscriptionListenerBroadcast(
                    mockListenerBroadcast);

    std::shared_ptr<MyTestProvider> testProvider(new MyTestProvider());
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(registerProviderWait);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);
    discoveryQos.setRetryInterval(250);

    qlonglong qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToLocation(
                subscriptionListenerAttribute,
                subscriptionQos);

    testProxy->subscribeToLocationBroadcast(
                subscriptionListenerBroadcast,
                subscriptionQos);

    waitForAttributeSubscriptionArrivedAtProvider(testProvider, "location");
    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "location");

    // Initial attribute publication
    ASSERT_TRUE(semaphore.tryAcquire(1, 500));

    QThreadSleep::msleep(minInterval_ms); //ensure to wait for the minInterval_ms before changing location

    // Change attribute
    testProvider->locationChanged(gpsLocation2);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 500));

    // Emit broadcast
    testProvider->fireLocation(gpsLocation3);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 500));

    delete testProxyBuilder;
}

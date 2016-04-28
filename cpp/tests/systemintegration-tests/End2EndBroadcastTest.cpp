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
#include <memory>
#include <string>
#include <cstdint>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"
#include "joynr/tests/TestBroadcastWithFilteringBroadcastFilter.h"
#include "joynr/TypeUtil.h"
#include "joynr/tests/testAbstractProvider.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/PrivateCopyAssign.h"

using namespace ::testing;
using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

static const std::string messagingPropertiesPersistenceFileName1(
        "End2EndBroadcastTest-runtime1-joynr.settings");
static const std::string messagingPropertiesPersistenceFileName2(
        "End2EndBroadcastTest-runtime2-joynr.settings");

namespace joynr {

class MyTestProvider : public tests::DefaulttestProvider {
public:
    void locationChanged(const joynr::types::Localisation::GpsLocation& location) override {
        tests::testAbstractProvider::locationChanged(location);
    }

    void fireLocation(const joynr::types::Localisation::GpsLocation& location) override {
        tests::testAbstractProvider::fireLocation(location);
    }

    void fireBroadcastWithEnumOutput(const joynr::tests::testTypes::TestEnum::Enum& testEnum) override {
        tests::testAbstractProvider::fireBroadcastWithEnumOutput(testEnum);
    }

    void fireLocationUpdate(const joynr::types::Localisation::GpsLocation& location) override {
        tests::testAbstractProvider::fireLocationUpdate(location);
    }

    void fireEmptyBroadcast(
    ) override {
        tests::testAbstractProvider::fireEmptyBroadcast();
    }

    void fireLocationUpdateWithSpeed(
            const joynr::types::Localisation::GpsLocation& location,
            const float& currentSpeed
    ) override {
        tests::testAbstractProvider::fireLocationUpdateWithSpeed(location, currentSpeed);
    }

    void fireLocationUpdateSelective(const joynr::types::Localisation::GpsLocation& location) override {
        tests::testAbstractProvider::fireLocationUpdateSelective(location);
    }

    void fireBroadcastWithByteBufferParameter(const joynr::ByteBuffer& byteBufferParameter) override {
        tests::testAbstractProvider::fireBroadcastWithByteBufferParameter(byteBufferParameter);
    }

    void fireBroadcastWithFiltering(
            const std::string& stringOut,
            const std::vector<std::string> & stringArrayOut,
            const std::vector<joynr::tests::testTypes::TestEnum::Enum>& enumerationArrayOut,
            const joynr::types::TestTypes::TEverythingStruct& structWithStringArrayOut,
            const std::vector<joynr::types::TestTypes::TEverythingStruct> & structWithStringArrayArrayOut
    ) override {
        tests::testAbstractProvider::fireBroadcastWithFiltering(stringOut,
                                                                stringArrayOut,
                                                                enumerationArrayOut,
                                                                structWithStringArrayOut,
                                                                structWithStringArrayArrayOut);
    }
};

class End2EndBroadcastTest : public TestWithParam< std::tuple<std::string, std::string> > {
public:
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    Settings *settings1;
    Settings *settings2;
    MessagingSettings messagingSettings1;
    MessagingSettings messagingSettings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    Semaphore semaphore;
    Semaphore altSemaphore;
    joynr::tests::TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> filter;
    unsigned long registerProviderWait;
    unsigned long subscribeToAttributeWait;
    unsigned long subscribeToBroadcastWait;
    joynr::types::Localisation::GpsLocation gpsLocation;
    joynr::types::Localisation::GpsLocation gpsLocation2;
    joynr::types::Localisation::GpsLocation gpsLocation3;
    joynr::types::Localisation::GpsLocation gpsLocation4;

    End2EndBroadcastTest() :
        runtime1(nullptr),
        runtime2(nullptr),
        settings1(new Settings(std::get<0>(GetParam()))),
        settings2(new Settings(std::get<1>(GetParam()))),
        messagingSettings1(*settings1),
        messagingSettings2(*settings2),
        baseUuid(util::createUuid()),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domainName("cppEnd2EndBroadcastTest_Domain" + uuid),
        semaphore(0),
        altSemaphore(0),
        filter(new MockLocationUpdatedSelectiveFilter),
        registerProviderWait(1000),
        subscribeToAttributeWait(2000),
        subscribeToBroadcastWait(2000),
        gpsLocation(types::Localisation::GpsLocation()),
        gpsLocation2(types::Localisation::GpsLocation(
                         9.0,
                         51.0,
                         508.0,
                         types::Localisation::GpsFixEnum::MODE2D,
                         0.0,
                         0.0,
                         0.0,
                         0.0,
                         444,
                         444,
                         2)),
        gpsLocation3(types::Localisation::GpsLocation(
                         9.0,
                         51.0,
                         508.0,
                         types::Localisation::GpsFixEnum::MODE2D,
                         0.0,
                         0.0,
                         0.0,
                         0.0,
                         444,
                         444,
                         3)),
        gpsLocation4(types::Localisation::GpsLocation(
                         9.0,
                         51.0,
                         508.0,
                         types::Localisation::GpsFixEnum::MODE2D,
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

        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings1, false);

        runtime1 = new JoynrClusterControllerRuntime(nullptr, settings1);

        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings2, false);

        runtime2 = new JoynrClusterControllerRuntime(nullptr, settings2);

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

        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

    /*
     *  This wait is necessary, because subcriptions are async, and a publication could occur
     * before the subscription has started.
     */
    void waitForAttributeSubscriptionArrivedAtProvider(
            std::shared_ptr<tests::testAbstractProvider> testProvider,
            const std::string& attributeName)
    {
        unsigned long delay = 0;

        while (testProvider->attributeListeners.find(attributeName) == testProvider->attributeListeners.cend()
               && delay <= subscribeToAttributeWait
        ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            delay+=50;
        }
        EXPECT_FALSE(testProvider->attributeListeners.find(attributeName) == testProvider->attributeListeners.cend() ||
                     testProvider->attributeListeners.find(attributeName)->second.empty());
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

        while (testProvider->broadcastListeners.find(broadcastName) == testProvider->broadcastListeners.cend()
               && delay <= subscribeToBroadcastWait
        ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            delay+=50;
        }
        EXPECT_FALSE(testProvider->broadcastListeners.find(broadcastName) == testProvider->broadcastListeners.cend() ||
                     testProvider->broadcastListeners.find(broadcastName)->second.empty());
    }

    ~End2EndBroadcastTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndBroadcastTest);

protected:
    template <typename FireBroadcast, typename SubscribeTo, typename T>
    void testOneShotBroadcastSubscription(const T& expectedValue,
                                          SubscribeTo subscribeTo,
                                          FireBroadcast fireBroadcast,
                                          const std::string& broadcastName) {
        MockSubscriptionListenerOneType<T>* mockListener =
                new MockSubscriptionListenerOneType<T>();

        // Use a semaphore to count and wait on calls to the mock listener
        ON_CALL(*mockListener, onReceive(Eq(expectedValue)))
                .WillByDefault(ReleaseSemaphore(&semaphore));

        std::shared_ptr<ISubscriptionListener<T>> subscriptionListener(
                        mockListener);
        testOneShotBroadcastSubscription(subscriptionListener,
                                         subscribeTo,
                                         fireBroadcast,
                                         broadcastName,
                                         expectedValue);
    }

    template <typename FireBroadcast, typename SubscribeTo, typename ...T>
    void testOneShotBroadcastSubscription(std::shared_ptr<ISubscriptionListener<T...>> subscriptionListener,
                                          SubscribeTo subscribeTo,
                                          FireBroadcast fireBroadcast,
                                          const std::string& broadcastName,
                                          T... expectedValues) {
        testOneShotBroadcastSubscriptionWithFiltering(subscriptionListener,
                                                      subscribeTo,
                                                      fireBroadcast,
                                                      broadcastName,
                                                      nullptr,
                                                      expectedValues...);
    }

    template <typename FireBroadcast, typename SubscribeTo, typename BroadcastFilter, typename ...T>
    void testOneShotBroadcastSubscriptionWithFiltering(std::shared_ptr<ISubscriptionListener<T...>> subscriptionListener,
                                          SubscribeTo subscribeTo,
                                          FireBroadcast fireBroadcast,
                                          const std::string& broadcastName,
                                          std::shared_ptr<BroadcastFilter> filter,
                                          T... expectedValues) {
        auto testProvider = std::make_shared<MyTestProvider>();
        runtime1->registerProvider<tests::testProvider>(domainName, testProvider);
        if (filter) {
            testProvider->addBroadcastFilter(filter);
        }
        //This wait is necessary, because registerProvider is async, and a lookup could occur
        // before the register has finished.
        std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

        ProxyBuilder<tests::testProxy>* testProxyBuilder
                = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(1000);
        discoveryQos.setRetryIntervalMs(250);

        std::int64_t qosRoundTripTTL = 500;

        // Send a message and expect to get a result
        tests::testProxy* testProxy = testProxyBuilder
                ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();

        std::int64_t minInterval_ms = 50;
        OnChangeSubscriptionQos subscriptionQos(
                    500000,   // validity_ms
                    minInterval_ms);  // minInterval_ms

        subscribeTo(testProxy, subscriptionListener, subscriptionQos);
        waitForBroadcastSubscriptionArrivedAtProvider(testProvider, broadcastName);

        (*testProvider.*fireBroadcast)(expectedValues...);

        // Wait for a subscription message to arrive
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

        delete testProxyBuilder;
        delete testProxy;
    }
};

} // namespace joynr

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithEnumOutput) {
    tests::testTypes::TestEnum::Enum expectedTestEnum = tests::testTypes::TestEnum::TWO;

    testOneShotBroadcastSubscription(expectedTestEnum,
                                 [](tests::testProxy* testProxy,
                                    std::shared_ptr<ISubscriptionListener<tests::testTypes::TestEnum::Enum>> subscriptionListener,
                                    const OnChangeSubscriptionQos& subscriptionQos) {
                                    testProxy->subscribeToBroadcastWithEnumOutputBroadcast(subscriptionListener, subscriptionQos);
                                 },
                                 &tests::testProvider::fireBroadcastWithEnumOutput,
                                 "broadcastWithEnumOutput");
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithByteBufferParameter) {
    joynr::ByteBuffer expectedByteBuffer {0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0};

    testOneShotBroadcastSubscription(expectedByteBuffer,
                                 [](tests::testProxy* testProxy,
                                    std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer>> subscriptionListener,
                                    const OnChangeSubscriptionQos& subscriptionQos) {
                                    testProxy->subscribeToBroadcastWithByteBufferParameterBroadcast(subscriptionListener, subscriptionQos);
                                 },
                                 &tests::testProvider::fireBroadcastWithByteBufferParameter,
                                 "broadcastWithByteBufferParameter");
}

class MockTestBroadcastWithFilteringBroadcastFilter : public joynr::tests::TestBroadcastWithFilteringBroadcastFilter {
public:
    MOCK_METHOD6(filter, bool(const std::string& stringOut,
                              const std::vector<std::string> & stringArrayOut,
                              const std::vector<joynr::tests::testTypes::TestEnum::Enum>& enumerationArrayOut,
                              const joynr::types::TestTypes::TEverythingStruct& structWithStringArrayOut,
                              const std::vector<joynr::types::TestTypes::TEverythingStruct> & structWithStringArrayArrayOut,
                              const joynr::tests::TestBroadcastWithFilteringBroadcastFilterParameters& filterParameters));
};

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithFiltering) {
    std::string stringOut = "expectedString";
    std::vector<std::string> stringArrayOut {stringOut};
    std::vector<joynr::tests::testTypes::TestEnum::Enum> enumerationArrayOut = {joynr::tests::testTypes::TestEnum::TWO};
    joynr::types::TestTypes::TEverythingStruct structWithStringArrayOut;
    std::vector<joynr::types::TestTypes::TEverythingStruct>  structWithStringArrayArrayOut {structWithStringArrayOut};

    MockSubscriptionListenerFiveTypes<std::string, std::vector<std::string> , std::vector<joynr::tests::testTypes::TestEnum::Enum>, joynr::types::TestTypes::TEverythingStruct, std::vector<joynr::types::TestTypes::TEverythingStruct> >* mockListener =
            new MockSubscriptionListenerFiveTypes<std::string, std::vector<std::string> , std::vector<joynr::tests::testTypes::TestEnum::Enum>, joynr::types::TestTypes::TEverythingStruct, std::vector<joynr::types::TestTypes::TEverythingStruct> >();

    // Use a semaphore to count and wait on calls to the mock listener
    ON_CALL(*mockListener, onReceive(Eq(stringOut),
                                     Eq(stringArrayOut),
                                     Eq(enumerationArrayOut),
                                     Eq(structWithStringArrayOut),
                                     Eq(structWithStringArrayArrayOut)))
            .WillByDefault(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<std::string, std::vector<std::string> , std::vector<joynr::tests::testTypes::TestEnum::Enum>, joynr::types::TestTypes::TEverythingStruct, std::vector<joynr::types::TestTypes::TEverythingStruct>>> subscriptionListener(
                    mockListener);


    auto filter = std::make_shared<MockTestBroadcastWithFilteringBroadcastFilter>();
    ON_CALL(*filter, filter(Eq(stringOut),
                            Eq(stringArrayOut),
                            Eq(enumerationArrayOut),
                            Eq(structWithStringArrayOut),
                            Eq(structWithStringArrayArrayOut),
                            _))
           .WillByDefault(DoAll(ReleaseSemaphore(&altSemaphore), Return(true)));

    testOneShotBroadcastSubscriptionWithFiltering(subscriptionListener,
                                     [](tests::testProxy* testProxy,
                                        std::shared_ptr<joynr::ISubscriptionListener<std::string, std::vector<std::string> , std::vector<joynr::tests::testTypes::TestEnum::Enum>, joynr::types::TestTypes::TEverythingStruct, std::vector<joynr::types::TestTypes::TEverythingStruct> > > subscriptionListener,
                                        const OnChangeSubscriptionQos& subscriptionQos) {
                                        joynr::tests::TestBroadcastWithFilteringBroadcastFilterParameters filterParameters;
                                        testProxy->subscribeToBroadcastWithFilteringBroadcast(filterParameters, subscriptionListener, subscriptionQos);
                                     },
                                     &tests::testProvider::fireBroadcastWithFiltering,
                                     "broadcastWithFiltering",
                                     filter,
                                     stringOut,
                                     stringArrayOut,
                                     enumerationArrayOut,
                                     structWithStringArrayOut,
                                     structWithStringArrayArrayOut);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(altSemaphore.waitFor(std::chrono::seconds(3)));

}

TEST_P(End2EndBroadcastTest, subscribeTwiceToSameBroadcast_OneOutput) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    MockGpsSubscriptionListener* mockListener2 = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(_))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));


    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener2, onReceive(_))
            .WillRepeatedly(ReleaseSemaphore(&altSemaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);


    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener2(
                    mockListener2);

    auto testProvider = std::make_shared<MyTestProvider>();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::string subscriptionId = testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    // This wait is necessary, because subcriptions are async, and a broadcast could occur
    // before the subscription has started.
    std::this_thread::sleep_for(std::chrono::milliseconds(subscribeToBroadcastWait));

    testProvider->fireLocationUpdate(
                gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between   occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // update subscription, much longer minInterval_ms
    subscriptionQos.setMinIntervalMs(5000);
    testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener2, subscriptionQos, subscriptionId);

    std::this_thread::sleep_for(std::chrono::milliseconds(subscribeToBroadcastWait));
    testProvider->fireLocationUpdate(gpsLocation2);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(altSemaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    //now, the next broadcast shall not be received, as the minInterval has been updated
    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_FALSE(altSemaphore.waitFor(std::chrono::seconds(1)));
    //the "old" semaphore shall not be touced, as listener has been replaced with listener2 as callback
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(1)));

    delete testProxyBuilder;
}

TEST_P(End2EndBroadcastTest, subscribeAndUnsubscribeFromBroadcast_OneOutput) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3)))
            .Times(0);

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    auto testProvider = std::make_shared<MyTestProvider>();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::string subscriptionId = testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    // This wait is necessary, because subcriptions are async, and a broadcast could occur
    // before the subscription has started.
    std::this_thread::sleep_for(std::chrono::milliseconds(subscribeToBroadcastWait));

    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));


    testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId);

    testProvider->fireLocationUpdate(gpsLocation3);
//     Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(2)));
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_OneOutput) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    auto testProvider = std::make_shared<MyTestProvider>();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToLocationUpdateBroadcast(subscriptionListener, subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "locationUpdate");

    testProvider->fireLocationUpdate(gpsLocation2);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdate(gpsLocation3);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdate(gpsLocation4);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    delete testProxyBuilder;
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_EmptyOutput) {

    MockSubscriptionListenerZeroTypes* mockListener = new MockSubscriptionListenerZeroTypes();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive())
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<void> > subscriptionListener(
                    mockListener);

    auto testProvider = std::make_shared<MyTestProvider>();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToEmptyBroadcastBroadcast(subscriptionListener, subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "emptyBroadcast");

    testProvider->fireEmptyBroadcast();

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireEmptyBroadcast();
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireEmptyBroadcast();
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    delete testProxyBuilder;
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_MultipleOutput) {

    MockGpsFloatSubscriptionListener* mockListener = new MockGpsFloatSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2), Eq(100)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3), Eq(200)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4), Eq(300)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation, float> > subscriptionListener(
                    mockListener);

    auto testProvider = std::make_shared<MyTestProvider>();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    testProxy->subscribeToLocationUpdateWithSpeedBroadcast(subscriptionListener, subscriptionQos);

    waitForBroadcastSubscriptionArrivedAtProvider(testProvider, "locationUpdateWithSpeed");

    // Change the location 3 times

    testProvider->fireLocationUpdateWithSpeed(gpsLocation2, 100);

//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateWithSpeed(gpsLocation3, 200);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateWithSpeed(gpsLocation4, 300);
//     Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    delete testProxyBuilder;
}

TEST_P(End2EndBroadcastTest, subscribeToSelectiveBroadcast_FilterSuccess) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    ON_CALL(*filter, filter(_, Eq(filterParameters))).WillByDefault(Return(true));

    auto testProvider = std::make_shared<MyTestProvider>();
    testProvider->addBroadcastFilter(filter);
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
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
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation3);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation4);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    delete testProxyBuilder;
}

TEST_P(End2EndBroadcastTest, subscribeToSelectiveBroadcast_FilterFail) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>())).
            WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    ON_CALL(*filter, filter(_, Eq(filterParameters))).WillByDefault(Return(false));

    auto testProvider = std::make_shared<MyTestProvider>();
    testProvider->addBroadcastFilter(filter);
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
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
    ASSERT_FALSE(semaphore.waitFor(std::chrono::milliseconds(500)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation3);

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.waitFor(std::chrono::milliseconds(500)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation4);

    // Wait for a subscription message to arrive
    ASSERT_FALSE(semaphore.waitFor(std::chrono::milliseconds(500)));

    delete testProxyBuilder;
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithSameNameAsAttribute) {

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

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListenerAttribute(
                    mockListenerAttribute);

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListenerBroadcast(
                    mockListenerBroadcast);

    auto testProvider = std::make_shared<MyTestProvider>();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);
    discoveryQos.setRetryIntervalMs(250);

    std::int64_t qosRoundTripTTL = 500;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 50;
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
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms)); //ensure to wait for the minInterval_ms before changing location

    // Change attribute
    testProvider->locationChanged(gpsLocation2);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    // Emit broadcast
    testProvider->fireLocation(gpsLocation3);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    delete testProxyBuilder;
}

INSTANTIATE_TEST_CASE_P(Http,
        End2EndBroadcastTest,
        testing::Values(
            std::make_tuple("test-resources/HttpSystemIntegrationTest1.settings","test-resources/HttpSystemIntegrationTest2.settings")
        )
);

INSTANTIATE_TEST_CASE_P(MqttWithHttpBackend,
        End2EndBroadcastTest,
        testing::Values(
            std::make_tuple("test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings","test-resources/MqttWithHttpBackendSystemIntegrationTest2.settings")
        )
);

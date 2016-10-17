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
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "JoynrTest.h"
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"
#include "joynr/tests/TestBroadcastWithFilteringBroadcastFilter.h"
#include "joynr/tests/testAbstractProvider.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Future.h"

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

    void fireLocation(
            const joynr::types::Localisation::GpsLocation& location,
            const std::vector<std::string>& partitions = std::vector<std::string>()
    ) override {
        tests::testAbstractProvider::fireLocation(location);
    }

    void fireBroadcastWithEnumOutput(
            const joynr::tests::testTypes::TestEnum::Enum& testEnum,
            const std::vector<std::string>& partitions = std::vector<std::string>()
    ) override {
        tests::testAbstractProvider::fireBroadcastWithEnumOutput(testEnum);
    }

    void fireLocationUpdate(
            const joynr::types::Localisation::GpsLocation& location,
            const std::vector<std::string>& partitions = std::vector<std::string>()
    ) override {
        tests::testAbstractProvider::fireLocationUpdate(location);
    }

    void fireEmptyBroadcast(
            const std::vector<std::string>& partitions = std::vector<std::string>()
    ) override {
        tests::testAbstractProvider::fireEmptyBroadcast();
    }

    void fireLocationUpdateWithSpeed(
            const joynr::types::Localisation::GpsLocation& location,
            const float& currentSpeed,
            const std::vector<std::string>& partitions = std::vector<std::string>()
    ) override {
        tests::testAbstractProvider::fireLocationUpdateWithSpeed(location, currentSpeed);
    }

    void fireLocationUpdateSelective(const joynr::types::Localisation::GpsLocation& location) override {
        tests::testAbstractProvider::fireLocationUpdateSelective(location);
    }

    void fireBroadcastWithByteBufferParameter(
            const joynr::ByteBuffer& byteBufferParameter,
            const std::vector<std::string>& partitions = std::vector<std::string>()
    ) override {
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
    std::unique_ptr<Settings> settings1;
    std::unique_ptr<Settings> settings2;
    MessagingSettings messagingSettings1;
    MessagingSettings messagingSettings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    Semaphore semaphore;
    Semaphore altSemaphore;
    joynr::tests::TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> filter;
    std::uint16_t registerProviderWait;
    std::uint16_t subscribeToAttributeWait;
    std::uint16_t subscribeToBroadcastWait;
    joynr::types::Localisation::GpsLocation gpsLocation;
    joynr::types::Localisation::GpsLocation gpsLocation2;
    joynr::types::Localisation::GpsLocation gpsLocation3;
    joynr::types::Localisation::GpsLocation gpsLocation4;

    End2EndBroadcastTest() :
        runtime1(nullptr),
        runtime2(nullptr),
        settings1(std::make_unique<Settings>(std::get<0>(GetParam()))),
        settings2(std::make_unique<Settings>(std::get<1>(GetParam()))),
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
                         4)),
        providerParticipantId()

    {
        messagingSettings1.setMessagingPropertiesPersistenceFilename(
                    messagingPropertiesPersistenceFileName1);
        messagingSettings2.setMessagingPropertiesPersistenceFilename(
                    messagingPropertiesPersistenceFileName2);

        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings1, false);

        runtime1 = new JoynrClusterControllerRuntime(std::move(settings1));

        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings2, false);

        runtime2 = new JoynrClusterControllerRuntime(std::move(settings2));

        filterParameters.setCountry("Germany");
        filterParameters.setStartTime("4.00 pm");
    }

    void SetUp() {
        runtime1->start();
        runtime2->start();
    }

    void TearDown() {
        if (!providerParticipantId.empty()) {
            runtime1->unregisterProvider(providerParticipantId);
        }
        bool deleteChannel = true;
        runtime1->stop(deleteChannel);
        runtime2->stop(deleteChannel);

        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

    ~End2EndBroadcastTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    std::string providerParticipantId;
    DISALLOW_COPY_AND_ASSIGN(End2EndBroadcastTest);

protected:
    bool usesHttpTransport() {
        std::string brokerProtocol = messagingSettings1.getBrokerUrl()
                .getBrokerChannelsBaseUrl().getProtocol();
        return (boost::iequals(brokerProtocol, "http")
                || boost::iequals(brokerProtocol, "https"));
    }

    std::shared_ptr<MyTestProvider> registerProvider() {
        auto testProvider = std::make_shared<MyTestProvider>();
        providerParticipantId = runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

        // This wait is necessary, because registerProvider is async, and a lookup could occur
        // before the register has finished.
        std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));

        return testProvider;
    }

    std::shared_ptr<tests::testProxy> buildProxy() {
        ProxyBuilder<tests::testProxy>* testProxyBuilder
                = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(1000);
        discoveryQos.setRetryIntervalMs(250);

        std::int64_t qosRoundTripTTL = 500;

        std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

        delete testProxyBuilder;
        return testProxy;
    }

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
        std::vector<std::string> partitions({}); // TODO test with real partitions
        std::shared_ptr<MyTestProvider> testProvider = registerProvider();

        std::shared_ptr<tests::testProxy> testProxy = buildProxy();

        std::int64_t minInterval_ms = 50;
        auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                    500000,   // validity_ms
                    minInterval_ms);  // minInterval_ms

        subscribeTo(testProxy.get(), subscriptionListener, subscriptionQos);

        (*testProvider.*fireBroadcast)(expectedValues..., partitions);

        // Wait for a subscription message to arrive
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    }

    template <typename BroadcastFilter>
    void addFilterToTestProvider(std::shared_ptr<MyTestProvider> testProvider, std::shared_ptr<BroadcastFilter> filter)
    {
        if (filter) {
            testProvider->addBroadcastFilter(filter);
        }
    }

    void addFilterToTestProvider(std::shared_ptr<MyTestProvider> testProvider, std::nullptr_t filter)
    {
        std::ignore = testProvider;
        std::ignore = filter;
    }

    template <typename FireBroadcast, typename SubscribeTo, typename BroadcastFilterPtr, typename ...T>
    void testOneShotBroadcastSubscriptionWithFiltering(std::shared_ptr<ISubscriptionListener<T...>> subscriptionListener,
                                          SubscribeTo subscribeTo,
                                          FireBroadcast fireBroadcast,
                                          const std::string& broadcastName,
                                          BroadcastFilterPtr filter,
                                          T... expectedValues) {
        std::shared_ptr<MyTestProvider> testProvider = registerProvider();
        addFilterToTestProvider(testProvider, filter);

        std::shared_ptr<tests::testProxy> testProxy = buildProxy();

        std::int64_t minInterval_ms = 50;
        auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                    500000,   // validity_ms
                    minInterval_ms);  // minInterval_ms

        subscribeTo(testProxy.get(), subscriptionListener, subscriptionQos);

        (*testProvider.*fireBroadcast)(expectedValues...);

        // Wait for a subscription message to arrive
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    }
};

} // namespace joynr

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithEnumOutput) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }
    tests::testTypes::TestEnum::Enum expectedTestEnum = tests::testTypes::TestEnum::TWO;

    testOneShotBroadcastSubscription(
        expectedTestEnum,
        [this](
            tests::testProxy* testProxy,
            std::shared_ptr<ISubscriptionListener<tests::testTypes::TestEnum::Enum>> subscriptionListener,
            std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos
        ) {
            std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                    testProxy->subscribeToBroadcastWithEnumOutputBroadcast(
                        subscriptionListener,
                        subscriptionQos
                    );
            std::string subscriptionId;
            JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));
        },
        &tests::testProvider::fireBroadcastWithEnumOutput,
        "broadcastWithEnumOutput"
    );
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithByteBufferParameter) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }
    joynr::ByteBuffer expectedByteBuffer {0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0};

    testOneShotBroadcastSubscription(
        expectedByteBuffer,
        [this](
            tests::testProxy* testProxy,
            std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer>> subscriptionListener,
            std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos
        ) {
            std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                    testProxy->subscribeToBroadcastWithByteBufferParameterBroadcast(
                        subscriptionListener,
                        subscriptionQos
                    );
            std::string subscriptionId;
            JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));
        },
        &tests::testProvider::fireBroadcastWithByteBufferParameter,
        "broadcastWithByteBufferParameter"
    );
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

    MockSubscriptionListenerFiveTypes<
            std::string,
            std::vector<std::string>,
            std::vector<joynr::tests::testTypes::TestEnum::Enum>,
            joynr::types::TestTypes::TEverythingStruct,
            std::vector<joynr::types::TestTypes::TEverythingStruct>
    >* mockListener = new MockSubscriptionListenerFiveTypes<
            std::string,
            std::vector<std::string>,
            std::vector<joynr::tests::testTypes::TestEnum::Enum>,
            joynr::types::TestTypes::TEverythingStruct,
            std::vector<joynr::types::TestTypes::TEverythingStruct>
    >();

    // Use a semaphore to count and wait on calls to the mock listener
    ON_CALL(*mockListener, onReceive(Eq(stringOut),
                                     Eq(stringArrayOut),
                                     Eq(enumerationArrayOut),
                                     Eq(structWithStringArrayOut),
                                     Eq(structWithStringArrayArrayOut)))
            .WillByDefault(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<
            std::string,
            std::vector<std::string>,
            std::vector<joynr::tests::testTypes::TestEnum::Enum>,
            joynr::types::TestTypes::TEverythingStruct,
            std::vector<joynr::types::TestTypes::TEverythingStruct>
    >> subscriptionListener(mockListener);


    auto filter = std::make_shared<MockTestBroadcastWithFilteringBroadcastFilter>();
    ON_CALL(*filter, filter(Eq(stringOut),
                            Eq(stringArrayOut),
                            Eq(enumerationArrayOut),
                            Eq(structWithStringArrayOut),
                            Eq(structWithStringArrayArrayOut),
                            _))
           .WillByDefault(DoAll(ReleaseSemaphore(&altSemaphore), Return(true)));

    testOneShotBroadcastSubscriptionWithFiltering(
        subscriptionListener,
        [this](
            tests::testProxy* testProxy,
            std::shared_ptr<joynr::ISubscriptionListener<
                std::string,
                std::vector<std::string>,
                std::vector<joynr::tests::testTypes::TestEnum::Enum>,
                joynr::types::TestTypes::TEverythingStruct,
                std::vector<joynr::types::TestTypes::TEverythingStruct>
            >> subscriptionListener,
            std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos
        ) {
            joynr::tests::TestBroadcastWithFilteringBroadcastFilterParameters filterParameters;
            std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                    testProxy->subscribeToBroadcastWithFilteringBroadcast(
                        filterParameters,
                        subscriptionListener,
                        subscriptionQos);
            std::string subscriptionId;
            JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));
        },
        &tests::testProvider::fireBroadcastWithFiltering,
        "broadcastWithFiltering",
        filter,
        stringOut,
        stringArrayOut,
        enumerationArrayOut,
        structWithStringArrayOut,
        structWithStringArrayArrayOut
    );

    // Wait for a subscription message to arrive
    ASSERT_TRUE(altSemaphore.waitFor(std::chrono::seconds(3)));

}

TEST_P(End2EndBroadcastTest, subscribeTwiceToSameBroadcast_OneOutput) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener =
            std::make_shared<MockGpsSubscriptionListener>();
    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener2 =
            std::make_shared<MockGpsSubscriptionListener>();

    // Use a semaphore to count and wait on calls to the mock listener
    // we expect to notifications before updating the subscription
    // on the second call we release the sync semaphore
    testing::Sequence semaphoreReleaseSequence;
    EXPECT_CALL(*mockSubscriptionListener, onReceive(_))
            .Times(1)
            .InSequence(semaphoreReleaseSequence);
    EXPECT_CALL(*mockSubscriptionListener, onReceive(_))
            .Times(1)
            .InSequence(semaphoreReleaseSequence)
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*mockSubscriptionListener2, onReceive(_))
            .Times(2);

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    auto future = testProxy->subscribeToLocationUpdateBroadcast(
        mockSubscriptionListener,
        subscriptionQos
    );

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW(future->get(5000, subscriptionId));

    testProvider->fireLocationUpdate(gpsLocation2);

    // Waiting between   occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdate(gpsLocation2);

    
    // make sure the last fireLocationUpdate is received by the first listener
    // before updating the subscription
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    // update subscription, much longer minInterval_ms
    subscriptionQos->setMinIntervalMs(5000);
    future = testProxy->subscribeToLocationUpdateBroadcast(
        subscriptionId,
        mockSubscriptionListener2,
        subscriptionQos
    );
    JOYNR_ASSERT_NO_THROW(future->get(5000, subscriptionId));

    testProvider->fireLocationUpdate(gpsLocation2);

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    //now, the next broadcast shall not be received, as the minInterval has been updated
    testProvider->fireLocationUpdate(gpsLocation2);

    //ensure to wait for the minInterval_ms before ending
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));
}

TEST_P(End2EndBroadcastTest, subscribeAndUnsubscribeFromBroadcast_OneOutput) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> subscriptionListener(
                std::make_shared<MockGpsSubscriptionListener>()
    );

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    const std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,         // validity_ms
                minInterval_ms  // minInterval_ms
    );

    auto future = testProxy->subscribeToLocationUpdateBroadcast(
                subscriptionListener,
                subscriptionQos
    );

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW(future->get(5000, subscriptionId));

    EXPECT_CALL(*subscriptionListener, onReceive(Eq(gpsLocation2)))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    testProvider->fireLocationUpdate(gpsLocation2);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    testProxy->unsubscribeFromLocationUpdateBroadcast(subscriptionId);
    
    EXPECT_CALL(*subscriptionListener, onReceive(Eq(gpsLocation3))).Times(0);
    testProvider->fireLocationUpdate(gpsLocation3);

    //ensure to wait for the minInterval_ms before ending
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_OneOutput) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> mockListener =
            std::make_shared<MockGpsSubscriptionListener>();

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToLocationUpdateBroadcast(
                mockListener,
                subscriptionQos
            );
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(subscribeToBroadcastWait, subscriptionId));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation2))).Times(1);
    testProvider->fireLocationUpdate(gpsLocation2);

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation3))).Times(1);
    testProvider->fireLocationUpdate(gpsLocation3);

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation4)))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));
    testProvider->fireLocationUpdate(gpsLocation4);

    //ensure to wait for the minInterval_ms before ending
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));
}

TEST_P(End2EndBroadcastTest, waitForSuccessfulSubscriptionRegistration) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    const std::int64_t minInterval_ms = 50;
    const std::int64_t validity_ms = 500000;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                validity_ms,
                minInterval_ms
            );

    std::string subscriptionIdFromListener;
    std::string subscriptionIdFromFuture;

    std::shared_ptr<MockGpsSubscriptionListener> mockListener =
            std::make_shared<MockGpsSubscriptionListener>();

    EXPECT_CALL(*mockListener, onSubscribed(_))
            .WillOnce(
                DoAll(
                    SaveArg<0>(&subscriptionIdFromListener),
                    ReleaseSemaphore(&semaphore)
                )
            );

    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToLocationUpdateBroadcast(
                mockListener,
                subscriptionQos
            );

    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(
                              subscribeToBroadcastWait,
                              subscriptionIdFromFuture)
                          );

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);
}

TEST_P(End2EndBroadcastTest, waitForSuccessfulSubscriptionUpdate) {
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
            .WillOnce(DoAll(SaveArg<0>(&initialSubscriptionIdFromListener), ReleaseSemaphore(&semaphore)))
            .WillOnce(DoAll(SaveArg<0>(&updateSubscriptionIdFromListener), ReleaseSemaphore(&semaphore)));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToLocationUpdateBroadcast(
                mockListener,
                subscriptionQos
            );
    // the sequence of calling the onReceive listener and the future resolve is not guaranteed
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(subscribeToBroadcastWait, initialSubscriptionIdFromFuture));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));
    
    EXPECT_EQ(initialSubscriptionIdFromListener, initialSubscriptionIdFromFuture);

    // update subscription
    subscriptionIdFuture = nullptr;
    subscriptionIdFuture = testProxy->subscribeToLocationUpdateBroadcast(
                                initialSubscriptionIdFromListener,
                                mockListener,
                                subscriptionQos
                            );

    // the sequence of calling the onReceive listener and the future resolve is not guaranteed
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(5000, updateSubscriptionIdFromFuture));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    EXPECT_EQ(updateSubscriptionIdFromListener, updateSubscriptionIdFromFuture);
    // subscription id from update is the same as the original subscription id
    EXPECT_EQ(initialSubscriptionIdFromListener, updateSubscriptionIdFromListener);
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_EmptyOutput) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockSubscriptionListenerZeroTypes> mockListener =
            std::make_shared<MockSubscriptionListenerZeroTypes>();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive())
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToEmptyBroadcastBroadcast(
                mockListener,
                subscriptionQos
            );

    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

    testProvider->fireEmptyBroadcast();

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireEmptyBroadcast();
    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireEmptyBroadcast();
    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcast_MultipleOutput) {
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

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation, float> > subscriptionListener(
                    mockListener);

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToLocationUpdateWithSpeedBroadcast(
                subscriptionListener,
                subscriptionQos
            );
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

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

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    testProvider->addBroadcastFilter(filter);

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                filterParameters,
                subscriptionListener,
                subscriptionQos);
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

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
}

TEST_P(End2EndBroadcastTest, subscribeToSelectiveBroadcast_FilterFail) {

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener =
            std::make_shared<MockGpsSubscriptionListener>();

    EXPECT_CALL(*mockSubscriptionListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .Times(0);

    ON_CALL(*filter, filter(_, Eq(filterParameters))).WillByDefault(Return(false));

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();
    testProvider->addBroadcastFilter(filter);

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult =
            testProxy->subscribeToLocationUpdateSelectiveBroadcast(
                filterParameters,
                mockSubscriptionListener,
                subscriptionQos);
    std::string subscriptionId;
    JOYNR_EXPECT_NO_THROW(subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

    // Change the location 3 times

    testProvider->fireLocationUpdate(gpsLocation2);

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation3);

    // Waiting between broadcast occurences for at least the minInterval is neccessary because
    // otherwise the publications could be omitted.
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    testProvider->fireLocationUpdateSelective(gpsLocation4);

    //ensure to wait for the minInterval_ms before ending
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));
}

TEST_P(End2EndBroadcastTest, subscribeToBroadcastWithSameNameAsAttribute) {
    if (usesHttpTransport()) {
        FAIL() << "multicast subscription via HTTP not implemented";
    }

    std::shared_ptr<MockGpsSubscriptionListener> mockListenerAttribute =
            std::make_shared<MockGpsSubscriptionListener>();
    std::shared_ptr<MockGpsSubscriptionListener> mockListenerBroadcast =
            std::make_shared<MockGpsSubscriptionListener>();

    std::shared_ptr<MyTestProvider> testProvider = registerProvider();

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms

    // Initial attribute publication on subscription
    EXPECT_CALL(*mockListenerAttribute, onReceive(Eq(gpsLocation)))
            .Times(1);

    std::shared_ptr<joynr::Future<std::string>> subscriptionAttributeResult = testProxy->subscribeToLocation(
                mockListenerAttribute,
                subscriptionQos);
    std::string subscriptionId;
    // Wait until the provider sends back a subscriptionReply, i.e. the subscription is
    // established successful

    JOYNR_EXPECT_NO_THROW(subscriptionAttributeResult->get(subscribeToAttributeWait, subscriptionId));

    std::shared_ptr<joynr::Future<std::string>> subscriptionBroadcastResult = testProxy->subscribeToLocationBroadcast(
                mockListenerBroadcast,
                subscriptionQos);
    // Wait until the provider sends back a subscriptionReply, i.e. the subscription is
    // established successful
    JOYNR_EXPECT_NO_THROW(subscriptionBroadcastResult->get(subscribeToBroadcastWait, subscriptionId));

    //ensure to wait for the minInterval_ms before changing location
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));

    // Expect initial attribute publication with default value
    EXPECT_CALL(*mockListenerAttribute, onReceive(Eq(gpsLocation2))).Times(1);

    // Change attribute
    testProvider->locationChanged(gpsLocation2);

    EXPECT_CALL(*mockListenerBroadcast, onReceive(Eq(gpsLocation3))).Times(1);

    // Emit broadcast
    testProvider->fireLocation(gpsLocation3);

    //ensure to wait for the minInterval_ms before ending
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms));
}

INSTANTIATE_TEST_CASE_P(DISABLED_Http,
        End2EndBroadcastTest,
        testing::Values(
            std::make_tuple(
                "test-resources/HttpSystemIntegrationTest1.settings",
                "test-resources/HttpSystemIntegrationTest2.settings"
            )
        )
);

INSTANTIATE_TEST_CASE_P(MqttWithHttpBackend,
        End2EndBroadcastTest,
        testing::Values(
            std::make_tuple(
                "test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings",
                "test-resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
            )
        )
);

/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"
#include <boost/algorithm/string/predicate.hpp>

#include "joynr/BrokerUrl.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Settings.h"
#include "joynr/tests/testAbstractProvider.h"
#include "joynr/tests/testProxy.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockLocationUpdatedSelectiveFilter.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/TestJoynrClusterControllerRuntime.h"
#include "tests/utils/MyTestProvider.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

static const std::string messagingPropertiesPersistenceFileName1(
        "End2EndBroadcastTest-runtime1-joynr.persist");
static const std::string messagingPropertiesPersistenceFileName2(
        "End2EndBroadcastTest-runtime2-joynr.persist");

namespace joynr
{

class End2EndBroadcastTestBase : public TestWithParam<std::tuple<std::string, std::string>>
{
public:
    std::shared_ptr<TestJoynrClusterControllerRuntime> runtime1;
    std::shared_ptr<TestJoynrClusterControllerRuntime> runtime2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    Semaphore semaphore;
    Semaphore altSemaphore;
    joynr::tests::TestLocationUpdateSelectiveBroadcastFilterParameters filterParameters;
    std::shared_ptr<MockLocationUpdatedSelectiveFilter> filter;
    std::uint16_t subscribeToAttributeWait;
    std::uint16_t subscribeToBroadcastWait;
    joynr::types::Localisation::GpsLocation gpsLocation;
    joynr::types::Localisation::GpsLocation gpsLocation2;
    joynr::types::Localisation::GpsLocation gpsLocation3;
    joynr::types::Localisation::GpsLocation gpsLocation4;

    End2EndBroadcastTestBase()
            : runtime1(),
              runtime2(),
              baseUuid(util::createUuid()),
              uuid("_" + baseUuid.substr(1, baseUuid.length() - 2)),
              domainName("cppEnd2EndBroadcastTest_Domain" + uuid),
              semaphore(0),
              altSemaphore(0),
              filter(std::make_shared<MockLocationUpdatedSelectiveFilter>()),
              subscribeToAttributeWait(2000),
              subscribeToBroadcastWait(2000),
              gpsLocation(types::Localisation::GpsLocation()),
              gpsLocation2(types::Localisation::GpsLocation(9.0,
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
              gpsLocation3(types::Localisation::GpsLocation(9.0,
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
              gpsLocation4(types::Localisation::GpsLocation(9.0,
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
              providerParticipantId(),
              integration1Settings("test-resources/libjoynrSystemIntegration1.settings"),
              integration2Settings("test-resources/libjoynrSystemIntegration2.settings")
    {
        auto settings1 = std::make_unique<Settings>(std::get<0>(GetParam()));
        auto settings2 = std::make_unique<Settings>(std::get<1>(GetParam()));
        MessagingSettings messagingSettings1(*settings1);
        MessagingSettings messagingSettings2(*settings2);
        messagingSettings1.setMessagingPropertiesPersistenceFilename(
                messagingPropertiesPersistenceFileName1);
        messagingSettings2.setMessagingPropertiesPersistenceFilename(
                messagingPropertiesPersistenceFileName2);

        Settings::merge(integration1Settings, *settings1, false);

        runtime1 = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::move(settings1), failOnFatalRuntimeError);
        runtime1->init();

        Settings::merge(integration2Settings, *settings2, false);

        runtime2 = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::move(settings2), failOnFatalRuntimeError);
        runtime2->init();

        filterParameters.setCountry("Germany");
        filterParameters.setStartTime("4.00 pm");
        runtime1->start();
        runtime2->start();
    }

    ~End2EndBroadcastTestBase() override
    {
        if (!providerParticipantId.empty()) {
            unregisterProvider();
        }

        runtime1->shutdown();
        runtime2->shutdown();

        test::util::resetAndWaitUntilDestroyed(runtime1);
        test::util::resetAndWaitUntilDestroyed(runtime2);

        // Delete persisted files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

private:
    std::string providerParticipantId;
    Settings integration1Settings;
    Settings integration2Settings;
    DISALLOW_COPY_AND_ASSIGN(End2EndBroadcastTestBase);

protected:
    std::shared_ptr<MyTestProvider> registerProvider()
    {
        return registerProvider(runtime1);
    }

    void unregisterProvider()
    {
        return runtime1->unregisterProvider(providerParticipantId);
    }

    std::shared_ptr<MyTestProvider> registerProvider(
            std::shared_ptr<TestJoynrClusterControllerRuntime> runtime)
    {
        auto testProvider = std::make_shared<MyTestProvider>();
        constexpr bool persist{true};
        constexpr bool awaitGlobalRegistration{true};
        types::ProviderQos providerQos;
        std::chrono::milliseconds millisSinceEpoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
        providerQos.setPriority(millisSinceEpoch.count());
        providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
        providerQos.setSupportsOnChangeSubscriptions(true);
        providerParticipantId = runtime->registerProvider<tests::testProvider>(
                domainName, testProvider, providerQos, persist, awaitGlobalRegistration);

        return testProvider;
    }

    std::shared_ptr<tests::testProxy> buildProxy()
    {
        return buildProxy(runtime2);
    }

    std::shared_ptr<tests::testProxy> buildProxy(
            std::shared_ptr<TestJoynrClusterControllerRuntime> runtime)
    {
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                runtime->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(30000);
        discoveryQos.setRetryIntervalMs(500);

        std::uint64_t qosRoundTripTTL = 40000;

        std::shared_ptr<tests::testProxy> testProxy(
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(discoveryQos)
                        ->build());

        return testProxy;
    }

    template <typename FireBroadcast, typename SubscribeTo, typename UnsubscribeFrom, typename T>
    void testOneShotBroadcastSubscription(const T& expectedValue,
                                          SubscribeTo subscribeTo,
                                          UnsubscribeFrom unsubscribeFrom,
                                          FireBroadcast fireBroadcast)
    {
        auto mockListener = std::make_shared<MockSubscriptionListenerOneType<T>>();

        // Use a semaphore to count and wait on calls to the mock listener
        EXPECT_CALL(*mockListener, onReceive(Eq(expectedValue)))
                .WillOnce(ReleaseSemaphore(&semaphore));

        testOneShotBroadcastSubscription(
                mockListener, subscribeTo, unsubscribeFrom, fireBroadcast, expectedValue);
    }

    template <typename SubscriptionListener,
              typename FireBroadcast,
              typename SubscribeTo,
              typename UnsubscribeFrom,
              typename... T>
    void testOneShotBroadcastSubscription(SubscriptionListener subscriptionListener,
                                          SubscribeTo subscribeTo,
                                          UnsubscribeFrom unsubscribeFrom,
                                          FireBroadcast fireBroadcast,
                                          T... expectedValues)
    {
        std::vector<std::string> partitions({}); // TODO test with real partitions
        std::shared_ptr<MyTestProvider> testProvider = registerProvider();

        std::shared_ptr<tests::testProxy> testProxy = buildProxy();

        auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
        subscriptionQos->setValidityMs(500000);

        std::string subscriptionId;
        subscribeTo(testProxy.get(), subscriptionListener, subscriptionQos, subscriptionId);

        delayForMqttSubscribeOrUnsubscribe();

        (*testProvider.*fireBroadcast)(expectedValues..., partitions);

        // Wait for a subscription message to arrive
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
        unsubscribeFrom(testProxy.get(), subscriptionId);

        delayForMqttSubscribeOrUnsubscribe();
    }

    template <typename BroadcastFilter>
    void addFilterToTestProvider(std::shared_ptr<MyTestProvider> testProvider,
                                 std::shared_ptr<BroadcastFilter> filter)
    {
        if (filter) {
            testProvider->addBroadcastFilter(filter);
        }
    }

    void addFilterToTestProvider(std::shared_ptr<MyTestProvider> testProvider,
                                 std::nullptr_t filter)
    {
        std::ignore = testProvider;
        std::ignore = filter;
    }

    template <typename SubscriptionListener,
              typename FireBroadcast,
              typename SubscribeTo,
              typename UnsubscribeFrom,
              typename BroadcastFilterPtr,
              typename... T>
    void testOneShotBroadcastSubscriptionWithFiltering(SubscriptionListener subscriptionListener,
                                                       SubscribeTo subscribeTo,
                                                       UnsubscribeFrom unsubscribeFrom,
                                                       FireBroadcast fireBroadcast,
                                                       BroadcastFilterPtr filter,
                                                       T... expectedValues)
    {
        std::shared_ptr<MyTestProvider> testProvider = registerProvider();
        addFilterToTestProvider(testProvider, filter);

        std::shared_ptr<tests::testProxy> testProxy = buildProxy();

        std::int64_t minInterval_ms = 50;
        std::string subscriptionId;
        auto subscriptionQos =
                std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                          1000,            // publication ttl
                                                          minInterval_ms); // minInterval_ms

        subscribeTo(testProxy.get(), subscriptionListener, subscriptionQos, subscriptionId);

        (*testProvider.*fireBroadcast)(expectedValues...);

        // Wait for a subscription message to arrive
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
        unsubscribeFrom(testProxy.get(), subscriptionId);
    }

    void delayForMqttSubscribeOrUnsubscribe()
    {
        // wait some time so that MQTT subscribe/unsubscribe can be
        // executed by MQTT client and MQTT broker
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
};

} // namespace joynr

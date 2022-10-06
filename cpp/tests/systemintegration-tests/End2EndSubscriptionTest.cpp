/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/MessagingSettings.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/testAbstractProvider.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/TestJoynrClusterControllerRuntime.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

namespace joynr
{

class End2EndSubscriptionTest : public TestWithParam<std::tuple<std::string, std::string>>
{
public:
    std::shared_ptr<JoynrClusterControllerRuntime> runtime1;
    std::shared_ptr<JoynrClusterControllerRuntime> runtime2;
    std::unique_ptr<Settings> settings1;
    std::unique_ptr<Settings> settings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    joynr::Semaphore semaphore;
    unsigned long registerProviderWait;
    unsigned long subscribeToAttributeWait;
    joynr::types::Localisation::GpsLocation gpsLocation;

    End2EndSubscriptionTest()
            : runtime1(),
              runtime2(),
              settings1(std::make_unique<Settings>(std::get<0>(GetParam()))),
              settings2(std::make_unique<Settings>(std::get<1>(GetParam()))),
              baseUuid(util::createUuid()),
              uuid("_" + baseUuid.substr(1, baseUuid.length() - 2)),
              domainName("cppEnd2EndSubscriptionTest_Domain" + uuid),
              semaphore(0),
              registerProviderWait(1000),
              subscribeToAttributeWait(2000),
              providerParticipantId()
    {
        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings1, false);

        runtime1 = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::move(settings1), failOnFatalRuntimeError);
        runtime1->init();
        runtime1->start();

        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings2, false);

        runtime2 = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::move(settings2), failOnFatalRuntimeError);
        runtime2->init();
        runtime2->start();
    }

    ~End2EndSubscriptionTest() override
    {
        if (!providerParticipantId.empty()) {
            runtime1->unregisterProvider(providerParticipantId);
        }

        runtime1->shutdown();
        runtime2->shutdown();

        test::util::resetAndWaitUntilDestroyed(runtime1);
        test::util::resetAndWaitUntilDestroyed(runtime2);

        // Delete persisted files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

    /*
     *  This wait is necessary, because subcriptions are async, and a publication could occur
     * before the subscription has started.
     */
    void waitForAttributeSubscriptionArrivedAtProvider(
            std::shared_ptr<tests::testAbstractProvider> testProvider,
            const std::string& attributeName)
    {
        std::uint64_t delay = 0;

        while (delay <= subscribeToAttributeWait) {
            {
                ReadLocker locker(testProvider->_lockAttributeListeners);
                if (testProvider->_attributeListeners.find(attributeName) !=
                    testProvider->_attributeListeners.cend()) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            delay += 50;
        }

        ReadLocker locker(testProvider->_lockAttributeListeners);
        EXPECT_FALSE(testProvider->_attributeListeners.find(attributeName) ==
                             testProvider->_attributeListeners.cend() ||
                     testProvider->_attributeListeners.find(attributeName)->second.empty());
    }

private:
    std::string providerParticipantId;
    DISALLOW_COPY_AND_ASSIGN(End2EndSubscriptionTest);

protected:
    std::shared_ptr<tests::DefaulttestProvider> registerProvider()
    {
        auto testProvider = std::make_shared<tests::DefaulttestProvider>();
        types::ProviderQos providerQos;
        std::chrono::milliseconds millisSinceEpoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
        providerQos.setPriority(millisSinceEpoch.count());
        providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
        providerQos.setSupportsOnChangeSubscriptions(true);
        providerParticipantId = runtime1->registerProvider<tests::testProvider>(
                domainName, testProvider, providerQos);

        // This wait is necessary, because registerProvider is async, and a lookup could occur
        // before the register has finished.
        std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));
        return testProvider;
    }

    std::shared_ptr<tests::testProxy> buildProxy()
    {
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(3000);
        discoveryQos.setRetryIntervalMs(250);

        std::uint64_t qosRoundTripTTL = 500;

        std::shared_ptr<tests::testProxy> testProxy =
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(discoveryQos)
                        ->build();
        return testProxy;
    }

    template <typename ChangeAttribute, typename SubscribeTo, typename UnsubscribeFrom, typename T>
    void testOneShotAttributeSubscription(const T& expectedValue,
                                          SubscribeTo subscribeTo,
                                          UnsubscribeFrom unsubscribeFrom,
                                          ChangeAttribute setAttribute,
                                          const std::string& attributeName)
    {
        MockSubscriptionListenerOneType<T>* mockListener = new MockSubscriptionListenerOneType<T>();

        // Use a semaphore to count and wait on calls to the mock listener
        ON_CALL(*mockListener, onReceive(Eq(expectedValue)))
                .WillByDefault(ReleaseSemaphore(&semaphore));

        std::shared_ptr<ISubscriptionListener<T>> subscriptionListener(mockListener);

        std::shared_ptr<tests::DefaulttestProvider> testProvider = registerProvider();

        (*testProvider.*setAttribute)(
                expectedValue, []() {}, [](const joynr::exceptions::ProviderRuntimeException&) {});

        std::shared_ptr<tests::testProxy> testProxy = buildProxy();

        std::int64_t minInterval_ms = 50;
        auto subscriptionQos =
                std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                          1000,            // publication ttl
                                                          minInterval_ms); // minInterval_ms

        std::string subscriptionId;
        subscribeTo(testProxy, subscriptionListener, subscriptionQos, subscriptionId);
        waitForAttributeSubscriptionArrivedAtProvider(testProvider, attributeName);

        // Wait for a subscription message to arrive
        EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
        unsubscribeFrom(testProxy, subscriptionId);
    }
};

} // namespace joynr

TEST_P(End2EndSubscriptionTest, waitForSuccessfulSubscriptionRegistration)
{
    auto mockListener = new MockSubscriptionListenerOneType<int32_t>();

    // Use a semaphore to wait for calls to the mock listener
    std::string subscriptionIdFromListener;
    std::string subscriptionIdFromFuture;
    EXPECT_CALL(*mockListener, onSubscribed(_))
            .WillOnce(DoAll(SaveArg<0>(&subscriptionIdFromListener), ReleaseSemaphore(&semaphore)));

    std::shared_ptr<ISubscriptionListener<int32_t>> subscriptionListener(mockListener);

    std::shared_ptr<tests::DefaulttestProvider> testProvider = registerProvider();

    testProvider->setTestAttribute(
            42,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException& error) {
                ADD_FAILURE() << "exception from setTestAttribute: " << error.getMessage();
            });

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos =
            std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                      1000,            // publication ttl
                                                      minInterval_ms); // minInterval_ms
    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos);

    waitForAttributeSubscriptionArrivedAtProvider(testProvider, "testAttribute");

    // Wait for a subscription reply message to arrive
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(5000, subscriptionIdFromFuture););
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);
    JOYNR_EXPECT_NO_THROW(testProxy->unsubscribeFromTestAttribute(subscriptionIdFromFuture));
}

TEST_P(End2EndSubscriptionTest, waitForSuccessfulSubscriptionUpdate)
{
    auto mockListener = new MockSubscriptionListenerOneType<int32_t>();

    // Use a semaphore to wait for calls to the mock listener
    std::string subscriptionIdFromListener;
    std::string subscriptionIdFromFuture;
    EXPECT_CALL(*mockListener, onSubscribed(_))
            .Times(2)
            .WillRepeatedly(
                    DoAll(SaveArg<0>(&subscriptionIdFromListener), ReleaseSemaphore(&semaphore)));

    std::shared_ptr<ISubscriptionListener<int32_t>> subscriptionListener(mockListener);

    std::shared_ptr<tests::DefaulttestProvider> testProvider = registerProvider();

    testProvider->setTestAttribute(
            42,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException& error) {
                ADD_FAILURE() << "exception from setTestAttribute: " << error.getMessage();
            });

    std::shared_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos =
            std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                      1000,            // publication ttl
                                                      minInterval_ms); // minInterval_ms
    std::shared_ptr<Future<std::string>> subscriptionIdFuture =
            testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos);

    waitForAttributeSubscriptionArrivedAtProvider(testProvider, "testAttribute");

    // Wait for a subscription reply message to arrive
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(5000, subscriptionIdFromFuture););
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);

    // update subscription
    subscriptionIdFuture = nullptr;
    std::string subscriptionId = subscriptionIdFromFuture;
    subscriptionIdFromFuture.clear();
    subscriptionIdFromListener.clear();
    subscriptionIdFuture = testProxy->subscribeToTestAttribute(
            subscriptionListener, subscriptionQos, subscriptionId);

    // Wait for a subscription reply message to arrive
    JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(5000, subscriptionIdFromFuture););
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);
    // subscription id from update is the same as the original subscription id
    EXPECT_EQ(subscriptionId, subscriptionIdFromFuture);
    JOYNR_EXPECT_NO_THROW(testProxy->unsubscribeFromTestAttribute(subscriptionId));
}

TEST_P(End2EndSubscriptionTest, subscribeToEnumAttribute)
{
    tests::testTypes::TestEnum::Enum expectedTestEnum = tests::testTypes::TestEnum::TWO;

    testOneShotAttributeSubscription(
            expectedTestEnum,
            [this](std::shared_ptr<tests::testProxy>& testProxy,
                   std::shared_ptr<ISubscriptionListener<tests::testTypes::TestEnum::Enum>>
                           subscriptionListener,
                   std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos,
                   std::string& subscriptionId) {
                std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                        testProxy->subscribeToEnumAttribute(subscriptionListener, subscriptionQos);
                JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(
                        static_cast<std::int64_t>(subscribeToAttributeWait), subscriptionId));
            },
            [](std::shared_ptr<tests::testProxy>& testProxy, std::string& subscriptionId) {
                testProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
            },
            &tests::testProvider::setEnumAttribute,
            "enumAttribute");
}

TEST_P(End2EndSubscriptionTest, subscribeToByteBufferAttribute)
{
    joynr::ByteBuffer expectedByteBuffer{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    testOneShotAttributeSubscription(
            expectedByteBuffer,
            [this](std::shared_ptr<tests::testProxy>& testProxy,
                   std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer>> subscriptionListener,
                   std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos,
                   std::string& subscriptionId) {
                std::shared_ptr<Future<std::string>> subscriptionIdFuture =
                        testProxy->subscribeToByteBufferAttribute(
                                subscriptionListener, subscriptionQos);
                JOYNR_EXPECT_NO_THROW(subscriptionIdFuture->get(
                        static_cast<std::int64_t>(subscribeToAttributeWait), subscriptionId));
            },
            [](std::shared_ptr<tests::testProxy>& testProxy, std::string& subscriptionId) {
                testProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
            },
            &tests::testProvider::setByteBufferAttribute,
            "byteBufferAttribute");
}

TEST_P(End2EndSubscriptionTest, publishAfterProxyDestruction)
{
    auto mockListener = new MockSubscriptionListenerOneType<int32_t>();

    ON_CALL(*mockListener, onReceive(Eq(42))).WillByDefault(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<int32_t>> subscriptionListener(mockListener);
    std::shared_ptr<tests::DefaulttestProvider> testProvider = registerProvider();

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    std::uint64_t qosRoundTripTTL = 500;

    std::shared_ptr<tests::testProxy> testProxy =
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(discoveryQos)
                    ->build();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos =
            std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                      1000,            // publication ttl
                                                      minInterval_ms); // minInterval_ms
    testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos);
    waitForAttributeSubscriptionArrivedAtProvider(testProvider, "testAttribute");
    std::string participantId{testProxy->getProxyParticipantId()};

    std::shared_ptr<ProxyBuilder<joynr::system::RoutingProxy>> routingProxyBuilder =
            runtime2->createProxyBuilder<joynr::system::RoutingProxy>("io.joynr.system");

    std::shared_ptr<joynr::system::RoutingProxy> routingProxy(
            routingProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    // Destruct the proxy
    test::util::resetAndWaitUntilDestroyed(testProxy);
    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(500)));

    testProvider->setTestAttribute(
            42,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException& error) {
                ADD_FAILURE() << "exception from setTestAttribute: " << error.getMessage();
            });
    // Wait for a subscription reply message to arrive
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    bool resolved{false};
    routingProxy->resolveNextHop(resolved, participantId);
    ASSERT_TRUE(resolved);
}

using namespace std::string_literals;

INSTANTIATE_TEST_SUITE_P(
        Mqtt,
        End2EndSubscriptionTest,
        testing::Values(std::make_tuple("test-resources/MqttSystemIntegrationTest1.settings"s,
                                        "test-resources/MqttSystemIntegrationTest2.settings"s)));

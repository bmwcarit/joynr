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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"

#include "libjoynrclustercontroller/capabilities-directory/GlobalCapabilitiesDirectoryClient.h"

#include "tests/JoynrTest.h"
#include "tests/utils/PtrUtils.h"

#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"
#include "tests/mock/MockTestProvider.h"

using namespace ::testing;
using namespace joynr;

static const std::string libJoynrSettingsFilename(
        "test-resources/libjoynrSystemIntegration1.settings");

class GlobalCapabilitiesMock
{
public:
    MOCK_METHOD1(capabilitiesReceived,
                 void(const std::vector<joynr::types::GlobalDiscoveryEntry>& results));
};

class GlobalCapabilitiesDirectoryIntegrationTest : public TestWithParam<std::string>
{
public:
    ADD_LOGGER(GlobalCapabilitiesDirectoryIntegrationTest)
    std::shared_ptr<JoynrClusterControllerRuntime> runtime;
    std::unique_ptr<Settings> settings;
    MessagingSettings messagingSettings;
    ClusterControllerSettings clusterControllerSettings;
    SystemServicesSettings sysSettings;

    GlobalCapabilitiesDirectoryIntegrationTest()
            : runtime(),
              settings(std::make_unique<Settings>(GetParam())),
              messagingSettings(*settings),
              clusterControllerSettings(*settings),
              sysSettings(*settings)
    {
        clusterControllerSettings.setCapabilitiesFreshnessUpdateIntervalMs(std::chrono::milliseconds(500));
    }

    void initCCRuntime()
    {
        Settings libjoynrSettings{libJoynrSettingsFilename};
        Settings::merge(libjoynrSettings, *settings, false);
        runtime = std::make_shared<JoynrClusterControllerRuntime>(std::move(settings), failOnFatalRuntimeError);
        runtime->init();
        runtime->start();
    }

    ~GlobalCapabilitiesDirectoryIntegrationTest() override
    {
        if(runtime) {
            runtime->shutdown();
            test::util::resetAndWaitUntilDestroyed(runtime);
        }

        // Delete persisted files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(GlobalCapabilitiesDirectoryIntegrationTest);
};

TEST_P(GlobalCapabilitiesDirectoryIntegrationTest, registerAndRetrieveCapability)
{
    initCCRuntime();
    const std::vector<std::string> gbids {"testGbid"};
    std::shared_ptr<ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
            capabilitiesProxyBuilder =
                    runtime->createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                            messagingSettings.getDiscoveryDirectoriesDomain());

    DiscoveryQos discoveryQos(10000);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.addCustomParameter(
            "fixedParticipantId", messagingSettings.getCapabilitiesDirectoryParticipantId());
    const MessagingQos messagingQos(10000);
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> cabilitiesProxy(
            capabilitiesProxyBuilder->setMessagingQos(messagingQos)
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    std::unique_ptr<GlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient(
            std::make_unique<GlobalCapabilitiesDirectoryClient>(clusterControllerSettings,
                std::make_unique<TaskSequencer<void>>(std::chrono::milliseconds(MessagingQos().getTtl()))));
    globalCapabilitiesDirectoryClient->setProxy(cabilitiesProxy);

    std::string capDomain("testDomain");
    std::string capInterface("testInterface");
    types::ProviderQos capProviderQos;
    std::string capParticipantId("testParticipantId");
    std::string capPublicKeyId("publicKeyId");
    joynr::types::Version providerVersion(47, 11);
    std::int64_t capLastSeenMs = 0;
    std::int64_t capExpiryDateMs = 1000;
    std::string capSerializedMqttAddress("{\"_typeName\":\"joynr.system.RoutingTypes.MqttAddress\",\"brokerUri\":\"testGbid\",\"topic\":\"testTopic}");
    types::GlobalDiscoveryEntry globalDiscoveryEntry(providerVersion,
                                                     capDomain,
                                                     capInterface,
                                                     capParticipantId,
                                                     capProviderQos,
                                                     capLastSeenMs,
                                                     capExpiryDateMs,
                                                     capPublicKeyId,
                                                     capSerializedMqttAddress);

    JOYNR_LOG_DEBUG(logger(), "Registering capabilities");
    globalCapabilitiesDirectoryClient->add(
                globalDiscoveryEntry,
                false,
                gbids,
                []() {},
                [](const joynr::types::DiscoveryError::Enum& /*error*/) {},
                [](const joynr::exceptions::JoynrRuntimeException& /*exception*/) {});
    JOYNR_LOG_DEBUG(logger(), "Registered capabilities");

    auto callback = std::make_shared<GlobalCapabilitiesMock>();

    // use a semaphore to wait for capabilities to be received
    Semaphore semaphore(0);
    EXPECT_CALL(
            *callback, capabilitiesReceived(A<const std::vector<types::GlobalDiscoveryEntry>&>()))
            .WillRepeatedly(DoAll(ReleaseSemaphore(&semaphore), Return()));
    std::function<void(const std::vector<types::GlobalDiscoveryEntry>&)> onSuccess =
            [&](const std::vector<types::GlobalDiscoveryEntry>& capabilities) {
        callback->capabilitiesReceived(capabilities);
    };

    JOYNR_LOG_DEBUG(logger(), "get capabilities");
    std::int64_t defaultDiscoveryMessageTtl = messagingSettings.getDiscoveryMessagesTtl();
    globalCapabilitiesDirectoryClient->lookup(
                {capDomain},
                capInterface,
                gbids,
                defaultDiscoveryMessageTtl,
                onSuccess,
                [](const joynr::types::DiscoveryError::Enum& /*error*/) {},
                [](const joynr::exceptions::JoynrRuntimeException& /*exception*/) {});
    semaphore.waitFor(std::chrono::seconds(10));
    JOYNR_LOG_DEBUG(logger(), "finished get capabilities");
}

/**
 * Test removing stale providers functionality of cluster controller when it is starting
 *
 * Pre-conditions: start cluster controller first time, register provider, shutdown cluster controller
 * without calling of unregisterProvider() method. Start cluster controller second time.
 *
 * Expected behavior: testProxyBuilder->build() throws an exception after the second start of
 * cluster controller, because stale provider has been removed at the start of cluster controller.
 */
TEST_P(GlobalCapabilitiesDirectoryIntegrationTest, testRemoveStaleWithoutDelay)
{
    // Setup
    std::int64_t removeStaleDelayMs = 0;
    std::string domain = "cppTestRsDomain";
    auto mockProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    auto millisSinceEpoch = TimePoint::now().toMilliseconds();
    providerQos.setPriority(millisSinceEpoch);
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    const std::int64_t discoveryTimeoutMs = 3000.0;
    joynr::DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setCacheMaxAgeMs(0.0);
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs + 50.0);

    // Start cluster controller runtime first time
    auto testRuntimeFirst = std::make_shared<JoynrClusterControllerRuntime>(
            std::make_unique<Settings>(GetParam()), failOnFatalRuntimeError, nullptr, nullptr, removeStaleDelayMs);
    testRuntimeFirst->init();
    testRuntimeFirst->start();

    std::string providerParticipantId = testRuntimeFirst->registerProvider<tests::testProvider>(domain, mockProvider, providerQos, true, true);

    auto testProxyBuilder = testRuntimeFirst->createProxyBuilder<tests::testProxy>(domain);

    auto testProxy = testProxyBuilder->setMessagingQos(MessagingQos())
                    ->setDiscoveryQos(discoveryQos)
                    ->build();

    testRuntimeFirst->shutdown();
    test::util::resetAndWaitUntilDestroyed(testRuntimeFirst);

    // Start cluster controller runtime second time
    auto testRuntimeSecond = std::make_shared<JoynrClusterControllerRuntime>(
            std::make_unique<Settings>(GetParam()), failOnFatalRuntimeError, nullptr, nullptr, removeStaleDelayMs);
    testRuntimeSecond->init();
    testRuntimeSecond->start();
    // wait some time to make sure that removeStale has been published and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    testProxyBuilder = testRuntimeSecond->createProxyBuilder<tests::testProxy>(domain);

    try {
        testProxy = testProxyBuilder->setMessagingQos(MessagingQos())
                ->setDiscoveryQos(discoveryQos)
                ->build();
        FAIL() << "Proxy creation succeeded unexpectedly";
    } catch (const exceptions::JoynrException& e) {
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "No entries found for domain";
        bool messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
        ASSERT_TRUE(messageFound);
    }

    testRuntimeSecond->shutdown();

    test::util::resetAndWaitUntilDestroyed(testRuntimeSecond);
}

/**
 * Test removing stale providers functionality of cluster controller with the delay after CC start
 *
 * Pre-conditions: start cluster controller first time, register two providers, shutdown cluster controller
 * without calling of unregisterProvider() method. Start cluster controller second time.
 *
 * Expected behavior: after the second start of cluster controller, first call of testProxyBuilder->build() will
 * be successful (because it is called before the call of removeStale) and it will throw an exception at the
 * second call because stale providers have been removed after waiting for a delay.
 */
TEST_P(GlobalCapabilitiesDirectoryIntegrationTest, testRemoveStaleWithDelay)
{
    // Setup
    std::int64_t removeStaleDelayMs = 2000;
    std::string domainFirst = "cppTestRsDomainFirst";
    std::string domainSecond = "cppTestRsDomainSecond";
    auto mockProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQosFirst;
    auto millisSinceEpoch = TimePoint::now().toMilliseconds();
    providerQosFirst.setPriority(millisSinceEpoch);
    providerQosFirst.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQosFirst.setSupportsOnChangeSubscriptions(true);

    types::ProviderQos providerQosSecond {providerQosFirst};
    providerQosSecond.setPriority(millisSinceEpoch + 10);

    const std::int64_t discoveryTimeoutMs = 3000;
    joynr::DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setCacheMaxAgeMs(0.0);
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs + 50);

    // Start cluster controller runtime first time
    auto settings1 = std::make_unique<Settings>(GetParam());
    ClusterControllerSettings ccSettings1(*settings1);
    ccSettings1.setUdsEnabled(false);
    auto testRuntimeFirst = std::make_shared<JoynrClusterControllerRuntime>(
            std::move(settings1), failOnFatalRuntimeError, nullptr, nullptr, removeStaleDelayMs);
    testRuntimeFirst->init();
    testRuntimeFirst->start();

    testRuntimeFirst->registerProvider<tests::testProvider>(domainFirst, mockProvider, providerQosFirst, true, true);
    testRuntimeFirst->registerProvider<tests::testProvider>(domainSecond, mockProvider, providerQosSecond, true, true);

    testRuntimeFirst->shutdown();
    test::util::resetAndWaitUntilDestroyed(testRuntimeFirst);

    // Start cluster controller runtime second time
    auto settings2 = std::make_unique<Settings>(GetParam());
    ClusterControllerSettings ccSettings2(*settings2);
    ccSettings2.setUdsEnabled(false);
    auto testRuntimeSecond = std::make_shared<JoynrClusterControllerRuntime>(
            std::move(settings2), failOnFatalRuntimeError, nullptr, nullptr, removeStaleDelayMs);
    testRuntimeSecond->init();
    testRuntimeSecond->start();
    // wait some time to make sure that cluster controller runtime has been started
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check if proxy is built before removeStale is called
    auto testProxyBuilder = testRuntimeSecond->createProxyBuilder<tests::testProxy>(domainFirst);
    try {
        auto testProxy = testProxyBuilder->setMessagingQos(MessagingQos())
                ->setDiscoveryQos(discoveryQos)
                ->build();
        ASSERT_NE(nullptr, testProxy);
    } catch (const exceptions::JoynrException& e) {
        FAIL() << "Proxy creation failed unexpectedly: " << e.getMessage();
    }

    // wait some time to make sure that removeStale has been published and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(removeStaleDelayMs + 200));

    testProxyBuilder = testRuntimeSecond->createProxyBuilder<tests::testProxy>(domainSecond);
    try {
        testProxyBuilder->setMessagingQos(MessagingQos())
                ->setDiscoveryQos(discoveryQos)
                ->build();
        FAIL() << "Proxy creation succeeded unexpectedly";
    } catch (const exceptions::JoynrException& e) {
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "No entries found for domain";
        bool messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
        ASSERT_TRUE(messageFound);
    }

    testRuntimeSecond->shutdown();
    test::util::resetAndWaitUntilDestroyed(testRuntimeSecond);
}

/**
 * Test touching only the providers actually registered in the clustercontroller
 */
TEST_P(GlobalCapabilitiesDirectoryIntegrationTest, testTouch_updatesGloballyRegisteredProvider)
{
    initCCRuntime();
    JOYNR_LOG_DEBUG(logger(), "testTouchOnlyUpdatesExistingParticipantIds started");
    // Setup
    const std::chrono::milliseconds touchInterval = clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs();
    const std::vector<std::string> lookupGbids = { messagingSettings.getGbid() };

    joynr::DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(3000);
    discoveryQos.setRetryIntervalMs(discoveryQos.getDiscoveryTimeoutMs() + 1);
    discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());

    // register provider
    const std::string domain = "cppTestTouchDomain";
    auto mockProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    const auto millisSinceEpoch = TimePoint::now().toMilliseconds();
    providerQos.setPriority(millisSinceEpoch);
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    const auto timeBeforeAdd = TimePoint::now().toMilliseconds();
    const std::string providerParticipantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockProvider,
                providerQos,
                true,
                true);

    // build GCD proxy
    std::shared_ptr<ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
            gcdProxyBuilder =
                    runtime->createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                            messagingSettings.getDiscoveryDirectoriesDomain());
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> gcdProxy(
            gcdProxyBuilder
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    // build LCD proxy
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    auto lcdProxyBuilder = runtime->createProxyBuilder<system::DiscoveryProxy>(sysSettings.getDomain());
    std::shared_ptr<system::DiscoveryProxy> lcdProxy = lcdProxyBuilder->setDiscoveryQos(discoveryQos)->build();

    JOYNR_LOG_DEBUG(logger(), "performing pre-touch lookup");
    types::GlobalDiscoveryEntry oldEntry;
    gcdProxy->lookup(oldEntry, providerParticipantId, lookupGbids);

    // wait some time to perform further lookups after at least one touch call
    std::chrono::milliseconds waitTime = touchInterval + touchInterval / 2;
    std::this_thread::sleep_for(waitTime);

    JOYNR_LOG_DEBUG(logger(), "performing post-touch lookups");
    // check global cache in LCD
    types::DiscoveryQos internalDiscoveryQos;
    internalDiscoveryQos.setCacheMaxAge(std::numeric_limits<std::int64_t>::max());
    internalDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    internalDiscoveryQos.setDiscoveryTimeout(3000);
    types::DiscoveryEntryWithMetaInfo cachedEntry;
    lcdProxy->lookup(cachedEntry, providerParticipantId, internalDiscoveryQos, lookupGbids);
    JOYNR_LOG_DEBUG(logger(), "old cap last seen: {} , cached {}",
                            oldEntry.getLastSeenDateMs(),
                            cachedEntry.getLastSeenDateMs());
    JOYNR_LOG_DEBUG(logger(), "old cap expiry date: {} , cached {}",
                            oldEntry.getExpiryDateMs(),
                            cachedEntry.getExpiryDateMs());

    auto timeAfterSecondLookup = TimePoint::now().toMilliseconds();
    int64_t deltaMax = timeAfterSecondLookup - timeBeforeAdd;

    // real interval between 2 touch calls might be less than FRESHNESS_UPDATE_INTERVAL_MS because of messaging delays
    const int64_t minNewLastSeenDate = oldEntry.getLastSeenDateMs() + touchInterval.count() / 2;
    const int64_t minNewExpiryDate = oldEntry.getExpiryDateMs() + touchInterval.count() / 2;
    EXPECT_TRUE(cachedEntry.getLastSeenDateMs() > minNewLastSeenDate);
    EXPECT_TRUE(cachedEntry.getLastSeenDateMs() < (oldEntry.getLastSeenDateMs() + deltaMax))
            << "delta: " << (cachedEntry.getLastSeenDateMs() - oldEntry.getLastSeenDateMs()) << ", deltaMax: " << deltaMax;
    EXPECT_TRUE(cachedEntry.getExpiryDateMs() > minNewExpiryDate);
    EXPECT_TRUE(cachedEntry.getExpiryDateMs() < (oldEntry.getExpiryDateMs() + deltaMax))
            << "delta: " << (cachedEntry.getLastSeenDateMs() - oldEntry.getLastSeenDateMs()) << ", deltaMax: " << deltaMax;

    // check local store in LCD
    internalDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    types::DiscoveryEntryWithMetaInfo localEntry;
    lcdProxy->lookup(localEntry, providerParticipantId, internalDiscoveryQos, lookupGbids);
    JOYNR_LOG_DEBUG(logger(), "old cap last seen: {} , local {}",
                            oldEntry.getLastSeenDateMs(),
                            localEntry.getLastSeenDateMs());
    JOYNR_LOG_DEBUG(logger(), "old cap expiry date: {} , local {}",
                            oldEntry.getExpiryDateMs(),
                            localEntry.getExpiryDateMs());

    timeAfterSecondLookup = TimePoint::now().toMilliseconds();
    deltaMax = timeAfterSecondLookup - timeBeforeAdd;

    EXPECT_TRUE(localEntry.getLastSeenDateMs() > minNewLastSeenDate);
    EXPECT_TRUE(localEntry.getLastSeenDateMs() < (oldEntry.getLastSeenDateMs() + deltaMax))
            << "delta: " << (cachedEntry.getLastSeenDateMs() - oldEntry.getLastSeenDateMs()) << ", deltaMax: " << deltaMax;
    EXPECT_TRUE(localEntry.getExpiryDateMs() > minNewExpiryDate);
    EXPECT_TRUE(localEntry.getExpiryDateMs() < (oldEntry.getExpiryDateMs() + deltaMax))
            << "delta: " << (cachedEntry.getLastSeenDateMs() - oldEntry.getLastSeenDateMs()) << ", deltaMax: " << deltaMax;

    // check entry in GCD
    types::GlobalDiscoveryEntry globalEntry;
    gcdProxy->lookup(globalEntry, providerParticipantId, lookupGbids);

    JOYNR_LOG_DEBUG(logger(), "old cap last seen: {} , new {}",
                            oldEntry.getLastSeenDateMs(),
                            globalEntry.getLastSeenDateMs());
    JOYNR_LOG_DEBUG(logger(), "old cap expiry date: {} , new {}",
                            oldEntry.getExpiryDateMs(),
                            globalEntry.getExpiryDateMs());

    timeAfterSecondLookup = TimePoint::now().toMilliseconds();
    deltaMax = timeAfterSecondLookup - timeBeforeAdd;

    EXPECT_TRUE(globalEntry.getLastSeenDateMs() > minNewLastSeenDate);
    EXPECT_TRUE(globalEntry.getLastSeenDateMs() < (oldEntry.getLastSeenDateMs() + deltaMax))
            << "delta: " << (cachedEntry.getLastSeenDateMs() - oldEntry.getLastSeenDateMs()) << ", deltaMax: " << deltaMax;
    EXPECT_TRUE(globalEntry.getExpiryDateMs() > minNewExpiryDate);
    EXPECT_TRUE(globalEntry.getExpiryDateMs() < (oldEntry.getExpiryDateMs() + deltaMax))
            << "delta: " << (cachedEntry.getLastSeenDateMs() - oldEntry.getLastSeenDateMs()) << ", deltaMax: " << deltaMax;

    // Unregister providers to not clutter the JDS
    runtime->unregisterProvider(providerParticipantId);
}

using namespace std::string_literals;

INSTANTIATE_TEST_SUITE_P(Mqtt,
                        GlobalCapabilitiesDirectoryIntegrationTest,
                        testing::Values("test-resources/MqttSystemIntegrationTest1.settings"s));

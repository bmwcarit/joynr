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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
#include "libjoynrclustercontroller/messaging/MessagingPropertiesPersistence.h"

#include "tests/JoynrTest.h"
#include "tests/utils/PtrUtils.h"

#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"
#include "tests/mock/MockTestProvider.h"

using namespace ::testing;
using namespace joynr;

static const std::string messagingPropertiesPersistenceFileName(
        "GlobalCapabilitiesDirectoryClientTest-joynr.settings");
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
        messagingSettings.setMessagingPropertiesPersistenceFilename(
                messagingPropertiesPersistenceFileName);
        MessagingPropertiesPersistence storage(
                messagingSettings.getMessagingPropertiesPersistenceFilename());
        Settings libjoynrSettings{libJoynrSettingsFilename};
        Settings::merge(libjoynrSettings, *settings, false);

        runtime = std::make_shared<JoynrClusterControllerRuntime>(std::move(settings), failOnFatalRuntimeError);
        runtime->init();
    }

    void SetUp() override
    {
        runtime->start();
    }

    ~GlobalCapabilitiesDirectoryIntegrationTest() override
    {
        runtime->shutdown();
        test::util::resetAndWaitUntilDestroyed(runtime);

        // Delete persisted files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(GlobalCapabilitiesDirectoryIntegrationTest);
};

TEST_P(GlobalCapabilitiesDirectoryIntegrationTest, registerAndRetrieveCapability)
{
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
            std::make_unique<GlobalCapabilitiesDirectoryClient>(clusterControllerSettings));
    globalCapabilitiesDirectoryClient->setProxy(cabilitiesProxy, messagingQos);

    std::string capDomain("testDomain");
    std::string capInterface("testInterface");
    types::ProviderQos capProviderQos;
    std::string capParticipantId("testParticipantId");
    std::string capPublicKeyId("publicKeyId");
    joynr::types::Version providerVersion(47, 11);
    std::int64_t capLastSeenMs = 0;
    std::int64_t capExpiryDateMs = 1000;
    std::string capSerializedChannelAddress("testChannelId");
    types::GlobalDiscoveryEntry globalDiscoveryEntry(providerVersion,
                                                     capDomain,
                                                     capInterface,
                                                     capParticipantId,
                                                     capProviderQos,
                                                     capLastSeenMs,
                                                     capExpiryDateMs,
                                                     capPublicKeyId,
                                                     capSerializedChannelAddress);

    JOYNR_LOG_DEBUG(logger(), "Registering capabilities");
    globalCapabilitiesDirectoryClient->add(
                globalDiscoveryEntry,
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
TEST_P(GlobalCapabilitiesDirectoryIntegrationTest, testRemoveStale)
{
    // Setup
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
            std::make_unique<Settings>(GetParam()), failOnFatalRuntimeError);
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
            std::make_unique<Settings>(GetParam()), failOnFatalRuntimeError);
    testRuntimeSecond->init();
    testRuntimeSecond->start();
    // wait some time to make sure that removeStale has been published and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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
 * Test touching only the providers actually registered in the clustercontroller
 */
TEST_P(GlobalCapabilitiesDirectoryIntegrationTest, testTouchOnlyUpdatesExistingParticipantIds)
{
    JOYNR_LOG_DEBUG(logger(), "testTouchOnlyUpdatesExistingParticipantIds started");
    // Setup
    const std::string domain = "cppTestTouchDomain";
    auto mockProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    auto millisSinceEpoch = TimePoint::now().toMilliseconds();
    providerQos.setPriority(millisSinceEpoch);
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    joynr::DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(3000);

    const std::string providerParticipantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockProvider,
                providerQos,
                true,
                true);

    std::shared_ptr<ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
            capabilitiesProxyBuilder =
                    runtime->createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                            messagingSettings.getDiscoveryDirectoriesDomain());
    const MessagingQos messagingQos;
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> cabilitiesProxy(
            capabilitiesProxyBuilder->setMessagingQos(messagingQos)
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    auto testProxyBuilder = runtime->createProxyBuilder<system::DiscoveryProxy>(sysSettings.getDomain());
    std::shared_ptr<system::DiscoveryProxy> lcdProxy = testProxyBuilder->setDiscoveryQos(discoveryQos)->build();

    const std::vector<std::string> lookupGbids = { messagingSettings.getGbid() };

    JOYNR_LOG_DEBUG(logger(), "performing pre-touch lookup");
    types::GlobalDiscoveryEntry oldEntry;
    cabilitiesProxy->lookup(oldEntry, providerParticipantId, lookupGbids);

    const std::chrono::milliseconds touchInterval = clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs();
    std::chrono::milliseconds waitTime = touchInterval + touchInterval / 2;
    std::this_thread::sleep_for(waitTime);

    JOYNR_LOG_DEBUG(logger(), "performing post-touch lookup");
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
    EXPECT_TRUE(cachedEntry.getLastSeenDateMs() > oldEntry.getLastSeenDateMs());
    EXPECT_TRUE(cachedEntry.getLastSeenDateMs() < ( oldEntry.getLastSeenDateMs() + 2 * touchInterval.count()));
    EXPECT_TRUE(cachedEntry.getExpiryDateMs() > oldEntry.getExpiryDateMs());
    EXPECT_TRUE(cachedEntry.getExpiryDateMs() < ( oldEntry.getExpiryDateMs() + 2 * touchInterval.count()));

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
    EXPECT_TRUE(localEntry.getLastSeenDateMs() > oldEntry.getLastSeenDateMs());
    EXPECT_TRUE(localEntry.getLastSeenDateMs() < ( oldEntry.getLastSeenDateMs() + 2 * touchInterval.count()));
    EXPECT_TRUE(localEntry.getExpiryDateMs() > oldEntry.getExpiryDateMs());
    EXPECT_TRUE(localEntry.getExpiryDateMs() < ( oldEntry.getExpiryDateMs() + 2 * touchInterval.count()));

    // check entry in GCD
    types::GlobalDiscoveryEntry result2;
    cabilitiesProxy->lookup(result2, providerParticipantId, lookupGbids);

    JOYNR_LOG_DEBUG(logger(), "old cap last seen: {} , new {}",
                            oldEntry.getLastSeenDateMs(),
                            result2.getLastSeenDateMs());
    JOYNR_LOG_DEBUG(logger(), "old cap expiry date: {} , new {}",
                            oldEntry.getExpiryDateMs(),
                            result2.getExpiryDateMs());
    EXPECT_TRUE(result2.getLastSeenDateMs() > oldEntry.getLastSeenDateMs());
    EXPECT_TRUE(result2.getLastSeenDateMs() < ( oldEntry.getLastSeenDateMs() + 2 * touchInterval.count()));
    EXPECT_TRUE(result2.getExpiryDateMs() > oldEntry.getExpiryDateMs());
    EXPECT_TRUE(result2.getExpiryDateMs() < ( oldEntry.getExpiryDateMs() + 2 * touchInterval.count()));

    // Unregister providers to not clutter the JDS
    runtime->unregisterProvider(providerParticipantId);
}

using namespace std::string_literals;

INSTANTIATE_TEST_CASE_P(DISABLED_Http,
                        GlobalCapabilitiesDirectoryIntegrationTest,
                        testing::Values("test-resources/HttpSystemIntegrationTest1.settings"s));

INSTANTIATE_TEST_CASE_P(Mqtt,
                        GlobalCapabilitiesDirectoryIntegrationTest,
                        testing::Values("test-resources/MqttSystemIntegrationTest1.settings"s));

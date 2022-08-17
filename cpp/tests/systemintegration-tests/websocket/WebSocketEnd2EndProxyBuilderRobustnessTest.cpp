/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/BrokerUrl.h"
#include "joynr/CcMessageRouter.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Future.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/vehicle/GpsProxy.h"
#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockGpsProvider.h"
#include "tests/utils/PtrUtils.h"
#include "tests/utils/TestLibJoynrWebSocketRuntime.h"

using namespace ::testing;

using namespace joynr;

namespace joynr
{

class WebSocketEnd2EndProxyBuilderRobustnessTest : public Test
{
public:
    WebSocketEnd2EndProxyBuilderRobustnessTest()
            : domain("cppWebSocketEnd2EndProxyBuilderRobustnessTest" + util::createUuid()),
              discoveryTimeoutMs(5000),
              retryIntervalMs(500),
              consumerRuntime(),
              providerRuntime(),
              ccRuntime(),
              discoveryQos()
    {
        auto settings = std::make_unique<Settings>();
        consumerRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(std::move(settings));

        settings = std::make_unique<Settings>();
        providerRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(std::move(settings));

        settings = std::make_unique<Settings>();
        MessagingSettings ccSettings(*settings);
        // use wrong broker-url to prevent global communication
        BrokerUrl brokerUrl("mqtt://localhost:12347");
        ccSettings.setBrokerUrl(brokerUrl);
        ccRuntime = std::make_shared<JoynrClusterControllerRuntime>(
                std::move(settings), failOnFatalRuntimeError);

        ccRuntime->init();
        ccRuntime->start();

        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
        discoveryQos.setCacheMaxAgeMs(discoveryTimeoutMs);
    }

    // Sets up the test fixture.
    void SetUp() override
    {
        ASSERT_TRUE(consumerRuntime->connect(std::chrono::milliseconds(10000)));
        ASSERT_TRUE(providerRuntime->connect(std::chrono::milliseconds(10000)));
    }

    void checkResolveNextHop(const std::string& participantId, bool expectProviderResolved)
    {
        Semaphore successCallbackCalled;
        ccRuntime->_ccMessageRouter->resolveNextHop(
            participantId,
            [&successCallbackCalled, expectProviderResolved](const bool& resolved) {
                if (resolved == expectProviderResolved) {
                    successCallbackCalled.notify();
                } else {
                    FAIL() << "resolve delivered unexpected result";
                    successCallbackCalled.notify();
                }
            },
            [&successCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                FAIL() << "resolveNextHop did not succeed.";
                successCallbackCalled.notify();
            });
        EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(3000)));
    }

    ~WebSocketEnd2EndProxyBuilderRobustnessTest() override
    {
        // shutdown consumer and provider runtime first to allow
        // graceful termination of connections
        consumerRuntime->shutdown();
        test::util::resetAndWaitUntilDestroyed(consumerRuntime);

        providerRuntime->shutdown();
        test::util::resetAndWaitUntilDestroyed(providerRuntime);

        ccRuntime->shutdown();
        test::util::resetAndWaitUntilDestroyed(ccRuntime);

        // Delete persisted files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
        std::this_thread::sleep_for(std::chrono::milliseconds(550));
    }

protected:
    std::string domain;
    const std::int64_t discoveryTimeoutMs;
    const std::int64_t retryIntervalMs;
    std::shared_ptr<TestLibJoynrWebSocketRuntime> consumerRuntime;
    std::shared_ptr<TestLibJoynrWebSocketRuntime> providerRuntime;
    std::shared_ptr<JoynrClusterControllerRuntime> ccRuntime;
    joynr::DiscoveryQos discoveryQos;

    void buildMultipleProxiesUsingSameProxyBuilder();
    void attemptBuildAsyncMultipleProxiesUsingSameProxyBuilder();
    void buildProxiesAndVerifyShutdownRuntimeCleansUpCcRuntime();
    void attemptBuildAsyncMultipleProxiesUsingSameProxyBuilderAndShutdownRuntime();
    void buildProxyBeforeProviderRegistration(const bool expectSuccess);

private:
    DISALLOW_COPY_AND_ASSIGN(WebSocketEnd2EndProxyBuilderRobustnessTest);
};

} // namespace joynr

void WebSocketEnd2EndProxyBuilderRobustnessTest::buildProxyBeforeProviderRegistration(
        const bool expectSuccess)
{
    auto semaphore = std::make_shared<Semaphore>(0);
    // prepare provider
    auto mockProvider = std::make_shared<MockGpsProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);

    // build proxy
    std::shared_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
            consumerRuntime->createProxyBuilder<vehicle::GpsProxy>(domain);

    auto onSuccess = [semaphore, expectSuccess](std::shared_ptr<vehicle::GpsProxy> gpsProxy) {
        if (!expectSuccess) {
            ADD_FAILURE() << "proxy building succeeded unexpectedly";
            semaphore->notify();
            return;
        }
        // call proxy method
        auto calculateOnSuccess = [semaphore](int value) {
            const int expectedValue = 42; // as defined in MockGpsProvider
            EXPECT_EQ(expectedValue, value);
            semaphore->notify();
        };
        gpsProxy->calculateAvailableSatellitesAsync(calculateOnSuccess);
    };

    auto onError = [semaphore, expectSuccess](const exceptions::DiscoveryException& exception) {
        if (expectSuccess) {
            ADD_FAILURE() << "proxy building failed unexpectedly, exception: "
                          << exception.getMessage();
        }
        semaphore->notify();
    };

    std::uint64_t qosRoundTripTTL = 10000;
    gpsProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
            ->setDiscoveryQos(discoveryQos)
            ->buildAsync(onSuccess, onError);

    // wait some time so that the lookup request is likely to be processed at the cluster controller
    // and make sure that no retry attempt has been started
    std::this_thread::sleep_for(std::chrono::milliseconds(retryIntervalMs / 2));

    // register provider
    std::string participantId = providerRuntime->registerProvider<vehicle::GpsProvider>(
            domain, mockProvider, providerQos);

    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(
            static_cast<std::int64_t>(qosRoundTripTTL) + discoveryTimeoutMs)));

    // unregister provider
    providerRuntime->unregisterProvider(participantId);
}

void WebSocketEnd2EndProxyBuilderRobustnessTest::buildMultipleProxiesUsingSameProxyBuilder()
{
    // prepare provider
    auto mockProvider = std::make_shared<MockGpsProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);

    // register provider
    std::string participantId = providerRuntime->registerProvider<vehicle::GpsProvider>(
            domain, mockProvider, providerQos);

    // create ProxyBuilder
    std::shared_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
            consumerRuntime->createProxyBuilder<vehicle::GpsProxy>(domain);

    // no proxy build done yet, number of arbitrators should be 0
    std::unique_lock<std::mutex> lock(gpsProxyBuilder->_arbitratorsMutex);
    EXPECT_EQ(0, gpsProxyBuilder->_arbitrators.size());
    lock.unlock();

    // synchronously build proxy 10 times
    for (int i = 0; i < 10; i++) {
        gpsProxyBuilder->setDiscoveryQos(discoveryQos)
            ->build();
    }

    // number of stored and finished arbitrators should be 1 now,
    // since each proxy build should have reclaimed the previously used
    // arbitrator which was already finished since the build was
    // synchronously done.
    lock.lock();
    EXPECT_EQ(1, gpsProxyBuilder->_arbitrators.size());
    lock.unlock();
    std::unique_lock<std::mutex> lock2(gpsProxyBuilder->_finishedArbitratorIdsMutex);
    EXPECT_EQ(1, gpsProxyBuilder->_finishedArbitratorIds.size());
    lock2.unlock();

    // unregister provider
    providerRuntime->unregisterProvider(participantId);
}

void WebSocketEnd2EndProxyBuilderRobustnessTest::attemptBuildAsyncMultipleProxiesUsingSameProxyBuilder()
{
    auto semaphore = std::make_shared<Semaphore>(0);

    // create ProxyBuilder; any build is supposed to fail because of the chosen domain
    std::string notExistingDomain("notExistingDomain");
    std::shared_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
            consumerRuntime->createProxyBuilder<vehicle::GpsProxy>(notExistingDomain);

    // no proxy build done yet, number of arbitrators should be 0
    std::unique_lock<std::mutex> lock(gpsProxyBuilder->_arbitratorsMutex);
    EXPECT_EQ(0, gpsProxyBuilder->_arbitrators.size());
    lock.unlock();

    // attempt to build 10 proxies asynchronously
    // the builds are intentionally expected to fail
    // make sure the calls do not finish immediately by using retryIntervalMs
    // this will cause a sleep within the arbitrator threads
    discoveryQos.setCacheMaxAgeMs(0);
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs - 1000);

    const int numberOfProxyBuilds = 10;
    for (int i = 0; i < numberOfProxyBuilds; i++) {
        auto onSuccess = [semaphore](std::shared_ptr<vehicle::GpsProxy>) {
            ADD_FAILURE() << "proxy building succeeded unexpectedly";
            semaphore->notify();
            return;
        };

        auto onError = [semaphore](const exceptions::DiscoveryException&) {
            semaphore->notify();
        };

        gpsProxyBuilder->setDiscoveryQos(discoveryQos)
                ->buildAsync(onSuccess, onError);
    }

    // the building attempts above should time out after discoveryTimeoutMs
    // with failure; that should have been sufficient time so that all 10 builds
    // have been started without finding already finished Arbitrators at
    // their invocation time.
    for (int i = 0; i < numberOfProxyBuilds; i++) {
        EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(discoveryTimeoutMs + 1000)));
    }

    // at this time all callbacks have been invoked and the arbitrators
    // should add themselves to the _finishedArbitratorIds dequeue;
    // allow some time for this to happen.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Now we should have numberOfProxyBuilds stored arbitrators, and all
    // of them should be available for reclaiming
    lock.lock();
    EXPECT_EQ(numberOfProxyBuilds, gpsProxyBuilder->_arbitrators.size());
    lock.unlock();
    std::unique_lock<std::mutex> lock2(gpsProxyBuilder->_finishedArbitratorIdsMutex);
    EXPECT_EQ(numberOfProxyBuilds, gpsProxyBuilder->_finishedArbitratorIds.size());
    lock2.unlock();

    // starting another proxy build attempt with no retry
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs + 1000);

    auto onSuccess = [semaphore](std::shared_ptr<vehicle::GpsProxy>) {
        ADD_FAILURE() << "proxy building succeeded unexpectedly";
        semaphore->notify();
        return;
    };

    auto onError = [semaphore](const exceptions::DiscoveryException&) {
        semaphore->notify();
    };

    gpsProxyBuilder->setDiscoveryQos(discoveryQos)
        ->buildAsync(onSuccess, onError);

    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(discoveryTimeoutMs + 1000)));

    // this should have reduced the number of arbitrators to 1
    lock.lock();
    EXPECT_EQ(1, gpsProxyBuilder->_arbitrators.size());
    lock.unlock();
    lock2.lock();
    EXPECT_EQ(1, gpsProxyBuilder->_finishedArbitratorIds.size());
    lock2.unlock();
}

void WebSocketEnd2EndProxyBuilderRobustnessTest::attemptBuildAsyncMultipleProxiesUsingSameProxyBuilderAndShutdownRuntime()
{
    auto semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<TestLibJoynrWebSocketRuntime> ownConsumerRuntime;

    auto settings = std::make_unique<Settings>();
    ownConsumerRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(std::move(settings));

    ASSERT_TRUE(ownConsumerRuntime->connect(std::chrono::milliseconds(10000)));

    // create ProxyBuilder; any build is supposed to fail because of the chosen domain
    std::string notExistingDomain("notExistingDomain");
    std::shared_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
            ownConsumerRuntime->createProxyBuilder<vehicle::GpsProxy>(notExistingDomain);

    // attempt to build 10 proxies asynchronously
    // the builds are intentionally expected to fail
    // make sure the calls do not finish immediately by using retryIntervalMs
    // this will cause a sleep within the arbitrator threads
    discoveryQos.setDiscoveryTimeoutMs(600000);
    discoveryQos.setCacheMaxAgeMs(0);
    discoveryQos.setRetryIntervalMs(590000);

    const int numberOfProxyBuilds = 10;
    for (int i = 0; i < numberOfProxyBuilds; i++) {
        auto onSuccess = [semaphore](std::shared_ptr<vehicle::GpsProxy>) {
            ADD_FAILURE() << "proxy building succeeded unexpectedly";
            semaphore->notify();
            return;
        };

        auto onError = [semaphore](const exceptions::DiscoveryException&) {
            semaphore->notify();
        };

        gpsProxyBuilder->setDiscoveryQos(discoveryQos)
                ->buildAsync(onSuccess, onError);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // allow the runtime to destruct proxyBuilder on shutdown by releasing own shared_ptr
    gpsProxyBuilder.reset();

    ownConsumerRuntime->shutdown();
    test::util::resetAndWaitUntilDestroyed(ownConsumerRuntime);

    for (int i = 0; i < numberOfProxyBuilds; i++) {
        EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(discoveryTimeoutMs + 1000)));
    }
}

void WebSocketEnd2EndProxyBuilderRobustnessTest::buildProxiesAndVerifyShutdownRuntimeCleansUpCcRuntime()
{
    // prepare provider
    auto mockProvider = std::make_shared<MockGpsProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);

    // register provider
    std::string participantId = providerRuntime->registerProvider<vehicle::GpsProvider>(
            domain, mockProvider, providerQos);

    auto semaphore = std::make_shared<Semaphore>(0);
    std::shared_ptr<TestLibJoynrWebSocketRuntime> ownConsumerRuntime;

    auto settings = std::make_unique<Settings>();
    ownConsumerRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(std::move(settings));

    ASSERT_TRUE(ownConsumerRuntime->connect(std::chrono::milliseconds(10000)));

    // create ProxyBuilder
    std::shared_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
            ownConsumerRuntime->createProxyBuilder<vehicle::GpsProxy>(domain);

    discoveryQos.setDiscoveryTimeoutMs(600000);
    discoveryQos.setCacheMaxAgeMs(0);
    discoveryQos.setRetryIntervalMs(590000);

    const int numberOfProxyBuilds = 10;
    std::vector<std::string> proxyParticipantIds;
    std::vector<std::shared_ptr<vehicle::GpsProxy>> gpsProxies;
    std::mutex proxyLock;

    // build some proxies asynchronously and keep their references
    // (to protect against destruction) and store their proxy participantIds
    // for later checking
    for (int i = 0; i < numberOfProxyBuilds; i++) {
        auto onSuccess = [semaphore, &proxyParticipantIds, &gpsProxies, &proxyLock](std::shared_ptr<vehicle::GpsProxy> gpsProxy) {
            std::lock_guard<std::mutex> lock(proxyLock);
            proxyParticipantIds.push_back(gpsProxy->getProxyParticipantId());
            gpsProxies.push_back(gpsProxy);
            semaphore->notify();
            return;
        };

        auto onError = [semaphore](const exceptions::DiscoveryException&) {
            ADD_FAILURE() << "proxy building failed unexpectedly";
            semaphore->notify();
        };

        gpsProxyBuilder->setDiscoveryQos(discoveryQos)
                ->buildAsync(onSuccess, onError);
    }
    for (int i = 0; i < numberOfProxyBuilds; i++) {
        EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(discoveryTimeoutMs + 1000)));
    }

    // allow the runtime to destruct proxyBuilder on shutdown by releasing own shared_ptr
    gpsProxyBuilder.reset();

    std::lock_guard<std::mutex> lock(proxyLock);
    // check that the proxy participantIds exist in RoutingTable of ccMessageRouter at this time
    for (auto participantId : proxyParticipantIds) {
        checkResolveNextHop(participantId, true);
    }

    // release the references to the proxies
    // this might cause there associated RoutingEntry records to be removed already from
    // RoutingTable here and in CC at a later time, when this got implemented
    gpsProxies.clear();

    ownConsumerRuntime->shutdown();
    test::util::resetAndWaitUntilDestroyed(ownConsumerRuntime);

    // wait a while
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Now that the consumer runtime has been destructed and the connction to
    // CC should have been closed. The CC should have detected this and invoked
    // cleanup for all RoutingEntry records associated with the connection
    // still existing at this time.
    for (auto participantId : proxyParticipantIds) {
        checkResolveNextHop(participantId, false);
    }

    // unregister provider
    providerRuntime->unregisterProvider(participantId);
}

// as soon as the provider gets registered, the lookup returns successful
TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_LocalThenGlobal_succeedsWithoutRetry)
{
    const bool expectedSuccess = true;
    // disable retries for provider lookup
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs * 2);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_LocalThenGlobal_succeedsWithRetry)
{
    const bool expectedSuccess = true;
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_LocalAndGlobal_failsWithoutRetry)
{
    const bool expectedSuccess = false;
    // disable retries for provider lookup
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs * 2);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

// no retry until global lookup succeeds or times out
TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_LocalAndGlobal_failsWithRetry)
{
    const bool expectedSuccess = false;
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_LocalOnly_failsWithoutRetry)
{
    const bool expectedSuccess = false;
    // disable retries for provider lookup
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs * 2);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_LocalOnly_succeedsWithRetry)
{
    const bool expectedSuccess = true;
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_GlobalOnly_failsWithoutRetry)
{
    const bool expectedSuccess = false;
    // disable retries for provider lookup
    discoveryQos.setRetryIntervalMs(discoveryTimeoutMs * 2);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

// no retry until global lookup succeeds or times out
TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
       buildProxyBeforeProviderRegistration_GlobalOnly_failsWithRetry)
{
    const bool expectedSuccess = false;
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    buildProxyBeforeProviderRegistration(expectedSuccess);
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
        ProxyBuilderReclaimsArbitrators)
{
    buildMultipleProxiesUsingSameProxyBuilder();
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
        ProxyBuilderAsyncReclaimsArbitrators)
{
    attemptBuildAsyncMultipleProxiesUsingSameProxyBuilder();
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest,
        ProxyBuilderAsyncDoesNotCrashWhenRuntimeIsShutdown)
{
    attemptBuildAsyncMultipleProxiesUsingSameProxyBuilderAndShutdownRuntime();
}

TEST_F(WebSocketEnd2EndProxyBuilderRobustnessTest, ConsumerRuntimeShutdownCleansRoutingTableOfCC)
{
    buildProxiesAndVerifyShutdownRuntimeCleansUpCcRuntime();
}

class WebSocketEnd2EndProxyBuild : public WebSocketEnd2EndProxyBuilderRobustnessTest
{
protected:
    void SetUp() override
    {
        WebSocketEnd2EndProxyBuilderRobustnessTest::SetUp();

        auto mockProvider = std::make_shared<MockGpsProvider>();
        types::ProviderQos providerQos;
        std::chrono::milliseconds millisSinceEpoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
        providerQos.setPriority(millisSinceEpoch.count());
        providerQos.setScope(joynr::types::ProviderScope::GLOBAL);

        providerParticipantId = providerRuntime->registerProvider<vehicle::GpsProvider>(
                domain, mockProvider, providerQos);
        gpsProxyBuilder = consumerRuntime->createProxyBuilder<vehicle::GpsProxy>(domain);
    }

    void TearDown() override
    {
        providerRuntime->unregisterProvider(providerParticipantId);
    }

    std::string providerParticipantId;
    std::shared_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder;
};

TEST_F(WebSocketEnd2EndProxyBuild, buildProxyWithoutSetMessagingQos)
{
    std::shared_ptr<vehicle::GpsProxy> gpsProxy;
    JOYNR_EXPECT_NO_THROW(gpsProxy = gpsProxyBuilder->setDiscoveryQos(discoveryQos)->build());
    ASSERT_TRUE(gpsProxy);
}

TEST_F(WebSocketEnd2EndProxyBuild, buildProxyWithoutSetDiscoveryQos)
{
    const std::int64_t qosRoundTripTTL = 10000;
    std::shared_ptr<vehicle::GpsProxy> gpsProxy;
    JOYNR_EXPECT_NO_THROW(
            gpsProxy = gpsProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))->build());
    ASSERT_TRUE(gpsProxy);
}

TEST_F(WebSocketEnd2EndProxyBuild, buildProxyWithoutSetMessagingQosAndWithoutSetDiscoveryQos)
{
    std::shared_ptr<vehicle::GpsProxy> gpsProxy;
    JOYNR_EXPECT_NO_THROW(gpsProxy = gpsProxyBuilder->build());
    ASSERT_TRUE(gpsProxy);
}

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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/tests/testProxy.h"
#include "joynr/Settings.h"

#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockTestProvider.h"

#include "tests/JoynrTest.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

class AsyncProxyBuilderTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(100);
        auto integrationSettings =
                std::make_unique<Settings>("test-resources/libjoynrSystemIntegration1.settings");
        Settings settings("test-resources/MqttSystemIntegrationTest1.settings");
        Settings::merge(settings, *integrationSettings, false);
        runtime = std::make_shared<JoynrClusterControllerRuntime>(
                std::move(integrationSettings), failOnFatalRuntimeError);
        runtime->init();
        runtime->start();
    }

    void TearDown() override
    {
        runtime->shutdown();
        test::util::resetAndWaitUntilDestroyed(runtime);

        // Delete persisted files
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

protected:
    std::shared_ptr<JoynrClusterControllerRuntime> runtime;
    DiscoveryQos discoveryQos;
};

TEST_F(AsyncProxyBuilderTest, createProxyAsync_succeeds)
{
    const std::string domain = "testDomain";
    auto testProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setScope(types::ProviderScope::LOCAL);

    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    Semaphore onSuccessCalledSemaphore;

    auto onSuccess = [&onSuccessCalledSemaphore](std::shared_ptr<tests::testProxy> proxy) {
        onSuccessCalledSemaphore.notify();
        EXPECT_NE(nullptr, proxy);
    };

    auto onFailure = [](const joynr::exceptions::DiscoveryException&) { FAIL(); };

    testProxyBuilder->setMessagingQos(MessagingQos(50000))
            ->setDiscoveryQos(discoveryQos)
            ->buildAsync(onSuccess, onFailure);

    EXPECT_TRUE(onSuccessCalledSemaphore.waitFor(std::chrono::seconds(10)));
    runtime->unregisterProvider(participantId);
}

TEST_F(AsyncProxyBuilderTest, createProxyAsync_exceptionThrown)
{
    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>("unknownDomain");

    Semaphore onErrorCalledSemaphore;

    auto onSuccess = [](std::shared_ptr<tests::testProxy>) { FAIL(); };

    auto onFailure = [&onErrorCalledSemaphore](const joynr::exceptions::DiscoveryException&) {
        onErrorCalledSemaphore.notify();
    };

    testProxyBuilder->setMessagingQos(MessagingQos(50000))
            ->setDiscoveryQos(discoveryQos)
            ->buildAsync(onSuccess, onFailure);

    EXPECT_TRUE(onErrorCalledSemaphore.waitFor(std::chrono::seconds(10)));
}

TEST_F(AsyncProxyBuilderTest, buildProxyWithArbitrationResult_succeeds)
{
    const std::string domain = "testArbitrationResultSuccessDomain";
    auto testProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setScope(types::ProviderScope::LOCAL);

    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    // Fake ArbitrationResult to build proxy
    joynr::types::Version interfaceVersion(tests::testProvider::MAJOR_VERSION, tests::testProvider::MINOR_VERSION);
    types::DiscoveryEntryWithMetaInfo discoveryEntry = types::DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                         domain,
                                                                                         tests::testProvider::INTERFACE_NAME(),
                                                                                         "participantId1",
                                                                                         providerQos,
                                                                                         9900,
                                                                                         15000,
                                                                                         "_publicKeyId1",
                                                                                         true);

    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries {discoveryEntry};
    ArbitrationResult arbitrationResult = ArbitrationResult(discoveryEntries);

    std::shared_ptr<tests::testProxy> proxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(50000))
            ->setDiscoveryQos(discoveryQos)
            ->build(arbitrationResult);
    EXPECT_NE(nullptr, proxy);

    runtime->unregisterProvider(participantId);
}

TEST_F(AsyncProxyBuilderTest, buildProxyWithArbitrationResult_throwsException_whenArbitrationResultEmpty)
{
    const std::string domain = "testArbitrationResultSuccessDomain";
    auto testProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setScope(types::ProviderScope::LOCAL);

    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    ArbitrationResult arbitrationResult = ArbitrationResult();

    bool exceptionThrown = false;
    bool messageFound = false;
    try {
        std::shared_ptr<tests::testProxy> proxy = testProxyBuilder
                ->setMessagingQos(MessagingQos(50000))
                ->setDiscoveryQos(discoveryQos)
                ->build(arbitrationResult);
    } catch (const exceptions::DiscoveryException& e) {
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "ArbitrationResult is empty";
        messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
        exceptionThrown = true;
    }

    EXPECT_TRUE(exceptionThrown);
    EXPECT_TRUE(messageFound);

    runtime->unregisterProvider(participantId);
}

TEST_F(AsyncProxyBuilderTest, buildProxyWithArbitrationResult_throwsException_whenParticipantIdEmpty)
{
    const std::string domain = "testArbitrationResultSuccessDomain";
    auto testProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setScope(types::ProviderScope::LOCAL);

    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    // Fake ArbitrationResult to build proxy
    joynr::types::Version interfaceVersion(tests::testProvider::MAJOR_VERSION, tests::testProvider::MINOR_VERSION);
    types::DiscoveryEntryWithMetaInfo discoveryEntry = types::DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                         domain,
                                                                                         tests::testProvider::INTERFACE_NAME(),
                                                                                         "",
                                                                                         providerQos,
                                                                                         9900,
                                                                                         15000,
                                                                                         "_publicKeyId1",
                                                                                         true);

    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries {discoveryEntry};
    ArbitrationResult arbitrationResult = ArbitrationResult(discoveryEntries);

    bool exceptionThrown = false;
    bool messageFound = false;
    try {
        std::shared_ptr<tests::testProxy> proxy = testProxyBuilder
                ->setMessagingQos(MessagingQos(50000))
                ->setDiscoveryQos(discoveryQos)
                ->build(arbitrationResult);
    } catch (const exceptions::DiscoveryException& e) {
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "ParticipantId is empty";
        messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
        exceptionThrown = true;
    }

    EXPECT_TRUE(exceptionThrown);
    EXPECT_TRUE(messageFound);

    runtime->unregisterProvider(participantId);
}

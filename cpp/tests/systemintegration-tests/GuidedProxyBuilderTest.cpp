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
#include <memory>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/exceptions/JoynrException.h"
#include "joynr/tests/testProxy.h"
#include "joynr/Settings.h"

#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockTestProvider.h"

#include "tests/JoynrTest.h"
#include "tests/mock/TestJoynrClusterControllerRuntime.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

class MockDifferentVersionProvider : public joynr::AbstractJoynrProvider {
public:
    static const std::int32_t MAJOR_VERSION;
    static const std::int32_t MINOR_VERSION;
    ~MockDifferentVersionProvider() override = default;
    const std::string& getInterfaceName() const override;
    static const std::string& INTERFACE_NAME();
};

const std::int32_t MockDifferentVersionProvider::MAJOR_VERSION = 58;
const std::int32_t MockDifferentVersionProvider::MINOR_VERSION = 42;

const std::string& MockDifferentVersionProvider::INTERFACE_NAME()
{
    static const std::string INTERFACE_NAME(tests::testProxy::INTERFACE_NAME());
    return INTERFACE_NAME;
}

const std::string& MockDifferentVersionProvider::getInterfaceName() const
{
    return INTERFACE_NAME();
}

class MockSameMajorAndHigherMinorVersionProvider : public joynr::AbstractJoynrProvider {
public:
    static const std::int32_t MAJOR_VERSION;
    static const std::int32_t MINOR_VERSION;
    ~MockSameMajorAndHigherMinorVersionProvider() override = default;
    const std::string& getInterfaceName() const override;
    static const std::string& INTERFACE_NAME();
};

const std::int32_t MockSameMajorAndHigherMinorVersionProvider::MAJOR_VERSION = 47;
const std::int32_t MockSameMajorAndHigherMinorVersionProvider::MINOR_VERSION = 42;

const std::string& MockSameMajorAndHigherMinorVersionProvider::INTERFACE_NAME()
{
    static const std::string INTERFACE_NAME(tests::testProxy::INTERFACE_NAME());
    return INTERFACE_NAME;
}

const std::string& MockSameMajorAndHigherMinorVersionProvider::getInterfaceName() const
{
    return INTERFACE_NAME();
}

class MockSameMajorAndLowerMinorVersionProvider : public joynr::AbstractJoynrProvider {
public:
    static const std::int32_t MAJOR_VERSION;
    static const std::int32_t MINOR_VERSION;
    ~MockSameMajorAndLowerMinorVersionProvider() override = default;
    const std::string& getInterfaceName() const override;
    static const std::string& INTERFACE_NAME();
};

const std::int32_t MockSameMajorAndLowerMinorVersionProvider::MAJOR_VERSION = 47;
const std::int32_t MockSameMajorAndLowerMinorVersionProvider::MINOR_VERSION = 10;

const std::string& MockSameMajorAndLowerMinorVersionProvider::INTERFACE_NAME()
{
    static const std::string INTERFACE_NAME(tests::testProxy::INTERFACE_NAME());
    return INTERFACE_NAME;
}

const std::string& MockSameMajorAndLowerMinorVersionProvider::getInterfaceName() const
{
    return INTERFACE_NAME();
}

namespace joynr
{
template <>
inline std::shared_ptr<RequestCaller> RequestCallerFactory::create<MockDifferentVersionProvider>(std::shared_ptr<MockDifferentVersionProvider> provider)
{
    std::ignore = provider;
    return std::shared_ptr<RequestCaller>(nullptr);
}

template <>
inline std::shared_ptr<RequestCaller> RequestCallerFactory::create<MockSameMajorAndHigherMinorVersionProvider>(std::shared_ptr<MockSameMajorAndHigherMinorVersionProvider> provider)
{
    std::ignore = provider;
    return std::shared_ptr<RequestCaller>(nullptr);
}

template <>
inline std::shared_ptr<RequestCaller> RequestCallerFactory::create<MockSameMajorAndLowerMinorVersionProvider>(std::shared_ptr<MockSameMajorAndLowerMinorVersionProvider> provider)
{
    std::ignore = provider;
    return std::shared_ptr<RequestCaller>(nullptr);
}
} // namespace joynr;

class GuidedProxyBuilderTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(100);
        messagingQos = MessagingQos(50000);
        auto integrationSettings =
                std::make_unique<Settings>("test-resources/libjoynrSystemIntegration1.settings");
        Settings settings("test-resources/MqttSystemIntegrationTest1.settings");
        Settings::merge(settings, *integrationSettings, false);
        runtime = std::make_shared<TestJoynrClusterControllerRuntime>(
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
    const std::string TEST_DOMAIN = "testDomain_GuidedProxyBuilder";
    std::shared_ptr<TestJoynrClusterControllerRuntime> runtime;
    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;

protected:
    std::string createAndRegisterProvider(const std::string& domain)
    {
        auto mockProvider = std::make_shared<MockTestProvider>();
        types::ProviderQos providerQos;
        providerQos.setScope(joynr::types::ProviderScope::LOCAL);
        std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain, mockProvider, providerQos);

        return participantId;
    }
};

TEST_F(GuidedProxyBuilderTest, discover_succeeds)
{
    const std::string domain = TEST_DOMAIN;

    // register first provider
    std::string participantId1 = createAndRegisterProvider(domain);

    // register secondProvider
    auto secondProvider = std::make_shared<MockDifferentVersionProvider>();
    types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    std::string participantId2 = runtime->registerProvider<MockDifferentVersionProvider>(
            domain, secondProvider, providerQos);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    DiscoveryResult actualDiscoveryResult = testGuidedProxyBuilder->discover();
    EXPECT_TRUE(actualDiscoveryResult.getAllDiscoveryEntries().size() == 2);

    bool foundParticipantId1 = false;
    bool foundParticipantId2 = false;
    std::vector<joynr::types::DiscoveryEntry> actualDiscoveryEntries = actualDiscoveryResult.getAllDiscoveryEntries();
    for (const auto& entry : actualDiscoveryEntries) {
        if (entry.getParticipantId() == participantId1) {
            foundParticipantId1 = true;
        }
        if (entry.getParticipantId() == participantId2) {
            foundParticipantId2 = true;
        }
    }

    EXPECT_TRUE(foundParticipantId1 && foundParticipantId2);

    runtime->unregisterProvider(participantId1);
    runtime->unregisterProvider(participantId2);
}

TEST_F(GuidedProxyBuilderTest, discoverAsync_succeeds)
{
    const std::string domain = TEST_DOMAIN;

    // register first provider
    std::string participantId1 = createAndRegisterProvider(domain);

    // register secondProvider
    auto secondProvider = std::make_shared<MockDifferentVersionProvider>();
    types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    std::string participantId2 = runtime->registerProvider<MockDifferentVersionProvider>(
            domain, secondProvider, providerQos);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    DiscoveryResult actualDiscoveryResult;

    try {
        auto discoveryFuture = testGuidedProxyBuilder->discoverAsync();
        discoveryFuture->get(actualDiscoveryResult);
    } catch (const exceptions::DiscoveryException&) {
        FAIL();
    }

    EXPECT_TRUE(actualDiscoveryResult.getAllDiscoveryEntries().size() == 2);

    bool foundParticipantId1 = false;
    bool foundParticipantId2 = false;
    std::vector<joynr::types::DiscoveryEntry> actualDiscoveryEntries = actualDiscoveryResult.getAllDiscoveryEntries();
    for (const auto& entry : actualDiscoveryEntries) {
        if (entry.getParticipantId() == participantId1) {
            foundParticipantId1 = true;
        }
        if (entry.getParticipantId() == participantId2) {
            foundParticipantId2 = true;
        }
    }

    EXPECT_TRUE(foundParticipantId1 && foundParticipantId2);

    runtime->unregisterProvider(participantId1);
    runtime->unregisterProvider(participantId2);
}

TEST_F(GuidedProxyBuilderTest, discover_throwsException)
{
    const std::string domain = TEST_DOMAIN;
    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    EXPECT_THROW(testGuidedProxyBuilder->discover(), exceptions::DiscoveryException);
}

TEST_F(GuidedProxyBuilderTest, buildProxy_succeeds_and_performs_proxyCall)
{
    const std::string domain = "testDomain_buildProxySuccess";
    std::string participantId = createAndRegisterProvider(domain);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    JOYNR_EXPECT_NO_THROW(testGuidedProxyBuilder->discover());
    std::shared_ptr<tests::testProxy> testProxy = testGuidedProxyBuilder->buildProxy<tests::testProxy>(participantId);

    EXPECT_NE(testProxy, nullptr);

    std::int32_t expectedResult = 42;
    std::int32_t actualResult;
    testProxy->addNumbers(actualResult, 0, 0, 0);

    EXPECT_EQ(actualResult, expectedResult);

    runtime->unregisterProvider(participantId);
}

TEST_F(GuidedProxyBuilderTest, buildProxy_throwsException_whenDiscoverHasNotBeenDone)
{
    const std::string domain = TEST_DOMAIN;
    std::string participantId = createAndRegisterProvider(domain);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    bool exceptionThrown = false;
    bool messageFound = false;
    try {
        std::shared_ptr<tests::testProxy> testProxy = testGuidedProxyBuilder->buildProxy<tests::testProxy>(participantId);
    } catch (const exceptions::DiscoveryException& e) {
        exceptionThrown = true;
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "Discovery has to be completed before building a proxy!";
        messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
    }

    EXPECT_TRUE(exceptionThrown);
    EXPECT_TRUE(messageFound);

    runtime->unregisterProvider(participantId);
}

TEST_F(GuidedProxyBuilderTest, buildProxy_throwsException_whenEntryWithParticipantIdNotFound)
{
    const std::string domain = TEST_DOMAIN;
    std::string participantId = createAndRegisterProvider(domain);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    JOYNR_EXPECT_NO_THROW(testGuidedProxyBuilder->discover());

    bool exceptionThrown = false;
    bool messageFound = false;
    std::string wrongParticipantId = "wrongParticipantId";
    try {
        std::shared_ptr<tests::testProxy> testProxy = testGuidedProxyBuilder->buildProxy<tests::testProxy>(wrongParticipantId);
    } catch (const exceptions::DiscoveryException& e) {
        exceptionThrown = true;
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "No provider with participant ID";
        messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
    }

    EXPECT_TRUE(exceptionThrown);
    EXPECT_TRUE(messageFound);

    runtime->unregisterProvider(participantId);
}

TEST_F(GuidedProxyBuilderTest, buildProxy_throwsException_whenVersionDoNotMatch)
{
    const std::string domain = TEST_DOMAIN;
    auto mockProvider = std::make_shared<MockDifferentVersionProvider>();
    types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    std::string participantId = runtime->registerProvider<MockDifferentVersionProvider>(
            domain, mockProvider, providerQos);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    JOYNR_EXPECT_NO_THROW(testGuidedProxyBuilder->discover());
    bool exceptionThrown = false;
    bool messageFound = false;
    try {
        std::shared_ptr<tests::testProxy> testProxy = testGuidedProxyBuilder->buildProxy<tests::testProxy>(participantId);
    } catch (const exceptions::DiscoveryException& e) {
        exceptionThrown = true;
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "have incompatible versions!";
        messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
    }

    EXPECT_TRUE(exceptionThrown);
    EXPECT_TRUE(messageFound);

    runtime->unregisterProvider(participantId);
}

TEST_F(GuidedProxyBuilderTest, buildProxy_succeeds_whenMinorProviderVersionHigher)
{
    const std::string domain = TEST_DOMAIN;
    auto mockProvider = std::make_shared<MockSameMajorAndHigherMinorVersionProvider>();
    types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    std::string participantId = runtime->registerProvider<MockSameMajorAndHigherMinorVersionProvider>(
            domain, mockProvider, providerQos);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    JOYNR_EXPECT_NO_THROW(testGuidedProxyBuilder->discover());
    std::shared_ptr<tests::testProxy> testProxy;
    JOYNR_EXPECT_NO_THROW(testProxy = testGuidedProxyBuilder->buildProxy<tests::testProxy>(participantId));
    EXPECT_NE(testProxy, nullptr);
    runtime->unregisterProvider(participantId);
}

TEST_F(GuidedProxyBuilderTest, buildProxy_throwsException_whenMinorProviderVersionLower)
{
    const std::string domain = TEST_DOMAIN;
    auto mockProvider = std::make_shared<MockSameMajorAndLowerMinorVersionProvider>();
    types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    std::string participantId = runtime->registerProvider<MockSameMajorAndLowerMinorVersionProvider>(
            domain, mockProvider, providerQos);

    std::shared_ptr<GuidedProxyBuilder> testGuidedProxyBuilder =
            runtime->createGuidedProxyBuilder<tests::testProxy>(domain)
            ->setMessagingQos(messagingQos)
            ->setDiscoveryQos(discoveryQos);

    JOYNR_EXPECT_NO_THROW(testGuidedProxyBuilder->discover());
    bool exceptionThrown = false;
    bool messageFound = false;
    try {
        std::shared_ptr<tests::testProxy> testProxy = testGuidedProxyBuilder->buildProxy<tests::testProxy>(participantId);
    } catch (const exceptions::DiscoveryException& e) {
        exceptionThrown = true;
        std::string exceptionMessage = e.getMessage();
        std::string expectedSubstring = "have incompatible versions!";
        messageFound = exceptionMessage.find(expectedSubstring) != std::string::npos ? true : false;
    }

    EXPECT_TRUE(exceptionThrown);
    EXPECT_TRUE(messageFound);

    runtime->unregisterProvider(participantId);
}

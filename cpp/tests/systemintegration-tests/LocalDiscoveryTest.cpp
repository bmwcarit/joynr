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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/MessagingSettings.h"
#include "joynr/Settings.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"

#include "joynr/JoynrClusterControllerRuntime.h"
#include "tests/utils/PtrUtils.h"
#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;

namespace joynr
{

class LocalDiscoveryTest : public ::testing::Test
{
public:
    LocalDiscoveryTest() : runtime1(), runtime2(), testDomain("testDomain")
    {
        auto settings1 =
                std::make_unique<Settings>("test-resources/MqttSystemIntegrationTest1.settings");
        auto settings2 =
                std::make_unique<Settings>("test-resources/MqttSystemIntegrationTest2.settings");

        MessagingSettings messagingSettings1(*settings1);
        MessagingSettings messagingSettings2(*settings2);

        messagingSettings1.setMessagingPropertiesPersistenceFilename(
                "End2EndBroadcastTest-runtime1-joynr.settings");
        messagingSettings2.setMessagingPropertiesPersistenceFilename(
                "End2EndBroadcastTest-runtime2-joynr.settings");

        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings1, false);

        runtime1 = std::make_shared<JoynrClusterControllerRuntime>(
                std::move(settings1), failOnFatalRuntimeError);
        runtime1->init();

        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings2, false);

        runtime2 = std::make_shared<JoynrClusterControllerRuntime>(
                std::move(settings2), failOnFatalRuntimeError);
        runtime2->init();

        runtime1->start();
        runtime2->start();

        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
        discoveryQos.setDiscoveryTimeoutMs(3000);
        discoveryQos.setRetryIntervalMs(250);

        messagingQos.setTtl(500);
    }

    ~LocalDiscoveryTest() override
    {

        runtime1->shutdown();
        test::util::resetAndWaitUntilDestroyed(runtime1);

        runtime2->shutdown();
        test::util::resetAndWaitUntilDestroyed(runtime2);

        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

protected:
    void registerProvider(std::shared_ptr<JoynrClusterControllerRuntime> runtime)
    {
        auto testProvider = std::make_shared<tests::DefaulttestProvider>();
        joynr::types::ProviderQos providerQos;
        runtime->registerProvider<tests::testProvider>(testDomain, testProvider, providerQos);
    }

    std::shared_ptr<JoynrClusterControllerRuntime> runtime1;
    std::shared_ptr<JoynrClusterControllerRuntime> runtime2;
    const std::string testDomain;
    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryTest);
};

} // namespace joynr

class LocalDiscoveryTestTestProxy : public tests::testProxy
{
public:
    LocalDiscoveryTestTestProxy(
            std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
            std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
            const std::string& domain,
            const joynr::MessagingQos& qosSettings)
            : joynr::ProxyBase(runtime, connectorFactory, domain, qosSettings),
              testProxyBase(runtime, connectorFactory, domain, qosSettings),
              testFireAndForgetProxy(runtime, connectorFactory, domain, qosSettings),
              testSyncProxy(runtime, connectorFactory, domain, qosSettings),
              testAsyncProxy(runtime, connectorFactory, domain, qosSettings),
              testProxy(runtime, connectorFactory, domain, qosSettings)
    {
    }

    using tests::testProxy::_providerDiscoveryEntry;
};

TEST_F(LocalDiscoveryTest, testLocalLookup)
{
    registerProvider(runtime1);

    std::shared_ptr<ProxyBuilder<LocalDiscoveryTestTestProxy>> testProxyBuilder(
            runtime1->createProxyBuilder<LocalDiscoveryTestTestProxy>(testDomain));

    std::shared_ptr<LocalDiscoveryTestTestProxy> testProxy(
            testProxyBuilder->setMessagingQos(messagingQos)
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    EXPECT_TRUE(testProxy->_providerDiscoveryEntry.getIsLocal());
}

TEST_F(LocalDiscoveryTest, testGloballLookup)
{
    registerProvider(runtime1);

    std::shared_ptr<ProxyBuilder<LocalDiscoveryTestTestProxy>> testProxyBuilder(
            runtime2->createProxyBuilder<LocalDiscoveryTestTestProxy>(testDomain));

    std::shared_ptr<LocalDiscoveryTestTestProxy> testProxy(
            testProxyBuilder->setMessagingQos(messagingQos)
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    EXPECT_FALSE(testProxy->_providerDiscoveryEntry.getIsLocal());
}

TEST_F(LocalDiscoveryTest, testAsyncRegistration)
{
    auto testProvider = std::make_shared<tests::DefaulttestProvider>();
    joynr::types::ProviderQos providerQos;
    auto onSuccess = []() {};
    auto onError = [](const exceptions::JoynrRuntimeException&) { FAIL(); };

    runtime1->registerProviderAsync<tests::testProvider>(
            testDomain, testProvider, providerQos, onSuccess, onError);

    std::shared_ptr<ProxyBuilder<LocalDiscoveryTestTestProxy>> testProxyBuilder(
            runtime2->createProxyBuilder<LocalDiscoveryTestTestProxy>(testDomain));

    std::shared_ptr<LocalDiscoveryTestTestProxy> testProxy(
            testProxyBuilder->setMessagingQos(messagingQos)
                    ->setDiscoveryQos(discoveryQos)
                    ->build());
}

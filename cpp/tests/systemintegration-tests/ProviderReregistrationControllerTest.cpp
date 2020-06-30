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
#include <chrono>

#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/system/ProviderReregistrationControllerProxy.h"
#include "joynr/DiscoveryQos.h"
#include "tests/JoynrTest.h"
#include "tests/utils/TestLibJoynrWebSocketRuntime.h"

using namespace ::testing;
using namespace joynr;

TEST(ProviderReregistrationControllerTest, queryProviderReregistrationControllerSucceedsOnCCRuntime)
{
    auto integrationSettings =
            std::make_unique<Settings>("test-resources/MqttSystemIntegrationTest1.settings");
    joynr::SystemServicesSettings systemServiceSettings(*integrationSettings);
    const std::string domain(systemServiceSettings.getDomain());

    auto runtime = std::make_shared<JoynrClusterControllerRuntime>(
            std::move(integrationSettings), failOnFatalRuntimeError);
    runtime->init();
    runtime->start();

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(3000);

    auto providerReregistrationControllerProxyBuilder =
            runtime->createProxyBuilder<joynr::system::ProviderReregistrationControllerProxy>(
                    domain);
    auto providerReregistrationControllerProxy =
            providerReregistrationControllerProxyBuilder->setDiscoveryQos(discoveryQos)->build();

    Semaphore finishedSemaphore;
    providerReregistrationControllerProxy->triggerGlobalProviderReregistrationAsync(
            [&finishedSemaphore]() { finishedSemaphore.notify(); }, nullptr);
    EXPECT_TRUE(finishedSemaphore.waitFor(std::chrono::seconds(10)));

    runtime->stop();
    runtime->shutdown();
}

TEST(ProviderReregistrationControllerTest, queryProviderReregistrationControllerSucceedsOnWsRuntime)
{
    auto integrationSettings =
            std::make_unique<Settings>("test-resources/MqttSystemIntegrationTest1.settings");
    joynr::SystemServicesSettings systemServiceSettings(*integrationSettings);
    const std::string domain(systemServiceSettings.getDomain());

    auto ccRuntime = std::make_shared<JoynrClusterControllerRuntime>(
            std::move(integrationSettings), failOnFatalRuntimeError);
    ccRuntime->init();
    ccRuntime->start();

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(3000);

    auto wsRuntimeSettings =
            std::make_unique<Settings>("test-resources/libjoynrSystemIntegration1.settings");
    auto wsRuntime = std::make_shared<TestLibJoynrWebSocketRuntime>(std::move(wsRuntimeSettings));
    ASSERT_TRUE(wsRuntime->connect(std::chrono::seconds(2)));

    auto providerReregistrationControllerProxyBuilder =
            wsRuntime->createProxyBuilder<joynr::system::ProviderReregistrationControllerProxy>(
                    domain);
    auto providerReregistrationControllerProxy =
            providerReregistrationControllerProxyBuilder->setDiscoveryQos(discoveryQos)->build();

    Semaphore finishedSemaphore;
    providerReregistrationControllerProxy->triggerGlobalProviderReregistrationAsync(
            [&finishedSemaphore]() { finishedSemaphore.notify(); },
            [](const joynr::exceptions::JoynrRuntimeException&) { FAIL(); });
    EXPECT_TRUE(finishedSemaphore.waitFor(std::chrono::seconds(10)));

    wsRuntime->shutdown();
    wsRuntime.reset();
    ccRuntime->stop();
    ccRuntime->shutdown();
}

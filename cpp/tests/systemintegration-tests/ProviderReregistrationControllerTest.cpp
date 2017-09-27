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

#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/system/ProviderReregistrationControllerProxy.h"

using namespace ::testing;
using namespace joynr;

TEST(ProviderReregistrationControllerTest, queryProviderReregistrationControllerSucceedsOnCCRuntime)
{
    auto integrationSettings = std::make_unique<Settings>("test-resources/MqttSystemIntegrationTest1.settings");
    joynr::SystemServicesSettings systemServiceSettings(*integrationSettings);
    const std::string domain(systemServiceSettings.getDomain());

    auto runtime = std::make_shared<JoynrClusterControllerRuntime>(std::move(integrationSettings));
    runtime->init();
    runtime->start();

    auto providerReregistrationControllerProxyBuilder = runtime->createProxyBuilder<joynr::system::ProviderReregistrationControllerProxy>(domain);
    auto providerReregistrationControllerProxy = providerReregistrationControllerProxyBuilder->build();

    Semaphore finishedSemaphore;
    providerReregistrationControllerProxy->triggerGlobalProviderReregistrationAsync([&finishedSemaphore]() { finishedSemaphore.notify(); }, nullptr);
    finishedSemaphore.waitFor(std::chrono::seconds(2));
}

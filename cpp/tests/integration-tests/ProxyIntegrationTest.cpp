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

#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/Settings.h"
#include "joynr/vehicle/GpsProxy.h"

#include "tests/mock/MockJoynrRuntime.h"

using ::testing::Return;

using namespace ::testing;
using namespace joynr;

TEST(ProxyIntegrationTest, proxyInitialisation)
{
    const std::string domain = "cppProxyIntegrationTestDomain";
    MessagingQos messagingQos;

    auto settings = std::make_unique<Settings>();
    auto runtime = std::make_shared<MockJoynrRuntime>(std::move(settings));
    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(nullptr, nullptr);
    auto proxy = std::make_unique<vehicle::GpsProxy>(
            runtime, std::move(joynrMessagingConnectorFactory), domain, messagingQos);

    ASSERT_TRUE(proxy != nullptr);
}

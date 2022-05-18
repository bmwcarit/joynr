/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
#include "joynr/Settings.h"

#include "tests/mock/MockTestProvider.h"

#include "tests/JoynrTest.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

class AwaitGlobalRegistrationTest : public ::testing::Test
{
public:
    void SetUp() override
    {
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
};

TEST_F(AwaitGlobalRegistrationTest, globalRegistrationFails)
{
    // disallow communication with global capabilities directory
    runtime->stopExternalCommunication();

    const std::string domain = "testDomain";
    auto testProvider = std::make_shared<MockTestProvider>();

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    const bool awaitGlobalRegistration = true;
    const bool persist = false;
    EXPECT_THROW(runtime->registerProvider<tests::testProvider>(
                         domain, testProvider, providerQos, persist, awaitGlobalRegistration),
                 exceptions::JoynrRuntimeException);
}

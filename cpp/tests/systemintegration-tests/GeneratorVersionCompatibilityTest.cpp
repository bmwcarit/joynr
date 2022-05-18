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
#include "tests/utils/Gtest.h"

#include "joynr/Settings.h"
#include "joynr/tests/v2/DefaultMultipleVersionsInterfaceProvider.h"
#include "joynr/tests/MultipleVersionsInterfaceProxy.h"

#include "joynr/JoynrRuntime.h"
#include "joynr/exceptions/JoynrException.h"
#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;

class GeneratorVersionCompatibilityTest : public Test
{
public:
    GeneratorVersionCompatibilityTest() : runtime(), testDomain("testDomain")
    {
        runtime =
                JoynrRuntime::createRuntime(std::make_unique<Settings>(), failOnFatalRuntimeError);
        discoveryQos.setDiscoveryTimeoutMs(100);
        discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);
    }

    ~GeneratorVersionCompatibilityTest() override
    {
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

protected:
    std::shared_ptr<JoynrRuntime> runtime;
    const std::string testDomain;
    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;

private:
    DISALLOW_COPY_AND_ASSIGN(GeneratorVersionCompatibilityTest);
};

TEST_F(GeneratorVersionCompatibilityTest, proxyCreationAgainstPackagedProviderSucceeds)
{
    auto testProvider = std::make_shared<tests::v2::DefaultMultipleVersionsInterfaceProvider>();
    joynr::types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    runtime->registerProvider<tests::v2::MultipleVersionsInterfaceProvider>(
            testDomain, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::MultipleVersionsInterfaceProxy>> testProxyBuilder(
            runtime->createProxyBuilder<tests::MultipleVersionsInterfaceProxy>(testDomain));

    EXPECT_NO_THROW(testProxyBuilder->setMessagingQos(messagingQos)
                            ->setDiscoveryQos(discoveryQos)
                            ->build());
}

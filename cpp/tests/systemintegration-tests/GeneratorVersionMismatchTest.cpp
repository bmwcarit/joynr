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
#include "joynr/tests/v1/DefaultMultipleVersionsInterfaceProvider.h"
#include "joynr/tests/v2/MultipleVersionsInterfaceProxy.h"

#include "joynr/JoynrRuntime.h"
#include "joynr/exceptions/JoynrException.h"
#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;

class GeneratorVersionMismatchTest : public Test
{
public:
    GeneratorVersionMismatchTest() : runtime(), testDomain("testDomain")
    {
        runtime =
                JoynrRuntime::createRuntime(std::make_unique<Settings>(), failOnFatalRuntimeError);
        discoveryQos.setDiscoveryTimeoutMs(100);
        discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);
    }

    ~GeneratorVersionMismatchTest() override
    {
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

protected:
    std::shared_ptr<JoynrRuntime> runtime;
    const std::string testDomain;
    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;

private:
    DISALLOW_COPY_AND_ASSIGN(GeneratorVersionMismatchTest);
};

TEST_F(GeneratorVersionMismatchTest, proxyCreationFails)
{
    auto testProvider = std::make_shared<tests::v1::DefaultMultipleVersionsInterfaceProvider>();
    joynr::types::ProviderQos providerQos;
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    runtime->registerProvider<tests::v1::MultipleVersionsInterfaceProvider>(
            testDomain, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::v2::MultipleVersionsInterfaceProxy>> testProxyBuilder(
            runtime->createProxyBuilder<tests::v2::MultipleVersionsInterfaceProxy>(testDomain));

    EXPECT_THROW(
            testProxyBuilder->setMessagingQos(messagingQos)->setDiscoveryQos(discoveryQos)->build(),
            joynr::exceptions::DiscoveryException);
}

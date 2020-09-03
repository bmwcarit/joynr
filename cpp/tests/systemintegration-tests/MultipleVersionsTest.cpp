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
#include <gtest/gtest.h>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Settings.h"
#include "joynr/tests/DefaultMultipleVersionsInterfaceProvider.h"
#include "joynr/tests/v1/DefaultMultipleVersionsInterfaceProvider.h"
#include "joynr/tests/v2/DefaultMultipleVersionsInterfaceProvider.h"
#include "joynr/tests/MultipleVersionsInterfaceProxy.h"
#include "joynr/tests/v1/MultipleVersionsInterfaceProxy.h"
#include "joynr/tests/v2/MultipleVersionsInterfaceProxy.h"

#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;

class MultipleVersionsTest : public Test
{
protected:
    MultipleVersionsTest()
    {
    }

    void SetUp() override
    {
        const Settings libjoynrSettings1("test-resources/libjoynrSystemIntegration1.settings");
        const Settings libjoynrSettings2("test-resources/libjoynrSystemIntegration2.settings");
        auto settings1 = std::make_unique<Settings>("test-resources/MqttSystemIntegrationTest1.settings");
        auto settings2 = std::make_unique<Settings>("test-resources/MqttSystemIntegrationTest2.settings");
        Settings::merge(libjoynrSettings1, *settings1, false);
        Settings::merge(libjoynrSettings2, *settings2, false);

        runtime1 = JoynrRuntime::createRuntime(std::move(settings1), failOnFatalRuntimeError);
        runtime2 = JoynrRuntime::createRuntime(std::move(settings2), failOnFatalRuntimeError);
        discoveryQos.setDiscoveryTimeoutMs(100);
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
        providerQos.setScope(types::ProviderScope::LOCAL);
    }

    void TearDown() override
    {
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

    template <typename T>
    std::shared_ptr<T> buildProxy(std::shared_ptr<JoynrRuntime> runtime)
    {
        std::shared_ptr<ProxyBuilder<T>> testProxyBuilder(
                runtime->createProxyBuilder<T>(testDomain));
        return testProxyBuilder->setMessagingQos(messagingQos)
                ->setDiscoveryQos(discoveryQos)
                ->build();
    }

    /**
     * Builds a proxy of type T in the specified runtime. Then sets UInt8Attribute1
     * to the specified value and checks if the set value can be retrieved correctly.
     *
     * @param runtime Builds the proxy in runtime.
     * @param value Sets the attribute to value.
     */
    template <typename T>
    void setAndCheckAttribute(std::shared_ptr<JoynrRuntime> runtime, const uint8_t value)
    {
        std::shared_ptr<T> testProxy = buildProxy<T>(runtime);

        uint8_t uInt8Result = 0;

        testProxy->setUInt8Attribute1(value);
        testProxy->getUInt8Attribute1(uInt8Result);

        ASSERT_EQ(value, uInt8Result);
    }

    /**
     * Registers 2 providers and a fitting proxy for each. Then checks if methods of the providers
     *can be called correctly.
     *
     * @param secondRuntime If this is set to true, the second provider and its proxy are
     *registered/build in a second runtime.
     */
    void buildTwoProvidersAndPerformChecks(bool secondRuntime)
    {
        std::shared_ptr<JoynrRuntime> selectedRuntime;
        if (secondRuntime) {
            selectedRuntime = runtime2;
        } else {
            selectedRuntime = runtime1;
        }

        auto testProvider1 = std::make_shared<tests::v1::DefaultMultipleVersionsInterfaceProvider>();
        runtime1->registerProvider<tests::v1::MultipleVersionsInterfaceProvider>(
                testDomain, testProvider1, providerQos);
        auto testProvider2 = std::make_shared<tests::v2::DefaultMultipleVersionsInterfaceProvider>();
        selectedRuntime->registerProvider<tests::v2::MultipleVersionsInterfaceProvider>(
                testDomain, testProvider2, providerQos);

        setAndCheckAttribute<tests::v1::MultipleVersionsInterfaceProxy>(
                runtime1, expectedUInt8Result1);
        setAndCheckAttribute<tests::v2::MultipleVersionsInterfaceProxy>(
                selectedRuntime, expectedUInt8Result2);

        runtime1->unregisterProvider<tests::v1::MultipleVersionsInterfaceProvider>(
                testDomain, testProvider1);
        selectedRuntime->unregisterProvider<tests::v2::MultipleVersionsInterfaceProvider>(
                testDomain, testProvider2);
    }

    const std::string testDomain = "multipleVersionsTestDomain";
    const uint8_t expectedUInt8Result1 = 50;
    const uint8_t expectedUInt8Result2 = 100;

    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;
    types::ProviderQos providerQos;
    std::shared_ptr<JoynrRuntime> runtime1;
    std::shared_ptr<JoynrRuntime> runtime2;

private:
    DISALLOW_COPY_AND_ASSIGN(MultipleVersionsTest);
};

TEST_F(MultipleVersionsTest, twoProxiesOfDifferentVersioningTypesVsOneProvider)
{
    auto testProvider = std::make_shared<tests::DefaultMultipleVersionsInterfaceProvider>();
    runtime1->registerProvider<tests::MultipleVersionsInterfaceProvider>(
            testDomain, testProvider, providerQos);

    //tests::MultipleVersionsInterfaceProxy has interface version 2 with no version generation
    setAndCheckAttribute<tests::MultipleVersionsInterfaceProxy>(runtime1, expectedUInt8Result1);
    setAndCheckAttribute<tests::v2::MultipleVersionsInterfaceProxy>(runtime1, expectedUInt8Result2);

    runtime1->unregisterProvider<tests::MultipleVersionsInterfaceProvider>(
            testDomain, testProvider);
}

TEST_F(MultipleVersionsTest, twoProvidersOfDifferentVersionsAndTwoFittingProxiesInSingleRuntime)
{
    buildTwoProvidersAndPerformChecks(false);
}

TEST_F(MultipleVersionsTest, twoProvidersWithFittingProxiesInDifferentRuntimes)
{
    buildTwoProvidersAndPerformChecks(true);
}

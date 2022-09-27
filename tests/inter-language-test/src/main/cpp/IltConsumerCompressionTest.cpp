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
#include "utils/Gmock.h"
#include "utils/Gtest.h"

#include "JoynrTest.h"
#include "joynr/interlanguagetest/TestInterfaceProxy.h"

#include "IltHelper.h"
#include "IltUtil.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQosEffort.h"

using namespace ::testing;

extern std::string globalIltProgramName;
extern std::string providerDomain;

/** Class to test the compression of joynr messages.
 */
class IltConsumerCompressionTest : public ::testing::Test
{

protected:
    void SetUp()
    {
        // Get the provider domain
        JOYNR_LOG_INFO(logger(), "Creating proxy for provider on domain {}", providerDomain);

        // Get the current program directory
        std::string dir(IltHelper::getAbsolutePathToExecutable(globalIltProgramName));

        // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
        // implementation.
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
                [&](const joynr::exceptions::JoynrRuntimeException& exception) {
                    JOYNR_LOG_ERROR(
                            logger(),
                            "Unexpected joynr runtime error occured: " + exception.getMessage());
                };

        // Initialize the joynr runtime
        std::string pathToMessagingSettings(dir + "/resources/ilt-consumer.settings");

        runtime = joynr::JoynrRuntime::createRuntime(pathToMessagingSettings, onFatalRuntimeError);

        // Create proxy builder
        proxyBuilder = runtime->createProxyBuilder<joynr::interlanguagetest::TestInterfaceProxy>(
                providerDomain);

        // Messaging Quality of service
        std::int64_t qosMsgTtl = 5000;
        joynr::MessagingQosEffort::Enum qosMsgEffort = joynr::MessagingQosEffort::Enum::NORMAL;
        bool qosMsgEncrypt = false;
        bool qosMsgCompress = true;

        // Find the provider with the highest priority set in ProviderQos
        joynr::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryTimeoutMs(60000);
        discoveryQos.setRetryIntervalMs(5000);
        discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());
        discoveryQos.setArbitrationStrategy(
                joynr::DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

        // Build a proxy
        testInterfaceProxy =
                proxyBuilder
                        ->setMessagingQos(joynr::MessagingQos(
                                qosMsgTtl, qosMsgEffort, qosMsgEncrypt, qosMsgCompress))
                        ->setDiscoveryQos(discoveryQos)
                        ->build();

        JOYNR_LOG_INFO(logger(), "***********************");
        JOYNR_LOG_INFO(logger(), "Proxy built.");
        JOYNR_LOG_INFO(logger(), "***********************");
    }

    void TearDown()
    {
        testInterfaceProxy.reset();
        proxyBuilder.reset();
        runtime.reset();
    }

    std::shared_ptr<joynr::JoynrRuntime> runtime;
    std::shared_ptr<joynr::interlanguagetest::TestInterfaceProxy> testInterfaceProxy;
    std::shared_ptr<joynr::ProxyBuilder<joynr::interlanguagetest::TestInterfaceProxy>> proxyBuilder;

private:
    ADD_LOGGER(IltConsumerCompressionTest)
};

TEST_F(IltConsumerCompressionTest, callMethodWithoutParameters)
{
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithoutParameters());
}

TEST_F(IltConsumerCompressionTest, callMethodWithMultiplePrimitiveParameters)
{
    int32_t arg1 = 2147483647;
    float arg2 = 47.11;
    bool arg3 = false;
    double doubleOut;
    std::string stringOut;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithMultiplePrimitiveParameters(
            doubleOut, stringOut, arg1, arg2, arg3));
    ASSERT_TRUE(IltUtil::cmpDouble(doubleOut, arg2));
    ASSERT_EQ(stringOut, std::to_string(arg1));
}

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
#include <cstdlib>
#include <memory>

#include <gtest/gtest.h>

#include "joynr/tests/robustness/TestInterfaceProxy.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/exceptions/JoynrException.h"

/* This is just broken...
#include "../cluster-controller-runtime/signal-handler/PosixSignalHandler.h"
*/
#include "../../../../../cpp/runtimes/cluster-controller-runtime/signal-handler/PosixSignalHandler.h"

using joynr::DiscoveryQos;
using joynr::MessagingQos;
using joynr::Semaphore;
using joynr::JoynrClusterControllerRuntime;
using joynr::tests::robustness::TestInterfaceProxy;
using joynr::exceptions::JoynrRuntimeException;
using joynr::ProxyBuilder;

class RobustnessTestMqttConnectionReset : public ::testing::Test
{
public:
    RobustnessTestMqttConnectionReset() = default;

protected:
    static void SetUpTestCase()
    {
        // Initialise the joynr runtime
        std::string pathToMessagingSettings("resources/robustness-tests-consumer.settings");

        runtime = joynr::JoynrClusterControllerRuntime::create(
                std::make_unique<joynr::Settings>(pathToMessagingSettings),
                [](const joynr::exceptions::JoynrRuntimeException& error) {
                    FAIL() << "Unexpected onFatalRuntimeError: " << error.what();
                });

        joynr::PosixSignalHandler::setHandleAndRegisterForSignals(runtime);

        // Create proxy builder
        proxyBuilder = runtime->createProxyBuilder<TestInterfaceProxy>(providerDomain);

        // Messaging Quality of service
        std::int64_t qosMsgTtl = 3000;

        // Find the provider with the highest priority set in ProviderQos
        DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryTimeoutMs(60000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

        // Build a proxy
        proxy = proxyBuilder->setMessagingQos(MessagingQos(qosMsgTtl))
                        ->setDiscoveryQos(discoveryQos)
                        ->build();
    }

    void pingProvider(const std::string& str)
    {
        proxy->ping();
        JOYNR_LOG_INFO(logger(), str);
    }

private:
    static std::shared_ptr<TestInterfaceProxy> proxy;
    static std::shared_ptr<ProxyBuilder<TestInterfaceProxy>> proxyBuilder;
    static std::shared_ptr<JoynrClusterControllerRuntime> runtime;
    static std::string providerDomain;

    ADD_LOGGER(RobustnessTestMqttConnectionReset)
};

TEST_F(RobustnessTestMqttConnectionReset, consumer_ping)
{
    // Call the ping for the first time while MQTT Broker is running
    EXPECT_NO_THROW(pingProvider("First Pong"));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // Call the ping for the second time while MQTT Broker is down
    EXPECT_THROW(pingProvider("Second Pong"), joynr::exceptions::JoynrTimeOutException);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // Call the ping for the third time while MQTT Broker is up again
    EXPECT_NO_THROW(pingProvider("Third Pong"));
}

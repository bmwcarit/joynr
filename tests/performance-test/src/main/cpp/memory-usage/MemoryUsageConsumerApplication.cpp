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
#include <chrono>
#include <csignal>
#include <cstdint>
#include <string>
#include <memory>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "joynr/DiscoveryQos.h"
#include "joynr/TimePoint.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"

#include "joynr/tests/performance/EchoProxy.h"

using namespace joynr;

bool isRunning = true;

void syncTest(std::shared_ptr<tests::performance::EchoProxy> proxy,
              const std::int64_t& periodMs,
              const std::int64_t& validityMs,
              const std::uint64_t& stringLength,
              joynr::Logger logger)
{
    std::string data = std::string(stringLength, '#');
    std::string responseData;

    const TimePoint expiryDate = TimePoint::fromRelativeMs(validityMs);
    while (isRunning && TimePoint::now() < expiryDate) {
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
            proxy->echoString(responseData, data);
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_ERROR(logger, "JoynrRuntimeException {}", e.getMessage());
        }
    }
}

void asyncTest(std::shared_ptr<tests::performance::EchoProxy> proxy,
               const std::int64_t& validityMs,
               const std::uint64_t& stringLength,
               joynr::Logger logger)
{
    std::string data = std::string(stringLength, '#');
    auto onSuccess = [](const std::string& responseData) { std::ignore = responseData; };
    auto onRuntimeError = [logger](const joynr::exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger, "JoynrRuntimeException {}", error.getMessage());
    };
    const TimePoint expiryDate = TimePoint::fromRelativeMs(validityMs);
    while (isRunning && TimePoint::now() < expiryDate) {
        proxy->echoStringAsync(data);
    }
}

void shutdown(int signum)
{
    isRunning = false;
}

int main(int argc, char* argv[])
{
    // handle signal SIGTERM
    signal(SIGTERM, shutdown);

    // Get a logger
    joynr::Logger logger("MemoryUsageConsumerApplication");

    // Check the usage
    std::string programName(argv[0]);
    if (argc != 5) {
        JOYNR_LOG_FATAL(logger,
                        "USAGE: {} <provider-domain> <CPP_MEMORY_SYNC|CPP_MEMORY_ASYNC> "
                        "<validityMs> <stringLengh>",
                        programName);
        return 1;
    }

    // Get the provider domain
    std::string providerDomain(argv[1]);
    // Get the test case
    std::string testCase(argv[2]);

    // Get the test case
    std::int64_t validityMs(std::stoll(argv[3]));

    // Get the string length
    std::uint64_t stringLength(std::stoull(argv[3]));

    JOYNR_LOG_INFO(logger,
                   "Creating proxy for provider on domain {}, test case {}, validity {}",
                   providerDomain,
                   testCase,
                   validityMs);

    // Get the current program directory
    boost::filesystem::path fullPath =
            boost::filesystem::system_complete(boost::filesystem::path(programName));
    std::string dir(fullPath.parent_path().string());

    // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
    // implementation.
    std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
            [&](const joynr::exceptions::JoynrRuntimeException& exception) {
        JOYNR_LOG_ERROR(
                logger, "Unexpected joynr runtime error occured: " + exception.getMessage());
    };

    // Initialise the joynr runtime
    std::string pathToMessagingSettings(dir + "/resources/memory-usage-consumer.settings");
    std::shared_ptr<JoynrRuntime> runtime =
            JoynrRuntime::createRuntime(pathToMessagingSettings, onFatalRuntimeError);

    // Create proxy builder
    std::shared_ptr<ProxyBuilder<tests::performance::EchoProxy>> proxyBuilder =
            runtime->createProxyBuilder<tests::performance::EchoProxy>(providerDomain);

    // Messaging Quality of service
    std::int64_t qosMsgTtl = 30000;                // Time to live is 30 secs in one direction
    std::int64_t qosCacheDataFreshnessMs = 400000; // Only consider data cached for < 400 secs

    // Find the provider with the highest priority set in ProviderQos
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(40000);
    discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

    // Build a proxy
    std::shared_ptr<tests::performance::EchoProxy> proxy;
    try {
        JOYNR_LOG_DEBUG(logger, "About to call proxyBuilder");
        proxy = proxyBuilder->setMessagingQos(MessagingQos(qosMsgTtl))
                        ->setDiscoveryQos(discoveryQos)
                        ->build();
        JOYNR_LOG_DEBUG(logger, "Call to Proxybuilder successfully completed");
        if (proxy == nullptr) {
            throw new joynr::exceptions::JoynrRuntimeException("received proxy == nullptr");
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_FATAL(logger, "Proxybuilder failed: {}", e.getMessage());
        exit(1);
    }

    // run the tests
    if (testCase == "CPP_MEMORY_SYNC") {
        syncTest(std::move(proxy), 100, validityMs, stringLength, logger);
    } else if (testCase == "CPP_MEMORY_ASYNC") {
        asyncTest(std::move(proxy), validityMs, stringLength, logger);
    } else {
        JOYNR_LOG_FATAL(logger, "Invalid choice for test case: {}", testCase);
        exit(1);
    }
}

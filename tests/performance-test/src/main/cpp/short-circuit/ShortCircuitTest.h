/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include <algorithm>
#include <memory>
#include <numeric>

#include "../common/PerformanceTest.h"
#include "../provider/PerformanceTestEchoProvider.h"
#include "joynr/Settings.h"
#include "joynr/tests/performance/EchoProxy.h"
#include "joynr/types/ProviderQos.h"

#include "ShortCircuitRuntime.h"

using namespace joynr;

struct ShortCircuitTest : public PerformanceTest {
    using ByteArray = std::vector<std::int8_t>;

    ShortCircuitTest(std::uint64_t runs)
            : runs(runs),
              runtime(std::make_shared<ShortCircuitRuntime>(std::make_unique<joynr::Settings>()))
    {
        echoProvider = std::make_shared<PerformanceTestEchoProvider>();
        // default uses a priority that is the current time,
        // causing arbitration to the last started instance if highest priority arbitrator is used
        std::chrono::milliseconds millisSinceEpoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
        types::ProviderQos echoProviderQos;
        echoProviderQos.setPriority(millisSinceEpoch.count());
        echoProviderQos.setScope(joynr::types::ProviderScope::GLOBAL);
        echoProviderQos.setSupportsOnChangeSubscriptions(true);
        runtime->registerProvider<tests::performance::EchoProvider>(
                domainName, echoProvider, echoProviderQos);
        std::shared_ptr<ProxyBuilder<tests::performance::EchoProxy>> proxyBuilder =
                runtime->createProxyBuilder<tests::performance::EchoProxy>(domainName);
        echoProxy = proxyBuilder->setDiscoveryQos(joynr::DiscoveryQos())->build();
    }

    ~ShortCircuitTest()
    {
        runtime->unregisterProvider<tests::performance::EchoProvider>(domainName, echoProvider);
    }

    void roundTripString(std::size_t length)
    {
        const std::string string(length, '#');
        auto fun = [&]() {
            std::string result;
            echoProxy->echoString(result, string);
            return result;
        };
        const std::string testName = "string length: " + std::to_string(length);
        runAndPrintAverage(runs, testName, fun);
    }

    void roundTripStruct(std::size_t length)
    {
        using joynr::tests::performance::Types::ComplexStruct;
        ComplexStruct complexStruct(32, 64, getFilledVector(length), std::string(length, '#'));

        auto fun = [&]() {
            ComplexStruct result;
            echoProxy->echoComplexStruct(result, complexStruct);
            return result;
        };

        const std::string testName = "byte[] size/string length: " + std::to_string(length);
        runAndPrintAverage(runs, testName, fun);
    }

    void roundTripByteArray(std::size_t length)
    {
        ByteArray data = getFilledVector(length);
        auto fun = [&]() {
            ByteArray result;
            echoProxy->echoByteArray(result, data);
            return result;
        };

        const std::string testName = "byte[] size: " + std::to_string(length);
        runAndPrintAverage(runs, testName, fun);
    }

private:
    ByteArray getFilledVector(std::size_t length)
    {
        ByteArray data(length);
        // fill data with sequentially increasing numbers
        std::iota(data.begin(), data.end(), 0);
        return data;
    }

    std::uint64_t runs;
    std::shared_ptr<ShortCircuitRuntime> runtime;
    std::shared_ptr<PerformanceTestEchoProvider> echoProvider;
    std::shared_ptr<tests::performance::EchoProxy> echoProxy;
    std::string domainName = "short-circuit";
};

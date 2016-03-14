/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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
#include <algorithm>
#include <numeric>

#include "joynr/tests/performance/EchoProxy.h"
#include "../provider/PerformanceTestEchoProvider.h"
#include "../common/PerformanceTest.h"

#include "ShortCircuitRuntime.h"

using namespace joynr;

struct ShortCircuitTest : public PerformanceTest<1000>
{
    using ByteArray = std::vector<std::int8_t>;

    ShortCircuitTest()
    {
        echoProvider = std::make_shared<PerformanceTestEchoProvider>();
        runtime.registerProvider<tests::performance::EchoProvider>(domainName, echoProvider);
        std::unique_ptr<ProxyBuilder<tests::performance::EchoProxy>> proxyBuilder(
                runtime.createProxyBuilder<tests::performance::EchoProxy>(domainName));
        echoProxy.reset(proxyBuilder->setDiscoveryQos(joynr::DiscoveryQos())->build());
    }

    ~ShortCircuitTest()
    {
        runtime.unregisterProvider<tests::performance::EchoProvider>(domainName, echoProvider);
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
        runAndPrintAverage(testName, fun);
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
        runAndPrintAverage(testName, fun);
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
        runAndPrintAverage(testName, fun);
    }

private:
    ByteArray getFilledVector(std::size_t length)
    {
        ByteArray data(length);
        // fill data with sequentially increasing numbers
        std::iota(data.begin(), data.end(), 0);
        return data;
    }

    ShortCircuitRuntime runtime;
    std::shared_ptr<PerformanceTestEchoProvider> echoProvider;
    std::unique_ptr<tests::performance::EchoProxy> echoProxy;
    std::string domainName = "short-circuit";
};

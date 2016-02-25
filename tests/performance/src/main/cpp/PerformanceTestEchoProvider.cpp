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

#include "PerformanceTestEchoProvider.h"

using namespace joynr;

PerformanceTestEchoProvider::PerformanceTestEchoProvider()
{
    // Set the provider's priority in such a way that a consumer uses the most recent provider.
    auto millisecondsSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());

    providerQos.setPriority(millisecondsSinceEpoch.count());
}

void PerformanceTestEchoProvider::echoString(
        const std::string& data,
        std::function<void(const std::string&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    onSuccess(data);
}

void PerformanceTestEchoProvider::echoByteArray(
        const std::vector<int8_t>& data,
        std::function<void(const std::vector<int8_t>&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    onSuccess(data);
}

void PerformanceTestEchoProvider::echoComplexStruct(
        const joynr::tests::performance::Types::ComplexStruct& data,
        std::function<void(const joynr::tests::performance::Types::ComplexStruct&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    onSuccess(data);
}

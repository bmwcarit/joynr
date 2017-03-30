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
#ifndef ROBUSTNESSTESTPROVIDER_H
#define ROBUSTNESSTESTPROVIDER_H
#include "joynr/tests/robustness/DefaultTestInterfaceProvider.h"

#include <cstdint>
#include <string>
#include <functional>

#include <memory>
#include "joynr/Logger.h"

namespace exceptions
{
class ProviderRuntimeException;
} // namespace exceptions

namespace joynr
{

class RobustnessTestProvider : public joynr::tests::robustness::DefaultTestInterfaceProvider
{
public:
    RobustnessTestProvider();
    ~RobustnessTestProvider() override = default;

    void methodWithStringParameters(
            const std::string& stringArg,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    void methodWithDelayedResponse(
            const std::int32_t& delayArg,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    void methodToFireBroadcastWithSingleStringParameter(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    void startFireBroadcastWithSingleStringParameter(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    void stopFireBroadcastWithSingleStringParameter(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    void fireBroadcastWithSingleStringParameterInternal(std::int64_t period_ms,
                                                        std::int64_t validity_ms,
                                                        std::shared_ptr<std::string> stringOut);

private:
    DISALLOW_COPY_AND_ASSIGN(RobustnessTestProvider);

    ADD_LOGGER(RobustnessTestProvider);
    std::atomic<bool> stopBroadcastWithSingleStringParameter;
    std::thread broadcastThread;
};

} // namespace joynr
#endif // ROBUSTNESSTESTPROVIDER_H

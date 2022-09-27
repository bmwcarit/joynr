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
#ifndef SYSTEM_INTEGRATION_TEST_PROVIDER_H
#define SYSTEM_INTEGRATION_TEST_PROVIDER_H

#include <cstdint>
#include <functional>

#include "joynr/test/DefaultSystemIntegrationTestProvider.h"

namespace joynr
{
namespace exceptions
{
class ProviderRuntimeException;
} // namespace exceptions
} // namespace joynr

class SystemIntegrationTestProvider : public joynr::test::DefaultSystemIntegrationTestProvider
{
public:
    explicit SystemIntegrationTestProvider(std::function<void(void)> successCallback);
    virtual ~SystemIntegrationTestProvider() override = default;

    // Implement interface method
    virtual void add(const std::int32_t& addendA,
                     const std::int32_t& addendB,
                     std::function<void(const std::int32_t& sum)> onSuccess,
                     std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                             onError) override;

private:
    DISALLOW_COPY_AND_ASSIGN(SystemIntegrationTestProvider);

    std::function<void(void)> successCallback;
};

#endif // SYSTEM_INTEGRATION_TEST_PROVIDER_H

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <cassert>
#include <chrono>
#include <memory>
#include <string>

#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderScope.h"
#include "SystemIntegrationTestProvider.h"

using namespace joynr;

int main(int argc, char* argv[])
{
    // Get a logger
    Logger logger("ProviderApplication");

    // Check the usage
    const std::string programName(argv[0]);
    if (argc != 2) {
        JOYNR_LOG_ERROR(logger, "USAGE: {} <provider-domain>", programName);
        return -1;
    }

    // Get the provider domain
    const std::string providerDomain(argv[1]);
    JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);

    JoynrRuntime* runtime = JoynrRuntime::createRuntime("");

    joynr::Semaphore semaphore;

    std::shared_ptr<SystemIntegrationTestProvider> provider(
            new SystemIntegrationTestProvider([&]() { semaphore.notify(); }));

    joynr::types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);

    // Register the provider
    runtime->registerProvider<test::SystemIntegrationTestProvider>(
            providerDomain, provider, providerQos);

    bool successful = semaphore.waitFor(std::chrono::milliseconds(30000));

    // Unregister the provider
    runtime->unregisterProvider<test::SystemIntegrationTestProvider>(providerDomain, provider);

    delete runtime;

    return successful ? 0 : -1;
}

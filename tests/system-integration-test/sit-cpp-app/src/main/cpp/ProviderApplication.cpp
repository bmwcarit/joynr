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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderScope.h"
#include "SystemIntegrationTestProvider.h"
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

using namespace joynr;

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JOYT", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    // Get a logger
    Logger logger("ProviderApplication");

    // Check the usage
    const std::string programName(argv[0]);
    bool runForever = false;

    if (argc < 2) {
        JOYNR_LOG_ERROR(logger, "USAGE: {} <provider-domain> [runForever]", programName);
        return -1;
    }

    if (argc == 3) {
        const std::string runForeverArg(argv[2]);
        runForever = runForeverArg == "runForever";
    }
    // Get the provider domain
    const std::string providerDomain(argv[1]);
    JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);

    boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
    std::string appDirectory =
            boost::filesystem::system_complete(appFilename).parent_path().string();
    std::string pathToSettings(appDirectory + "/resources/systemintegrationtest-provider.settings");

    std::unique_ptr<JoynrRuntime> runtime = JoynrRuntime::createRuntime(pathToSettings);

    joynr::Semaphore semaphore;

    auto provider = std::make_shared<SystemIntegrationTestProvider>([&]() { semaphore.notify(); });

    joynr::types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);

    // Register the provider
    runtime->registerProvider<test::SystemIntegrationTestProvider>(
            providerDomain, provider, providerQos);

    if (runForever) {
        while (true) {
            semaphore.wait();
        }
    } else {
        bool successful = semaphore.waitFor(std::chrono::milliseconds(30000));

        // Unregister the provider
        runtime->unregisterProvider<test::SystemIntegrationTestProvider>(providerDomain, provider);

        return successful ? 0 : -1;
    }
}

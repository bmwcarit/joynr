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
#include "RobustnessTestProvider.h"

#include <csignal>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "joynr/JoynrRuntime.h"
#include "joynr/Semaphore.h"
#include "joynr/types/ProviderQos.h"
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

using joynr::JoynrRuntime;
using joynr::Semaphore;
using joynr::Logger;

Semaphore semaphore;

void releaseSemaphore(int signum)
{
    semaphore.notify();
}

int main(int argc, char** argv)
{
    // handle signal SIGTERM
    signal(SIGTERM, releaseSemaphore);

#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYRP", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    // Get a logger
    Logger logger("RobustnessTestProviderApplication");

    // Check the usage
    std::string programName(argv[0]);
    if (argc != 2) {
        JOYNR_LOG_ERROR(logger, "USAGE: {} <provider-domain>", programName);
        return 1;
    }

    // Get the provider domain
    std::string providerDomain(argv[1]);
    JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);

    // Get the current program directory
    boost::filesystem::path fullPath =
            boost::filesystem::system_complete(boost::filesystem::path(programName));
    std::string dir = fullPath.parent_path().string();

    // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
    // implementation.
    std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
            [&](const joynr::exceptions::JoynrRuntimeException& exception) {
        JOYNR_LOG_ERROR(
                logger, "Unexpected joynr runtime error occured: " + exception.getMessage());
    };

    // Initialize the joynr runtime
    std::string pathToLibJoynrSettings(dir + "/resources/robustness-tests-provider.settings");
    std::shared_ptr<JoynrRuntime> runtime = JoynrRuntime::createRuntime(pathToLibJoynrSettings);

    // create provider instance
    std::shared_ptr<joynr::RobustnessTestProvider> provider(new joynr::RobustnessTestProvider());

    // default uses a priority that is the current time,
    // causing arbitration to the last started instance if highest priority arbitrator is used
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    joynr::types::ProviderQos robustnessProviderQos;
    robustnessProviderQos.setPriority(millisSinceEpoch.count());
    robustnessProviderQos.setScope(joynr::types::ProviderScope::GLOBAL);
    robustnessProviderQos.setSupportsOnChangeSubscriptions(true);

    // Register the provider
    std::string providerParticipantId =
            runtime->registerProvider<joynr::tests::robustness::TestInterfaceProvider>(
                    providerDomain, provider, robustnessProviderQos);

    JOYNR_LOG_INFO(logger, "***********************");
    JOYNR_LOG_INFO(logger, "Provider is registered. ParticipantId: {}", providerParticipantId);
    JOYNR_LOG_INFO(logger, "***********************");

    auto onError = [&](const joynr::exceptions::ProviderRuntimeException& exception) {
        JOYNR_LOG_ERROR(logger, "!!!!!!!Exception: " + exception.getMessage());
    };

    semaphore.wait();

    JOYNR_LOG_INFO(logger, "***********************");
    JOYNR_LOG_INFO(logger, "Unregister provider. ParticipantId: {}", providerParticipantId);
    JOYNR_LOG_INFO(logger, "***********************");

    // Unregister the provider
    providerParticipantId =
            runtime->unregisterProvider<joynr::tests::robustness::TestInterfaceProvider>(
                    providerDomain, provider);

    JOYNR_LOG_DEBUG(logger, "**********************");
    JOYNR_LOG_DEBUG(
            logger, "unregisterProvider finished. ParticipantId: {}", providerParticipantId);
    JOYNR_LOG_DEBUG(logger, "**********************");

    return 0;
}

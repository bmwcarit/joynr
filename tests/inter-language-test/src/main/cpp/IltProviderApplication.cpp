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

#include "IltHelper.h"
#include "IltProvider.h"
#include "joynr/JoynrRuntime.h"
#include "IltStringBroadcastFilter.h"
#include "joynr/Semaphore.h"
#include "joynr/types/ProviderQos.h"
#include <memory>
#include <string>
#include <iostream>
#include <signal.h>
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

using namespace joynr;

Semaphore semaphore;

int main(int argc, char* argv[])
{
    // setup alternate signal stack
    stack_t ss;
    ss.ss_sp = malloc(SIGSTKSZ);
    if (ss.ss_sp == nullptr) {
        std::cerr << "unable to allocate SIGSTKSZ bytes of stack\n" << std::endl;
        exit(1);
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, nullptr) == -1) {
        std::cerr << "unable to call sigaltstack\n" << std::endl;
        exit(1);
    }

#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYIP", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    // Get a logger
    Logger logger("IltProviderApplication");

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
    std::string dir(IltHelper::getAbsolutePathToExecutable(programName));

    // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
    // implementation.
    std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
            [&](const joynr::exceptions::JoynrRuntimeException& exception) {
        JOYNR_LOG_ERROR(
                logger, "Unexpected joynr runtime error occured: " + exception.getMessage());
        semaphore.notify();
    };

    // Initialise the JOYn runtime
    std::string pathToMessagingSettings(dir + "/resources/ilt-provider.settings");
    // not used
    // std::string pathToLibJoynrSettings(dir.toStdString() +
    // "/resources/test-app-provider.libjoynr.settings");
    std::shared_ptr<JoynrRuntime> runtime =
            JoynrRuntime::createRuntime(pathToMessagingSettings, onFatalRuntimeError);

    // create provider instance
    std::shared_ptr<IltProvider> provider(new IltProvider());
    // Initialise the quality of service settings
    // Set the priority so that the consumer application always uses the most recently
    // started provider
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    joynr::types::ProviderQos iltProviderQos;
    iltProviderQos.setPriority(millisSinceEpoch.count());

    // add any broadcast filters here (later)
    std::shared_ptr<IltStringBroadcastFilter> myStringBroadcastFilter(
            new IltStringBroadcastFilter());
    provider->addBroadcastFilter(myStringBroadcastFilter);

    // Register the provider
    runtime->registerProvider<interlanguagetest::TestInterfaceProvider>(
            providerDomain, provider, iltProviderQos);

    JOYNR_LOG_INFO(logger, "********************************************************************");
    JOYNR_LOG_INFO(logger, "Provider is registered");
    JOYNR_LOG_INFO(logger, "********************************************************************");

    semaphore.wait();

    // Unregister the provider
    runtime->unregisterProvider<interlanguagetest::TestInterfaceProvider>(providerDomain, provider);

    return 0;
}

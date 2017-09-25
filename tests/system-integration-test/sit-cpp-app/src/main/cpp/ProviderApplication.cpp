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

#include <cassert>
#include <chrono>
#include <memory>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

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
    DLT_REGISTER_APP("JYSP", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    // Get a logger
    Logger logger("ProviderApplication");

    namespace po = boost::program_options;

    po::positional_options_description positionalCmdLineOptions;
    positionalCmdLineOptions.add("domain", 1);
    positionalCmdLineOptions.add("runForever", 1);

    std::string providerDomain;
    bool runForever = false;
    std::string pathToSettings;

    po::options_description cmdLineOptions;
    cmdLineOptions.add_options()("domain,d", po::value(&providerDomain)->required())(
            "runForever,r", po::value(&runForever)->default_value(false))(
            "pathtosettings,p", po::value(&pathToSettings));

    try {
        po::variables_map variablesMap;
        po::store(po::command_line_parser(argc, argv)
                          .options(cmdLineOptions)
                          .positional(positionalCmdLineOptions)
                          .run(),
                  variablesMap);
        po::notify(variablesMap);
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return -1;
    }

    JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);

    if (pathToSettings.empty()) {
        boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
        std::string appDirectory =
                boost::filesystem::system_complete(appFilename).parent_path().string();
        pathToSettings = appDirectory + "/resources/systemintegrationtest-provider.settings";
    }

    std::shared_ptr<JoynrRuntime> runtime = JoynrRuntime::createRuntime(pathToSettings);

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

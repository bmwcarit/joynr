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

#include <string>
#include <chrono>
#include <thread>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include "../provider/PerformanceTestEchoProvider.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

using namespace joynr;

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYPP", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    Logger logger("PerformanceTestProviderApplication");

    std::string domainName;
    bool globalScope = false;

    boost::program_options::options_description optionsDescription("Available options");
    optionsDescription.add_options()("help,h", "produce help message")(
            "domain,d", boost::program_options::value(&domainName)->required(), "domain")(
            "globalscope,g", boost::program_options::value(&globalScope)->default_value(false));

    try {
        boost::program_options::variables_map optionsMap;
        boost::program_options::store(
                boost::program_options::parse_command_line(argc, argv, optionsDescription),
                optionsMap);
        boost::program_options::notify(optionsMap);

        if (optionsMap.count("help") > 0) {
            std::cout << optionsDescription << std::endl;
            return 0;
        }

        boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
        std::string appDirectory =
                boost::filesystem::system_complete(appFilename).parent_path().string();
        std::string pathToSettings(appDirectory + "/resources/performancetest-provider.settings");

        std::shared_ptr<JoynrRuntime> runtime(JoynrRuntime::createRuntime(pathToSettings));
        std::shared_ptr<PerformanceTestEchoProvider> provider =
                std::make_shared<PerformanceTestEchoProvider>();

        // Set the provider's priority in such a way that a consumer uses the most recent provider.
        auto millisecondsSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch());

        types::ProviderQos providerQos;
        providerQos.setPriority(millisecondsSinceEpoch.count());
        providerQos.setScope(globalScope ? joynr::types::ProviderScope::GLOBAL
                                         : joynr::types::ProviderScope::LOCAL);

        runtime->registerProvider<tests::performance::EchoProvider>(
                domainName, provider, providerQos);

        JOYNR_LOG_INFO(
                logger, "********************************************************************");
        JOYNR_LOG_INFO(logger, "Provider is registered");
        JOYNR_LOG_INFO(
                logger, "********************************************************************");

        // Run the provider for a week (which should be enough for any testcase).
        std::this_thread::sleep_for(std::chrono::hours(24 * 7));

        runtime->unregisterProvider<tests::performance::EchoProvider>(domainName, provider);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}

/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

#include <csignal>
#include <chrono>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include "joynr/Semaphore.h"

#include "ProviderTest.h"

using joynr::Logger;

constexpr std::size_t DEFAULT_THREAD_DELAY = 100;

// signal handling
namespace
{
    joynr::Semaphore semaphore;
}

void signal_handler(int)
{
    semaphore.notify();
}

// main
int main(int argc, char** argv)
{
    namespace fs = boost::filesystem;
    namespace po = boost::program_options;

    ProviderTestParameters input;
    input.numOfRuntimes = 1;
    input.numOfProviders = 1;
    input.threadDelayMS = DEFAULT_THREAD_DELAY;
    std::string programName(argv[0]);

    std::signal(SIGINT, signal_handler);

    Logger logger("StabilityRobustnessTestProvider");

    // process the command arguments
    po::options_description param("Command parameters");
    param.add_options()
        ("help", "Usage message")
        ("domain,d", po::value<std::string>(&input.providerDomain)->required(), "consumer registers to this domain")
        ("runtime,r", po::value<int>(&input.numOfRuntimes)->required(), "number of runtimes")
        ("provider,p", po::value<int>(&input.numOfProviders)->required(), "number of providers")
        ("tdelay,t", po::value<int>(&input.threadDelayMS), "time delay between thread calls in ms");

    po::variables_map args;

    try {
        po::store( po::parse_command_line(argc, argv, param), args );
    }
    catch (po::error const& e) {
        std::cerr << e.what() << '\n';
        exit( EXIT_FAILURE );
    }
    po::notify(args);

    // Get the current program directory
    fs::path fullPath = fs::system_complete(fs::path(programName));
    std::string dir = fullPath.parent_path().string();
    input.pathToLibJoynrSettings = dir + "/resources/provider-app.settings";
    input.pathToMessagingSettings = dir + "/resources/default-messaging.settings";

    // initialise the provider operations
    ProviderTest providerTest(input);
    providerTest.init();

    // wait here until a notify signal has been recieved.
    semaphore.wait();

    providerTest.shutdown();

    JOYNR_LOG_INFO(logger, "Program {} exiting", programName);
    return 0;
} // main

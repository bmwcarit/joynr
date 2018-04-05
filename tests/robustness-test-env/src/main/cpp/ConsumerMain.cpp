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

#include <atomic>
#include <chrono>
#include <csignal>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include "joynr/Semaphore.h"

#include "ConsumerProxy.h"
#include "ConsumerTest.h"

constexpr std::size_t DEFAULT_NO_OF_TEST_CYCLES = 1;
constexpr std::size_t DEFAULT_THREAD_DELAY_MS = 100;
constexpr std::size_t DEFAULT_TEST_CYCLE_TIME_SEC = 10;
constexpr std::size_t ONE_SECOND_IN_MS = 1000;

// signal handling
namespace
{
std::atomic<bool> exitRequest{false};
joynr::Semaphore semaphore;
}

void signal_handler(int)
{
    semaphore.notify();
    exitRequest = true;
}

int main(int argc, char** argv)
{
    namespace fs = boost::filesystem;
    namespace po = boost::program_options;

    ConsumerTestParameters input;
    input.threadDelayMS = DEFAULT_THREAD_DELAY_MS;
    input.numOfRuntimes = 1;
    input.numOfProxyBuilders = 1;
    input.numOfProxies = 1;

    int testCycleTime = DEFAULT_TEST_CYCLE_TIME_SEC;
    int numOfTestCycles = DEFAULT_NO_OF_TEST_CYCLES;
    unsigned int testCase;
    std::string programName(argv[0]);

    joynr::Logger logger("StabilityRobustnessTestConsumer");

    std::signal(SIGINT, signal_handler);

    // process the command arguments
    po::options_description param("Command parameters");
    param.add_options()("help", "Usage message")(
            "domain,d", po::value<std::string>(&input.providerDomain)->required(), "consumer registers to this domain")(
            "runtime,r", po::value<int>(&input.numOfRuntimes)->required(), "number of runtimes")(
            "proxbuild,b", po::value<int>(&input.numOfProxyBuilders)->required(), "number of proxy builders")(
            "proxies,p", po::value<int>(&input.numOfProxies)->required(), "number of proxies")(
            "testcase,c", po::value<unsigned int>(&testCase)->required(), "test case")(
            "testcycles", po::value<int>(&numOfTestCycles), "number of test cycles")(
            "tdelay", po::value<int>(&input.threadDelayMS), "time delay between thread calls in ms")(
            "testcycletime", po::value<int>(&testCycleTime), "duration of each test cycle in seconds");

    po::variables_map args;

    try {
        po::store(po::parse_command_line(argc, argv, param), args);
    } catch (po::error const& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    po::notify(args);

    // Get the current program directory
    fs::path fullPath = fs::system_complete(fs::path(programName));
    std::string dir = fullPath.parent_path().string();
    input.pathToLibJoynrSettings = dir + "/resources/consumer-app.settings";
    input.pathToMessagingSettings = dir + "/resources/default-messaging.settings";

    std::chrono::milliseconds timeout(testCycleTime * ONE_SECOND_IN_MS);

    for (int testCycleIndex = 0; testCycleIndex < numOfTestCycles; testCycleIndex++) {
        ConsumerTest consumerTest(input);
        consumerTest.init();

        // wait here until the timout (testCycleTime) has been reached
        // or the notify signal has been recieved.
        semaphore.waitFor(timeout);

        consumerTest.shutDown(testCase);

        // If an external terminate signal was recieved,
        // break out of the loop in order to prematurely exit the application
        if (exitRequest)
            break;

    } // for testCycleIndex

    JOYNR_LOG_INFO(logger, "Program {} exiting", programName);
    return 0;
}

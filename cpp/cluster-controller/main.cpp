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

#include <memory>
#include <string>

#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#define JOYNR_STRINGIFY(s) JOYNR_STRINGIFY_INTERNAL(s)
#define JOYNR_STRINGIFY_INTERNAL(s) #s
#endif // JOYNR_ENABLE_DLT_LOGGING

#include "joynr/JoynrClusterControllerRuntime.h"

#include "joynr/JoynrVersion.h"
#include "joynr/Logger.h"
#include "runtimes/cluster-controller-runtime/signal-handler/PosixSignalHandler.h"

namespace
{

std::string getVersionInfo()
{
    return "Joynr cluster-controller."
           "\nJoynr version: " JOYNR_VERSION_STRING "."
           "\nPackage revision: " JOYNR_PACKAGE_REVISION " build on " JOYNR_BUILD_TIME;
}

void printUsage(joynr::Logger& logger, const std::string& programName)
{
    JOYNR_LOG_INFO(logger, "Joynr package revision: " JOYNR_PACKAGE_REVISION ".");
    JOYNR_LOG_INFO(logger, "USAGE: No settings provided. Starting with default settings.");
    JOYNR_LOG_INFO(logger,
                   "USAGE: {}  <file.settings>... [-d <discoveryEntries.json>] [--version|-v]",
                   programName);
}

void printVersionToStdOut()
{
    std::cout << getVersionInfo() << std::endl;
}
}

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging.
    DLT_REGISTER_APP(JOYNR_STRINGIFY(JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID),
                     JOYNR_STRINGIFY(JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION));
#endif // JOYNR_ENABLE_DLT_LOGGING

    // init a logger
    joynr::Logger logger("Runtime");

    // Check the usage.
    const std::string programName(argv[0]);
    if (argc == 1) {
        printUsage(logger, programName);
    }

    // Always print Joynr version.
    printVersionToStdOut();

    // create the cluster controller runtime
    std::shared_ptr<joynr::JoynrClusterControllerRuntime> clusterControllerRuntime;

    try {
        clusterControllerRuntime =
                joynr::JoynrClusterControllerRuntime::create(static_cast<std::size_t>(argc), argv);
    } catch (const joynr::exceptions::JoynrConfigurationException& exception) {
        JOYNR_LOG_FATAL(logger, "Configuration exception: {}", exception.getMessage());
        return 1;
    }

    if (!clusterControllerRuntime) {
        printUsage(logger, programName);
        return 1;
    }

    // the following call creates a background thread for signal handling
    joynr::PosixSignalHandler::setHandleAndRegisterForSignals(clusterControllerRuntime);

    // run the cluster controller forever
    clusterControllerRuntime->runForever();

    // join the signal handling background thread before terminating
    joynr::PosixSignalHandler::stopSignalHandling();

    return 0;
}

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
#include <string>

#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#define JOYNR_STRINGIFY(s) JOYNR_STRINGIFY_INTERNAL(s)
#define JOYNR_STRINGIFY_INTERNAL(s) #s
#endif // JOYNR_ENABLE_DLT_LOGGING

#include "cluster-controller-runtime/JoynrClusterControllerRuntime.h"

#include "joynr/Logger.h"
#include "joynr/Settings.h"
#include "joynr/Util.h"
#include "joynr/JoynrVersion.h"

using namespace joynr;

namespace
{
static const std::string getVersionInfo()
{
    return "Joynr clust-controller."
           "\nJoynr version: " JOYNR_VERSION "."
           "\nPackage revision: " JOYNR_PACKAGE_REVISION " build on " JOYNR_BUILD_TIME;
}

void printUsage(Logger& logger, const std::string& programName)
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
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP(JOYNR_STRINGIFY(JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID),
                     JOYNR_STRINGIFY(JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION));
#endif // JOYNR_ENABLE_DLT_LOGGING

    // init a logger
    Logger logger("Runtime");

    // Check the usage
    const std::string programName(argv[0]);
    if (argc == 1) {
        printUsage(logger, programName);
    }

    // Always print Joynr version
    printVersionToStdOut();

    // Object that holds all the settings
    auto settings = std::make_unique<Settings>();

    // Discovery entry file name
    std::string discoveryEntriesFile;

    // Walk the argument list and
    //  - merge all the settings files into the settings object
    //  - read in input file name to inject discovery entries
    for (int i = 1; i < argc; ++i) {

        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            // exit immediately if only --version was asked
            return 0;
        } else if (std::strcmp(argv[i], "-d") == 0) {
            if (++i < argc) {
                discoveryEntriesFile = argv[i];
            } else {
                printUsage(logger, programName);
            }
            break;
        }

        const std::string settingsFileName(argv[i]);

        // Read the settings file
        JOYNR_LOG_INFO(logger, "Loading settings file: {}", settingsFileName);
        Settings currentSettings(settingsFileName);

        // Check for errors
        if (!currentSettings.isLoaded()) {
            JOYNR_LOG_FATAL(
                    logger, "Provided settings file {} could not be loaded.", settingsFileName);
            return 1;
        }

        // Merge
        Settings::merge(currentSettings, *settings, true);
    }

    // create the cluster controller runtime
    std::unique_ptr<JoynrClusterControllerRuntime> clusterControllerRuntime =
            JoynrClusterControllerRuntime::create(std::move(settings), discoveryEntriesFile);

    // run the cluster controller forever
    clusterControllerRuntime->runForever();
}

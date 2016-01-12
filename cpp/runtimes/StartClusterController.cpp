/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/Logger.h"
#include "joynr/Util.h"
#include <string>

using namespace joynr;

int main(int argc, char* argv[])
{
    // init a logger
    Logger logger("Runtime");

    // Check the usage
    std::string programName(argv[0]);
    if (argc == 1) {
        JOYNR_LOG_INFO(logger) << "USAGE: No settings provided. Starting with default settings.";
        JOYNR_LOG_INFO(logger) << "USAGE: " << programName << "  <file.settings>...";
    }

    // Object that holds all the settings
    Settings settings;

    // Merge all the settings files into the settings object
    for (int i = 1; i < argc; i++) {

        std::string settingsFileName(argv[i]);

        // Read the settings file
        JOYNR_LOG_INFO(logger) << "Loading settings file: " << settingsFileName;
        Settings currentSettings(settingsFileName);

        // Check for errors
        if (!currentSettings.isLoaded()) {
            JOYNR_LOG_FATAL(logger) << "Settings file \"" << settingsFileName
                                    << "\" doesn't exist.";
            return 1;
        }

        // Merge
        Settings::merge(currentSettings, settings, true);
    }

    // create the cluster controller runtime
    JoynrClusterControllerRuntime* clusterControllerRuntime =
            JoynrClusterControllerRuntime::create(&settings);

    // run the cluster controller forever
    clusterControllerRuntime->runForever();
}

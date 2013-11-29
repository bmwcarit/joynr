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
#include "joynr/HttpCommunicationManager.h"
#include "joynr/joynrlogging.h"
#include "joynr/SettingsMerger.h"

using namespace joynr;

int main( int argc, char* argv[] )
{
    // init a logger
    joynr_logging::Logger* logger = joynr_logging::Logging::getInstance()->getLogger("ClusterController", "Runtime");

    // Check the usage
    QString programName(argv[0]);
    if(argc != 3) {
        LOG_ERROR(logger, QString("USAGE: %1 <messageSettingsFile> <libjoynrSettingsFile>").arg(programName));
        return 1;
    }

    // load messaging settings
    QString messageSettingsFilename(argv[1]);
    // load libjoynr settings
    QString libjoynrSettingsFilename(argv[2]);

    // print the configuration
    LOG_INFO(logger, "MessagingSettingsFile: " + messageSettingsFilename);
    LOG_INFO(logger, "LibJoynrSettingsFile: " + libjoynrSettingsFilename);

    QSettings* settings = SettingsMerger::mergeSettings(messageSettingsFilename);
    SettingsMerger::mergeSettings(libjoynrSettingsFilename, settings);

    // create the cluster controller runtime
    JoynrClusterControllerRuntime* clusterControllerRuntime = JoynrClusterControllerRuntime::create(settings);

    // run the cluster controller forever
    clusterControllerRuntime->runForever();
}

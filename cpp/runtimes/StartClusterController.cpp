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
#include <QFile>
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
    if(argc != 1 && argc != 3) {
        LOG_FATAL(logger, QString("USAGE: %1 <messagingSettingsFile> <libjoynrSettingsFile>").arg(programName));
        return 1;
    }
    QString messagingSettingsFilename("resources/default-messaging.settings");
    QString libjoynrSettingsFilename("resources/default-libjoynr.settings");
    if(argc == 3) {
        messagingSettingsFilename = QString(argv[1]);
        libjoynrSettingsFilename = QString(argv[2]);
    } else {
        LOG_INFO(logger, QString("No settings files provided. Try to load default settings..."));
    }

    QFile messagingSettingsFile(messagingSettingsFilename);
    if(!messagingSettingsFile.exists()) {
        LOG_FATAL(logger, QString("messaging settings file not found: %1").arg(messagingSettingsFilename));
        return 1;
    }
    QFile libjoynrSettingsFile(libjoynrSettingsFilename);
    if(!libjoynrSettingsFile.exists()) {
        LOG_FATAL(logger, QString("libjoynr settings file not found: %1").arg(libjoynrSettingsFilename));
        return 1;
    }

    // print the configuration
    LOG_INFO(logger, QString("using messaging settings file: %1").arg(messagingSettingsFilename));
    LOG_INFO(logger, QString("using libjyonr settings file: %1").arg(libjoynrSettingsFilename));

    QSettings* settings = SettingsMerger::mergeSettings(messagingSettingsFilename);
    SettingsMerger::mergeSettings(libjoynrSettingsFilename, settings);

    // create the cluster controller runtime
    JoynrClusterControllerRuntime* clusterControllerRuntime = JoynrClusterControllerRuntime::create(settings);

    // run the cluster controller forever
    clusterControllerRuntime->runForever();
}

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
#include <QSettings>
#include "cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/joynrlogging.h"
#include "joynr/SettingsMerger.h"
#include "joynr/Util.h"

using namespace joynr;

int main( int argc, char* argv[] )
{
    // init a logger
    joynr_logging::Logger* logger = joynr_logging::Logging::getInstance()->getLogger("ClusterController", "Runtime");

    // Check the usage
    QString programName(argv[0]);
    if(argc == 1) {
        LOG_INFO(logger, QString("USAGE: No settings provided. Starting with default settings."));
        LOG_INFO(logger, QString("USAGE: %1 <file.settings>...").arg(programName));
    }

    QString organization("io.joynr");
    QString application = QString("cluster-controller-%1").arg(joynr::Util::createUuid());
    // TODO we should not use QSettings for non-persistent memory-based settings
    // consider using QMap<QString, QVariant> (cf. QSettings documentation)
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, organization, application);
    for(int i = 1; i < argc; i++) {
        QString settingsFileName(argv[i]);
        QFile settingsFile(settingsFileName);
        if(!settingsFile.exists()) {
            LOG_FATAL(logger, QString("Settings file \"%1\" doesn't exist.").arg(settingsFileName));
            return 1;
        }
        LOG_INFO(logger, QString("Loading settings file: %1").arg(settingsFileName));
        QSettings currentSettings(settingsFileName, QSettings::IniFormat);
        SettingsMerger::mergeSettings(currentSettings, settings, true);
    }

    // create the cluster controller runtime
    JoynrClusterControllerRuntime* clusterControllerRuntime = JoynrClusterControllerRuntime::create(&settings);

    // run the cluster controller forever
    clusterControllerRuntime->runForever();
}

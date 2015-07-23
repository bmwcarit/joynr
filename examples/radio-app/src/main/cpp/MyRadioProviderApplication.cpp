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

#include "MyRadioHelper.h"
#include "MyRadioProvider.h"
#include "TrafficServiceBroadcastFilter.h"
#include "GeocastBroadcastFilter.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/TypeUtil.h"

#include <QString>
#include <QSettings>
#include <QFileInfo>
#include <memory>
#include <string>

using namespace joynr;
using joynr_logging::Logger;
using joynr_logging::Logging;

int main(int argc, char* argv[])
{

    // Get a logger
    Logger* logger = Logging::getInstance()->getLogger("DEMO", "MyRadioProviderApplication");

    // Check the usage
    QString programName(argv[0]);
    if (argc != 2) {
        LOG_ERROR(logger, QString("USAGE: %1 <provider-domain>").arg(programName));
        return 1;
    }

    // Get the provider domain
    std::string providerDomain(argv[1]);
    LOG_INFO(logger,
             QString("Registering provider on domain \"%1\"").arg(TypeUtil::toQt(providerDomain)));

    // Get the current program directory
    QString dir(QFileInfo(programName).absolutePath());

    // Initialise the JOYn runtime
    QString pathToMessagingSettings(dir + QString("/resources/radio-app-provider.settings"));
    QString pathToLibJoynrSettings(dir +
                                   QString("/resources/radio-app-provider.libjoynr.settings"));
    JoynrRuntime* runtime = JoynrRuntime::createRuntime(
            TypeUtil::toStd(pathToLibJoynrSettings), TypeUtil::toStd(pathToMessagingSettings));

    // create provider instance
    std::shared_ptr<MyRadioProvider> provider(new MyRadioProvider());
    // add broadcast filters
    QSharedPointer<TrafficServiceBroadcastFilter> trafficServiceBroadcastFilter(
            new TrafficServiceBroadcastFilter());
    provider->addBroadcastFilter(trafficServiceBroadcastFilter);
    QSharedPointer<GeocastBroadcastFilter> geocastBroadcastFilter(new GeocastBroadcastFilter());
    provider->addBroadcastFilter(geocastBroadcastFilter);

    // Register the provider
    runtime->registerProvider<vehicle::RadioProvider>(providerDomain, provider);

    // Run until the user hits q
    int key;
    while ((key = MyRadioHelper::getch()) != 'q') {
        switch (key) {
        case 's':
            provider->shuffleStations([]() {});
            break;
        case 'w':
            provider->fireWeakSignalBroadcast();
            break;
        case 'n':
            provider->fireNewStationDiscoveredBroadcast();
            break;
        default:
            MyRadioHelper::prettyLog(logger,
                                     QString("USAGE press\n"
                                             " q\tto quit\n"
                                             " s\tto shuffle stations\n"
                                             " w\tto fire weak signal broadcast\n"
                                             " n\tto fire new station discovered broadcast"));
            break;
        }
    }

    // Unregister the provider
    runtime->unregisterProvider<vehicle::RadioProvider>(providerDomain, provider);

    delete runtime;
    delete logger;
    return 0;
}

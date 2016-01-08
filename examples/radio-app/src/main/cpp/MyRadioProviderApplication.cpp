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
#include "joynr/QtTypeUtil.h"
#include "joynr/Logger.h"

#include <QString>
#include <memory>
#include <string>

using namespace joynr;

int main(int argc, char* argv[])
{

    // Get a logger
    Logger logger("MyRadioProviderApplication");

    // Check the usage
    QString programName(argv[0]);
    if (argc != 2) {
        JOYNR_LOG_ERROR(logger, "USAGE: {} <provider-domain>", programName.toStdString());
        return 1;
    }

    // Get the provider domain
    std::string providerDomain(argv[1]);
    JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);

    // Get the current program directory
    QString dir(QString::fromStdString(
            MyRadioHelper::getAbsolutePathToExectuable(programName.toStdString())));

    // Initialise the JOYn runtime
    QString pathToMessagingSettings(dir + QString("/resources/radio-app-provider.settings"));
    QString pathToLibJoynrSettings(dir +
                                   QString("/resources/radio-app-provider.libjoynr.settings"));
    JoynrRuntime* runtime = JoynrRuntime::createRuntime(
            QtTypeUtil::toStd(pathToLibJoynrSettings), QtTypeUtil::toStd(pathToMessagingSettings));

    // create provider instance
    std::shared_ptr<MyRadioProvider> provider(new MyRadioProvider());
    // add broadcast filters
    std::shared_ptr<TrafficServiceBroadcastFilter> trafficServiceBroadcastFilter(
            new TrafficServiceBroadcastFilter());
    provider->addBroadcastFilter(trafficServiceBroadcastFilter);
    std::shared_ptr<GeocastBroadcastFilter> geocastBroadcastFilter(new GeocastBroadcastFilter());
    provider->addBroadcastFilter(geocastBroadcastFilter);

    // Register the provider
    runtime->registerProvider<vehicle::RadioProvider>(providerDomain, provider);

    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
            [&](const joynr::exceptions::ProviderRuntimeException& exception) {
        MyRadioHelper::prettyLog(
                logger, QString("Exception: %1").arg(QtTypeUtil::toQt(exception.getMessage())));
    };

    // Run until the user hits q
    int key;
    while ((key = MyRadioHelper::getch()) != 'q') {
        switch (key) {
        case 's':
            provider->shuffleStations([]() {}, onError);
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
    return 0;
}

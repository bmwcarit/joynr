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
#include "joynr/JoynrRuntime.h"

#include <QString>
#include <QSettings>
#include <QFileInfo>

using namespace joynr;
using joynr_logging::Logger;
using joynr_logging::Logging;

int main(int argc, char* argv[]) {

    // Get a logger
    Logger* logger = Logging::getInstance()->getLogger("DEMO", "MyRadioProviderApplication");

    // Check the usage
    QString programName(argv[0]);
    if(argc != 2) {
        LOG_ERROR(logger, QString("USAGE: %1 <provider-domain>").arg(programName));
        return 1;
    }

    // Get the provider domain
    QString providerDomain(argv[1]);
    LOG_INFO(logger, QString("Registering provider on domain \"%1\"").arg(providerDomain));

    // Get the current program directory
    QString dir(QFileInfo(programName).absolutePath());

    // Initialise the JOYn runtime
    QString pathToMessagingSettings(dir + QString("/resources/radio-app-provider.settings"));
    QString pathToLibJoynrSettings(dir + QString("/resources/radio-app-provider.libjoyn.settings"));
    JoynrRuntime* runtime = JoynrRuntime::createRuntime(pathToLibJoynrSettings, pathToMessagingSettings);

    // Initialise the quality of service settings
    // Set the priority so that the consumer application always uses the most recently
    // started provider
    types::ProviderQos providerQos;
    providerQos.setPriority(QDateTime::currentDateTime().toMSecsSinceEpoch());

    // Register the provider
    QSharedPointer<MyRadioProvider> provider(new MyRadioProvider(providerQos));
    QString authenticationToken("MyRadioProvider_authToken");
    runtime->registerCapability<vehicle::RadioProvider>(providerDomain,
                                                        provider,
                                                        authenticationToken);

    // Run until the user hits q
    MyRadioHelper::pressQToContinue();

    // Unregister the provider
    runtime->unregisterCapability<vehicle::RadioProvider>(providerDomain,
                                                        provider,
                                                        authenticationToken);

    delete runtime;
    delete logger;
    return 0;
}


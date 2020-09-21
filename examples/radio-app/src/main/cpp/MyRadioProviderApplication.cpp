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

#include "MyRadioHelper.h"
#include "MyRadioProvider.h"
#include "TrafficServiceBroadcastFilter.h"
#include "GeocastBroadcastFilter.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/types/ProviderQos.h"

#include <atomic>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <chrono>
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

static int fds[2];

using namespace joynr;

int main(int argc, char* argv[])
{
// Register app at the dlt-daemon for logging
#ifdef JOYNR_ENABLE_DLT_LOGGING
    DLT_REGISTER_APP("JYRP", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    // Get a logger
    Logger logger("MyRadioProviderApplication");

    // Check the usage
    std::string programName(argv[0]);
    if (argc != 2) {
        JOYNR_LOG_ERROR(logger, "USAGE: {} <provider-domain>", programName);
        return 1;
    }

    // Get the provider domain
    std::string providerDomain(argv[1]);
    JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);

    // Get the current program directory
    std::string dir(MyRadioHelper::getAbsolutePathToExectuable(programName));

    // Initialise the JOYn runtime
    // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
    // implementation.
    if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) == -1) {
        MyRadioHelper::prettyLog(
                logger,
                "Could not create socketpair to handle unblocking stdin when a callback is called");
        return 1;
    }
    std::atomic_bool isRuntimeOkay(true);
    std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
            [&](const joynr::exceptions::JoynrRuntimeException& exception) {
        isRuntimeOkay.store(false);
        MyRadioHelper::prettyLog(
                logger, "Unexpected joynr runtime error occured: " + exception.getMessage());
        // make sure the background thread terminates, do not block
        int opt = fcntl(fds[0], F_GETFL, 0);
        fcntl(fds[0], F_SETFL, opt | O_NONBLOCK);
        write(fds[0], "q", 1);
    };

    std::string pathToMessagingSettings(dir + "/resources/radio-app-provider.settings");
    std::string pathToLibJoynrSettings(dir + "/resources/radio-app-provider.libjoynr.settings");
    std::shared_ptr<JoynrRuntime> runtime = JoynrRuntime::createRuntime(
            pathToLibJoynrSettings, onFatalRuntimeError, pathToMessagingSettings);

    // create provider instance
    std::shared_ptr<MyRadioProvider> provider(new MyRadioProvider());
    // default uses a priority that is the current time,
    // causing arbitration to the last started instance if highest priority arbitrator is used
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    types::ProviderQos providerQos;
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    // add broadcast filters
    std::shared_ptr<TrafficServiceBroadcastFilter> trafficServiceBroadcastFilter(
            new TrafficServiceBroadcastFilter());
    provider->addBroadcastFilter(trafficServiceBroadcastFilter);
    std::shared_ptr<GeocastBroadcastFilter> geocastBroadcastFilter(new GeocastBroadcastFilter());
    provider->addBroadcastFilter(geocastBroadcastFilter);

    // Register the provider
    runtime->registerProvider<vehicle::RadioProvider>(providerDomain, provider, providerQos);

    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
            [&](const joynr::exceptions::ProviderRuntimeException& exception) {
        MyRadioHelper::prettyLog(logger, "Exception: " + exception.getMessage());
    };

    // Run until the user hits q or a fatal runtime error happens (onFatalRuntimeError is called)
    int key;
    int retval;

    MyRadioHelper::setDirectInputMode();
    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(fds[1], &rfds);
        retval = select(fds[1] + 1, &rfds, NULL, NULL, NULL);
        if (retval == -1) {
            MyRadioHelper::prettyLog(logger, "Select returned error, leaving loop");
            break;
        }
        if (FD_ISSET(fds[1], &rfds)) {
            MyRadioHelper::prettyLog(logger, "Fatal runtime error detected, leaving loop");
            break;
        }
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            key = getchar();
            if (key == 'q') {
                break;
            }
        }

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
                                     "USAGE press\n"
                                     " q\tto quit\n"
                                     " s\tto shuffle stations\n"
                                     " w\tto fire weak signal broadcast\n"
                                     " n\tto fire new station discovered broadcast");
            break;
        }
    }
    MyRadioHelper::restoreInputMode();

    if (isRuntimeOkay.load()) {
        // Unregister the provider
        runtime->unregisterProvider<vehicle::RadioProvider>(providerDomain, provider);
    }

    return isRuntimeOkay.load() ? 0 : 1;
}

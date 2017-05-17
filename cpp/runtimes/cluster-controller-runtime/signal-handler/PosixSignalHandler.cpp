/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#include "PosixSignalHandler.h"

#include <csignal>

#include "joynr/IClusterControllerSignalHandler.h"

namespace joynr
{

INIT_LOGGER(PosixSignalHandler);

std::weak_ptr<IClusterControllerSignalHandler> PosixSignalHandler::clusterControllerPtr;

void PosixSignalHandler::setHandleAndRegisterForSignals(
        std::weak_ptr<IClusterControllerSignalHandler> clusterControllerRuntime)
{
    clusterControllerPtr = std::move(clusterControllerRuntime);

    std::signal(SIGTERM, handleSignal);
    std::signal(SIGUSR1, handleSignal);
    std::signal(SIGUSR2, handleSignal);
}

void PosixSignalHandler::handleSignal(int signal)
{
    if (auto ptr = clusterControllerPtr.lock()) {
        switch (signal) {
        case SIGTERM:
            JOYNR_LOG_TRACE(logger, "Received signal: SIGTERM");
            ptr->shutdown();
            break;
        case SIGUSR1:
            JOYNR_LOG_TRACE(logger, "Received signal: SIGUSR1");
            ptr->startExternalCommunication();
            break;
        case SIGUSR2:
            JOYNR_LOG_TRACE(logger, "Received signal: SIGUSR2");
            ptr->stopExternalCommunication();
            break;
        default:
            JOYNR_LOG_WARN(logger, "Signal Handler did not register for signal: {}", signal);
        }
        return;
    }
    JOYNR_LOG_TRACE(
            logger, "Received Signal: {} but Cluster Controller Pointer is not valid", signal);
}
} // namespace joynr

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
#include <thread>
#include <unistd.h>
#include <sys/socket.h>

#include "joynr/IClusterControllerSignalHandler.h"

namespace joynr
{

std::weak_ptr<IClusterControllerSignalHandler> PosixSignalHandler::clusterControllerPtr;
std::thread PosixSignalHandler::signalHandlingThread;
int PosixSignalHandler::sigReadFd = -1;
int PosixSignalHandler::sigWriteFd = -1;
const char PosixSignalHandler::sigUsr1CharValue = 'a';
const char PosixSignalHandler::sigUsr2CharValue = 'b';
const char PosixSignalHandler::sigTermCharValue = 'c';

void PosixSignalHandler::setHandleAndRegisterForSignals(
        std::weak_ptr<IClusterControllerSignalHandler> clusterControllerRuntime)
{
    clusterControllerPtr = std::move(clusterControllerRuntime);
    int fds[2];

    if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) == -1) {
        JOYNR_LOG_FATAL(logger(), "Could not create socketpair for signal handling");
        return;
    }
    PosixSignalHandler::sigWriteFd = fds[0];
    PosixSignalHandler::sigReadFd = fds[1];

    PosixSignalHandler::signalHandlingThread =
            std::thread(PosixSignalHandler::signalHandlerThreadFunction);

    std::signal(SIGTERM, handleSignal);
    std::signal(SIGUSR1, handleSignal);
    std::signal(SIGUSR2, handleSignal);
    JOYNR_LOG_INFO(logger(), "Signal handling setup completed");
}

void PosixSignalHandler::stopSignalHandling()
{
    if (PosixSignalHandler::sigWriteFd == -1) {
        // cannot join thread since signal handling has not been setup
        JOYNR_LOG_DEBUG(logger(), "Signal handling terminated, no thread had been started");
        return;
    }
    std::signal(SIGTERM, SIG_IGN);
    std::signal(SIGUSR1, SIG_IGN);
    std::signal(SIGUSR2, SIG_IGN);

    // make sure the background thread terminates, do not block
    int opt = fcntl(PosixSignalHandler::sigWriteFd, F_GETFL, 0);
    fcntl(PosixSignalHandler::sigWriteFd, F_SETFL, opt | O_NONBLOCK);
    ssize_t bytesWritten = write(PosixSignalHandler::sigWriteFd, &sigTermCharValue, 1);
    std::ignore = bytesWritten;

    JOYNR_LOG_INFO(logger(), "Joining signal handling thread");
    PosixSignalHandler::signalHandlingThread.join();
    if (PosixSignalHandler::sigWriteFd != -1) {
        close(PosixSignalHandler::sigWriteFd);
    }
    if (PosixSignalHandler::sigReadFd != -1) {
        close(PosixSignalHandler::sigReadFd);
    }
    JOYNR_LOG_INFO(logger(), "Signal handling terminated");
}

void PosixSignalHandler::signalHandlerThreadFunction()
{
    char buf;
    JOYNR_LOG_INFO(logger(), "Signal handling thread waiting for input ...");
    while (read(PosixSignalHandler::sigReadFd, &buf, 1) != -1) {
        auto ptr = clusterControllerPtr.lock();
        if (!ptr) {
            return;
        }
        switch (buf) {
        case sigUsr1CharValue:
            // SIGUSR1
            JOYNR_LOG_INFO(logger(), "Received signal: SIGUSR1");
            ptr->startExternalCommunication();
            break;
        case sigUsr2CharValue:
            // SIGUSR2
            JOYNR_LOG_INFO(logger(), "Received signal: SIGUSR2");
            ptr->stopExternalCommunication();
            break;
        case sigTermCharValue:
            // SIGTERM
            JOYNR_LOG_INFO(logger(), "Received signal: SIGTERM");
            ptr->shutdownClusterController();
            JOYNR_LOG_INFO(logger(), "Leaving Signal handling thread due to shutdown");
            return;
        default:
            // ignore
            break;
        }
        JOYNR_LOG_INFO(logger(), "Signal handling thread waiting for input ...");
    }
    JOYNR_LOG_ERROR(logger(), "Leaving Signal handling thread due to socket read error");
}

void PosixSignalHandler::handleSignal(int signal)
{
    // ret: return the number written bytes to the FD, or -1.
    ssize_t bytesWritten = 0;
    switch (signal) {
    case SIGUSR1:
        bytesWritten = write(PosixSignalHandler::sigWriteFd, &sigUsr1CharValue, 1);
        break;
    case SIGUSR2:
        bytesWritten = write(PosixSignalHandler::sigWriteFd, &sigUsr2CharValue, 1);
        break;
    case SIGTERM:
        bytesWritten = write(PosixSignalHandler::sigWriteFd, &sigTermCharValue, 1);
        break;
    default:
        // ignore
        break;
    }
    std::ignore = bytesWritten;
    return;
}
} // namespace joynr

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

#ifndef POSIXSIGNALHANDLER_H
#define POSIXSIGNALHANDLER_H

#include <memory>

#include "joynr/Logger.h"

namespace joynr
{
class IClusterControllerSignalHandler;

class PosixSignalHandler
{
public:
    static void setHandleAndRegisterForSignals(
            std::weak_ptr<IClusterControllerSignalHandler> clusterControllerRuntime);

private:
    ADD_LOGGER(PosixSignalHandler);
    PosixSignalHandler() = delete;
    ~PosixSignalHandler() = delete;

    static void handleSignal(int signal);

    static std::weak_ptr<IClusterControllerSignalHandler> clusterControllerPtr;
};
} // namespace joynr

#endif // POSIXSIGNALHANDLER_H

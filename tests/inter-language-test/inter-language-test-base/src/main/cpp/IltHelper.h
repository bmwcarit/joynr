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
#ifndef MY_HELPER_H
#define MY_HELPER_H

// #include "joynr/joynrlogging.h"
#include "joynr/Logger.h"

/**
 * A helper class for use by the Ilt consumer and provider applications
 */
class IltHelper
{
public:
    IltHelper();

    /**
     * Wait until the user presses the q key
     */
    static void pressQToContinue();

    static std::string getAbsolutePathToExecutable(const std::string& executableName);

    static int getch();

    /**
     * Output a prominent log message at level INFO
     */
    static void prettyLog(joynr::Logger& logger(), const std::string& message);

private:
    // static joynr::joynr_logging::Logger* logger;
    ADD_LOGGER(IltHelper)
};

#endif

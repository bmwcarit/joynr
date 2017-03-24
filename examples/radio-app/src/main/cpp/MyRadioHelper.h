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
#ifndef MY_RADIO_HELPER_H
#define MY_RADIO_HELPER_H

#include "joynr/Logger.h"
#include <string>

/**
 * A helper class for use by the MyRadio consumer and provider applications
 */
class MyRadioHelper
{
public:
    MyRadioHelper();

    /**
     * Wait until the user presses the q key
     */
    static void pressQToContinue();

    static std::string getAbsolutePathToExectuable(const std::string& executableName);

    static int getch();

    static const std::string& MISSING_NAME();
    /**
     * Output a prominent log message at level INFO
     */
    static void prettyLog(joynr::Logger& logger, const std::string& message);

private:
    ADD_LOGGER(MyRadioHelper);
};

#endif

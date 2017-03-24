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
#include <termios.h>
#include <unistd.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace joynr;

INIT_LOGGER(MyRadioHelper);

MyRadioHelper::MyRadioHelper()
{
}

int MyRadioHelper::getch()
{
    termios terminalSettingsBackup;
    tcgetattr(STDIN_FILENO, &terminalSettingsBackup);

    termios terminalSettings = terminalSettingsBackup;
    terminalSettings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &terminalSettings);

    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &terminalSettingsBackup);

    return ch;
}

const std::string& MyRadioHelper::MISSING_NAME()
{
    static const std::string missingName("MISSING_NAME");
    return missingName;
}

void MyRadioHelper::pressQToContinue()
{
    JOYNR_LOG_INFO(logger, "*****************************************************");
    JOYNR_LOG_INFO(logger, "Please press \"q\" to quit the application\n");
    JOYNR_LOG_INFO(logger, "*****************************************************");

    while (getch() != 'q')
        ;
}

std::string MyRadioHelper::getAbsolutePathToExectuable(const std::string& executableName)
{
    boost::filesystem::path fullPath =
            boost::filesystem::system_complete(boost::filesystem::path(executableName));
    return fullPath.parent_path().string();
}

void MyRadioHelper::prettyLog(joynr::Logger& logger, const std::string& message)
{
    JOYNR_LOG_INFO(logger, "--------------------------------------------------");
    JOYNR_LOG_INFO(logger, message);
    JOYNR_LOG_INFO(logger, "--------------------------------------------------");
}

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
#include <QTextStream>
#include <termios.h>
#include <unistd.h>

using namespace joynr;

joynr_logging::Logger* MyRadioHelper::logger =
        joynr_logging::Logging::getInstance()->getLogger("DEMO", "MyRadioHelper");

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
    LOG_INFO(logger, "*****************************************************");
    LOG_INFO(logger, "Please press \"q\" to quit the application\n");
    LOG_INFO(logger, "*****************************************************");

    while (getch() != 'q')
        ;
}

void MyRadioHelper::prettyLog(joynr_logging::Logger* logger, const QString& message)
{
    LOG_INFO(logger, QString("--------------------------------------------------"));
    LOG_INFO(logger, message);
    LOG_INFO(logger, QString("--------------------------------------------------"));
}

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

using namespace joynr;

joynr_logging::Logger* MyRadioHelper::logger = joynr_logging::Logging::getInstance()->getLogger("DEMO", "MyRadioHelper");

MyRadioHelper::MyRadioHelper()
{
}

void MyRadioHelper::pressQToContinue()
{
    LOG_INFO(logger, "*****************************************************");
    LOG_INFO(logger, "Please press \"q + <Enter>\" to quit the application\n");
    LOG_INFO(logger, "*****************************************************");

    QTextStream cin(stdin);
    QString input;
    do {
        input = cin.readLine();
    } while (input != QString("q"));
}

void MyRadioHelper::prettyLog(joynr_logging::Logger *logger, const QString& message)
{
    LOG_INFO(logger, QString("--------------------------------------------------"));
    LOG_INFO(logger, message);
    LOG_INFO(logger, QString("--------------------------------------------------"));
}

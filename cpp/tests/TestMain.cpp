/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include "PrettyPrint.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QObject>
#include <QCoreApplication>
#include <QTimer>
#include <QThread>


using namespace joynr;

// executable: g_IntegrationTests
int main(int argc, char *argv[])
{
    // set the name of the main thread, used for logging
    QThread::currentThread()->setObjectName(QString("main"));

    // the QCoreApplication is needed to have an event loop
    // it seems the event loop is running, although exec was
    // not called on the QCoreApplication
    QCoreApplication app(argc, argv);

    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}




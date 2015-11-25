/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#include "runtimes/libjoynr-runtime/JoynrRuntimeExecutor.h"

#include <QtConcurrent/QtConcurrent>

#include "LibJoynrRuntime.h"
#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
#include "joynr/Settings.h"

namespace joynr
{

JoynrRuntimeExecutor::JoynrRuntimeExecutor(Settings* settings)
        : QObject(),
          coreApplication(Q_NULLPTR),
          runtimeThread(new QThread()),
          settings(settings),
          runtime(Q_NULLPTR),
          runtimeSemaphore(0)
{
    if (QCoreApplication::instance() == Q_NULLPTR) {
        int argc = 0;
        char* argv[] = {0};
        coreApplication = new QCoreApplication(argc, argv);
    }
    runtimeThread->setObjectName(QString("LibJoynrRuntime-Thread"));
    this->moveToThread(runtimeThread);
    connect(runtimeThread, &QThread::finished, this, &QObject::deleteLater);
    connect(runtimeThread, &QThread::started, this, &JoynrRuntimeExecutor::createRuntime);
    runtimeThread->start();
}

JoynrRuntimeExecutor::~JoynrRuntimeExecutor()
{
    if (coreApplication != Q_NULLPTR) {
        coreApplication->deleteLater();
        coreApplication = Q_NULLPTR;
    }
    runtimeThread->deleteLater();
    delete runtime;
    runtime = Q_NULLPTR;
}

LibJoynrRuntime* JoynrRuntimeExecutor::getRuntime()
{
    runtimeSemaphore.acquire();
    LibJoynrRuntime* runtimeTmp = runtime;
    runtime = Q_NULLPTR;
    return runtimeTmp;
}

void JoynrRuntimeExecutor::stop()
{
    runtimeThread->exit();
}

} // namespace joynr

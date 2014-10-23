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

#ifndef JOYNRWEBSOCKETRUNTIMEEXECUTOR_H
#define JOYNRWEBSOCKETRUNTIMEEXECUTOR_H

#include <QtCore/QSettings>

#include "joynr/PrivateCopyAssign.h"
#include "runtimes/libjoynr-runtime/JoynrRuntimeExecutor.h"

namespace joynr
{

class LibJoynrRuntime;

class JoynrWebSocketRuntimeExecutor : public JoynrRuntimeExecutor
{
    Q_OBJECT

public:
    JoynrWebSocketRuntimeExecutor(QSettings* settings);
    ~JoynrWebSocketRuntimeExecutor()
    {
    }

public slots:
    virtual void createRuntime();

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrWebSocketRuntimeExecutor);
};

} // namespace joynr
#endif // JOYNRWEBSOCKETRUNTIMEEXECUTOR_H

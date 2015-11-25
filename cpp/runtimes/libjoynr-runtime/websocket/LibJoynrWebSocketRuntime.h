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

#ifndef LIBJOYNRWEBSOCKETRUNTIME_H
#define LIBJOYNRWEBSOCKETRUNTIME_H

#include <QtCore/QSettings>
#include <QtWebSockets/QWebSocket>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/joynrlogging.h"
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
#include "libjoynr/websocket/WebSocketSettings.h"

namespace joynr
{

class WebSocketLibJoynrMessagingSkeleton;

class LibJoynrWebSocketRuntime : public LibJoynrRuntime
{
    WebSocketSettings wsSettings;
    QWebSocket* websocket;

public:
    LibJoynrWebSocketRuntime(Settings* settings);
    virtual ~LibJoynrWebSocketRuntime();

protected:
    WebSocketLibJoynrMessagingSkeleton* wsLibJoynrMessagingSkeleton;

    virtual void startLibJoynrMessagingSkeleton(MessageRouter& messageRouter);

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrWebSocketRuntime);
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // LIBJOYNRWEBSOCKETRUNTIME_H

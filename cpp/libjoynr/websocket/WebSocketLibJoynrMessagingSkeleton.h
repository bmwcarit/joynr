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
#ifndef WEBSOCKETLIBJOYNRMESSAGINGSKELETON_H
#define WEBSOCKETLIBJOYNRMESSAGINGSKELETON_H

#include <QtCore/QObject>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/joynrlogging.h"

#include "joynr/MessageRouter.h"
#include "joynr/IMessaging.h"

class QWebSocketServer;
class QWebSocket;

namespace joynr {

class WebSocketLibJoynrMessagingSkeleton : public QObject, public IMessaging
{
    Q_OBJECT
public:
    WebSocketLibJoynrMessagingSkeleton(
            MessageRouter& messageRouter
    );

    ~WebSocketLibJoynrMessagingSkeleton();

    virtual void transmit(JoynrMessage& message);

public Q_SLOTS:
    void onTextMessageReceived(const QString& message);

private:
    DISALLOW_COPY_AND_ASSIGN(WebSocketLibJoynrMessagingSkeleton);
    static joynr_logging::Logger* logger;
    MessageRouter& messageRouter;
};


} // namespace joynr
#endif // WEBSOCKETLIBJOYNRMESSAGINGSKELETON_H

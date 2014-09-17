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
#ifndef WEBSOCKETMESSAGINSTUB_H
#define WEBSOCKETMESSAGINSTUB_H

#include <QtCore/QObject>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/joynrlogging.h"

#include "joynr/IMessaging.h"

class QWebSocket;

namespace joynr {

namespace system {
    class Address;
}

class WebSocketMessagingStub : public QObject, public IMessaging
{
    Q_OBJECT
public:
    WebSocketMessagingStub(system::Address* address, QWebSocket* webSocket, QObject* parent = Q_NULLPTR);
    virtual ~WebSocketMessagingStub();
    virtual void transmit(JoynrMessage &message, const MessagingQos &qos);

Q_SIGNALS:
    void closed(const joynr::system::Address& address);
private Q_SLOTS:
    void onSocketDisconnected();
    void sendTextMessage(const QString &message);

private:
    static joynr_logging::Logger* logger;
    system::Address* address;
    QWebSocket* webSocket;
    DISALLOW_COPY_AND_ASSIGN(WebSocketMessagingStub);
};


} // namespace joynr
#endif // WEBSOCKETMESSAGINSTUB_H

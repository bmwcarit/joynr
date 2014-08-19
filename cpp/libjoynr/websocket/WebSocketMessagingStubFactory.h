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
#ifndef WEBSOCKETMESSAGINGSTUBFACTORY_H
#define WEBSOCKETMESSAGINGSTUBFACTORY_H

#include <QHash>
#include <QMutex>

#include "joynr/joynrlogging.h"
#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/system/WebSocketAddress.h"

class QWebSocket;

uint qHash(joynr::system::WebSocketAddress key);

namespace joynr {

class WebSocketMessagingStubFactory : public IMiddlewareMessagingStubFactory {

public:
    WebSocketMessagingStubFactory();
    QSharedPointer<IMessaging> create(const joynr::system::Address& destAddress);
    bool canCreate(const joynr::system::Address& destAddress);
    void addClient(const joynr::system::WebSocketAddress& clientAddress, QWebSocket* webSocket);
    void removeClient(const joynr::system::WebSocketAddress& clientAddress);

private:
    QHash<joynr::system::WebSocketAddress, QSharedPointer<IMessaging>> stubMap;
    QMutex mutex;

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // WEBSOCKETMESSAGINGSTUBFACTORY_H

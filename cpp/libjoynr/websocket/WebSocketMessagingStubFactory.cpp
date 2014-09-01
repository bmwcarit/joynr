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
#include "WebSocketMessagingStubFactory.h"

#include <QMutexLocker>
#include <assert.h>

#include "common/websocket/WebSocketMessagingStub.h"

namespace  joynr {

joynr_logging::Logger* WebSocketMessagingStubFactory::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "WebSocketMessagingStubFactory");

WebSocketMessagingStubFactory::WebSocketMessagingStubFactory():
    stubMap(),
    mutex()
{
}

bool WebSocketMessagingStubFactory::canCreate(const joynr::system::Address& destAddress) {
    return destAddress.inherits(system::WebSocketAddress::staticMetaObject.className());
}

QSharedPointer<IMessaging> WebSocketMessagingStubFactory::create(const joynr::system::Address& destAddress) {
    const system::WebSocketAddress* webSocketAddress = dynamic_cast<const system::WebSocketAddress*>(&destAddress);
    // lookup address
    {
        QMutexLocker locker(&mutex);
        if(!stubMap.contains(*webSocketAddress)) {
            LOG_ERROR(logger, QString("No websocket found for address %0").arg(webSocketAddress->toString()));
        }
    }
    return stubMap.value(*webSocketAddress, QSharedPointer<IMessaging>());
}

void WebSocketMessagingStubFactory::addClient(
        const joynr::system::WebSocketAddress& clientAddress,
        QWebSocket* webSocket
) {
    QSharedPointer<IMessaging> clientStub(new WebSocketMessagingStub(webSocket));
    stubMap.insert(clientAddress, clientStub);
}

void WebSocketMessagingStubFactory::removeClient(const joynr::system::WebSocketAddress& clientAddress) {
    stubMap.remove(clientAddress);
}


} // namespace joynr

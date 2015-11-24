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

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <memory>
#include <mutex>

#include "joynr/joynrlogging.h"
#include "joynr/IMiddlewareMessagingStubFactory.h"

class QWebSocket;

namespace joynr
{

namespace system
{

namespace RoutingTypes
{
class QtAddress;
class QtWebSocketAddress;
class QtWebSocketClientAddress;
}
}

class WebSocketMessagingStubFactory : public QObject, public IMiddlewareMessagingStubFactory
{
    Q_OBJECT

public:
    WebSocketMessagingStubFactory(QObject* parent = Q_NULLPTR);
    std::shared_ptr<IMessaging> create(const joynr::system::RoutingTypes::QtAddress& destAddress);
    bool canCreate(const joynr::system::RoutingTypes::QtAddress& destAddress);
    void addClient(const joynr::system::RoutingTypes::QtWebSocketClientAddress& clientAddress,
                   QWebSocket* webSocket);
    void removeClient(const joynr::system::RoutingTypes::QtWebSocketClientAddress& clientAddress);
    void addServer(const joynr::system::RoutingTypes::QtWebSocketAddress& serverAddress,
                   QWebSocket* webSocket);

    static QUrl convertWebSocketAddressToUrl(
            const joynr::system::RoutingTypes::QtWebSocketAddress& address);

private Q_SLOTS:
    void onMessagingStubClosed(const joynr::system::RoutingTypes::QtAddress& address);

private:
    QHash<joynr::system::RoutingTypes::QtWebSocketAddress, std::shared_ptr<IMessaging>>
            serverStubMap;
    QHash<joynr::system::RoutingTypes::QtWebSocketClientAddress, std::shared_ptr<IMessaging>>
            clientStubMap;
    std::mutex mutex;

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // WEBSOCKETMESSAGINGSTUBFACTORY_H

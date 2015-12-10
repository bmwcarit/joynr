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
#include <unordered_map>
#include <QtCore/QUrl>
#include <memory>
#include <mutex>

#include "joynr/joynrlogging.h"
#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

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
    explicit WebSocketMessagingStubFactory(QObject* parent = nullptr);
    std::shared_ptr<IMessaging> create(const joynr::system::RoutingTypes::Address& destAddress);
    bool canCreate(const joynr::system::RoutingTypes::Address& destAddress);
    void addClient(const system::RoutingTypes::WebSocketClientAddress* clientAddress,
                   QWebSocket* webSocket);
    void removeClient(const joynr::system::RoutingTypes::WebSocketClientAddress& clientAddress);
    void addServer(const joynr::system::RoutingTypes::WebSocketAddress& serverAddress,
                   QWebSocket* webSocket);

    static QUrl convertWebSocketAddressToUrl(
            const joynr::system::RoutingTypes::WebSocketAddress& address);

private Q_SLOTS:
    void onMessagingStubClosed(const system::RoutingTypes::Address& address);

private:
    std::unordered_map<joynr::system::RoutingTypes::WebSocketAddress, std::shared_ptr<IMessaging>>
            serverStubMap;
    std::unordered_map<joynr::system::RoutingTypes::WebSocketClientAddress,
                       std::shared_ptr<IMessaging>> clientStubMap;
    std::mutex mutex;

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // WEBSOCKETMESSAGINGSTUBFACTORY_H

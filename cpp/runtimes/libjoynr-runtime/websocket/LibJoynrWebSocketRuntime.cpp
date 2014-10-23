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
#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"

#include <QtCore/QObject>

#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "joynr/system/WebSocketClientAddress.h"
#include "libjoynr/websocket/WebSocketLibJoynrMessagingSkeleton.h"
#include "joynr/Util.h"
#include "joynr/JsonSerializer.h"

namespace joynr
{

joynr_logging::Logger* LibJoynrWebSocketRuntime::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "LibJoynrWebSocketRuntime");

LibJoynrWebSocketRuntime::LibJoynrWebSocketRuntime(QSettings* settings)
        : LibJoynrRuntime(settings),
          wsSettings(*settings),
          websocket(Q_NULLPTR),
          wsLibJoynrMessagingSkeleton(Q_NULLPTR)
{
    QString messagingUuid = Util::createUuid().replace("-", "");
    QString libjoynrMessagingId("libjoynr.messaging.participantid_" + messagingUuid);
    QSharedPointer<joynr::system::Address> libjoynrMessagingAddress(
            new system::WebSocketClientAddress(libjoynrMessagingId));

    // create connection to parent routing service
    QSharedPointer<joynr::system::WebSocketAddress> ccMessagingAddress(
            new joynr::system::WebSocketAddress(
                    wsSettings.createClusterControllerMessagingAddress()));

    websocket = new QWebSocket();

    // wait synchronously until websocket is connected
    QEventLoop loop;
    QObject::connect(websocket, &QWebSocket::connected, &loop, &QEventLoop::quit);
    websocket->open(
            WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(*ccMessagingAddress));
    loop.exec();

    // send intialization message containing libjoynr messaging address
    QString initializationMsg(JsonSerializer::serialize(*libjoynrMessagingAddress));
    LOG_TRACE(logger,
              QString("OUTGOING sending websocket intialization message\nmessage: %0\nto: %1")
                      .arg(initializationMsg)
                      .arg(libjoynrMessagingAddress->toString()));
    websocket->sendTextMessage(initializationMsg);

    WebSocketMessagingStubFactory* factory = new WebSocketMessagingStubFactory();
    factory->addServer(*ccMessagingAddress, websocket);

    LibJoynrRuntime::init(factory, libjoynrMessagingAddress, ccMessagingAddress);
}

LibJoynrWebSocketRuntime::~LibJoynrWebSocketRuntime()
{
    delete wsLibJoynrMessagingSkeleton;
    wsLibJoynrMessagingSkeleton = Q_NULLPTR;
}

void LibJoynrWebSocketRuntime::startLibJoynrMessagingSkeleton(MessageRouter& messageRouter)
{
    // create messaging skeleton using uuid
    wsLibJoynrMessagingSkeleton = new WebSocketLibJoynrMessagingSkeleton(messageRouter);

    QObject::connect(websocket,
                     &QWebSocket::textMessageReceived,
                     wsLibJoynrMessagingSkeleton,
                     &WebSocketLibJoynrMessagingSkeleton::onTextMessageReceived);
}

} // namespace joynr

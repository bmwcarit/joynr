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
#include "joynr/MessageRouter.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/Directory.h"
#include "joynr/joynrlogging.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/system/Address.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/RequestStatusCode.h"
#include "joynr/JsonSerializer.h"

#include <QMutexLocker>

#include <cassert>

namespace joynr {

using namespace joynr_logging;
Logger* MessageRouter::logger = Logging::getInstance()->getLogger("MSG", "MessageRouter");

MessageRouter::~MessageRouter() {
    threadPool.waitForDone();
    if(parentRouter != NULL) {
        delete parentRouter;
    }
    delete delayedScheduler;
    delete messagingStubFactory;
    delete messageQueue;
    delete runningParentResolves;
}

MessageRouter::MessageRouter(
        Directory<QString, joynr::system::Address>* routingTable,
        IMessagingStubFactory* messagingStubFactory,
        int messageSendRetryInterval,
        int maxThreads
) :
        joynr::system::RoutingProvider(joynr::types::ProviderQos(
                QList<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // provider version
                1,                                      // provider priority
                joynr::types::ProviderScope::LOCAL,     // provider discovery scope
                false                                   // supports on change subscriptions
        )),
        messagingStubFactory(messagingStubFactory),
        routingTable(routingTable),
        routingTableMutex(),
        threadPool(),
        delayedScheduler(),
        parentRouter(NULL),
        parentAddress(NULL),
        incomingAddress(),
        messageQueue(new QMap<QString, QPair<JoynrMessage, MessagingQos>>()),
        messageQueueMutex(),
        runningParentResolves(new QSet<QString>()),
        parentResolveMutex()
{
    init(messageSendRetryInterval, maxThreads);
}

MessageRouter::MessageRouter(
        Directory<QString, joynr::system::Address>* routingTable,
        IMessagingStubFactory* messagingStubFactory,
        QSharedPointer<joynr::system::Address> incomingAddress,
        int messageSendRetryInterval,
        int maxThreads
) :
    joynr::system::RoutingProvider(joynr::types::ProviderQos(
            QList<joynr::types::CustomParameter>(), // custom provider parameters
            1,                                      // provider version
            1,                                      // provider priority
            joynr::types::ProviderScope::LOCAL,     // provider discovery scope
            false                                   // supports on change subscriptions
    )),
    messagingStubFactory(messagingStubFactory),
    routingTable(routingTable),
    routingTableMutex(),
    threadPool(),
    delayedScheduler(),
    parentRouter(NULL),
    parentAddress(NULL),
    incomingAddress(incomingAddress),
    messageQueue(new QMap<QString, QPair<JoynrMessage, MessagingQos>>()),
    messageQueueMutex(),
    runningParentResolves(new QSet<QString>()),
    parentResolveMutex()
{
    init(messageSendRetryInterval, maxThreads);
}

void MessageRouter::init(int messageSendRetryInterval, int maxThreads)
{
    threadPool.setMaxThreadCount(maxThreads);
    delayedScheduler = new ThreadPoolDelayedScheduler(
                threadPool,
                QString("MessageRouter-DelayedScheduler"),
                messageSendRetryInterval
    );
}

void MessageRouter::addProvisionedNextHop(QString participantId, QSharedPointer<joynr::system::Address> address) {
    addToRoutingTable(participantId, address);
}

void MessageRouter::setParentRouter(joynr::system::RoutingProxy* parentRouter, QSharedPointer<joynr::system::Address> parentAddress, QString parentParticipantId) {
    this->parentRouter = parentRouter;
    this->parentAddress = parentAddress;

    // add the next hop to parent router
    // this is necessary because during normal registration, the parent proxy is not yet set
    joynr::RequestStatus status;
    addNextHop(parentParticipantId, parentAddress);
    addNextHopToParent(status, parentRouter->getProxyParticipantId());
}

bool MessageRouter::isChildMessageRouter(){
    if(incomingAddress.isNull()) {
        return false;
    }
    // if an incoming address is set, a parent message router is needed for correct configuration
    return parentRouter != NULL && !parentAddress.isNull();
}

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void MessageRouter::route(const JoynrMessage& message, const MessagingQos& qos) {
    assert(messagingStubFactory != NULL);
    // neither JoynrMessage nor MessagingQos give a decaytime, so it doesn't make sense to check for
    // a passed TTL. The TTL itself is only relative, not absolute, so it cannot be used here.
    /*
    if (QDateTime::currentMSecsSinceEpoch() >  qos.getRoundTripTtl_ms()) {
        LOG_DEBUG(logger, "received an expired Message. Dropping the message");
        return;
    }
    */

    // search for the destination address
    QString destinationPartId = message.getHeaderTo();
    QSharedPointer<joynr::system::Address> destAddress(NULL);
    {
        QMutexLocker locker(&routingTableMutex);
        destAddress = routingTable->lookup(destinationPartId);
    }

    // schedule message for sending
    if (!destAddress.isNull()) {
        sendMessage(message, qos, destAddress);
        return;
    }

    // try to resolve via parent message router
    if(isChildMessageRouter()){
        auto pair = QPair<JoynrMessage, MessagingQos>(message, qos);
        {
            QMutexLocker locker(&messageQueueMutex);
            messageQueue->insertMulti(destinationPartId, pair);
        }

        {
            QMutexLocker locker(&parentResolveMutex);
            if(!runningParentResolves->contains(destinationPartId)) {
                runningParentResolves->insert(destinationPartId);
                auto callBack = QSharedPointer<ICallback<bool>>(new ResolveCallBack(*this, destinationPartId));
                parentRouter->resolveNextHop(callBack, destinationPartId);
            }
        }
    }
}

void MessageRouter::sendMessageToParticipant(QString& destinationPartId) {
    {
        QMutexLocker locker(&parentResolveMutex);
        runningParentResolves->remove(destinationPartId);
    }
    while(true) {
        QPair<JoynrMessage, MessagingQos> pair;
        {
            QMutexLocker locker(&messageQueueMutex);
            if(messageQueue->contains(destinationPartId)) {
                pair = messageQueue->take(destinationPartId);
            } else {
                break;
            }
        }
        sendMessage(pair.first, pair.second, parentAddress);
    }
}

void MessageRouter::sendMessage(const JoynrMessage& message,
                                const MessagingQos& qos,
                                QSharedPointer<joynr::system::Address> destAddress) {
    auto stub = messagingStubFactory->create(message.getHeaderTo(), *destAddress);
    if(!stub.isNull()) {
        threadPool.start(new MessageRunnable(message, qos, stub));
    }
}

void MessageRouter::addNextHop(
        QString participantId,
        QSharedPointer<joynr::system::Address> inprocessAddress
) {
    addToRoutingTable(participantId, inprocessAddress);

    joynr::RequestStatus status;
    addNextHopToParent(status, participantId);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::ChannelAddress channelAddress
) {
    QSharedPointer<joynr::system::ChannelAddress> address(
                new joynr::system::ChannelAddress(channelAddress)
    );
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::CommonApiDbusAddress commonApiDbusAddress
) {
    QSharedPointer<joynr::system::CommonApiDbusAddress> address(
                new joynr::system::CommonApiDbusAddress(commonApiDbusAddress)
    );
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::BrowserAddress browserAddress
) {
    QSharedPointer<joynr::system::BrowserAddress> address(
                new joynr::system::BrowserAddress(browserAddress)
    );
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::WebSocketAddress webSocketAddress
) {
    QSharedPointer<joynr::system::WebSocketAddress> address(
                new joynr::system::WebSocketAddress(webSocketAddress)
    );
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);
}

void MessageRouter::addNextHopToParent(joynr::RequestStatus& joynrInternalStatus, QString participantId) {
    // add to parent router
    if(isChildMessageRouter()) {
        if(incomingAddress->inherits("joynr::system::ChannelAddress")) {
            parentRouter->addNextHop(joynrInternalStatus, participantId, *dynamic_cast<joynr::system::ChannelAddress*>(incomingAddress.data()));
        }
        if(incomingAddress->inherits("joynr::system::CommonApiDbusAddress")) {
            parentRouter->addNextHop(joynrInternalStatus, participantId, *dynamic_cast<joynr::system::CommonApiDbusAddress*>(incomingAddress.data()));
        }
        if(incomingAddress->inherits("joynr::system::BrowserAddress")) {
            parentRouter->addNextHop(joynrInternalStatus, participantId, *dynamic_cast<joynr::system::BrowserAddress*>(incomingAddress.data()));
        }
        if(incomingAddress->inherits("joynr::system::WebSocketAddress")) {
            parentRouter->addNextHop(joynrInternalStatus, participantId, *dynamic_cast<joynr::system::WebSocketAddress*>(incomingAddress.data()));
        }
    }
}

void MessageRouter::addToRoutingTable(QString participantId, QSharedPointer<joynr::system::Address> address) {
    QMutexLocker locker(&routingTableMutex);
    routingTable->add(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::removeNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId
) {
    {
        QMutexLocker locker(&routingTableMutex);
        routingTable->remove(participantId);
    }
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    // remove from parent router
    if(isChildMessageRouter()) {
        parentRouter->removeNextHop(joynrInternalStatus, participantId);
    }
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::resolveNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        bool& resolved,
        QString participantId
) {
    {
        QMutexLocker locker(&routingTableMutex);
        resolved = routingTable->contains(participantId);
    }
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}


/**
 * IMPLEMENTATION of ResolveCallBack class
 */

Logger* ResolveCallBack::logger = Logging::getInstance()->getLogger("MSG", "ResolveCallBack");

ResolveCallBack::ResolveCallBack(MessageRouter& messageRouter, QString destinationPartId):
    messageRouter(messageRouter),
    destinationPartId(destinationPartId)
{
}

void ResolveCallBack::onFailure(const RequestStatus status) {
    LOG_ERROR(logger, "Failed to resolve next hop for participant " + destinationPartId + ": " + status.toString());
}

void ResolveCallBack::onSuccess(const RequestStatus status, bool resolved) {
    if(status.successful() && resolved) {
        LOG_INFO(logger, "Got destination address for participant " + destinationPartId);
        // save next hop in the routing table
        messageRouter.addProvisionedNextHop(destinationPartId, messageRouter.parentAddress);
        messageRouter.sendMessageToParticipant(destinationPartId);
    }
}

/**
 * IMPLEMENTATION of MessageRunnable class
 */

Logger* MessageRunnable::logger = Logging::getInstance()->getLogger("MSG", "MessageRunnable");

MessageRunnable::MessageRunnable(const JoynrMessage& message,
                                 const MessagingQos& qos,
                                 QSharedPointer<IMessaging> messagingStub):
    ObjectWithDecayTime(DispatcherUtils::convertTtlToAbsoluteTime(qos.getTtl())),
    message(message),
    qos(qos),
    messagingStub(messagingStub)
{
}

void MessageRunnable::run() {
    if(!isExpired()) {
        messagingStub->transmit(message, qos);
    } else {
        LOG_ERROR(logger, "Message expired: dropping!");
    }
}

} // namespace joynr

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
#include "joynr/system/Address.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/RequestStatusCode.h"
#include "joynr/JsonSerializer.h"

#include <QMutexLocker>

#include <cassert>

namespace joynr
{

using namespace joynr_logging;
Logger* MessageRouter::logger = Logging::getInstance()->getLogger("MSG", "MessageRouter");

MessageRouter::~MessageRouter()
{
    messageQueueCleanerRunnable->stop();
    threadPool.waitForDone();
    if (parentRouter != NULL) {
        delete parentRouter;
    }
    delete messagingStubFactory;
    delete messageQueue;
    delete runningParentResolves;
}

MessageRouter::MessageRouter(IMessagingStubFactory* messagingStubFactory,
                             int maxThreads,
                             MessageQueue* messageQueue)
        : joynr::system::RoutingProvider(joynr::types::ProviderQos(
                  QList<joynr::types::CustomParameter>(), // custom provider parameters
                  1,                                      // provider version
                  1,                                      // provider priority
                  joynr::types::ProviderScope::LOCAL,     // provider discovery scope
                  false                                   // supports on change subscriptions
                  )),
          messagingStubFactory(messagingStubFactory),
          routingTable(QString("MessageRouter-RoutingTable")),
          routingTableMutex(),
          threadPool(),
          parentRouter(NULL),
          parentAddress(NULL),
          incomingAddress(),
          messageQueue(messageQueue),
          messageQueueCleanerRunnable(new MessageQueueCleanerRunnable(*messageQueue)),
          runningParentResolves(new QSet<QString>()),
          parentResolveMutex()
{
    init(maxThreads);
}

MessageRouter::MessageRouter(IMessagingStubFactory* messagingStubFactory,
                             QSharedPointer<joynr::system::Address> incomingAddress,
                             int maxThreads,
                             MessageQueue* messageQueue)
        : joynr::system::RoutingProvider(joynr::types::ProviderQos(
                  QList<joynr::types::CustomParameter>(), // custom provider parameters
                  1,                                      // provider version
                  1,                                      // provider priority
                  joynr::types::ProviderScope::LOCAL,     // provider discovery scope
                  false                                   // supports on change subscriptions
                  )),
          messagingStubFactory(messagingStubFactory),
          routingTable(QString("MessageRouter-RoutingTable")),
          routingTableMutex(),
          threadPool(),
          parentRouter(NULL),
          parentAddress(NULL),
          incomingAddress(incomingAddress),
          messageQueue(messageQueue),
          messageQueueCleanerRunnable(new MessageQueueCleanerRunnable(*messageQueue)),
          runningParentResolves(new QSet<QString>()),
          parentResolveMutex()
{
    init(maxThreads);
}

void MessageRouter::init(int maxThreads)
{
    threadPool.setMaxThreadCount(maxThreads);
    threadPool.start(messageQueueCleanerRunnable);
}

void MessageRouter::addProvisionedNextHop(QString participantId,
                                          QSharedPointer<joynr::system::Address> address)
{
    addToRoutingTable(participantId, address);
}

void MessageRouter::setParentRouter(joynr::system::RoutingProxy* parentRouter,
                                    QSharedPointer<joynr::system::Address> parentAddress,
                                    QString parentParticipantId)
{
    this->parentRouter = parentRouter;
    this->parentAddress = parentAddress;

    // add the next hop to parent router
    // this is necessary because during normal registration, the parent proxy is not yet set
    joynr::RequestStatus status;
    addProvisionedNextHop(parentParticipantId, parentAddress);
    addNextHopToParent(status, parentRouter->getProxyParticipantId());
}

bool MessageRouter::isChildMessageRouter()
{
    if (incomingAddress.isNull()) {
        return false;
    }
    // if an incoming address is set, a parent message router is needed for correct configuration
    return parentRouter != NULL && !parentAddress.isNull();
}

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void MessageRouter::route(const JoynrMessage& message)
{
    assert(messagingStubFactory != NULL);
    if (QDateTime::currentDateTimeUtc() > message.getHeaderExpiryDate()) {
        LOG_WARN(logger,
                 QString("Received expired message. Dropping the message (ID: %1).")
                         .arg(message.getHeaderMessageId()));
        return;
    }

    // search for the destination address
    const QString destinationPartId = message.getHeaderTo();
    QSharedPointer<joynr::system::Address> destAddress(NULL);
    {
        QMutexLocker locker(&routingTableMutex);
        destAddress = routingTable.lookup(destinationPartId);
    }

    // schedule message for sending
    if (!destAddress.isNull()) {
        sendMessage(message, destAddress);
        return;
    }

    // save message for later delivery
    messageQueue->queueMessage(message);

    // try to resolve destination address via parent message router
    if (isChildMessageRouter()) {
        QMutexLocker locker(&parentResolveMutex);
        if (!runningParentResolves->contains(destinationPartId)) {
            runningParentResolves->insert(destinationPartId);
            std::function<void(const joynr::RequestStatus&, const bool&)> callbackFct =
                    [this, destinationPartId](
                            const joynr::RequestStatus& status, const bool& resolved) {
                if (status.successful() && resolved) {
                    LOG_INFO(this->logger,
                             "Got destination address for participant " + destinationPartId);
                    // save next hop in the routing table
                    this->addProvisionedNextHop(destinationPartId, this->parentAddress);
                    this->removeRunningParentResolvers(destinationPartId);
                    this->sendMessages(destinationPartId, this->parentAddress);
                } else {
                    LOG_ERROR(this->logger,
                              "Failed to resolve next hop for participant " + destinationPartId +
                                      ": " + status.toString());
                    // TODO error handling in case of failing submission (?)
                }
            };

            parentRouter->resolveNextHop(destinationPartId, callbackFct);
        }
    }
}

void MessageRouter::removeRunningParentResolvers(const QString& destinationPartId)
{
    QMutexLocker locker(&parentResolveMutex);
    runningParentResolves->remove(destinationPartId);
}

void MessageRouter::sendMessages(const QString& destinationPartId,
                                 QSharedPointer<joynr::system::Address> address)
{
    while (true) {
        MessageQueueItem* item = messageQueue->getNextMessageForParticipant(destinationPartId);
        if (!item) {
            break;
        }
        sendMessage(item->getContent(), address);
        delete item;
    }
}

void MessageRouter::sendMessage(const JoynrMessage& message,
                                QSharedPointer<joynr::system::Address> destAddress)
{
    auto stub = messagingStubFactory->create(message.getHeaderTo(), *destAddress);
    if (!stub.isNull()) {
        threadPool.start(new MessageRunnable(message, stub));
    }
}

void MessageRouter::addNextHop(QString participantId,
                               QSharedPointer<joynr::system::Address> inprocessAddress)
{
    addToRoutingTable(participantId, inprocessAddress);

    joynr::RequestStatus status;
    addNextHopToParent(status, participantId);

    sendMessages(participantId, inprocessAddress);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(joynr::RequestStatus& joynrInternalStatus,
                               QString participantId,
                               joynr::system::ChannelAddress channelAddress)
{
    QSharedPointer<joynr::system::ChannelAddress> address(
            new joynr::system::ChannelAddress(channelAddress));
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(joynr::RequestStatus& joynrInternalStatus,
                               QString participantId,
                               joynr::system::CommonApiDbusAddress commonApiDbusAddress)
{
    QSharedPointer<joynr::system::CommonApiDbusAddress> address(
            new joynr::system::CommonApiDbusAddress(commonApiDbusAddress));
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(joynr::RequestStatus& joynrInternalStatus,
                               QString participantId,
                               joynr::system::BrowserAddress browserAddress)
{
    QSharedPointer<joynr::system::BrowserAddress> address(
            new joynr::system::BrowserAddress(browserAddress));
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(joynr::RequestStatus& joynrInternalStatus,
                               QString participantId,
                               joynr::system::WebSocketAddress webSocketAddress)
{
    QSharedPointer<joynr::system::WebSocketAddress> address(
            new joynr::system::WebSocketAddress(webSocketAddress));
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(joynr::RequestStatus& joynrInternalStatus,
                               QString participantId,
                               joynr::system::WebSocketClientAddress webSocketClientAddress)
{
    QSharedPointer<joynr::system::WebSocketClientAddress> address(
            new joynr::system::WebSocketClientAddress(webSocketClientAddress));
    addToRoutingTable(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    addNextHopToParent(joynrInternalStatus, participantId);

    sendMessages(participantId, address);
}

void MessageRouter::addNextHopToParent(joynr::RequestStatus& joynrInternalStatus,
                                       QString participantId)
{
    // add to parent router
    if (isChildMessageRouter()) {
        if (incomingAddress->inherits("joynr::system::ChannelAddress")) {
            parentRouter->addNextHop(
                    joynrInternalStatus,
                    participantId,
                    *dynamic_cast<joynr::system::ChannelAddress*>(incomingAddress.data()));
        }
        if (incomingAddress->inherits("joynr::system::CommonApiDbusAddress")) {
            parentRouter->addNextHop(
                    joynrInternalStatus,
                    participantId,
                    *dynamic_cast<joynr::system::CommonApiDbusAddress*>(incomingAddress.data()));
        }
        if (incomingAddress->inherits("joynr::system::BrowserAddress")) {
            parentRouter->addNextHop(
                    joynrInternalStatus,
                    participantId,
                    *dynamic_cast<joynr::system::BrowserAddress*>(incomingAddress.data()));
        }
        if (incomingAddress->inherits("joynr::system::WebSocketAddress")) {
            parentRouter->addNextHop(
                    joynrInternalStatus,
                    participantId,
                    *dynamic_cast<joynr::system::WebSocketAddress*>(incomingAddress.data()));
        }
        if (incomingAddress->inherits("joynr::system::WebSocketClientAddress")) {
            parentRouter->addNextHop(
                    joynrInternalStatus,
                    participantId,
                    *dynamic_cast<joynr::system::WebSocketClientAddress*>(incomingAddress.data()));
        }
    }
}

void MessageRouter::addToRoutingTable(QString participantId,
                                      QSharedPointer<joynr::system::Address> address)
{
    QMutexLocker locker(&routingTableMutex);
    routingTable.add(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::removeNextHop(joynr::RequestStatus& joynrInternalStatus, QString participantId)
{
    {
        QMutexLocker locker(&routingTableMutex);
        routingTable.remove(participantId);
    }
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);

    // remove from parent router
    if (isChildMessageRouter()) {
        parentRouter->removeNextHop(joynrInternalStatus, participantId);
    }
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::resolveNextHop(joynr::RequestStatus& joynrInternalStatus,
                                   bool& resolved,
                                   QString participantId)
{
    {
        QMutexLocker locker(&routingTableMutex);
        resolved = routingTable.contains(participantId);
    }
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}

/**
 * IMPLEMENTATION of MessageRunnable class
 */

Logger* MessageRunnable::logger = Logging::getInstance()->getLogger("MSG", "MessageRunnable");

MessageRunnable::MessageRunnable(const JoynrMessage& message,
                                 QSharedPointer<IMessaging> messagingStub)
        : ObjectWithDecayTime(message.getHeaderExpiryDate()),
          message(message),
          messagingStub(messagingStub)
{
}

void MessageRunnable::run()
{
    if (!isExpired()) {
        messagingStub->transmit(message);
    } else {
        LOG_ERROR(
                logger,
                QString("Message with ID %1 expired: dropping!").arg(message.getHeaderMessageId()));
    }
}

} // namespace joynr

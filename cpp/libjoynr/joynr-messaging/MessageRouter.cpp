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
#include "cluster-controller/access-control/IAccessController.h"
#include "joynr/IPlatformSecurityManager.h"

#include <QMutexLocker>

#include <cassert>

namespace joynr
{

using namespace joynr_logging;
Logger* MessageRouter::logger = Logging::getInstance()->getLogger("MSG", "MessageRouter");

//------ ConsumerPermissionCallback --------------------------------------------

class ConsumerPermissionCallback : public IAccessController::IHasConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback(MessageRouter& owningMessageRouter,
                               const JoynrMessage& message,
                               QSharedPointer<system::Address> destination);

    void hasConsumerPermission(bool hasPermission);

private:
    MessageRouter& owningMessageRouter;
    JoynrMessage message;
    QSharedPointer<system::Address> destination;
};

//------ MessageRouter ---------------------------------------------------------

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
    delete securityManager;
}

MessageRouter::MessageRouter(IMessagingStubFactory* messagingStubFactory,
                             IPlatformSecurityManager* securityManager,
                             int maxThreads,
                             MessageQueue* messageQueue)
        : joynr::system::RoutingProvider(),
          messagingStubFactory(messagingStubFactory),
          routingTable("MessageRouter-RoutingTable"),
          routingTableLock(),
          threadPool(),
          parentRouter(NULL),
          parentAddress(NULL),
          incomingAddress(),
          messageQueue(messageQueue),
          messageQueueCleanerRunnable(new MessageQueueCleanerRunnable(*messageQueue)),
          runningParentResolves(new QSet<QString>()),
          accessController(NULL),
          securityManager(securityManager),
          parentResolveMutex()
{
    providerQos.setCustomParameters(QList<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
    init(maxThreads);
}

MessageRouter::MessageRouter(IMessagingStubFactory* messagingStubFactory,
                             QSharedPointer<joynr::system::Address> incomingAddress,
                             int maxThreads,
                             MessageQueue* messageQueue)
        : joynr::system::RoutingProvider(),
          messagingStubFactory(messagingStubFactory),
          routingTable("MessageRouter-RoutingTable"),
          routingTableLock(),
          threadPool(),
          parentRouter(NULL),
          parentAddress(NULL),
          incomingAddress(incomingAddress),
          messageQueue(messageQueue),
          messageQueueCleanerRunnable(new MessageQueueCleanerRunnable(*messageQueue)),
          runningParentResolves(new QSet<QString>()),
          accessController(NULL),
          securityManager(NULL),
          parentResolveMutex()
{
    providerQos.setCustomParameters(QList<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
    init(maxThreads);
}

void MessageRouter::init(int maxThreads)
{
    threadPool.setMaxThreadCount(maxThreads);
    threadPool.start(messageQueueCleanerRunnable);
}

void MessageRouter::addProvisionedNextHop(std::string participantId,
                                          QSharedPointer<joynr::system::Address> address)
{
    addToRoutingTable(participantId, address);
}

void MessageRouter::setAccessController(QSharedPointer<IAccessController> accessController)
{
    this->accessController = accessController;
}

void MessageRouter::setParentRouter(joynr::system::RoutingProxy* parentRouter,
                                    QSharedPointer<joynr::system::Address> parentAddress,
                                    std::string parentParticipantId)
{
    this->parentRouter = parentRouter;
    this->parentAddress = parentAddress;

    // add the next hop to parent router
    // this is necessary because during normal registration, the parent proxy is not yet set
    addProvisionedNextHop(parentParticipantId, parentAddress);
    addNextHopToParent(parentRouter->getProxyParticipantId());
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
    LOG_DEBUG(logger,
              QString("Route message with Id %1 and payload %2")
                      .arg(message.getHeaderMessageId())
                      .arg(QString(message.getPayload())));
    if (QDateTime::currentDateTimeUtc() > message.getHeaderExpiryDate()) {
        LOG_WARN(logger,
                 QString("Received expired message. Dropping the message (ID: %1).")
                         .arg(message.getHeaderMessageId()));
        return;
    }

    // Validate the message if possible
    if (securityManager != NULL && !securityManager->validate(message)) {
        LOG_ERROR(logger,
                  QString("messageId %1 failed validation").arg(message.getHeaderMessageId()));
        return;
    }

    // search for the destination address
    const QString destinationPartId = message.getHeaderTo();
    QSharedPointer<joynr::system::Address> destAddress(NULL);

    routingTableLock.lockForRead();
    destAddress = routingTable.lookup(destinationPartId.toStdString());
    routingTableLock.unlock();
    // if destination address is not known
    if (destAddress.isNull()) {
        // save the message for later delivery
        messageQueue->queueMessage(message);
        LOG_DEBUG(logger, QString("message queued: %1").arg(QString(message.getPayload())));

        // and try to resolve destination address via parent message router
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
                        this->addProvisionedNextHop(
                                destinationPartId.toStdString(), this->parentAddress);
                        this->removeRunningParentResolvers(destinationPartId);
                        this->sendMessages(destinationPartId.toStdString(), this->parentAddress);
                    } else {
                        LOG_ERROR(this->logger,
                                  "Failed to resolve next hop for participant " +
                                          destinationPartId + ": " + status.toString());
                        // TODO error handling in case of failing submission (?)
                    }
                };

                parentRouter->resolveNextHop(destinationPartId.toStdString(), callbackFct);
            }
        } else {
            // no parent message router to resolve destination address
            LOG_WARN(logger,
                     QString("No routing information found for destination participant ID \"%1\" "
                             "so far. Waiting for participant registration. "
                             "Queueing message (ID : %2)")
                             .arg(message.getHeaderTo())
                             .arg(message.getHeaderMessageId()));
        }
        return;
    }

    if (!accessController.isNull()) {
        // Access control checks are asynchronous, callback will send message
        // if access is granted
        QSharedPointer<IAccessController::IHasConsumerPermissionCallback> callback(
                new ConsumerPermissionCallback(*this, message, destAddress));
        accessController->hasConsumerPermission(message, callback);
        return;
    }

    // If this point is reached, the message can be sent without delay
    sendMessage(message, destAddress);
}

void MessageRouter::removeRunningParentResolvers(const QString& destinationPartId)
{
    QMutexLocker locker(&parentResolveMutex);
    runningParentResolves->remove(destinationPartId);
}

void MessageRouter::sendMessages(const std::string& destinationPartId,
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
    auto stub = messagingStubFactory->create(message.getHeaderTo().toStdString(), *destAddress);
    if (!stub.isNull()) {
        threadPool.start(new MessageRunnable(message, stub));
    }
}

void MessageRouter::addNextHop(
        const std::string& participantId,
        const QSharedPointer<joynr::system::Address>& inprocessAddress,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus)> callbackFct)
{
    addToRoutingTable(participantId, inprocessAddress);

    addNextHopToParent(participantId, callbackFct);

    sendMessages(participantId, inprocessAddress);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const joynr::system::ChannelAddress& channelAddress,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus)> callbackFct)
{
    QSharedPointer<joynr::system::ChannelAddress> address(
            new joynr::system::ChannelAddress(channelAddress));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, callbackFct);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const joynr::system::CommonApiDbusAddress& commonApiDbusAddress,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus)> callbackFct)
{
    QSharedPointer<joynr::system::CommonApiDbusAddress> address(
            new joynr::system::CommonApiDbusAddress(commonApiDbusAddress));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, callbackFct);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const joynr::system::BrowserAddress& browserAddress,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus)> callbackFct)
{
    QSharedPointer<joynr::system::BrowserAddress> address(
            new joynr::system::BrowserAddress(browserAddress));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, callbackFct);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const joynr::system::WebSocketAddress& webSocketAddress,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus)> callbackFct)
{
    QSharedPointer<joynr::system::WebSocketAddress> address(
            new joynr::system::WebSocketAddress(webSocketAddress));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, callbackFct);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const joynr::system::WebSocketClientAddress& webSocketClientAddress,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus)> callbackFct)
{
    QSharedPointer<joynr::system::WebSocketClientAddress> address(
            new joynr::system::WebSocketClientAddress(webSocketClientAddress));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, callbackFct);

    sendMessages(participantId, address);
}

void MessageRouter::addNextHopToParent(
        std::string participantId,
        std::function<void(const joynr::RequestStatus& status)> callbackFct)
{
    // add to parent router
    if (isChildMessageRouter()) {
        if (incomingAddress->inherits("joynr::system::ChannelAddress")) {
            parentRouter->addNextHop(
                    participantId,
                    *dynamic_cast<joynr::system::ChannelAddress*>(incomingAddress.data()),
                    callbackFct);
        }
        if (incomingAddress->inherits("joynr::system::CommonApiDbusAddress")) {
            parentRouter->addNextHop(
                    participantId,
                    *dynamic_cast<joynr::system::CommonApiDbusAddress*>(incomingAddress.data()),
                    callbackFct);
        }
        if (incomingAddress->inherits("joynr::system::BrowserAddress")) {
            parentRouter->addNextHop(
                    participantId,
                    *dynamic_cast<joynr::system::BrowserAddress*>(incomingAddress.data()),
                    callbackFct);
        }
        if (incomingAddress->inherits("joynr::system::WebSocketAddress")) {
            parentRouter->addNextHop(
                    participantId,
                    *dynamic_cast<joynr::system::WebSocketAddress*>(incomingAddress.data()),
                    callbackFct);
        }
        if (incomingAddress->inherits("joynr::system::WebSocketClientAddress")) {
            parentRouter->addNextHop(
                    participantId,
                    *dynamic_cast<joynr::system::WebSocketClientAddress*>(incomingAddress.data()),
                    callbackFct);
        }
    } else if (callbackFct) {
        callbackFct(joynr::RequestStatus(RequestStatusCode::OK));
    }
}

void MessageRouter::addToRoutingTable(std::string participantId,
                                      QSharedPointer<joynr::system::Address> address)
{
    routingTableLock.lockForWrite();
    routingTable.add(participantId, address);
    routingTableLock.unlock();
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::removeNextHop(
        const std::string& participantId,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus)> callbackFct)
{
    routingTableLock.lockForWrite();
    routingTable.remove(participantId);
    routingTableLock.unlock();

    // remove from parent router
    if (isChildMessageRouter()) {
        parentRouter->removeNextHop(participantId, callbackFct);
    } else if (callbackFct) {
        callbackFct(joynr::RequestStatus(RequestStatusCode::OK));
    }
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::resolveNextHop(
        const std::string& participantId,
        std::function<void(const joynr::RequestStatus& joynrInternalStatus, const bool& resolved)>
                callbackFct)
{
    routingTableLock.lockForRead();
    bool resolved = routingTable.contains(participantId);
    routingTableLock.unlock();
    callbackFct(joynr::RequestStatus(joynr::RequestStatusCode::OK), resolved);
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

/**
 * IMPLEMENTATION of ConsumerPermissionCallback class
 */

ConsumerPermissionCallback::ConsumerPermissionCallback(MessageRouter& owningMessageRouter,
                                                       const JoynrMessage& message,
                                                       QSharedPointer<system::Address> destination)
        : owningMessageRouter(owningMessageRouter), message(message), destination(destination)
{
}

void ConsumerPermissionCallback::hasConsumerPermission(bool hasPermission)
{
    if (hasPermission) {
        owningMessageRouter.sendMessage(message, destination);
    }
}

} // namespace joynr

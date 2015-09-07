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
#include "joynr/system/routingtypes/QtAddress.h"
#include "joynr/types/QtProviderQos.h"
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
                               QSharedPointer<system::routingtypes::QtAddress> destination);

    void hasConsumerPermission(bool hasPermission);

private:
    MessageRouter& owningMessageRouter;
    JoynrMessage message;
    QSharedPointer<system::routingtypes::QtAddress> destination;
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
        : joynr::system::RoutingAbstractProvider(),
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
    providerQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
    init(maxThreads);
}

MessageRouter::MessageRouter(IMessagingStubFactory* messagingStubFactory,
                             QSharedPointer<joynr::system::routingtypes::QtAddress> incomingAddress,
                             int maxThreads,
                             MessageQueue* messageQueue)
        : joynr::system::RoutingAbstractProvider(),
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
    providerQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
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

void MessageRouter::addProvisionedNextHop(
        std::string participantId,
        QSharedPointer<joynr::system::routingtypes::QtAddress> address)
{
    addToRoutingTable(participantId, address);
}

void MessageRouter::setAccessController(QSharedPointer<IAccessController> accessController)
{
    this->accessController = accessController;
}

void MessageRouter::setParentRouter(
        joynr::system::RoutingProxy* parentRouter,
        QSharedPointer<joynr::system::routingtypes::QtAddress> parentAddress,
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
    QSharedPointer<joynr::system::routingtypes::QtAddress> destAddress(NULL);

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
                std::function<void(const bool&)> onSuccess =
                        [this, destinationPartId](const bool& resolved) {
                    if (resolved) {
                        LOG_INFO(this->logger,
                                 "Got destination address for participant " + destinationPartId);
                        // save next hop in the routing table
                        this->addProvisionedNextHop(
                                destinationPartId.toStdString(), this->parentAddress);
                        this->removeRunningParentResolvers(destinationPartId);
                        this->sendMessages(destinationPartId.toStdString(), this->parentAddress);
                    } else {
                        LOG_ERROR(
                                this->logger,
                                "Failed to resolve next hop for participant " + destinationPartId);
                        // TODO error handling in case of failing submission (?)
                    }
                };

                // TODO error handling in case of failing submission (?)
                parentRouter->resolveNextHopAsync(destinationPartId.toStdString(), onSuccess);
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
                                 QSharedPointer<joynr::system::routingtypes::QtAddress> address)
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
                                QSharedPointer<joynr::system::routingtypes::QtAddress> destAddress)
{
    auto stub = messagingStubFactory->create(message.getHeaderTo().toStdString(), *destAddress);
    if (!stub.isNull()) {
        threadPool.start(new MessageRunnable(message, stub));
    }
}

void MessageRouter::addNextHop(
        const std::string& participantId,
        const QSharedPointer<joynr::system::routingtypes::QtAddress>& inprocessAddress,
        std::function<void()> onSuccess)
{
    addToRoutingTable(participantId, inprocessAddress);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, inprocessAddress);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(const std::string& participantId,
                               const system::routingtypes::ChannelAddress& channelAddress,
                               std::function<void()> onSuccess)
{
    QSharedPointer<joynr::system::routingtypes::QtChannelAddress> address(
            new joynr::system::routingtypes::QtChannelAddress(
                    joynr::system::routingtypes::QtChannelAddress::createQt(channelAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::routingtypes::CommonApiDbusAddress& commonApiDbusAddress,
        std::function<void()> onSuccess)
{
    QSharedPointer<joynr::system::routingtypes::QtCommonApiDbusAddress> address(
            new joynr::system::routingtypes::QtCommonApiDbusAddress(
                    joynr::system::routingtypes::QtCommonApiDbusAddress::createQt(
                            commonApiDbusAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(const std::string& participantId,
                               const system::routingtypes::BrowserAddress& browserAddress,
                               std::function<void()> onSuccess)
{
    QSharedPointer<joynr::system::routingtypes::QtBrowserAddress> address(
            new joynr::system::routingtypes::QtBrowserAddress(
                    joynr::system::routingtypes::QtBrowserAddress::createQt(browserAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(const std::string& participantId,
                               const system::routingtypes::WebSocketAddress& webSocketAddress,
                               std::function<void()> onSuccess)
{
    QSharedPointer<joynr::system::routingtypes::QtWebSocketAddress> address(
            new joynr::system::routingtypes::QtWebSocketAddress(
                    joynr::system::routingtypes::QtWebSocketAddress::createQt(webSocketAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::routingtypes::WebSocketClientAddress& webSocketClientAddress,
        std::function<void()> onSuccess)
{
    QSharedPointer<joynr::system::routingtypes::QtWebSocketClientAddress> address(
            new joynr::system::routingtypes::QtWebSocketClientAddress(
                    joynr::system::routingtypes::QtWebSocketClientAddress::createQt(
                            webSocketClientAddress)));
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, address);
}

void MessageRouter::addNextHopToParent(std::string participantId,
                                       std::function<void(void)> onSuccess)
{
    // add to parent router
    if (isChildMessageRouter()) {
        if (incomingAddress->inherits("joynr::system::routingtypes::QtChannelAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::routingtypes::QtChannelAddress::createStd(
                            *dynamic_cast<joynr::system::routingtypes::QtChannelAddress*>(
                                    incomingAddress.data())),
                    onSuccess);
        }
        if (incomingAddress->inherits("joynr::system::routingtypes::QtCommonApiDbusAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::routingtypes::QtCommonApiDbusAddress::createStd(
                            *dynamic_cast<joynr::system::routingtypes::QtCommonApiDbusAddress*>(
                                    incomingAddress.data())),
                    onSuccess);
        }
        if (incomingAddress->inherits("joynr::system::routingtypes::QtBrowserAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::routingtypes::QtBrowserAddress::createStd(
                            *dynamic_cast<joynr::system::routingtypes::QtBrowserAddress*>(
                                    incomingAddress.data())),
                    onSuccess);
        }
        if (incomingAddress->inherits("joynr::system::routingtypes::QtWebSocketAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::routingtypes::QtWebSocketAddress::createStd(
                            *dynamic_cast<joynr::system::routingtypes::QtWebSocketAddress*>(
                                    incomingAddress.data())),
                    onSuccess);
        }
        if (incomingAddress->inherits("joynr::system::routingtypes::QtWebSocketClientAddress")) {
            parentRouter->addNextHopAsync(
                    participantId,
                    joynr::system::routingtypes::QtWebSocketClientAddress::createStd(
                            *dynamic_cast<joynr::system::routingtypes::QtWebSocketClientAddress*>(
                                    incomingAddress.data())),
                    onSuccess);
        }
    } else if (onSuccess) {
        onSuccess();
    }
}

void MessageRouter::addToRoutingTable(
        std::string participantId,
        QSharedPointer<joynr::system::routingtypes::QtAddress> address)
{
    routingTableLock.lockForWrite();
    routingTable.add(participantId, address);
    routingTableLock.unlock();
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::removeNextHop(const std::string& participantId, std::function<void()> onSuccess)
{
    routingTableLock.lockForWrite();
    routingTable.remove(participantId);
    routingTableLock.unlock();

    // remove from parent router
    if (isChildMessageRouter()) {
        parentRouter->removeNextHopAsync(participantId, onSuccess);
    } else if (onSuccess) {
        onSuccess();
    }
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::resolveNextHop(const std::string& participantId,
                                   std::function<void(const bool& resolved)> onSuccess)
{
    routingTableLock.lockForRead();
    bool resolved = routingTable.contains(participantId);
    routingTableLock.unlock();
    onSuccess(resolved);
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

ConsumerPermissionCallback::ConsumerPermissionCallback(
        MessageRouter& owningMessageRouter,
        const JoynrMessage& message,
        QSharedPointer<system::routingtypes::QtAddress> destination)
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

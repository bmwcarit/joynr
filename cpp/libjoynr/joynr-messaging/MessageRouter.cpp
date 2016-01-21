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
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/RequestStatusCode.h"
#include "joynr/JsonSerializer.h"
#include "cluster-controller/access-control/IAccessController.h"
#include "joynr/IPlatformSecurityManager.h"

#include <chrono>

#include <cassert>

namespace joynr
{

INIT_LOGGER(MessageRouter);

//------ ConsumerPermissionCallback --------------------------------------------

class ConsumerPermissionCallback : public IAccessController::IHasConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback(MessageRouter& owningMessageRouter,
                               const JoynrMessage& message,
                               std::shared_ptr<system::RoutingTypes::Address> destination);

    void hasConsumerPermission(bool hasPermission) override;

    MessageRouter& owningMessageRouter;
    JoynrMessage message;
    std::shared_ptr<system::RoutingTypes::Address> destination;
};

//------ MessageRouter ---------------------------------------------------------

MessageRouter::~MessageRouter()
{
    messageQueueCleanerTimer.shutdown();
    threadPool.shutdown();
    if (parentRouter != nullptr) {
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
          threadPool("MessageRouter", maxThreads),
          parentRouter(nullptr),
          parentAddress(nullptr),
          incomingAddress(),
          messageQueue(messageQueue),
          messageQueueCleanerTimer(),
          runningParentResolves(new std::unordered_set<std::string>()),
          accessController(nullptr),
          securityManager(securityManager),
          parentResolveMutex()
{
    messageQueueCleanerTimer.addTimer(
            [this](Timer::TimerId) { this->messageQueue->removeOutdatedMessages(); },
            [](Timer::TimerId) {},
            1000,
            true);

    providerQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
}

MessageRouter::MessageRouter(IMessagingStubFactory* messagingStubFactory,
                             std::shared_ptr<joynr::system::RoutingTypes::Address> incomingAddress,
                             int maxThreads,
                             MessageQueue* messageQueue)
        : joynr::system::RoutingAbstractProvider(),
          messagingStubFactory(messagingStubFactory),
          routingTable("MessageRouter-RoutingTable"),
          routingTableLock(),
          threadPool("MessageRouter", maxThreads),
          parentRouter(nullptr),
          parentAddress(nullptr),
          incomingAddress(incomingAddress),
          messageQueue(messageQueue),
          messageQueueCleanerTimer(),
          runningParentResolves(new std::unordered_set<std::string>()),
          accessController(nullptr),
          securityManager(nullptr),
          parentResolveMutex()
{
    messageQueueCleanerTimer.addTimer(
            [this](Timer::TimerId) { this->messageQueue->removeOutdatedMessages(); },
            [](Timer::TimerId) {},
            1000,
            true);

    providerQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    providerQos.setProviderVersion(1);
    providerQos.setPriority(1);
    providerQos.setScope(joynr::types::ProviderScope::LOCAL);
    providerQos.setSupportsOnChangeSubscriptions(false);
}

void MessageRouter::addProvisionedNextHop(
        std::string participantId,
        std::shared_ptr<joynr::system::RoutingTypes::Address> address)
{
    addToRoutingTable(participantId, address);
}

void MessageRouter::setAccessController(std::shared_ptr<IAccessController> accessController)
{
    assert(accessController);
    this->accessController = accessController;
}

void MessageRouter::setParentRouter(
        joynr::system::RoutingProxy* parentRouter,
        std::shared_ptr<joynr::system::RoutingTypes::Address> parentAddress,
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
    if (!incomingAddress) {
        return false;
    }
    // if an incoming address is set, a parent message router is needed for correct configuration
    return parentRouter != nullptr && parentAddress;
}

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void MessageRouter::route(const JoynrMessage& message)
{
    assert(messagingStubFactory != nullptr);
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    if (now > message.getHeaderExpiryDate()) {
        JOYNR_LOG_WARN(logger,
                       "Received expired message. Dropping the message (ID: {}).",
                       message.getHeaderMessageId());
        return;
    }

    // Validate the message if possible
    if (securityManager != nullptr && !securityManager->validate(message)) {
        JOYNR_LOG_ERROR(logger, "messageId {} failed validation", message.getHeaderMessageId());
        return;
    }

    JOYNR_LOG_DEBUG(logger,
                    "Route message with Id {} and payload {}",
                    message.getHeaderMessageId(),
                    message.getPayload());
    // search for the destination address
    const std::string destinationPartId = message.getHeaderTo();
    std::shared_ptr<joynr::system::RoutingTypes::Address> destAddress(nullptr);

    {
        ReadLocker lock(routingTableLock);
        destAddress = routingTable.lookup(destinationPartId);
    }
    // if destination address is not known
    if (!destAddress) {
        // save the message for later delivery
        messageQueue->queueMessage(message);
        JOYNR_LOG_DEBUG(logger, "message queued: {}", message.getPayload());

        // and try to resolve destination address via parent message router
        if (isChildMessageRouter()) {
            std::lock_guard<std::mutex> lock(parentResolveMutex);
            if (runningParentResolves->find(destinationPartId) == runningParentResolves->end()) {
                runningParentResolves->insert(destinationPartId);
                std::function<void(const bool&)> onSuccess =
                        [this, destinationPartId](const bool& resolved) {
                    if (resolved) {
                        JOYNR_LOG_INFO(logger,
                                       "Got destination address for participant {}",
                                       destinationPartId);
                        // save next hop in the routing table
                        this->addProvisionedNextHop(destinationPartId, this->parentAddress);
                        this->removeRunningParentResolvers(destinationPartId);
                        this->sendMessages(destinationPartId, this->parentAddress);
                    } else {
                        JOYNR_LOG_ERROR(logger,
                                        "Failed to resolve next hop for participant {}",
                                        destinationPartId);
                        // TODO error handling in case of failing submission (?)
                    }
                };

                // TODO error handling in case of failing submission (?)
                parentRouter->resolveNextHopAsync(destinationPartId, onSuccess);
            }
        } else {
            // no parent message router to resolve destination address
            JOYNR_LOG_WARN(logger,
                           "No routing information found for destination participant ID \"{}\" "
                           "so far. Waiting for participant registration. "
                           "Queueing message (ID : {})",
                           message.getHeaderTo(),
                           message.getHeaderMessageId());
        }
        return;
    }

    if (accessController) {
        // Access control checks are asynchronous, callback will send message
        // if access is granted
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback(
                new ConsumerPermissionCallback(*this, message, destAddress));
        accessController->hasConsumerPermission(message, callback);
        return;
    }

    // If this point is reached, the message can be sent without delay
    sendMessage(message, destAddress);
}

void MessageRouter::removeRunningParentResolvers(const std::string& destinationPartId)
{
    std::lock_guard<std::mutex> lock(parentResolveMutex);
    if (runningParentResolves->find(destinationPartId) != runningParentResolves->end()) {
        runningParentResolves->erase(destinationPartId);
    }
}

void MessageRouter::sendMessages(const std::string& destinationPartId,
                                 std::shared_ptr<joynr::system::RoutingTypes::Address> address)
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
                                std::shared_ptr<joynr::system::RoutingTypes::Address> destAddress)
{
    auto stub = messagingStubFactory->create(*destAddress);
    if (stub) {
        threadPool.execute(new MessageRunnable(message, stub));
    } else {
        JOYNR_LOG_WARN(logger,
                       "Messag with payload {}  could not be send to {}. Stub creation failed",
                       message.getPayload(),
                       (*destAddress).toString());
    }
}

void MessageRouter::addNextHop(
        const std::string& participantId,
        const std::shared_ptr<joynr::system::RoutingTypes::Address>& inprocessAddress,
        std::function<void()> onSuccess)
{
    addToRoutingTable(participantId, inprocessAddress);

    addNextHopToParent(participantId, onSuccess);

    sendMessages(participantId, inprocessAddress);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::ChannelAddress& channelAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address = std::make_shared<joynr::system::RoutingTypes::ChannelAddress>(channelAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::CommonApiDbusAddress& commonApiDbusAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address = std::make_shared<joynr::system::RoutingTypes::CommonApiDbusAddress>(
            commonApiDbusAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::BrowserAddress& browserAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address = std::make_shared<joynr::system::RoutingTypes::BrowserAddress>(browserAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketAddress& webSocketAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address =
            std::make_shared<joynr::system::RoutingTypes::WebSocketAddress>(webSocketAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address = std::make_shared<joynr::system::RoutingTypes::WebSocketClientAddress>(
            webSocketClientAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, onSuccess, onError);

    sendMessages(participantId, address);
}

void MessageRouter::addNextHopToParent(
        std::string participantId,
        std::function<void(void)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError](const exceptions::JoynrException& error) {
        if (onError) {
            onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
        } else {
            JOYNR_LOG_WARN(logger,
                           "Unable to report error (received by calling "
                           "parentRouter->addNextHopAsync), since onError function is "
                           "empty. Error message: {}",
                           error.getMessage());
        }
    };

    // add to parent router
    if (isChildMessageRouter()) {
        if (auto channelAddress =
                    std::dynamic_pointer_cast<joynr::system::RoutingTypes::ChannelAddress>(
                            incomingAddress)) {
            parentRouter->addNextHopAsync(
                    participantId, *channelAddress, onSuccess, onErrorWrapper);
        }
        if (auto commonApiDbusAddress =
                    std::dynamic_pointer_cast<joynr::system::RoutingTypes::CommonApiDbusAddress>(
                            incomingAddress)) {
            parentRouter->addNextHopAsync(
                    participantId, *commonApiDbusAddress, onSuccess, onErrorWrapper);
        }
        if (auto browserAddress =
                    std::dynamic_pointer_cast<joynr::system::RoutingTypes::BrowserAddress>(
                            incomingAddress)) {
            parentRouter->addNextHopAsync(
                    participantId, *browserAddress, onSuccess, onErrorWrapper);
        }
        if (auto webSocketAddress =
                    std::dynamic_pointer_cast<joynr::system::RoutingTypes::WebSocketAddress>(
                            incomingAddress)) {
            parentRouter->addNextHopAsync(
                    participantId, *webSocketAddress, onSuccess, onErrorWrapper);
        }
        if (auto webSocketClientAddress =
                    std::dynamic_pointer_cast<joynr::system::RoutingTypes::WebSocketClientAddress>(
                            incomingAddress)) {
            parentRouter->addNextHopAsync(
                    participantId, *webSocketClientAddress, onSuccess, onErrorWrapper);
        }
    } else if (onSuccess) {
        onSuccess();
    }
}

void MessageRouter::addToRoutingTable(std::string participantId,
                                      std::shared_ptr<joynr::system::RoutingTypes::Address> address)
{
    WriteLocker lock(routingTableLock);
    routingTable.add(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::removeNextHop(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    {
        WriteLocker lock(routingTableLock);
        routingTable.remove(participantId);
    }

    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError](const exceptions::JoynrException& error) {
        onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
    };

    // remove from parent router
    if (isChildMessageRouter()) {
        parentRouter->removeNextHopAsync(participantId, onSuccess, onErrorWrapper);
    } else if (onSuccess) {
        onSuccess();
    }
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::resolveNextHop(
        const std::string& participantId,
        std::function<void(const bool& resolved)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> /*onError*/)
{
    ReadLocker lock(routingTableLock);
    bool resolved = routingTable.contains(participantId);
    onSuccess(resolved);
}

/**
 * IMPLEMENTATION of MessageRunnable class
 */

INIT_LOGGER(MessageRunnable);

MessageRunnable::MessageRunnable(const JoynrMessage& message,
                                 std::shared_ptr<IMessaging> messagingStub)
        : Runnable(true),
          ObjectWithDecayTime(message.getHeaderExpiryDate()),
          message(message),
          messagingStub(messagingStub)
{
}

void MessageRunnable::shutdown()
{
}

void MessageRunnable::run()
{
    if (!isExpired()) {
        messagingStub->transmit(message);
    } else {
        JOYNR_LOG_ERROR(
                logger, "Message with ID {}  expired: dropping!", message.getHeaderMessageId());
    }
}

/**
 * IMPLEMENTATION of ConsumerPermissionCallback class
 */

ConsumerPermissionCallback::ConsumerPermissionCallback(
        MessageRouter& owningMessageRouter,
        const JoynrMessage& message,
        std::shared_ptr<system::RoutingTypes::Address> destination)
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

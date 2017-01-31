/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <cassert>
#include <functional>

#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/IMessaging.h"
#include "joynr/IMessagingMulticastSubscriber.h"
#include "joynr/IMessagingStubFactory.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/IPlatformSecurityManager.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/SteadyTimer.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "cluster-controller/access-control/IAccessController.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/Util.h"

namespace joynr
{

INIT_LOGGER(MessageRouter);

//------ ConsumerPermissionCallback --------------------------------------------

class ConsumerPermissionCallback : public IAccessController::IHasConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback(
            MessageRouter& owningMessageRouter,
            const JoynrMessage& message,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destination);

    void hasConsumerPermission(bool hasPermission) override;

    MessageRouter& owningMessageRouter;
    JoynrMessage message;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destination;

private:
    ADD_LOGGER(ConsumerPermissionCallback);
};

//------ MessageRouter ---------------------------------------------------------

MessageRouter::~MessageRouter()
{
    messageQueueCleanerTimer.cancel();
    messageScheduler.shutdown();
}

auto filterFun = [](std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress) {
    const InProcessMessagingAddress* inprocessAddress =
            dynamic_cast<const InProcessMessagingAddress*>(destAddress.get());
    return inprocessAddress == nullptr;
};

MessageRouter::MessageRouter(
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory,
        std::unique_ptr<IPlatformSecurityManager> securityManager,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        int maxThreads,
        std::unique_ptr<MessageQueue> messageQueue)
        : joynr::system::RoutingAbstractProvider(),
          messagingStubFactory(std::move(messagingStubFactory)),
          multicastMessagingSkeletonDirectory(multicastMessagingSkeletonDirectory),
          routingTable("MessageRouter-RoutingTable", ioService, filterFun),
          routingTableLock(),
          multicastReceiverDirectory(),
          messageScheduler(maxThreads, "MessageRouter", ioService),
          addressCalculator(std::move(addressCalculator)),
          parentRouter(nullptr),
          parentAddress(nullptr),
          incomingAddress(),
          messageQueue(std::move(messageQueue)),
          runningParentResolves(),
          accessController(nullptr),
          securityManager(std::move(securityManager)),
          parentResolveMutex(),
          routingTableFileName(),
          multicastReceveiverDirectoryFilename(),
          messageQueueCleanerTimer(ioService),
          messageQueueCleanerTimerPeriodMs(std::chrono::milliseconds(1000))
{
    activateMessageCleanerTimer();
}

MessageRouter::MessageRouter(
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> incomingAddress,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        int maxThreads,
        std::unique_ptr<MessageQueue> messageQueue)
        : joynr::system::RoutingAbstractProvider(),
          messagingStubFactory(std::move(messagingStubFactory)),
          multicastMessagingSkeletonDirectory(nullptr),
          routingTable("MessageRouter-RoutingTable", ioService, filterFun),
          routingTableLock(),
          multicastReceiverDirectory(),
          messageScheduler(maxThreads, "MessageRouter", ioService),
          addressCalculator(std::move(addressCalculator)),
          parentRouter(nullptr),
          parentAddress(nullptr),
          incomingAddress(incomingAddress),
          messageQueue(std::move(messageQueue)),
          runningParentResolves(),
          accessController(nullptr),
          securityManager(nullptr),
          parentResolveMutex(),
          routingTableFileName(),
          messageQueueCleanerTimer(ioService),
          messageQueueCleanerTimerPeriodMs(std::chrono::milliseconds(1000))
{
    activateMessageCleanerTimer();
}

void MessageRouter::addProvisionedNextHop(
        std::string participantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    addToRoutingTable(participantId, address);
}

void MessageRouter::setAccessController(std::shared_ptr<IAccessController> accessController)
{
    assert(accessController);
    this->accessController = accessController;
}

void MessageRouter::setParentRouter(
        std::unique_ptr<system::RoutingProxy> parentRouter,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> parentAddress,
        std::string parentParticipantId)
{
    this->parentRouter = std::move(parentRouter);
    this->parentAddress = std::move(parentAddress);

    // add the next hop to parent router
    // this is necessary because during normal registration, the parent proxy is not yet set
    addProvisionedNextHop(parentParticipantId, this->parentAddress);
    addNextHopToParent(this->parentRouter->getProxyParticipantId());
}

bool MessageRouter::isChildMessageRouter()
{
    if (!incomingAddress) {
        return false;
    }
    // if an incoming address is set, a parent message router is needed for correct configuration
    return parentRouter && parentAddress;
}

std::unordered_set<std::shared_ptr<const system::RoutingTypes::Address>> MessageRouter::
        lookupAddresses(const std::unordered_set<std::string>& participantIds)
{
    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> addresses;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress;
    for (const auto& participantId : participantIds) {
        destAddress = routingTable.lookup(participantId);
        if (destAddress) {
            addresses.insert(destAddress);
        }
    }
    return addresses;
}

std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> MessageRouter::
        getDestinationAddresses(const JoynrMessage& message)
{
    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> addresses;
    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST) {
        std::string multicastId = message.getHeaderTo();

        // lookup local multicast receivers
        std::unordered_set<std::string> multicastReceivers =
                multicastReceiverDirectory.getReceivers(multicastId);
        addresses = lookupAddresses(multicastReceivers);

        // add global transport address if message is NOT received from global
        if (!message.isReceivedFromGlobal() && addressCalculator) {
            std::shared_ptr<const joynr::system::RoutingTypes::Address> globalTransport =
                    addressCalculator->compute(message);
            if (globalTransport) {
                addresses.insert(globalTransport);
            }
        }
    } else {
        const std::string destinationPartId = message.getHeaderTo();
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress =
                routingTable.lookup(destinationPartId);
        if (destAddress) {
            addresses.insert(destAddress);
        }
    }
    return addresses;
}

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void MessageRouter::route(const JoynrMessage& message, std::uint32_t tryCount)
{
    assert(messagingStubFactory != nullptr);
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    if (now > message.getHeaderExpiryDate()) {
        std::string errorMessage("Received expired message. Dropping the message (ID: " +
                                 message.getHeaderMessageId() + ").");
        JOYNR_LOG_WARN(logger, errorMessage);
        throw exceptions::JoynrMessageNotSentException(errorMessage);
    }

    // Validate the message if possible
    if (securityManager != nullptr && !securityManager->validate(message)) {
        std::string errorMessage("messageId " + message.getHeaderMessageId() +
                                 " failed validation");
        JOYNR_LOG_ERROR(logger, errorMessage);
        throw exceptions::JoynrMessageNotSentException(errorMessage);
    }

    JOYNR_LOG_TRACE(logger,
                    "Route message with Id {} and payload {}",
                    message.getHeaderMessageId(),
                    message.getPayload());

    // search for the destination addresses
    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> destAddresses =
            getDestinationAddresses(message);
    // if destination address is not known
    if (destAddresses.empty()) {
        if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST) {
            // Do not queue multicast messages for future multicast receivers.
            return;
        }

        // save the message for later delivery
        messageQueue->queueMessage(message);
        JOYNR_LOG_TRACE(logger, "message queued: {}", message.getPayload());

        // and try to resolve destination address via parent message router
        if (isChildMessageRouter()) {
            std::lock_guard<std::mutex> lock(parentResolveMutex);
            const std::string destinationPartId = message.getHeaderTo();
            if (runningParentResolves.find(destinationPartId) == runningParentResolves.end()) {
                runningParentResolves.insert(destinationPartId);
                std::function<void(const bool&)> onSuccess =
                        [this, destinationPartId](const bool& resolved) {
                    if (resolved) {
                        JOYNR_LOG_DEBUG(logger,
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

    for (std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress : destAddresses) {
        if (accessController) {
            // Access control checks are asynchronous, callback will send message
            // if access is granted
            auto callback =
                    std::make_shared<ConsumerPermissionCallback>(*this, message, destAddress);
            accessController->hasConsumerPermission(message, callback);
            return;
        }

        // If this point is reached, the message can be sent without delay
        sendMessage(message, destAddress, tryCount);
    }
}

void MessageRouter::removeRunningParentResolvers(const std::string& destinationPartId)
{
    std::lock_guard<std::mutex> lock(parentResolveMutex);
    if (runningParentResolves.find(destinationPartId) != runningParentResolves.end()) {
        runningParentResolves.erase(destinationPartId);
    }
}

void MessageRouter::sendMessages(
        const std::string& destinationPartId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    while (true) {
        // We have to check all the time whether the messaging stub is still available because
        // it will be deleted if a disconnect occurs (this may happen while this method
        // is being executed).
        auto messagingStub = messagingStubFactory->create(address);

        if (messagingStub == nullptr) {
            break;
        }

        std::unique_ptr<MessageQueueItem> item(
                messageQueue->getNextMessageForParticipant(destinationPartId));

        if (!item) {
            break;
        }

        try {
            const std::uint32_t tryCount = 0;
            messageScheduler.schedule(
                    new MessageRunnable(
                            item->getContent(), messagingStub, address, *this, tryCount),
                    std::chrono::milliseconds(0));
        } catch (const exceptions::JoynrMessageNotSentException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Message with Id {} could not be sent. Error: {}",
                            item->getContent().getHeaderMessageId(),
                            e.getMessage());
        }
    }
}

void MessageRouter::sendMessage(
        const JoynrMessage& message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::uint32_t tryCount)
{
    scheduleMessage(message, destAddress, tryCount);
}

void MessageRouter::scheduleMessage(
        const JoynrMessage& message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::uint32_t tryCount,
        std::chrono::milliseconds delay)
{
    auto stub = messagingStubFactory->create(destAddress);
    if (stub) {
        messageScheduler.schedule(
                new MessageRunnable(message, stub, destAddress, *this, tryCount), delay);
    } else {
        std::string errorMessage("Message with payload " + message.getPayload() +
                                 "  could not be send to " + destAddress->toString() +
                                 ". Stub creation failed. Queueing message.");
        JOYNR_LOG_WARN(logger, errorMessage);
        // save the message for later delivery
        messageQueue->queueMessage(message);
    }
}

void MessageRouter::activateMessageCleanerTimer()
{
    messageQueueCleanerTimer.expiresFromNow(messageQueueCleanerTimerPeriodMs);
    messageQueueCleanerTimer.asyncWait(
            std::bind(&MessageRouter::onMessageCleanerTimerExpired, this, std::placeholders::_1));
}

void MessageRouter::onMessageCleanerTimerExpired(const boost::system::error_code& errorCode)
{
    if (!errorCode) {
        messageQueue->removeOutdatedMessages();
        activateMessageCleanerTimer();
    } else if (errorCode != boost::system::errc::operation_canceled) {
        JOYNR_LOG_ERROR(logger,
                        "Failed to schedule timer to remove outdated messages: {}",
                        errorCode.message());
    }
}

void MessageRouter::addNextHop(
        const std::string& participantId,
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& inprocessAddress,
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
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(channelAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, std::move(onSuccess), std::move(onError));

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::MqttAddress& mqttAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(mqttAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, std::move(onSuccess), std::move(onError));

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::CommonApiDbusAddress& commonApiDbusAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address = std::make_shared<const joynr::system::RoutingTypes::CommonApiDbusAddress>(
            commonApiDbusAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, std::move(onSuccess), std::move(onError));

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::BrowserAddress& browserAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::BrowserAddress>(browserAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, std::move(onSuccess), std::move(onError));

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
            std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(webSocketAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, std::move(onSuccess), std::move(onError));

    sendMessages(participantId, address);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto address = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
            webSocketClientAddress);
    addToRoutingTable(participantId, address);

    addNextHopToParent(participantId, std::move(onSuccess), std::move(onError));

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
                    std::dynamic_pointer_cast<const joynr::system::RoutingTypes::ChannelAddress>(
                            incomingAddress)) {
            parentRouter->addNextHopAsync(participantId,
                                          *channelAddress,
                                          std::move(onSuccess),
                                          std::move(onErrorWrapper));
        } else if (auto mqttAddress = std::dynamic_pointer_cast<
                           const joynr::system::RoutingTypes::MqttAddress>(incomingAddress)) {
            parentRouter->addNextHopAsync(
                    participantId, *mqttAddress, std::move(onSuccess), std::move(onErrorWrapper));
        } else if (auto commonApiDbusAddress = std::dynamic_pointer_cast<
                           const joynr::system::RoutingTypes::CommonApiDbusAddress>(
                           incomingAddress)) {
            parentRouter->addNextHopAsync(participantId,
                                          *commonApiDbusAddress,
                                          std::move(onSuccess),
                                          std::move(onErrorWrapper));
        } else if (auto browserAddress = std::dynamic_pointer_cast<
                           const joynr::system::RoutingTypes::BrowserAddress>(incomingAddress)) {
            parentRouter->addNextHopAsync(participantId,
                                          *browserAddress,
                                          std::move(onSuccess),
                                          std::move(onErrorWrapper));
        } else if (auto webSocketAddress = std::dynamic_pointer_cast<
                           const joynr::system::RoutingTypes::WebSocketAddress>(incomingAddress)) {
            parentRouter->addNextHopAsync(participantId,
                                          *webSocketAddress,
                                          std::move(onSuccess),
                                          std::move(onErrorWrapper));
        } else if (auto webSocketClientAddress = std::dynamic_pointer_cast<
                           const joynr::system::RoutingTypes::WebSocketClientAddress>(
                           incomingAddress)) {
            parentRouter->addNextHopAsync(participantId,
                                          *webSocketClientAddress,
                                          std::move(onSuccess),
                                          std::move(onErrorWrapper));
        }
    } else if (onSuccess) {
        onSuccess();
    }
}

void MessageRouter::loadRoutingTable(std::string fileName)
{
    // always update reference file
    if (fileName != routingTableFileName) {
        routingTableFileName = std::move(fileName);
    }

    if (!joynr::util::fileExists(routingTableFileName)) {
        return;
    }

    WriteLocker lock(routingTableLock);
    try {
        joynr::serializer::deserializeFromJson(
                routingTable, joynr::util::loadStringFromFile(routingTableFileName));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger, "could not deserialize from JSON: {}", ex.what());
    }
}

void MessageRouter::saveRoutingTable()
{
    WriteLocker lock(routingTableLock);
    try {
        joynr::util::saveStringToFile(
                routingTableFileName, joynr::serializer::serializeToJson(routingTable));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_WARN(logger, ex.what());
    }
}

void MessageRouter::saveMulticastReceiverDirectory() const
{
    if (multicastReceveiverDirectoryFilename.empty()) {
        JOYNR_LOG_WARN(logger, "Did not save multicast receiver directory: No filename specified");
        return;
    }

    try {
        joynr::util::saveStringToFile(
                multicastReceveiverDirectoryFilename,
                joynr::serializer::serializeToJson(multicastReceiverDirectory));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_WARN(logger, ex.what());
    }
}

void MessageRouter::loadMulticastReceiverDirectory(std::string filename)
{
    multicastReceveiverDirectoryFilename = std::move(filename);

    try {
        joynr::serializer::deserializeFromJson(
                multicastReceiverDirectory,
                joynr::util::loadStringFromFile(multicastReceveiverDirectoryFilename));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
        return;
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger, "Deserialization from JSON failed: {}", ex.what());
        return;
    }

    reestablishMulticastSubscriptions();
}

void MessageRouter::reestablishMulticastSubscriptions()
{
    for (const auto& multicastId : multicastReceiverDirectory.getMulticastIds()) {
        std::string providerParticipantId;

        try {
            providerParticipantId = util::extractParticipantIdFromMulticastId(multicastId);
        } catch (std::invalid_argument& ex) {
            JOYNR_LOG_ERROR(logger,
                            "Persisted multicast receivers: Invalid multicast ID found {}",
                            multicastId);
            continue;
        }

        std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress =
                routingTable.lookup(providerParticipantId);

        if (!providerAddress) {
            JOYNR_LOG_WARN(
                    logger,
                    "Persisted multicast receivers: No provider address found for multicast ID {}",
                    multicastId);
            continue;
        }

        std::shared_ptr<IMessagingMulticastSubscriber> multicastSubscriber =
                multicastMessagingSkeletonDirectory->getSkeleton(providerAddress);

        if (!multicastSubscriber) {
            JOYNR_LOG_WARN(logger,
                           "Persisted multicast receivers: No multicast subscriber found for "
                           "multicast ID {}",
                           multicastId);
            continue;
        }

        multicastSubscriber->registerMulticastSubscription(multicastId);
    }
}

void MessageRouter::addToRoutingTable(
        std::string participantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    {
        WriteLocker lock(routingTableLock);
        routingTable.add(participantId, address);
    }
    saveRoutingTable();
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
    saveRoutingTable();

    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError](const exceptions::JoynrException& error) {
        if (onError) {
            onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
        } else {
            JOYNR_LOG_ERROR(logger,
                            "Unable to report error (received by calling "
                            "parentRouter->removeNextHopAsync), since onError function is "
                            "empty. Error message: {}",
                            error.getMessage());
        }
    };

    // remove from parent router
    if (isChildMessageRouter()) {
        parentRouter->removeNextHopAsync(
                participantId, std::move(onSuccess), std::move(onErrorWrapper));
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
    bool resolved;
    {
        ReadLocker lock(routingTableLock);
        resolved = routingTable.contains(participantId);
    }
    onSuccess(resolved);
}

void MessageRouter::registerMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError)
{
    if (isChildMessageRouter()) {
        // TODO do not invoke parent message router for InProcessMessagingAddress
        parentRouter->addMulticastReceiverAsync(multicastId,
                                                subscriberParticipantId,
                                                providerParticipantId,
                                                std::move(onSuccess),
                                                std::move(onError));
    } else {
        std::shared_ptr<IMessagingMulticastSubscriber> skeleton =
                multicastMessagingSkeletonDirectory->getSkeleton(providerAddress);
        if (skeleton) {
            try {
                skeleton->registerMulticastSubscription(multicastId);
                onSuccess();
            } catch (const exceptions::JoynrRuntimeException& error) {
                onError(error);
            }
        } else {
            JOYNR_LOG_DEBUG(logger,
                            "No messaging skeleton found for multicast "
                            "provider (address=" +
                                    providerAddress->toString() + ").");
            onSuccess();
        }
    }
};

void MessageRouter::addMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress;
    {
        ReadLocker lock(routingTableLock);
        providerAddress = routingTable.lookup(providerParticipantId);
    }

    std::function<void()> onSuccessWrapper =
            [this, multicastId, subscriberParticipantId, onSuccess]() {
        multicastReceiverDirectory.registerMulticastReceiver(multicastId, subscriberParticipantId);
        JOYNR_LOG_DEBUG(logger,
                        "added multicast receiver={} for multicastId={}",
                        subscriberParticipantId,
                        multicastId);

        if (!isChildMessageRouter()) {
            saveMulticastReceiverDirectory();
        }

        onSuccess();
    };
    std::function<void(const exceptions::JoynrRuntimeException&)> onErrorWrapper =
            [onError, subscriberParticipantId, multicastId](
                    const exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger,
                        "error adding multicast receiver={} for multicastId={}, error: {}",
                        subscriberParticipantId,
                        multicastId,
                        error.getMessage());
        onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
    };

    if (!providerAddress) {
        if (isChildMessageRouter()) {
            // try to resolve destination address via parent message router
            auto onResolved = [this,
                               multicastId,
                               subscriberParticipantId,
                               providerParticipantId,
                               onSuccessWrapper,
                               onErrorWrapper](const bool& resolved) {
                if (resolved) {
                    addProvisionedNextHop(providerParticipantId, parentAddress);
                    registerMulticastReceiver(multicastId,
                                              subscriberParticipantId,
                                              providerParticipantId,
                                              parentAddress,
                                              onSuccessWrapper,
                                              onErrorWrapper);
                } else {
                    exceptions::ProviderRuntimeException exception(
                            "No routing entry for multicast provider (providerParticipantId=" +
                            providerParticipantId + ") found in parent router.");
                    onErrorWrapper(exception);
                }
            };
            auto onResolveError = [this, onErrorWrapper, providerParticipantId](
                    const joynr::exceptions::JoynrRuntimeException& error) {
                exceptions::ProviderRuntimeException exception(
                        "error resolving next hop for multicast provider (providerParticipantId=" +
                        providerParticipantId + "). Error from parent router: " +
                        error.getMessage());
                onErrorWrapper(exception);
            };
            parentRouter->resolveNextHopAsync(
                    providerParticipantId, std::move(onResolved), std::move(onResolveError));
        } else {
            exceptions::ProviderRuntimeException exception(
                    "No routing entry for multicast provider (providerParticipantId=" +
                    providerParticipantId + ") found.");
            onErrorWrapper(exception);
        }
        return;
    }
    registerMulticastReceiver(multicastId,
                              subscriberParticipantId,
                              providerParticipantId,
                              providerAddress,
                              onSuccessWrapper,
                              onErrorWrapper);
}

void MessageRouter::removeMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, subscriberParticipantId);

    if (!isChildMessageRouter()) {
        saveMulticastReceiverDirectory();
    }

    std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress;
    {
        ReadLocker lock(routingTableLock);
        providerAddress = routingTable.lookup(providerParticipantId);
    }
    if (!providerAddress) {
        exceptions::ProviderRuntimeException exception(
                "No routing entry for multicast provider (providerParticipantId=" +
                providerParticipantId + ") found.");
        JOYNR_LOG_ERROR(logger, exception.getMessage());
        onError(exception);
        return;
    } else if (isChildMessageRouter()) {
        // TODO do not invoke parent message router for InProcessMessagingAddress
        std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
                [onError, subscriberParticipantId, multicastId](
                        const exceptions::JoynrException& error) {
            JOYNR_LOG_ERROR(logger,
                            "error removing multicast receiver={} for multicastId={}, error: {}",
                            subscriberParticipantId,
                            multicastId,
                            error.getMessage());
            onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
        };
        parentRouter->removeMulticastReceiverAsync(multicastId,
                                                   subscriberParticipantId,
                                                   providerParticipantId,
                                                   std::move(onSuccess),
                                                   std::move(onErrorWrapper));
    } else {
        std::shared_ptr<IMessagingMulticastSubscriber> skeleton =
                multicastMessagingSkeletonDirectory->getSkeleton(providerAddress);
        if (skeleton) {
            skeleton->unregisterMulticastSubscription(multicastId);
        } else {
            JOYNR_LOG_DEBUG(logger,
                            "No messaging skeleton found for multicast "
                            "provider (address=" +
                                    providerAddress->toString() + ").");
        }
        onSuccess();
    }
}

/**
 * IMPLEMENTATION of MessageRunnable class
 */

INIT_LOGGER(MessageRunnable);

MessageRunnable::MessageRunnable(
        const JoynrMessage& message,
        std::shared_ptr<IMessaging> messagingStub,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        MessageRouter& messageRouter,
        std::uint32_t tryCount)
        : Runnable(true),
          ObjectWithDecayTime(message.getHeaderExpiryDate()),
          message(message),
          messagingStub(messagingStub),
          destAddress(destAddress),
          messageRouter(messageRouter),
          tryCount(tryCount)
{
}

void MessageRunnable::shutdown()
{
}

void MessageRunnable::run()
{
    if (!isExpired()) {
        auto onFailure = [this](const exceptions::JoynrRuntimeException& e) {
            try {
                exceptions::JoynrDelayMessageException& delayException =
                        dynamic_cast<exceptions::JoynrDelayMessageException&>(
                                const_cast<exceptions::JoynrRuntimeException&>(e));
                std::chrono::milliseconds delay = delayException.getDelayMs();

                JOYNR_LOG_ERROR(logger,
                                "Rescheduling message after error: messageId: {}, new delay {}ms, "
                                "error: {}",
                                message.getHeaderMessageId(),
                                delay.count(),
                                e.getMessage());
                messageRouter.scheduleMessage(message, destAddress, tryCount + 1, delay);
            } catch (std::bad_cast& castError) {
                JOYNR_LOG_ERROR(logger,
                                "Message with ID {} could not be sent! reason: {}",
                                message.getHeaderMessageId(),
                                e.getMessage());
            }
        };
        messagingStub->transmit(message, onFailure);
    } else {
        JOYNR_LOG_ERROR(
                logger, "Message with ID {}  expired: dropping!", message.getHeaderMessageId());
    }
}

/**
 * IMPLEMENTATION of ConsumerPermissionCallback class
 */
INIT_LOGGER(ConsumerPermissionCallback);

ConsumerPermissionCallback::ConsumerPermissionCallback(
        MessageRouter& owningMessageRouter,
        const JoynrMessage& message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destination)
        : owningMessageRouter(owningMessageRouter), message(message), destination(destination)
{
}

void ConsumerPermissionCallback::hasConsumerPermission(bool hasPermission)
{
    if (hasPermission) {
        try {
            owningMessageRouter.sendMessage(message, destination);
        } catch (const exceptions::JoynrMessageNotSentException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Message with Id {} could not be sent. Error: {}",
                            message.getHeaderMessageId(),
                            e.getMessage());
        }
    }
}

} // namespace joynr

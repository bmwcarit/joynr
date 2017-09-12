/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include "joynr/CcMessageRouter.h"

#include <cassert>
#include <functional>

#include "joynr/IMessagingMulticastSubscriber.h"
#include "joynr/IMessagingStubFactory.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/IPlatformSecurityManager.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Message.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/Util.h"
#include "joynr/access-control/IAccessController.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/MessageNotificationAbstractProvider.h"
#include "joynr/system/MessageNotificationMessageQueuedForDeliveryBroadcastFilter.h"
#include "joynr/system/MessageNotificationMessageQueuedForDeliveryBroadcastFilterParameters.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{

INIT_LOGGER(CcMessageRouter);

//------ ConsumerPermissionCallback --------------------------------------------

class ConsumerPermissionCallback : public IAccessController::IHasConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback(
            CcMessageRouter& owningMessageRouter,
            std::shared_ptr<ImmutableMessage> message,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destination);

    void hasConsumerPermission(bool hasPermission) override;

    CcMessageRouter& owningMessageRouter;
    std::shared_ptr<ImmutableMessage> message;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destination;

private:
    ADD_LOGGER(ConsumerPermissionCallback);
};

//------ MessageNotification ---------------------------------------------------

class CcMessageNotificationProvider : public joynr::system::MessageNotificationAbstractProvider
{
public:
    virtual ~CcMessageNotificationProvider() = default;
    using MessageNotificationAbstractProvider::fireMessageQueuedForDelivery;
};

class MessageQueuedForDeliveryBroadcastFilter
        : public joynr::system::MessageNotificationMessageQueuedForDeliveryBroadcastFilter
{
    bool filter(const std::string& participantId,
                const std::string& messageType,
                const joynr::system::
                        MessageNotificationMessageQueuedForDeliveryBroadcastFilterParameters&
                                filterParameters) override
    {
        const bool isParticipantIdSet = !filterParameters.getParticipantId().empty();
        const bool isMessageTypeSet = !filterParameters.getMessageType().empty();

        // if no filter parameters are set, always send broadcast
        if (!isParticipantIdSet && !isMessageTypeSet) {
            return true;
        }
        // if message type is empty, check if participant id matches
        if (!isMessageTypeSet) {
            return filterParameters.getParticipantId() == participantId;
        }
        // if participant type is empty, check if message type matches
        if (!isParticipantIdSet) {
            return filterParameters.getMessageType() == messageType;
        }
        return filterParameters.getParticipantId() == participantId &&
               filterParameters.getMessageType() == messageType;
    }
};

//------ MessageRouter ---------------------------------------------------------

CcMessageRouter::CcMessageRouter(
        MessagingSettings& messagingSettings,
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory,
        std::unique_ptr<IPlatformSecurityManager> securityManager,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        const std::string& globalClusterControllerAddress,
        const std::string& messageNotificationProviderParticipantId,
        std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
        int maxThreads,
        std::unique_ptr<MessageQueue<std::string>> messageQueue,
        std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> transportNotAvailableQueue)
        : AbstractMessageRouter(messagingSettings,
                                std::move(messagingStubFactory),
                                ioService,
                                std::move(addressCalculator),
                                maxThreads,
                                std::move(transportStatuses),
                                std::move(messageQueue),
                                std::move(transportNotAvailableQueue)),
          joynr::system::RoutingAbstractProvider(),
          multicastMessagingSkeletonDirectory(multicastMessagingSkeletonDirectory),
          securityManager(std::move(securityManager)),
          accessController(),
          multicastReceiverDirectoryFilename(),
          globalClusterControllerAddress(globalClusterControllerAddress),
          messageNotificationProvider(std::make_shared<CcMessageNotificationProvider>()),
          messageNotificationProviderParticipantId(messageNotificationProviderParticipantId)
{
    messageNotificationProvider->addBroadcastFilter(
            std::make_shared<MessageQueuedForDeliveryBroadcastFilter>());
}

CcMessageRouter::~CcMessageRouter()
{
}

void CcMessageRouter::setAccessController(std::weak_ptr<IAccessController> accessController)
{
    this->accessController = std::move(accessController);
}

void CcMessageRouter::saveMulticastReceiverDirectory() const
{
    if (multicastReceiverDirectoryFilename.empty()) {
        JOYNR_LOG_INFO(logger, "Did not save multicast receiver directory: No filename specified");
        return;
    }

    try {
        joynr::util::saveStringToFile(
                multicastReceiverDirectoryFilename,
                joynr::serializer::serializeToJson(multicastReceiverDirectory));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger, ex.what());
    }
}

void CcMessageRouter::loadMulticastReceiverDirectory(std::string filename)
{
    multicastReceiverDirectoryFilename = std::move(filename);

    try {
        joynr::serializer::deserializeFromJson(
                multicastReceiverDirectory,
                joynr::util::loadStringFromFile(multicastReceiverDirectoryFilename));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
        return;
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger, "Deserialization from JSON failed: {}", ex.what());
        return;
    }

    reestablishMulticastSubscriptions();
}

std::shared_ptr<system::MessageNotificationProvider> CcMessageRouter::
        getMessageNotificationProvider() const
{
    return messageNotificationProvider;
}

void CcMessageRouter::getGlobalAddress(
        std::function<void(const std::string&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (globalClusterControllerAddress.empty()) {
        onError(joynr::exceptions::ProviderRuntimeException(
                "No cluster-controller global address available."));
    } else {
        onSuccess(globalClusterControllerAddress);
    }
}

void CcMessageRouter::getReplyToAddress(
        std::function<void(const std::string&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (globalClusterControllerAddress.empty()) {
        onError(joynr::exceptions::ProviderRuntimeException(
                "No cluster-controller reply address available."));
    } else {
        onSuccess(globalClusterControllerAddress);
    }
}

void CcMessageRouter::reestablishMulticastSubscriptions()
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

        ReadLocker lock(routingTableLock);
        const auto routingEntry =
                routingTable.lookupRoutingEntryByParticipantId(providerParticipantId);
        if (!routingEntry) {
            JOYNR_LOG_WARN(logger,
                           "Persisted multicast receivers: No provider address found for "
                           "multicast ID {}",
                           multicastId);
            continue;
        }

        const auto providerAddress = routingEntry->address;
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

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void CcMessageRouter::routeInternal(std::shared_ptr<ImmutableMessage> message,
                                    std::uint32_t tryCount)
{
    assert(message);
    // Validate the message if possible
    if (securityManager != nullptr && !securityManager->validate(*message)) {
        std::string errorMessage("messageId " + message->getId() + " failed validation");
        JOYNR_LOG_ERROR(logger, errorMessage);
        throw exceptions::JoynrMessageNotSentException(errorMessage);
    }

    registerGlobalRoutingEntryIfRequired(*message);

    JOYNR_LOG_TRACE(logger, "Route message with Id {}", message->getId());
    // search for the destination addresses
    AbstractMessageRouter::AddressUnorderedSet destAddresses = getDestinationAddresses(*message);
    // if destination address is not known
    if (destAddresses.empty()) {
        if (message->getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
            // Do not queue multicast messages for future multicast receivers.
            return;
        }

        // save the message for later delivery
        JOYNR_LOG_WARN(logger,
                       "No routing information found for destination participant ID \"{}\" "
                       "so far. Waiting for participant registration. "
                       "Queueing message (ID : {})",
                       message->getRecipient(),
                       message->getId());
        queueMessage(std::move(message));
        return;
    }

    for (std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress : destAddresses) {
        if (auto gotAccessController = accessController.lock()) {
            // Access control checks are asynchronous, callback will send message
            // if access is granted
            auto callback =
                    std::make_shared<ConsumerPermissionCallback>(*this, message, destAddress);
            gotAccessController->hasConsumerPermission(message, callback);
            return;
        }

        // If this point is reached, the message can be sent without delay
        scheduleMessage(message, destAddress, tryCount);
    }
}

bool CcMessageRouter::publishToGlobal(const ImmutableMessage& message)
{
    // Caution: Do not lock routingTableLock here, it must have been called from outside
    // method gets called from AbstractMessageRouter
    const std::string& participantId = message.getSender();
    const auto routingEntry = routingTable.lookupRoutingEntryByParticipantId(participantId);
    if (routingEntry && routingEntry->isGloballyVisible) {
        return true;
    }
    return false;
}

// inherited from joynr::IMessageRouter and joynr::system::RoutingProvider
void CcMessageRouter::removeNextHop(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;

    {
        WriteLocker lock(routingTableLock);
        routingTable.remove(participantId);
    }
    saveRoutingTable();

    if (onSuccess) {
        onSuccess();
    }
}

void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address,
        bool isGloballyVisible,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    assert(address);
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    addToRoutingTable(participantId, isGloballyVisible, address, expiryDateMs, isSticky);
    sendMessages(participantId, address);
    if (onSuccess) {
        onSuccess();
    }
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::ChannelAddress& channelAddress,
        const bool& isGloballyVisible,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(channelAddress);
    addNextHop(participantId, std::move(address), isGloballyVisible, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::MqttAddress& mqttAddress,
        const bool& isGloballyVisible,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(mqttAddress);
    addNextHop(participantId, std::move(address), isGloballyVisible, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::CommonApiDbusAddress& commonApiDbusAddress,
        const bool& isGloballyVisible,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address = std::make_shared<const joynr::system::RoutingTypes::CommonApiDbusAddress>(
            commonApiDbusAddress);
    addNextHop(participantId, std::move(address), isGloballyVisible, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::BrowserAddress& browserAddress,
        const bool& isGloballyVisible,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::BrowserAddress>(browserAddress);
    addNextHop(participantId, std::move(address), isGloballyVisible, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketAddress& webSocketAddress,
        const bool& isGloballyVisible,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(webSocketAddress);
    addNextHop(participantId, std::move(address), isGloballyVisible, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
        const bool& isGloballyVisible,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
            webSocketClientAddress);
    addNextHop(participantId, std::move(address), isGloballyVisible, std::move(onSuccess));
}

void CcMessageRouter::resolveNextHop(
        const std::string& participantId,
        std::function<void(const bool& resolved)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;

    bool resolved;
    {
        ReadLocker lock(routingTableLock);
        resolved = routingTable.containsParticipantId(participantId);
    }
    onSuccess(resolved);
}

void CcMessageRouter::registerMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError)
{
    std::ignore = subscriberParticipantId;
    std::ignore = providerParticipantId;

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
        JOYNR_LOG_TRACE(logger,
                        "No messaging skeleton found for multicast "
                        "provider (address=" +
                                providerAddress->toString() + ").");
        onSuccess();
    }
}

void CcMessageRouter::addMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    boost::optional<routingtable::RoutingEntry> routingEntry;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress;
    {
        ReadLocker lock(routingTableLock);
        routingEntry = routingTable.lookupRoutingEntryByParticipantId(providerParticipantId);
    }

    std::function<void()> onSuccessWrapper =
            [ this, multicastId, subscriberParticipantId, onSuccess = std::move(onSuccess) ]()
    {
        multicastReceiverDirectory.registerMulticastReceiver(multicastId, subscriberParticipantId);
        JOYNR_LOG_TRACE(logger,
                        "added multicast receiver={} for multicastId={}",
                        subscriberParticipantId,
                        multicastId);
        saveMulticastReceiverDirectory();
        if (onSuccess) {
            onSuccess();
        }
    };
    std::function<void(const exceptions::JoynrRuntimeException&)> onErrorWrapper =
            [ onError = std::move(onError), subscriberParticipantId, multicastId ](
                    const exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_ERROR(logger,
                        "error adding multicast receiver={} for multicastId={}, error: {}",
                        subscriberParticipantId,
                        multicastId,
                        error.getMessage());
        if (onError) {
            onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
        }
    };

    if (!routingEntry) {
        exceptions::ProviderRuntimeException exception(
                "No routing entry for multicast provider (providerParticipantId=" +
                providerParticipantId + ") found.");
        onErrorWrapper(exception);
        return;
    }
    providerAddress = routingEntry->address;

    registerMulticastReceiver(multicastId,
                              subscriberParticipantId,
                              providerParticipantId,
                              std::move(providerAddress),
                              std::move(onSuccessWrapper),
                              std::move(onErrorWrapper));
}

void CcMessageRouter::removeMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, subscriberParticipantId);

    saveMulticastReceiverDirectory();

    boost::optional<routingtable::RoutingEntry> routingEntry;
    {
        ReadLocker lock(routingTableLock);
        routingEntry = routingTable.lookupRoutingEntryByParticipantId(providerParticipantId);
    }

    if (!routingEntry) {
        exceptions::ProviderRuntimeException exception(
                "No routing entry for multicast provider (providerParticipantId=" +
                providerParticipantId + ") found.");
        JOYNR_LOG_ERROR(logger, exception.getMessage());
        if (onError) {
            onError(exception);
        }
        return;
    } else {
        const auto providerAddress = routingEntry->address;
        std::shared_ptr<IMessagingMulticastSubscriber> skeleton =
                multicastMessagingSkeletonDirectory->getSkeleton(providerAddress);
        if (skeleton) {
            skeleton->unregisterMulticastSubscription(multicastId);
        } else {
            JOYNR_LOG_TRACE(logger,
                            "No messaging skeleton found for multicast "
                            "provider (address=" +
                                    providerAddress->toString() + ").");
        }
        if (onSuccess) {
            onSuccess();
        }
    }
}

void CcMessageRouter::queueMessage(std::shared_ptr<ImmutableMessage> message)
{
    JOYNR_LOG_TRACE(logger, "message queued: {}", message->toLogMessage());
    // do not fire a broadcast for an undeliverable message sent by
    // messageNotificationProvider (e.g. messageQueueForDelivery publication)
    // since it may cause an endless loop
    if (message->getSender() != messageNotificationProviderParticipantId) {
        messageNotificationProvider->fireMessageQueuedForDelivery(
                message->getRecipient(), message->getType());
    }
    std::string recipient = message->getRecipient();
    messageQueue->queueMessage(std::move(recipient), std::move(message));
}

/**
 * IMPLEMENTATION of ConsumerPermissionCallback class
 */

INIT_LOGGER(ConsumerPermissionCallback);

ConsumerPermissionCallback::ConsumerPermissionCallback(
        CcMessageRouter& owningMessageRouter,
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destination)
        : owningMessageRouter(owningMessageRouter), message(message), destination(destination)
{
}

void ConsumerPermissionCallback::hasConsumerPermission(bool hasPermission)
{
    if (hasPermission) {
        try {
            owningMessageRouter.scheduleMessage(message, destination);
        } catch (const exceptions::JoynrMessageNotSentException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Message with Id {} could not be sent. Error: {}",
                            message->getId(),
                            e.getMessage());
        }
    }
}

} // namespace joynr

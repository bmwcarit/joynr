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

#include <boost/asio/io_service.hpp>

#include "joynr/access-control/IAccessController.h"
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
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/Util.h"

namespace joynr
{

INIT_LOGGER(CcMessageRouter);

//------ ConsumerPermissionCallback --------------------------------------------

class ConsumerPermissionCallback : public IAccessController::IHasConsumerPermissionCallback
{
public:
    ConsumerPermissionCallback(
            CcMessageRouter& owningMessageRouter,
            const JoynrMessage& message,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destination);

    void hasConsumerPermission(bool hasPermission) override;

    CcMessageRouter& owningMessageRouter;
    JoynrMessage message;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destination;

private:
    ADD_LOGGER(ConsumerPermissionCallback);
};

//------ MessageRouter ---------------------------------------------------------

CcMessageRouter::CcMessageRouter(
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory,
        std::unique_ptr<IPlatformSecurityManager> securityManager,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        const std::string& globalClusterControllerAddress,
        int maxThreads,
        std::unique_ptr<MessageQueue> messageQueue)
        : AbstractMessageRouter(std::move(messagingStubFactory),
                                ioService,
                                std::move(addressCalculator),
                                maxThreads,
                                std::move(messageQueue)),
          joynr::system::RoutingAbstractProvider(),
          multicastMessagingSkeletonDirectory(multicastMessagingSkeletonDirectory),
          securityManager(std::move(securityManager)),
          multicastReceveiverDirectoryFilename(),
          globalClusterControllerAddress(globalClusterControllerAddress)
{
}

CcMessageRouter::~CcMessageRouter()
{
}

void CcMessageRouter::setAccessController(std::shared_ptr<IAccessController> accessController)
{
    assert(accessController);
    this->accessController = accessController;
}

void CcMessageRouter::saveMulticastReceiverDirectory() const
{
    if (multicastReceveiverDirectoryFilename.empty()) {
        JOYNR_LOG_INFO(logger, "Did not save multicast receiver directory: No filename specified");
        return;
    }

    try {
        joynr::util::saveStringToFile(
                multicastReceveiverDirectoryFilename,
                joynr::serializer::serializeToJson(multicastReceiverDirectory));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger, ex.what());
    }
}

void CcMessageRouter::loadMulticastReceiverDirectory(std::string filename)
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

        std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress =
                routingTable.lookup(providerParticipantId);

        if (!providerAddress) {
            JOYNR_LOG_WARN(logger,
                           "Persisted multicast receivers: No provider address found for "
                           "multicast ID {}",
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

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void CcMessageRouter::route(JoynrMessage& message, std::uint32_t tryCount)
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

    if (message.getHeaderReplyAddress().empty() && !message.isLocalMessage() &&
        (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
         message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
         message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST ||
         message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)) {
        message.setHeaderReplyAddress(globalClusterControllerAddress);
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
        JOYNR_LOG_WARN(logger,
                       "No routing information found for destination participant ID \"{}\" "
                       "so far. Waiting for participant registration. "
                       "Queueing message (ID : {})",
                       message.getHeaderTo(),
                       message.getHeaderMessageId());
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
        scheduleMessage(message, destAddress, tryCount);
    }
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
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& inprocessAddress,
        std::function<void()> onSuccess)
{
    addToRoutingTable(participantId, inprocessAddress);
    sendMessages(participantId, inprocessAddress);
    if (onSuccess) {
        onSuccess();
    }
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::ChannelAddress& channelAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(channelAddress);
    addNextHop(participantId, address, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::MqttAddress& mqttAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(mqttAddress);
    addNextHop(participantId, address, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::CommonApiDbusAddress& commonApiDbusAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address = std::make_shared<const joynr::system::RoutingTypes::CommonApiDbusAddress>(
            commonApiDbusAddress);
    addNextHop(participantId, address, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::BrowserAddress& browserAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::BrowserAddress>(browserAddress);
    addNextHop(participantId, address, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketAddress& webSocketAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(webSocketAddress);
    addNextHop(participantId, address, std::move(onSuccess));
}

// inherited from joynr::system::RoutingProvider
void CcMessageRouter::addNextHop(
        const std::string& participantId,
        const system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    auto address = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
            webSocketClientAddress);
    addNextHop(participantId, address, std::move(onSuccess));
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
        resolved = routingTable.contains(participantId);
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
    std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress;
    {
        ReadLocker lock(routingTableLock);
        providerAddress = routingTable.lookup(providerParticipantId);
    }

    std::function<void()> onSuccessWrapper =
            [this, multicastId, subscriberParticipantId, onSuccess]() {
        multicastReceiverDirectory.registerMulticastReceiver(multicastId, subscriberParticipantId);
        JOYNR_LOG_TRACE(logger,
                        "added multicast receiver={} for multicastId={}",
                        subscriberParticipantId,
                        multicastId);
        saveMulticastReceiverDirectory();
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
        exceptions::ProviderRuntimeException exception(
                "No routing entry for multicast provider (providerParticipantId=" +
                providerParticipantId + ") found.");
        onErrorWrapper(exception);
        return;
    }

    registerMulticastReceiver(multicastId,
                              subscriberParticipantId,
                              providerParticipantId,
                              providerAddress,
                              onSuccessWrapper,
                              onErrorWrapper);
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
    } else {
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
        onSuccess();
    }
}

/**
 * IMPLEMENTATION of ConsumerPermissionCallback class
 */

INIT_LOGGER(ConsumerPermissionCallback);

ConsumerPermissionCallback::ConsumerPermissionCallback(
        CcMessageRouter& owningMessageRouter,
        const JoynrMessage& message,
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
                            message.getHeaderMessageId(),
                            e.getMessage());
        }
    }
}

} // namespace joynr

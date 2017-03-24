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
#include "joynr/LibJoynrMessageRouter.h"

#include <cassert>
#include <functional>

#include <boost/asio/io_service.hpp>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/IMessaging.h"
#include "joynr/IMessagingMulticastSubscriber.h"
#include "joynr/IMessagingStubFactory.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

#define EXIT_IF_PARENT_MESSAGE_ROUTER_IS_NOT_SET()                                                 \
    if (!isParentMessageRouterSet())                                                               \
        return;

namespace joynr
{

INIT_LOGGER(LibJoynrMessageRouter);

//------ MessageRouter ---------------------------------------------------------

LibJoynrMessageRouter::~LibJoynrMessageRouter()
{
}

LibJoynrMessageRouter::LibJoynrMessageRouter(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> incomingAddress,
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        int maxThreads,
        std::unique_ptr<MessageQueue> messageQueue)
        : AbstractMessageRouter(std::move(messagingStubFactory),
                                ioService,
                                std::move(addressCalculator),
                                maxThreads,
                                std::move(messageQueue)),
          parentRouter(nullptr),
          parentAddress(nullptr),
          incomingAddress(incomingAddress),
          runningParentResolves(),
          parentResolveMutex(),
          globalParentClusterControllerAddressMutex(),
          globalParentClusterControllerAddress()
{
}

void LibJoynrMessageRouter::setParentRouter(
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

void LibJoynrMessageRouter::queryGlobalClusterControllerAddress(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError)
{
    assert(parentRouter);

    auto onSuccessWrapper = [ onSuccess = std::move(onSuccess), this ](
            const std::string& globalAddress)
    {
        {
            std::lock_guard<std::mutex> lock(globalParentClusterControllerAddressMutex);
            globalParentClusterControllerAddress = globalAddress;
        }

        onSuccess();
    };

    parentRouter->getReplyToAddressAsync(std::move(onSuccessWrapper), std::move(onError));
}

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void LibJoynrMessageRouter::route(JoynrMessage& message, std::uint32_t tryCount)
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

    JOYNR_LOG_TRACE(logger,
                    "Route message with Id {} and payload {}",
                    message.getHeaderMessageId(),
                    message.getPayload());
    // search for the destination addresses
    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> destAddresses =
            getDestinationAddresses(message);

    if (!message.isLocalMessage() &&
        (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
         message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
         message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST ||
         message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)) {
        std::lock_guard<std::mutex> lock(globalParentClusterControllerAddressMutex);
        message.setHeaderReplyAddress(globalParentClusterControllerAddress);
    }

    // if destination address is not known
    if (destAddresses.empty()) {
        if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST) {
            // Do not queue multicast messages for future multicast receivers.
            return;
        }

        // save the message for later delivery
        queueMessage(message);

        // and try to resolve destination address via parent message router
        std::lock_guard<std::mutex> lock(parentResolveMutex);
        const std::string destinationPartId = message.getHeaderTo();
        if (runningParentResolves.find(destinationPartId) == runningParentResolves.end()) {
            runningParentResolves.insert(destinationPartId);
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

            EXIT_IF_PARENT_MESSAGE_ROUTER_IS_NOT_SET();

            // TODO error handling in case of failing submission (?)
            parentRouter->resolveNextHopAsync(destinationPartId, onSuccess);
        }
        return;
    }

    // If this point is reached, the message can be sent without delay
    for (std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress : destAddresses) {
        scheduleMessage(message, destAddress, tryCount);
    }
}

bool LibJoynrMessageRouter::isParentMessageRouterSet()
{
    if (!parentRouter) {
        JOYNR_LOG_TRACE(logger,
                        "Parent message router not set. Discard this message if it appears "
                        "during libJoynr initlization. It can be related to a configuration "
                        "problem. Check setting file.");
        return false;
    }
    return true;
}

void LibJoynrMessageRouter::addNextHopToParent(
        std::string participantId,
        std::function<void(void)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    EXIT_IF_PARENT_MESSAGE_ROUTER_IS_NOT_SET();

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
    if (auto channelAddress =
                std::dynamic_pointer_cast<const joynr::system::RoutingTypes::ChannelAddress>(
                        incomingAddress)) {
        parentRouter->addNextHopAsync(
                participantId, *channelAddress, std::move(onSuccess), std::move(onErrorWrapper));
    } else if (auto mqttAddress =
                       std::dynamic_pointer_cast<const joynr::system::RoutingTypes::MqttAddress>(
                               incomingAddress)) {
        parentRouter->addNextHopAsync(
                participantId, *mqttAddress, std::move(onSuccess), std::move(onErrorWrapper));
    } else if (auto commonApiDbusAddress = std::dynamic_pointer_cast<
                       const joynr::system::RoutingTypes::CommonApiDbusAddress>(incomingAddress)) {
        parentRouter->addNextHopAsync(participantId,
                                      *commonApiDbusAddress,
                                      std::move(onSuccess),
                                      std::move(onErrorWrapper));
    } else if (auto browserAddress =
                       std::dynamic_pointer_cast<const joynr::system::RoutingTypes::BrowserAddress>(
                               incomingAddress)) {
        parentRouter->addNextHopAsync(
                participantId, *browserAddress, std::move(onSuccess), std::move(onErrorWrapper));
    } else if (auto webSocketAddress = std::dynamic_pointer_cast<
                       const joynr::system::RoutingTypes::WebSocketAddress>(incomingAddress)) {
        parentRouter->addNextHopAsync(
                participantId, *webSocketAddress, std::move(onSuccess), std::move(onErrorWrapper));
    } else if (auto webSocketClientAddress = std::dynamic_pointer_cast<
                       const joynr::system::RoutingTypes::WebSocketClientAddress>(
                       incomingAddress)) {
        parentRouter->addNextHopAsync(participantId,
                                      *webSocketClientAddress,
                                      std::move(onSuccess),
                                      std::move(onErrorWrapper));
    }
}

void LibJoynrMessageRouter::addNextHop(
        const std::string& participantId,
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    addToRoutingTable(participantId, address);
    addNextHopToParent(participantId, std::move(onSuccess), std::move(onError));
    sendMessages(participantId, address);
}

void LibJoynrMessageRouter::removeNextHop(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    {
        WriteLocker lock(routingTableLock);
        routingTable.remove(participantId);
    }
    saveRoutingTable();

    std::function<void(const exceptions::JoynrRuntimeException&)> onErrorWrapper =
            [onError](const exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to report error (received by calling "
                        "parentRouter->removeNextHopAsync), since onError function is "
                        "empty. Error message: {}",
                        error.getMessage());
        if (onError) {
            onError(exceptions::ProviderRuntimeException(error.getMessage()));
        }
    };

    // remove from parent router
    parentRouter->removeNextHopAsync(
            participantId, std::move(onSuccess), std::move(onErrorWrapper));
}

void LibJoynrMessageRouter::addMulticastReceiver(
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
        // try to resolve destination address via parent message router
        auto onResolved = [this,
                           multicastId,
                           subscriberParticipantId,
                           providerParticipantId,
                           onSuccessWrapper,
                           onErrorWrapper](const bool& resolved) {
            if (resolved) {
                addProvisionedNextHop(providerParticipantId, parentAddress);
                parentRouter->addMulticastReceiverAsync(multicastId,
                                                        subscriberParticipantId,
                                                        providerParticipantId,
                                                        std::move(onSuccessWrapper),
                                                        std::move(onErrorWrapper));
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
                    providerParticipantId + "). Error from parent router: " + error.getMessage());
            onErrorWrapper(exception);
        };
        parentRouter->resolveNextHopAsync(
                providerParticipantId, std::move(onResolved), std::move(onResolveError));
        return;
    }

    parentRouter->addMulticastReceiverAsync(multicastId,
                                            subscriberParticipantId,
                                            providerParticipantId,
                                            std::move(onSuccessWrapper),
                                            std::move(onErrorWrapper));
}

void LibJoynrMessageRouter::removeMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, subscriberParticipantId);

    std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress;
    {
        ReadLocker lock(routingTableLock);
        providerAddress = routingTable.lookup(providerParticipantId);
    }

    if (!providerAddress) {
        JOYNR_LOG_ERROR(logger,
                        "No routing entry for multicast provider (providerParticipantId=" +
                                providerParticipantId + ") found.");
        return;
    }

    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError, subscriberParticipantId, multicastId](
                    const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger,
                        "error removing multicast receiver={} for multicastId={}, error: {}",
                        subscriberParticipantId,
                        multicastId,
                        error.getMessage());
        if (onError) {
            onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
        }
    };
    parentRouter->removeMulticastReceiverAsync(multicastId,
                                               subscriberParticipantId,
                                               providerParticipantId,
                                               std::move(onSuccess),
                                               std::move(onErrorWrapper));
}

void LibJoynrMessageRouter::removeRunningParentResolvers(const std::string& destinationPartId)
{
    std::lock_guard<std::mutex> lock(parentResolveMutex);
    if (runningParentResolves.find(destinationPartId) != runningParentResolves.cend()) {
        runningParentResolves.erase(destinationPartId);
    }
}

} // namespace joynr

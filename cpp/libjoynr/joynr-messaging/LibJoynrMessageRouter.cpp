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
#include <limits>

#include <boost/asio/io_service.hpp>

#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Message.h"
#include "joynr/MessageQueue.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/RoutingTable.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{

class MessagingSettings;

//------ MessageRouter ---------------------------------------------------------

LibJoynrMessageRouter::~LibJoynrMessageRouter()
{
}

LibJoynrMessageRouter::LibJoynrMessageRouter(
        MessagingSettings& messagingSettings,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> incomingAddress,
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
        std::unique_ptr<MessageQueue<std::string>> messageQueue,
        std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> transportNotAvailableQueue)
        : AbstractMessageRouter(messagingSettings,
                                std::move(messagingStubFactory),
                                ioService,
                                std::move(addressCalculator),
                                std::move(transportStatuses),
                                std::move(messageQueue),
                                std::move(transportNotAvailableQueue)),
          _parentRouter(nullptr),
          _parentAddress(nullptr),
          _incomingAddress(incomingAddress),
          _runningParentResolves(),
          _parentResolveMutex(),
          _parentClusterControllerReplyToAddressMutex(),
          _parentClusterControllerReplyToAddress(),
          _DEFAULT_IS_GLOBALLY_VISIBLE(false)
{
    _printRoutedMessages = false;
}

void LibJoynrMessageRouter::shutdown()
{
    AbstractMessageRouter::shutdown();
    _parentRouter.reset();
    _parentAddress.reset();
}

void LibJoynrMessageRouter::setParentAddress(
        std::string parentParticipantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> parentAddress)
{
    this->_parentAddress = std::move(parentAddress);
    addProvisionedNextHop(parentParticipantId, this->_parentAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);
}

void LibJoynrMessageRouter::setParentRouter(
        std::shared_ptr<system::RoutingProxy> parentRouter,
        std::function<void(void)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    assert(_parentAddress);
    this->_parentRouter = std::move(parentRouter);

    // add the next hop to parent router
    // this is necessary because during normal registration, the parent proxy is not yet set
    // because the routing provider is local, therefore isGloballyVisible is false
    const bool isGloballyVisible = false;
    addNextHopToParent(this->_parentRouter->getProxyParticipantId(),
                       isGloballyVisible,
                       std::move(onSuccess),
                       std::move(onError));
}

void LibJoynrMessageRouter::setToKnown(const std::string& participantId)
{
    JOYNR_LOG_TRACE(logger(),
                    "LibJoynrMessageRouter::setToKnown called for participantId {}",
                    participantId);
    bool isGloballyVisible = _DEFAULT_IS_GLOBALLY_VISIBLE;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    if (_parentAddress) {
        (void)addToRoutingTable(
                participantId, isGloballyVisible, _parentAddress, expiryDateMs, isSticky);
    }
}

/**
 * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
 * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
 */
void LibJoynrMessageRouter::routeInternal(std::shared_ptr<ImmutableMessage> message,
                                          std::uint32_t tryCount)
{
    JOYNR_LOG_TRACE(logger(), "Route message with Id {}", message->getId());
    AbstractMessageRouter::AddressUnorderedSet destAddresses;
    {
        ReadLocker lock(_messageQueueRetryLock);
        // search for the destination addresses
        destAddresses = getDestinationAddresses(*message, lock);

        // if destination address is not known
        if (destAddresses.empty()) {
            if (message->getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
                // Do not queue multicast messages for future multicast receivers.
                return;
            }

            // save the message for later delivery
            const std::string destinationPartId = message->getRecipient();
            queueMessage(std::move(message), lock);

            // and try to resolve destination address via parent message router
            std::unique_lock<std::mutex> parentResolveLock(_parentResolveMutex);
            if (_runningParentResolves.find(destinationPartId) == _runningParentResolves.end()) {
                if (!isParentMessageRouterSet()) {
                    // ignore, in case routing is not possible
                    return;
                }

                _runningParentResolves.insert(destinationPartId);
                parentResolveLock.unlock();

                std::function<void(const bool&)> onSuccess =
                        [destinationPartId,
                         thisWeakPtr = joynr::util::as_weak_ptr(
                                 std::dynamic_pointer_cast<LibJoynrMessageRouter>(
                                         shared_from_this()))](const bool& resolved) {
                            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                                if (resolved) {
                                    JOYNR_LOG_INFO(logger(),
                                                   "Got destination address for participant {}",
                                                   destinationPartId);
                                    WriteLocker lock1(thisSharedPtr->_messageQueueRetryLock);
                                    // save next hop in the routing table
                                    constexpr std::int64_t expiryDateMs =
                                            std::numeric_limits<std::int64_t>::max();
                                    const bool isSticky = false;
                                    bool addToRoutingTableSuccessful =
                                            thisSharedPtr->addToRoutingTable(
                                                    destinationPartId,
                                                    thisSharedPtr->_DEFAULT_IS_GLOBALLY_VISIBLE,
                                                    thisSharedPtr->_parentAddress,
                                                    expiryDateMs,
                                                    isSticky);
                                    if (addToRoutingTableSuccessful) {
                                        thisSharedPtr->sendQueuedMessages(
                                                destinationPartId,
                                                thisSharedPtr->_parentAddress,
                                                std::move(lock1));
                                    } else {
                                        JOYNR_LOG_ERROR(
                                                logger(),
                                                "Failed to add participant {} to routing table",
                                                destinationPartId);
                                    }
                                } else {
                                    JOYNR_LOG_ERROR(logger(),
                                                    "Failed to resolve next hop for participant {}",
                                                    destinationPartId);
                                }
                                thisSharedPtr->removeRunningParentResolvers(destinationPartId);
                            } else {
                                JOYNR_LOG_ERROR(
                                        logger(),
                                        "Failed to resolve next hop for participant {} because "
                                        "LibJoynrMessageRouter is no longer available",
                                        destinationPartId);
                            }
                        };

                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError =
                        [destinationPartId,
                         thisWeakPtr = joynr::util::as_weak_ptr(
                                 std::dynamic_pointer_cast<LibJoynrMessageRouter>(
                                         shared_from_this()))](
                                const joynr::exceptions::JoynrRuntimeException& error) {
                            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                                JOYNR_LOG_ERROR(logger(),
                                                "Failed to resolve next hop for participant {}: {}",
                                                destinationPartId,
                                                error.getMessage());
                                thisSharedPtr->removeRunningParentResolvers(destinationPartId);
                            }
                        };

                _parentRouter->resolveNextHopAsync(
                        destinationPartId, std::move(onSuccess), std::move(onError));
            }
            return;
        }
    }

    // If this point is reached, the message can be sent without delay
    for (std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress : destAddresses) {
        scheduleMessage(message, destAddress, tryCount);
    }
}

void LibJoynrMessageRouter::sendMessage(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::uint32_t tryCount)
{
    scheduleMessage(message, destAddress, tryCount);
}

bool LibJoynrMessageRouter::publishToGlobal(const ImmutableMessage& message)
{
    std::ignore = message;

    // LibJoynr always has to publish multicast messages to its cluster controller
    return true;
}

bool LibJoynrMessageRouter::isParentMessageRouterSet()
{
    if (!_parentRouter) {
        JOYNR_LOG_TRACE(logger(),
                        "Parent message router not set. Discard this message if it appears "
                        "during libJoynr initialization. It can be related to a configuration "
                        "problem. Check setting file.");
        return false;
    }
    return true;
}

void LibJoynrMessageRouter::addNextHopToParent(
        std::string participantId,
        bool isGloballyVisible,
        std::function<void(void)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (!isParentMessageRouterSet()) {
        // this special case happens if we get here while the routing proxy is being built.
        // consider this case as ok, the addNextHop() will be done at another place after
        // the proxy has been built.
        if (onSuccess) {
            onSuccess();
        }
        return;
    }

    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError](const exceptions::JoynrException& error) {
                if (onError) {
                    JOYNR_LOG_TRACE(logger(),
                                    "parentRouter->addNextHopAsync failed: {}",
                                    error.getMessage());
                    onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
                } else {
                    JOYNR_LOG_WARN(logger(),
                                   "Unable to report error (received by calling "
                                   "parentRouter->addNextHopAsync), since onError function is "
                                   "empty. Error message: {}",
                                   error.getMessage());
                }
            };

    // add to parent router
    if (auto mqttAddress =
                std::dynamic_pointer_cast<const joynr::system::RoutingTypes::MqttAddress>(
                        _incomingAddress)) {
        _parentRouter->addNextHopAsync(participantId,
                                       *mqttAddress,
                                       isGloballyVisible,
                                       std::move(onSuccess),
                                       std::move(onErrorWrapper));
    } else if (auto webSocketAddress = std::dynamic_pointer_cast<
                       const joynr::system::RoutingTypes::WebSocketAddress>(_incomingAddress)) {
        _parentRouter->addNextHopAsync(participantId,
                                       *webSocketAddress,
                                       isGloballyVisible,
                                       std::move(onSuccess),
                                       std::move(onErrorWrapper));
    } else if (auto webSocketClientAddress = std::dynamic_pointer_cast<
                       const joynr::system::RoutingTypes::WebSocketClientAddress>(
                       _incomingAddress)) {
        _parentRouter->addNextHopAsync(participantId,
                                       *webSocketClientAddress,
                                       isGloballyVisible,
                                       std::move(onSuccess),
                                       std::move(onErrorWrapper));
    } else if (auto udsClientAddress = std::dynamic_pointer_cast<
                       const joynr::system::RoutingTypes::UdsClientAddress>(_incomingAddress)) {
        _parentRouter->addNextHopAsync(participantId,
                                       *udsClientAddress,
                                       isGloballyVisible,
                                       std::move(onSuccess),
                                       std::move(onErrorWrapper));
    } else if (onError) {
        std::string errorMsg = "Unsupported incoming address - ";
        errorMsg += _incomingAddress ? _incomingAddress->toString() : " NULL pointer";
        JOYNR_LOG_ERROR(logger(), errorMsg);
        onError(joynr::exceptions::ProviderRuntimeException(errorMsg));
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to report error from addNextHopToParent, since onError function is "
                        "empty. Error: Unsupported incoming address - {}",
                        _incomingAddress ? _incomingAddress->toString() : " NULL pointer");
    }
}

bool LibJoynrMessageRouter::isValidForRoutingTable(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    assert(address);
    if (!address) {
        JOYNR_LOG_FATAL(
                logger(),
                "internal error in LibJoynrMessageRouter::isValidForRoutingTable: address is null");
        return false;
    }
    if (dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(address.get()) != nullptr ||
        dynamic_cast<const system::RoutingTypes::UdsAddress*>(address.get()) != nullptr ||
        dynamic_cast<const InProcessMessagingAddress*>(address.get()) != nullptr) {
        return true;
    }
    JOYNR_LOG_ERROR(logger(),
                    "An address which is neither of type WebSocketAddress/UdsAddress nor "
                    "InProcessMessagingAddress will not be used for libjoynr Routing Table: {}",
                    address->toString());
    return false;
}

bool LibJoynrMessageRouter::allowRoutingEntryUpdate(const routingtable::RoutingEntry& oldEntry,
                                                    const system::RoutingTypes::Address& newAddress)
{
    // precedence: InProcessAddress > WebSocketAddress/UdsAddress
    // > WebSocketClientAddress/UdsClientAddress > MqttAddress
    if (typeid(newAddress) == typeid(InProcessMessagingAddress)) {
        return true;
    }

    if (dynamic_cast<const InProcessMessagingAddress*>(oldEntry.address.get()) != nullptr) {
        return false;
    }

    if (typeid(newAddress) == typeid(system::RoutingTypes::WebSocketAddress) ||
        typeid(newAddress) == typeid(system::RoutingTypes::UdsAddress)) {
        return true;
    }

    if (dynamic_cast<const system::RoutingTypes::WebSocketAddress*>(oldEntry.address.get()) !=
                nullptr ||
        dynamic_cast<const system::RoutingTypes::UdsAddress*>(oldEntry.address.get()) != nullptr) {
        return false;
    }

    // this means old address is one of those addresses:
    // WebSocketClientAddress/UdsClientAddress or MqttAddress
    // new address of type WebSocketClientAddress/UdsClientAddress have precedence, therefore update
    if (typeid(newAddress) == typeid(system::RoutingTypes::WebSocketClientAddress) ||
        typeid(newAddress) == typeid(system::RoutingTypes::UdsClientAddress)) {
        return true;
    }

    if (dynamic_cast<const system::RoutingTypes::WebSocketClientAddress*>(oldEntry.address.get()) !=
                nullptr ||
        dynamic_cast<const system::RoutingTypes::UdsClientAddress*>(oldEntry.address.get()) !=
                nullptr) {
        return false;
    }

    // this means old address is MqttAddress, latest precedence. Update entry!
    if (typeid(newAddress) == typeid(system::RoutingTypes::MqttAddress)) {
        return true;
    }

    // do not update if address is unknown
    return false;
}

bool LibJoynrMessageRouter::canMessageBeTransmitted(std::shared_ptr<ImmutableMessage> message) const
{
    // No need for ACL check in LibJoynrMessageRouter
    std::ignore = message;
    return true;
}

void LibJoynrMessageRouter::addNextHop(
        const std::string& participantId,
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address,
        bool isGloballyVisible,
        const std::int64_t expiryDateMs,
        const bool isSticky,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    assert(address);
    WriteLocker lock(_messageQueueRetryLock);

    bool addToRoutingTableSuccessful =
            addToRoutingTable(participantId, isGloballyVisible, address, expiryDateMs, isSticky);
    if (addToRoutingTableSuccessful) {
        sendQueuedMessages(participantId, address, std::move(lock));

        addNextHopToParent(
                participantId, isGloballyVisible, std::move(onSuccess), std::move(onError));
    } else {
        JOYNR_LOG_WARN(logger(),
                       "Unable to addNextHop for participant {}, as addToRoutingTable "
                       "failed. Removing from routing table.");
        lock.unlock();

        if (onError) {
            onError(exceptions::ProviderRuntimeException(
                    "unable to addNextHop, as addToRoutingTable failed"));
        }
    }
}

void LibJoynrMessageRouter::removeNextHop(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    {
        WriteLocker lock(_routingTableLock);
        _routingTable.remove(participantId);
    }

    if (!isParentMessageRouterSet()) {
        if (onError) {
            onError(exceptions::ProviderRuntimeException(
                    "unable to removeNextHop since parentRouter is not available"));
        }
        return;
    }

    std::function<void(const exceptions::JoynrRuntimeException&)> onErrorWrapper =
            [onError = std::move(onError)](const exceptions::JoynrRuntimeException& error) {
                if (onError) {
                    JOYNR_LOG_TRACE(logger(),
                                    "parentRouter->removeNextHopAsync failed: {}",
                                    error.getMessage());
                    onError(exceptions::ProviderRuntimeException(error.getMessage()));
                } else {
                    JOYNR_LOG_WARN(logger(),
                                   "Unable to report error (received by calling "
                                   "parentRouter->removeNextHopAsync), since onError function is "
                                   "empty. Error message: {}",
                                   error.getMessage());
                }
            };

    // remove from parent router
    _parentRouter->removeNextHopAsync(
            participantId, std::move(onSuccess), std::move(onErrorWrapper));
}

void LibJoynrMessageRouter::addMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (!isParentMessageRouterSet()) {
        if (onError) {
            onError(exceptions::ProviderRuntimeException(
                    "unable to addMulticastReceiver since parentRouter is not available"));
        }
        return;
    }
    std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress;
    const auto routingEntry = getRoutingEntry(providerParticipantId);
    if (routingEntry) {
        providerAddress = routingEntry->address;
    }

    std::function<void()> onSuccessWrapper =
            [thisWeakPtr = joynr::util::as_weak_ptr(
                     std::dynamic_pointer_cast<LibJoynrMessageRouter>(shared_from_this())),
             multicastId,
             subscriberParticipantId,
             onSuccess = std::move(onSuccess)]() {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    thisSharedPtr->_multicastReceiverDirectory.registerMulticastReceiver(
                            multicastId, subscriberParticipantId);
                    JOYNR_LOG_TRACE(logger(),
                                    "added multicast receiver={} for multicastId={}",
                                    subscriberParticipantId,
                                    multicastId);
                    if (onSuccess) {
                        onSuccess();
                    }
                } else {
                    JOYNR_LOG_ERROR(logger(),
                                    "error adding multicast receiver={} for multicastId={} because "
                                    "LibJoynrMessageRouter is no longer available",
                                    subscriberParticipantId,
                                    multicastId);
                }
            };
    std::function<void(const exceptions::JoynrRuntimeException&)> onErrorWrapper =
            [onError = std::move(onError), subscriberParticipantId, multicastId](
                    const exceptions::JoynrRuntimeException& error) {
                JOYNR_LOG_ERROR(logger(),
                                "error adding multicast receiver={} for multicastId={}, error: {}",
                                subscriberParticipantId,
                                multicastId,
                                error.getMessage());
                if (onError) {
                    onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
                }
            };

    if (!providerAddress) {
        // try to resolve destination address via parent message router
        auto onResolved = [thisWeakPtr = joynr::util::as_weak_ptr(
                                   std::dynamic_pointer_cast<LibJoynrMessageRouter>(
                                           shared_from_this())),
                           multicastId,
                           subscriberParticipantId,
                           providerParticipantId,
                           onSuccessWrapper = std::move(onSuccessWrapper),
                           onErrorWrapper](const bool& resolved) mutable {
            if (resolved) {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
                    const bool isSticky = false;
                    bool addToRoutingTableSuccessful = thisSharedPtr->addToRoutingTable(
                            providerParticipantId,
                            thisSharedPtr->_DEFAULT_IS_GLOBALLY_VISIBLE,
                            thisSharedPtr->_parentAddress,
                            expiryDateMs,
                            isSticky);
                    if (addToRoutingTableSuccessful) {
                        thisSharedPtr->_parentRouter->addMulticastReceiverAsync(
                                multicastId,
                                subscriberParticipantId,
                                providerParticipantId,
                                std::move(onSuccessWrapper),
                                std::move(onErrorWrapper));
                    } else {
                        exceptions::ProviderRuntimeException exception(
                                "addToRoutingTable failed for providerParticipantId:"
                                "providerParticipantId");

                        onErrorWrapper(exception);
                    }
                } else {
                    exceptions::ProviderRuntimeException exception(
                            "No routing entry for multicast provider (providerParticipantId=" +
                            providerParticipantId +
                            ") found in parent router because "
                            "LibJoynrMessageRouter is no longer "
                            "available.");
                    onErrorWrapper(exception);
                }
            } else {
                exceptions::ProviderRuntimeException exception(
                        "No routing entry for multicast provider (providerParticipantId=" +
                        providerParticipantId + ") found in parent router.");
                onErrorWrapper(exception);
            }
        };
        auto onResolveError = [onErrorWrapper = std::move(onErrorWrapper), providerParticipantId](
                                      const joynr::exceptions::JoynrRuntimeException& error) {
            exceptions::ProviderRuntimeException exception(
                    "error resolving next hop for multicast provider (providerParticipantId=" +
                    providerParticipantId + "). Error from parent router: " + error.getMessage());
            onErrorWrapper(exception);
        };
        _parentRouter->resolveNextHopAsync(
                providerParticipantId, std::move(onResolved), std::move(onResolveError));
        return;
    }

    auto inProcessAddress =
            std::dynamic_pointer_cast<const joynr::InProcessMessagingAddress>(providerAddress);
    if (!inProcessAddress) {
        _parentRouter->addMulticastReceiverAsync(multicastId,
                                                 subscriberParticipantId,
                                                 providerParticipantId,
                                                 std::move(onSuccessWrapper),
                                                 std::move(onErrorWrapper));
    } else {
        onSuccessWrapper();
    }
}

void LibJoynrMessageRouter::removeMulticastReceiver(
        const std::string& multicastId,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    _multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, subscriberParticipantId);

    std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress;
    const auto routingEntry = getRoutingEntry(providerParticipantId);
    if (routingEntry) {
        providerAddress = routingEntry->address;
    }

    if (!providerAddress) {
        const std::string errorMsg =
                "No routing entry for multicast provider (providerParticipantId=" +
                providerParticipantId + ") found.";
        JOYNR_LOG_ERROR(logger(), errorMsg);
        if (onError) {
            onError(exceptions::ProviderRuntimeException("unable to removeMulticastReceiver. " +
                                                         errorMsg));
        }
        return;
    }

    if (!isParentMessageRouterSet()) {
        if (onError) {
            onError(exceptions::ProviderRuntimeException(
                    "unable to removeMulticastReceiver since parentRouter is not available"));
        }
        return;
    }

    std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
            [onError = std::move(onError), subscriberParticipantId, multicastId](
                    const exceptions::JoynrException& error) {
                JOYNR_LOG_ERROR(
                        logger(),
                        "error removing multicast receiver={} for multicastId={}, error: {}",
                        subscriberParticipantId,
                        multicastId,
                        error.getMessage());
                if (onError) {
                    onError(joynr::exceptions::ProviderRuntimeException(error.getMessage()));
                }
            };
    _parentRouter->removeMulticastReceiverAsync(multicastId,
                                                subscriberParticipantId,
                                                providerParticipantId,
                                                std::move(onSuccess),
                                                std::move(onErrorWrapper));
}

void LibJoynrMessageRouter::removeRunningParentResolvers(const std::string& destinationPartId)
{
    std::lock_guard<std::mutex> lock(_parentResolveMutex);
    if (_runningParentResolves.find(destinationPartId) != _runningParentResolves.cend()) {
        _runningParentResolves.erase(destinationPartId);
    }
}

} // namespace joynr

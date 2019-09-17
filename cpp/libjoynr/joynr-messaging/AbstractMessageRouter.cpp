/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
#include "joynr/AbstractMessageRouter.h"

#include <cassert>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <functional>
#include <limits>
#include <sstream>

#include <boost/asio/io_service.hpp>
#include <spdlog/fmt/fmt.h>

#include "joynr/IMessagingStub.h"
#include "joynr/IMessagingStubFactory.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Message.h"
#include "joynr/MessageQueue.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/access-control/IAccessController.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/Util.h"
#include "libjoynrclustercontroller/include/joynr/ITransportStatus.h"

namespace joynr
{

//------ AbstractMessageRouter ---------------------------------------------------------
AbstractMessageRouter::AbstractMessageRouter(
        MessagingSettings& messagingSettings,
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        bool persistRoutingTable,
        std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
        std::unique_ptr<MessageQueue<std::string>> messageQueue,
        std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> transportNotAvailableQueue)
        : IMessageRouter(),
          enable_shared_from_this<AbstractMessageRouter>(),
          _routingTable(messagingSettings.getCapabilitiesDirectoryParticipantId()),
          _routingTableLock(),
          _multicastReceiverDirectory(),
          _messagingSettings(messagingSettings),
          _persistRoutingTable(persistRoutingTable),
          _messagingStubFactory(std::move(messagingStubFactory)),
          _messageScheduler(std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                                         "AbstractMessageRouter",
                                                                         ioService)),
          _messageQueue(std::move(messageQueue)),
          _messageQueueRetryLock(),
          _transportNotAvailableQueue(std::move(transportNotAvailableQueue)),
          _transportAvailabilityMutex(),
          _routingTableFileName(),
          _addressCalculator(std::move(addressCalculator)),
          _messageQueueCleanerTimer(ioService),
          _messageQueueCleanerTimerPeriodMs(std::chrono::milliseconds(1000)),
          _routingTableCleanerTimer(ioService),
          _transportStatuses(std::move(transportStatuses)),
          _isShuttingDown(false),
          _numberOfRoutedMessages(0),
          _maxAclRetryIntervalMs(
                  60 * 60 *
                  1000) // Max retry value is empirical and should practically fit many use-case
{
}

AbstractMessageRouter::~AbstractMessageRouter()
{
    // make sure shutdown() has been called earlier
    assert(_isShuttingDown);
}

void AbstractMessageRouter::init()
{
    activateMessageCleanerTimer();
    activateRoutingTableCleanerTimer();
    registerTransportStatusCallbacks();
}

void AbstractMessageRouter::shutdown()
{
    // make sure shutdown() code is executed only once
    bool previousValue =
            std::atomic_exchange_explicit(&_isShuttingDown, true, std::memory_order_acquire);
    assert(!previousValue);
    // bail out in case assert is disabled
    if (previousValue) {
        return;
    }

    _messageQueueCleanerTimer.cancel();
    _routingTableCleanerTimer.cancel();
    _messageScheduler->shutdown();
    if (_messagingStubFactory) {
        _messagingStubFactory->shutdown();
    }
}

void AbstractMessageRouter::addProvisionedNextHop(
        std::string participantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
        bool isGloballyVisible)
{
    assert(address);
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = true;
    addToRoutingTable(participantId, isGloballyVisible, address, expiryDateMs, isSticky);
}

AbstractMessageRouter::AddressUnorderedSet AbstractMessageRouter::lookupAddresses(
        const std::unordered_set<std::string>& participantIds)
{
    // Caution: Do not lock routingTableLock here, it must have been locked from outside
    // this method gets called from getDestinationAddresses()
    AbstractMessageRouter::AddressUnorderedSet addresses;

    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress;
    for (const auto& participantId : participantIds) {
        const auto routingEntry = _routingTable.lookupRoutingEntryByParticipantId(participantId);
        if (routingEntry) {
            destAddress = routingEntry->_address;
            addresses.insert(destAddress);
        }
    }
    assert(addresses.size() <= participantIds.size());
    return addresses;
}

AbstractMessageRouter::AddressUnorderedSet AbstractMessageRouter::getDestinationAddresses(
        const ImmutableMessage& message,
        const ReadLocker& messageQueueRetryReadLock)
{
    assert(messageQueueRetryReadLock.owns_lock());
    ReadLocker lock(_routingTableLock);
    AbstractMessageRouter::AddressUnorderedSet addresses;
    if (message.getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        const std::string& multicastId = message.getRecipient();

        // lookup local multicast receivers
        std::unordered_set<std::string> multicastReceivers =
                _multicastReceiverDirectory.getReceivers(multicastId);
        addresses = lookupAddresses(multicastReceivers);

        // add global transport address if message is NOT received from global
        // AND provider is globally visible
        if (!message.isReceivedFromGlobal() && _addressCalculator && publishToGlobal(message)) {
            std::vector<std::shared_ptr<const joynr::system::RoutingTypes::Address>>
                    globalTransportVector = _addressCalculator->compute(message);
            addresses.insert(globalTransportVector.begin(), globalTransportVector.end());
        }
    } else {
        const std::string& destinationPartId = message.getRecipient();
        boost::optional<joynr::routingtable::RoutingEntry> routingEntry;
        auto customHeaders = message.getCustomHeaders();
        auto customHeaderGbidEntry = customHeaders.find(joynr::Message::CUSTOM_HEADER_GBID_KEY());
        if (customHeaderGbidEntry != customHeaders.end()) {
            std::string gbid = customHeaderGbidEntry->second;
            routingEntry =
                    _routingTable.lookupRoutingEntryByParticipantIdAndGbid(destinationPartId, gbid);
        } else {
            routingEntry = _routingTable.lookupRoutingEntryByParticipantId(destinationPartId);
        }
        if (routingEntry) {
            addresses.insert(routingEntry->_address);
        }
    }
    return addresses;
}

void AbstractMessageRouter::checkExpiryDate(const ImmutableMessage& message)
{
    const auto now = TimePoint::now();
    if (now > message.getExpiryDate()) {
        const std::string errorMessage =
                fmt::format("Received expired message (now={}). Dropping the message {}",
                            now.toMilliseconds(),
                            message.getTrackingInfo());
        JOYNR_LOG_WARN(logger(), errorMessage);
        throw exceptions::JoynrMessageNotSentException(errorMessage);
    }
}

void AbstractMessageRouter::route(std::shared_ptr<ImmutableMessage> message, std::uint32_t tryCount)
{
    assert(_messagingStubFactory);
    assert(message);
    _numberOfRoutedMessages++;
    checkExpiryDate(*message);
    routeInternal(std::move(message), tryCount);
}

// following method may be overridden by subclass
void AbstractMessageRouter::setToKnown(const std::string& participantId)
{
    std::ignore = participantId;
    JOYNR_LOG_TRACE(logger(), "AbstractMessageRouter::setToKnown");
}

void AbstractMessageRouter::sendQueuedMessages(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    JOYNR_LOG_TRACE(logger(), "sendMessages: sending messages for {}", address->toString());
    std::unordered_set<std::string> participantIdSet;
    {
        ReadLocker lock(_routingTableLock);
        participantIdSet = _routingTable.lookupParticipantIdsByAddress(address);
    }
    if (participantIdSet.size() > 0) {
        WriteLocker lock(_messageQueueRetryLock);
        for (const auto& participantId : participantIdSet) {
            sendQueuedMessages(participantId, address, lock);
        }
    }
}
void AbstractMessageRouter::doAccessControlCheckOrScheduleMessage(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::uint32_t tryCount)
{
    std::ignore = message;
    std::ignore = destAddress;
    std::ignore = tryCount;
    // no implementation needed when this method is called by LibjoynrMessageRouter
}

void AbstractMessageRouter::scheduleMessage(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::uint32_t tryCount,
        std::chrono::milliseconds delay)
{
    for (const auto& transportStatus : _transportStatuses) {
        if (transportStatus->isReponsibleFor(destAddress)) {
            if (!transportStatus->isAvailable()) {
                // We need to lock the mutex to ensure that the queue isn't processed right now.
                std::lock_guard<std::mutex> lock(_transportAvailabilityMutex);
                if (!transportStatus->isAvailable()) {
                    JOYNR_LOG_TRACE(logger(),
                                    "Transport not available. Message queued: {}",
                                    message->getTrackingInfo());

                    _transportNotAvailableQueue->queueMessage(transportStatus, std::move(message));
                    return;
                }
            }
        }
    }

    auto stub = _messagingStubFactory->create(destAddress);
    if (stub) {
        _messageScheduler->schedule(std::make_shared<MessageRunnable>(std::move(message),
                                                                      std::move(stub),
                                                                      std::move(destAddress),
                                                                      shared_from_this(),
                                                                      tryCount),
                                    delay);
    } else {
        if (message->getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
            // do not queue a multicast message since it would get stored under
            // the multicast participantId so that it will never be unqueued
            // again.
            JOYNR_LOG_TRACE(logger(),
                            "Multicast message {} could not be sent to recipient, {}. Stub "
                            "creation failed. => Discarding "
                            "message.",
                            message->getTrackingInfo(),
                            destAddress->toString());
            removeMulticastReceiver(message->getRecipient(), destAddress, message->getSender());
        } else if (message->getType() == Message::VALUE_MESSAGE_TYPE_PUBLICATION()) {
            JOYNR_LOG_TRACE(logger(),
                            "Publication message {} could not be sent to recipient, {}. Stub "
                            "creation failed. => Discarding "
                            "message & attempting to stop publication.",
                            message->getTrackingInfo(),
                            destAddress->toString());
            stopSubscription(message);
        } else {
            JOYNR_LOG_WARN(logger(),
                           "Message {} could not be sent to recipient, {}. Stub "
                           "creation failed. => Queueing "
                           "message.",
                           message->getTrackingInfo(),
                           destAddress->toString());
            ReadLocker lock(_messageQueueRetryLock);

            // save the message for later delivery
            queueMessage(std::move(message), lock);
        }
    }
}

void AbstractMessageRouter::activateMessageCleanerTimer()
{
    _messageQueueCleanerTimer.expiresFromNow(_messageQueueCleanerTimerPeriodMs);
    _messageQueueCleanerTimer
            .asyncWait([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
                    const boost::system::error_code& errorCode) {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    thisSharedPtr->onMessageCleanerTimerExpired(thisSharedPtr, errorCode);
                }
            });
}

void AbstractMessageRouter::activateRoutingTableCleanerTimer()
{
    _routingTableCleanerTimer.expiresFromNow(
            std::chrono::milliseconds(_messagingSettings.getRoutingTableCleanupIntervalMs()));
    _routingTableCleanerTimer
            .asyncWait([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
                    const boost::system::error_code& errorCode) {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    thisSharedPtr->onRoutingTableCleanerTimerExpired(errorCode);
                }
            });
}

void AbstractMessageRouter::registerTransportStatusCallbacks()
{
    for (auto& transportStatus : _transportStatuses) {
        transportStatus->setAvailabilityChangedCallback(
                [ thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()), transportStatus ](
                        bool isAvailable) {
                    if (auto thisSharedPtr = thisWeakPtr.lock()) {
                        if (isAvailable) {
                            thisSharedPtr->rescheduleQueuedMessagesForTransport(transportStatus);
                        }
                    }
                });
    }
}

void AbstractMessageRouter::rescheduleQueuedMessagesForTransport(
        std::shared_ptr<ITransportStatus> transportStatus)
{
    // We need to lock the mutex to prevent other threads from adding new content for the queue
    // while we process it.
    std::lock_guard<std::mutex> lock(_transportAvailabilityMutex);
    while (auto nextImmutableMessage =
                   _transportNotAvailableQueue->getNextMessageFor(transportStatus)) {
        try {
            route(nextImmutableMessage);
        } catch (const exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_DEBUG(logger(),
                            "could not route queued message {} due to '{}'",
                            nextImmutableMessage->getTrackingInfo(),
                            e.getMessage());
        }
    }
}

void AbstractMessageRouter::onMessageCleanerTimerExpired(
        std::shared_ptr<AbstractMessageRouter> thisSharedPtr,
        const boost::system::error_code& errorCode)
{
    if (!errorCode) {
        std::stringstream thisAsHexString;
        thisAsHexString << static_cast<void*>(thisSharedPtr.get());
        JOYNR_LOG_INFO(logger(),
                       "#routedMessages[this={}]: {}",
                       thisAsHexString.str(),
                       thisSharedPtr->_numberOfRoutedMessages);
        WriteLocker lock(thisSharedPtr->_messageQueueRetryLock);
        thisSharedPtr->_messageQueue->removeOutdatedMessages();
        thisSharedPtr->_transportNotAvailableQueue->removeOutdatedMessages();
        thisSharedPtr->activateMessageCleanerTimer();
    } else if (errorCode != boost::system::errc::operation_canceled) {
        JOYNR_LOG_ERROR(logger(),
                        "Failed to schedule timer to remove outdated messages: {}",
                        errorCode.message());
    }
}

void AbstractMessageRouter::onRoutingTableCleanerTimerExpired(
        const boost::system::error_code& errorCode)
{
    JOYNR_LOG_DEBUG(logger(), "AbstractMessageRouter::onRoutingTableCleanerTimerExpired");

    if (!errorCode) {
        WriteLocker lock(_routingTableLock);
        _routingTable.purge();
        activateRoutingTableCleanerTimer();
    } else if (errorCode != boost::system::errc::operation_canceled) {
        JOYNR_LOG_ERROR(logger(),
                        "Failed to schedule timer to remove outdated routing table entries: {}",
                        errorCode.message());
    }
}

void AbstractMessageRouter::queueMessage(std::shared_ptr<ImmutableMessage> message,
                                         const ReadLocker& messageQueueRetryReadLock)
{
    assert(messageQueueRetryReadLock.owns_lock());
    JOYNR_LOG_TRACE(logger(), "message queued: {}", message->getTrackingInfo());
    std::string recipient = message->getRecipient();
    _messageQueue->queueMessage(std::move(recipient), std::move(message));
}

void AbstractMessageRouter::loadRoutingTable(std::string fileName)
{

    if (!_persistRoutingTable) {
        return;
    }

    // update reference file
    if (fileName != _routingTableFileName) {
        _routingTableFileName = std::move(fileName);
    }

    if (!joynr::util::fileExists(_routingTableFileName)) {
        return;
    }

    WriteLocker lock(_routingTableLock);
    try {
        joynr::serializer::deserializeFromJson(
                _routingTable, joynr::util::loadStringFromFile(_routingTableFileName));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(), "could not deserialize from JSON: {}", ex.what());
    }
}

void AbstractMessageRouter::saveRoutingTable()
{
    if (!_persistRoutingTable) {
        return;
    }
    WriteLocker lock(_routingTableLock);
    try {
        joynr::util::saveStringToFile(
                _routingTableFileName, joynr::serializer::serializeToJson(_routingTable));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger(), ex.what());
    }
}

bool AbstractMessageRouter::addToRoutingTable(
        std::string participantId,
        bool isGloballyVisible,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
        std::int64_t expiryDateMs,
        bool isSticky)
{
    if (!isValidForRoutingTable(address)) {
        JOYNR_LOG_TRACE(logger(),
                        "participantId={} has an unsupported address within this process",
                        participantId);
        return false;
    }
    {
        WriteLocker lock(_routingTableLock);
        auto oldRoutingEntry = _routingTable.lookupRoutingEntryByParticipantId(participantId);
        if (oldRoutingEntry) {
            const bool addressOrVisibilityOfRoutingEntryChanged =
                    (!oldRoutingEntry->_address->equals(*address, joynr::util::MAX_ULPS)) ||
                    (oldRoutingEntry->_isGloballyVisible != isGloballyVisible);
            if (addressOrVisibilityOfRoutingEntryChanged) {
                if (oldRoutingEntry->_isSticky) {
                    JOYNR_LOG_ERROR(
                            logger(),
                            "unable to update participantId={} in routing table, since "
                            "the participantId is already associated with STICKY routing entry {}.",
                            participantId,
                            oldRoutingEntry->toString());
                    return false;
                }
                if (!allowRoutingEntryUpdate(*oldRoutingEntry, *address)) {
                    JOYNR_LOG_WARN(logger(),
                                   "unable to update participantId={} in routing table, since "
                                   "the participantId is already associated with routing entry {}.",
                                   participantId,
                                   oldRoutingEntry->toString());
                    return false;
                }
                JOYNR_LOG_TRACE(
                        logger(), "updating participantId={} in routing table", participantId);
            } else {
                JOYNR_LOG_TRACE(
                        logger(),
                        "Updating expiryDate and sticky-flag of participantId={} in routing table.",
                        participantId);
            }
            // keep longest lifetime
            if (oldRoutingEntry->_expiryDateMs > expiryDateMs) {
                expiryDateMs = oldRoutingEntry->_expiryDateMs;
            }
            if (oldRoutingEntry->_isSticky) {
                isSticky = true;
            }
        }
        // manual removal of old entry is not required here since routingTable.add() automatically
        // calls replace in case insert fails
        _routingTable.add(
                std::move(participantId), isGloballyVisible, address, expiryDateMs, isSticky);
    }
    const joynr::InProcessMessagingAddress* inprocessAddress =
            dynamic_cast<const joynr::InProcessMessagingAddress*>(address.get());
    if (!inprocessAddress) {
        saveRoutingTable();
    }

    return true;
}

std::uint64_t AbstractMessageRouter::getNumberOfRoutedMessages() const
{
    return _numberOfRoutedMessages;
}

std::chrono::milliseconds AbstractMessageRouter::createDelayWithExponentialBackoff(
        std::uint32_t sendMsgRetryIntervalMs,
        std::uint32_t tryCount) const
{
    JOYNR_LOG_TRACE(logger(),
                    "Number of tries to reach the provider during the permitted delay: {} ",
                    tryCount);
    errno = 0;
    std::feclearexcept(FE_ALL_EXCEPT);
    std::uint64_t retryInterval = static_cast<std::uint64_t>(
            std::llround(std::pow(2, tryCount) * sendMsgRetryIntervalMs));
    const bool overflowOccur = (errno != 0 || std::fetestexcept(FE_INVALID | FE_DIVBYZERO |
                                                                FE_OVERFLOW | FE_UNDERFLOW));

    if (overflowOccur || (retryInterval > _maxAclRetryIntervalMs)) {
        retryInterval = _maxAclRetryIntervalMs;
        JOYNR_LOG_TRACE(logger(),
                        "Set exponential backoff delay in ms to {} since the maxAclRetryIntervalMs "
                        "is {}",
                        retryInterval,
                        _maxAclRetryIntervalMs);
    }
    JOYNR_LOG_TRACE(
            logger(), "New exponential backoff delay of the message in ms: {}", retryInterval);
    return std::chrono::milliseconds(retryInterval);
}

void AbstractMessageRouter::removeMulticastReceiver(
        const std::string& multicastId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        const std::string& providerParticipantId)
{
    // empty implementation
    std::ignore = multicastId;
    std::ignore = destAddress;
    std::ignore = providerParticipantId;
}

void AbstractMessageRouter::stopSubscription(std::shared_ptr<ImmutableMessage> message)
{
    // empty implementation
    std::ignore = message;
}

/**
 * IMPLEMENTATION of MessageRunnable class
 */

MessageRunnable::MessageRunnable(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<IMessagingStub> messagingStub,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::weak_ptr<AbstractMessageRouter> messageRouter,
        std::uint32_t tryCount)
        : Runnable(),
          ObjectWithDecayTime(message->getExpiryDate()),
          _message(message),
          _messagingStub(messagingStub),
          _destAddress(destAddress),
          _messageRouter(messageRouter),
          _tryCount(tryCount)
{
}

void MessageRunnable::shutdown()
{
}

void MessageRunnable::run()
{
    if (!isExpired()) {
        // TODO is it safe to capture (this) here? rather capture members by value!
        auto onFailure = [thisWeakPtr = joynr::util::as_weak_ptr(
                                  std::dynamic_pointer_cast<MessageRunnable>(shared_from_this()))](
                const exceptions::JoynrRuntimeException& e)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                try {
                    exceptions::JoynrDelayMessageException& delayException =
                            dynamic_cast<exceptions::JoynrDelayMessageException&>(
                                    const_cast<exceptions::JoynrRuntimeException&>(e));
                    std::chrono::milliseconds delay = delayException.getDelayMs();

                    if (auto messageRouterSharedPtr = thisSharedPtr->_messageRouter.lock()) {
                        JOYNR_LOG_TRACE(
                                logger(),
                                "Rescheduling message after error: message {}, new delay {}ms, "
                                "reason: {}",
                                thisSharedPtr->_message->getTrackingInfo(),
                                delay.count(),
                                e.getMessage());
                        messageRouterSharedPtr->scheduleMessage(thisSharedPtr->_message,
                                                                thisSharedPtr->_destAddress,
                                                                thisSharedPtr->_tryCount + 1,
                                                                delay);
                    } else {
                        JOYNR_LOG_ERROR(logger(),
                                        "Message {} could not be sent! reason: messageRouter "
                                        "not available",
                                        thisSharedPtr->_message->getTrackingInfo());
                    }
                } catch (const std::bad_cast&) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Message {} could not be sent! reason: {}",
                                    thisSharedPtr->_message->getTrackingInfo(),
                                    e.getMessage());
                }
            } else {
                JOYNR_LOG_ERROR(logger(),
                                "Message could not be sent! reason: MessageRunnable not available");
            }
        };

        auto messageRouterSharedPtr = _messageRouter.lock();
        if (!messageRouterSharedPtr) {
            JOYNR_LOG_ERROR(logger(),
                            "Message {} could not be sent! reason: messageRouter "
                            "not available",
                            _message->getTrackingInfo());
            return;
        }

        if (messageRouterSharedPtr->canMessageBeTransmitted(_message)) {
            _messagingStub->transmit(_message, onFailure);
        } else {
            messageRouterSharedPtr->doAccessControlCheckOrScheduleMessage(
                    _message, _destAddress, _tryCount);
        }

    } else {
        JOYNR_LOG_ERROR(logger(), "Message {} expired: dropping!", _message->getTrackingInfo());
    }
}

} // namespace joynr

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
#include <deque>
#include <limits>
#include <sstream>

#include <boost/asio/io_service.hpp>

#include "joynr/IMessageSender.h"
#include "joynr/IMessagingStub.h"
#include "joynr/IMessagingStubFactory.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/ITransportStatus.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Message.h"
#include "joynr/MessageQueue.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/TimePoint.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

//------ AbstractMessageRouter ---------------------------------------------------------
AbstractMessageRouter::AbstractMessageRouter(
        MessagingSettings& messagingSettings,
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
        std::unique_ptr<MessageQueue<std::string>> messageQueue,
        std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> transportNotAvailableQueue,
        const std::vector<std::string>& knownGbids)
        : IMessageRouter(),
          enable_shared_from_this<AbstractMessageRouter>(),
          _routingTable(messagingSettings.getCapabilitiesDirectoryParticipantId(), knownGbids),
          _routingTableLock(),
          _multicastReceiverDirectory(),
          _messagingSettings(messagingSettings),
          _messagingStubFactory(std::move(messagingStubFactory)),
          _messageScheduler(std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                                         "AbstractMessageRouter",
                                                                         ioService)),
          _messageQueue(std::move(messageQueue)),
          _messageSender(),
          _messageQueueRetryLock(),
          _transportNotAvailableQueue(std::move(transportNotAvailableQueue)),
          _transportAvailabilityMutex(),
          _addressCalculator(std::move(addressCalculator)),
          _messageQueueCleanerTimer(ioService),
          _messageQueueCleanerTimerPeriodMs(std::chrono::milliseconds(1000)),
          _routingTableCleanerTimer(ioService),
          _transportStatuses(std::move(transportStatuses)),
          _printRoutedMessages(false),
          _routedMessagePrintIntervalS(10u),
          _isShuttingDown(false),
          _numberOfRoutedMessages(0),
          _maxAclRetryIntervalMs(
                  60 * 60 *
                  1000), // Max retry value is empirical and should practically fit many use-case
          _messageCleaningCycleCounter(0)
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

void AbstractMessageRouter::removeRoutingEntries(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    JOYNR_LOG_TRACE(logger(), "removeRoutingEntries: removing entries for {}", address->toString());
    WriteLocker lock(_routingTableLock);
    auto participantIdSet = _routingTable.lookupParticipantIdsByAddress(address);
    for (const auto& participantId : participantIdSet) {
        _routingTable.remove(participantId);
    }
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
            destAddress = routingEntry->address;
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
    std::ignore = messageQueueRetryReadLock;
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
            addresses.insert(routingEntry->address);
        }
    }
    return addresses;
}

void AbstractMessageRouter::checkExpiryDate(const ImmutableMessage& message)
{
    const auto now = TimePoint::now();
    if (now > message.getExpiryDate()) {
        const std::string errorMessage =
                fmt::format("Message expired (now={}). Dropping the message {}",
                            now.toMilliseconds(),
                            message.getTrackingInfo());
        throw exceptions::JoynrMessageExpiredException(errorMessage);
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
    if (participantIdSet.empty()) {
        return;
    }
    for (const auto& participantId : participantIdSet) {
        WriteLocker lock(_messageQueueRetryLock);
        sendQueuedMessages(participantId, address, std::move(lock));
    }
}

void AbstractMessageRouter::onMsgsDropped(
        std::deque<std::shared_ptr<ImmutableMessage>>& droppedMessages)
{
    while (droppedMessages.size() > 0) {
        auto droppedMessage = droppedMessages.back();
        droppedMessages.pop_back();
        if (droppedMessage) {
            if (droppedMessage->getType() != Message::VALUE_MESSAGE_TYPE_REQUEST()) {
                continue;
            }

            // deserialize the request
            Request request;
            try {
                joynr::serializer::deserializeFromJson(
                        request, droppedMessage->getUnencryptedBody());
            } catch (const std::invalid_argument& e) {

                JOYNR_LOG_ERROR(logger(),
                                "Failed to prepare reply message with error for "
                                "{}: unable to deserialize request object - error: {}",
                                droppedMessage->toLogMessage(),
                                e.what());
                continue;
            }

            Reply reply;
            const std::string& requestReplyId = request.getRequestReplyId();
            TimePoint requestExpiryDate = droppedMessage->getExpiryDate();
            reply.setRequestReplyId(requestReplyId);
            auto error = std::make_shared<joynr::exceptions::ProviderRuntimeException>(
                    "Request Message {" + droppedMessage->getTrackingInfo() +
                    "} dropped due to the exhaustion of the message queue.");

            reply.setError(error);

            std::string senderId = droppedMessage->getSender();
            std::string receiverId = droppedMessage->getRecipient();

            const std::chrono::milliseconds ttl = requestExpiryDate.relativeFromNow();
            MessagingQos messagingQos(static_cast<std::uint64_t>(ttl.count()));
            messagingQos.setCompress(droppedMessage->isCompressed());
            const boost::optional<std::string> effort = droppedMessage->getEffort();
            if (effort) {
                try {
                    messagingQos.setEffort(MessagingQosEffort::getEnum(*effort));
                } catch (const std::invalid_argument&) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Request message (id: {}) uses invalid effort: {}. Using "
                                    "default effort for reply message.",
                                    droppedMessage->getId(),
                                    *effort);
                }
            }
            if (auto messageSenderSharedPtr = _messageSender.lock()) {
                JOYNR_LOG_TRACE(logger(),
                                "Sending fake reply for the dropped request message. "
                                "RequestReplyId: {}, "
                                "sender of droppedMsg: {}, receiver of droppedMsg: {}",
                                requestReplyId,
                                senderId,
                                receiverId);
                messageSenderSharedPtr->sendReply(
                        receiverId, // the receiver of the dropped messasge is the sender of the
                                    // reply
                        senderId,   // the sender of the dropped message is the receiver of reply
                        messagingQos,
                        droppedMessage->getPrefixedCustomHeaders(),
                        std::move(reply));
            }
        }
    }
}

void AbstractMessageRouter::setMessageSender(std::weak_ptr<IMessageSender> messageSender)
{
    this->_messageSender = std::move(messageSender);
}

void AbstractMessageRouter::sendQueuedMessages(
        const std::string& destinationPartId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
        WriteLocker&& messageQueueRetryWriteLock)
{
    assert(messageQueueRetryWriteLock.owns_lock());
    JOYNR_LOG_TRACE(logger(),
                    "sendMessages: sending messages for destinationPartId {} and {}",
                    destinationPartId,
                    address->toString());
    std::vector<std::shared_ptr<ImmutableMessage>> messages;
    while (auto item = _messageQueue->getNextMessageFor(destinationPartId)) {
        messages.push_back(item);
    }
    // _messageQueueRetryLock must be released before calling sendMessage
    // to prevent deadlock in case the message cannot be sent (e.g. if the stub
    // creation fails) and it has to be queued again
    messageQueueRetryWriteLock.unlock();
    for (auto message : messages) {
        sendMessage(message, address);
    }
}

void AbstractMessageRouter::scheduleMessage(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::uint32_t tryCount,
        std::chrono::milliseconds delay)
{
    for (const auto& transportStatus : _transportStatuses) {
        if (transportStatus->isReponsibleFor(destAddress)) {
            std::deque<std::shared_ptr<ImmutableMessage>> droppedMessagesToBeReplied;
            if (!transportStatus->isAvailable()) {
                // We need to lock the mutex to ensure that the queue isn't processed right now.
                std::lock_guard<std::mutex> lock(_transportAvailabilityMutex);
                if (!transportStatus->isAvailable()) {
                    JOYNR_LOG_TRACE(logger(),
                                    "Transport not available. Message queued: {}",
                                    message->getTrackingInfo());

                    droppedMessagesToBeReplied = _transportNotAvailableQueue->queueMessage(
                            transportStatus, std::move(message));
                    if (droppedMessagesToBeReplied.empty()) {
                        return;
                    }
                }
            }
            // This can only contain entries if the Transport was not available
            if (!droppedMessagesToBeReplied.empty()) {
                onMsgsDropped(droppedMessagesToBeReplied);
                return;
            }
            break;
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
            JOYNR_LOG_ERROR(logger(),
                            "Multicast message {} could not be sent to recipient, {}. Stub "
                            "creation failed. => Discarding "
                            "message.",
                            message->getTrackingInfo(),
                            destAddress->toString());
            removeUnreachableMulticastReceivers(
                    message->getRecipient(), destAddress, message->getSender());
        } else if (message->getType() == Message::VALUE_MESSAGE_TYPE_PUBLICATION()) {
            JOYNR_LOG_ERROR(logger(),
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
    _messageQueueCleanerTimer.asyncWait(
            [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
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
    _routingTableCleanerTimer.asyncWait(
            [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
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
                [thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                 transportStatusWeakPtr =
                         joynr::util::as_weak_ptr(transportStatus)](bool isAvailable) {
                    if (auto thisSharedPtr = thisWeakPtr.lock()) {
                        if (isAvailable) {
                            if (auto transportStatusSharedPtr = transportStatusWeakPtr.lock()) {
                                thisSharedPtr->rescheduleQueuedMessagesForTransport(
                                        transportStatusSharedPtr);
                            }
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
        } catch (const exceptions::JoynrMessageExpiredException& e) {
            JOYNR_LOG_WARN(logger(), "could not route queued message. {}", e.getMessage());
        } catch (const exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_ERROR(logger(),
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
        _messageCleaningCycleCounter++;
        std::stringstream thisAsHexString;
        thisAsHexString << static_cast<void*>(thisSharedPtr.get());
        if (_printRoutedMessages &&
            _messageCleaningCycleCounter % _routedMessagePrintIntervalS == 0) {
            JOYNR_LOG_INFO(logger(),
                           "#routedMessages[this={}]: {}",
                           thisAsHexString.str(),
                           thisSharedPtr->_numberOfRoutedMessages);
        }
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
                                         ReadLocker& messageQueueRetryReadLock)
{
    assert(messageQueueRetryReadLock.owns_lock());
    std::ignore = messageQueueRetryReadLock;
    JOYNR_LOG_TRACE(logger(), "message queued: {}", message->getTrackingInfo());
    std::string recipient = message->getRecipient();
    auto droppedMessagesToBeReplied =
            _messageQueue->queueMessage(std::move(recipient), std::move(message));
    messageQueueRetryReadLock.unlock();
    if (!droppedMessagesToBeReplied.empty()) {
        onMsgsDropped(droppedMessagesToBeReplied);
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
                    (!oldRoutingEntry->address->equals(*address, joynr::util::MAX_ULPS)) ||
                    (oldRoutingEntry->isGloballyVisible != isGloballyVisible);
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

void AbstractMessageRouter::removeUnreachableMulticastReceivers(
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

boost::optional<routingtable::RoutingEntry> AbstractMessageRouter::getRoutingEntry(
        const std::string& participantId)
{
    ReadLocker lock(_routingTableLock);
    return _routingTable.lookupRoutingEntryByParticipantId(participantId);
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
        auto onFailure = [messageRouter = _messageRouter,
                          message = _message,
                          destAddress = _destAddress,
                          tryCount = _tryCount](const exceptions::JoynrRuntimeException& e) {
            try {
                exceptions::JoynrDelayMessageException& delayException =
                        dynamic_cast<exceptions::JoynrDelayMessageException&>(
                                const_cast<exceptions::JoynrRuntimeException&>(e));
                std::chrono::milliseconds delay = delayException.getDelayMs();

                if (auto messageRouterSharedPtr = messageRouter.lock()) {
                    JOYNR_LOG_TRACE(logger(),
                                    "Rescheduling message after error: message {}, new delay {}ms, "
                                    "reason: {}",
                                    message->getTrackingInfo(),
                                    delay.count(),
                                    e.getMessage());
                    messageRouterSharedPtr->scheduleMessage(
                            message, destAddress, tryCount + 1, delay);
                } else {
                    JOYNR_LOG_ERROR(logger(),
                                    "Message {} could not be sent! reason: messageRouter "
                                    "not available",
                                    message->getTrackingInfo());
                }
            } catch (const std::bad_cast&) {
                JOYNR_LOG_ERROR(logger(),
                                "Message {} could not be sent! reason: {}",
                                message->getTrackingInfo(),
                                e.getMessage());
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
            messageRouterSharedPtr->sendMessage(_message, _destAddress, _tryCount);
        }

    } else {
        const auto now = TimePoint::now();
        JOYNR_LOG_WARN(logger(),
                       "Message expired (now={}). Dropping: {}",
                       now.toMilliseconds(),
                       _message->getTrackingInfo());
    }
}

} // namespace joynr

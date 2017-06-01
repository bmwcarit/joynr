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
#include "joynr/AbstractMessageRouter.h"

#include <cassert>
#include <functional>

#include <boost/asio/io_service.hpp>

#include "joynr/IMessagingStub.h"
#include "joynr/IMessagingStubFactory.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Message.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/access-control/IAccessController.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{

INIT_LOGGER(AbstractMessageRouter);

//------ AbstractMessageRouter ---------------------------------------------------------
AbstractMessageRouter::AbstractMessageRouter(
        std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
        boost::asio::io_service& ioService,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        int maxThreads,
        std::unique_ptr<MessageQueue> messageQueue)
        : routingTable("AbstractMessageRouter-RoutingTable",
                       ioService,
                       std::bind(&AbstractMessageRouter::routingTableSaveFilterFunc,
                                 this,
                                 std::placeholders::_1)),
          routingTableLock(),
          multicastReceiverDirectory(),
          messagingStubFactory(std::move(messagingStubFactory)),
          messageScheduler(maxThreads, "AbstractMessageRouter", ioService),
          messageQueue(std::move(messageQueue)),
          routingTableFileName(),
          addressCalculator(std::move(addressCalculator)),
          messageQueueCleanerTimer(ioService),
          messageQueueCleanerTimerPeriodMs(std::chrono::milliseconds(1000))
{
    activateMessageCleanerTimer();
}

AbstractMessageRouter::~AbstractMessageRouter()
{
    messageQueueCleanerTimer.cancel();
    messageScheduler.shutdown();
}

bool AbstractMessageRouter::routingTableSaveFilterFunc(std::shared_ptr<RoutingEntry> routingEntry)
{
    const auto destAddress = routingEntry->address;
    const joynr::InProcessMessagingAddress* inprocessAddress =
            dynamic_cast<const joynr::InProcessMessagingAddress*>(destAddress.get());
    return inprocessAddress == nullptr;
}

void AbstractMessageRouter::addProvisionedNextHop(
        std::string participantId,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
        bool isGloballyVisible)
{
    assert(address);
    const auto routingEntry = std::make_shared<RoutingEntry>(std::move(address), isGloballyVisible);
    addToRoutingTable(participantId, std::move(routingEntry));
}

std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>>
AbstractMessageRouter::lookupAddresses(const std::unordered_set<std::string>& participantIds)
{
    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> addresses;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress;
    for (const auto& participantId : participantIds) {
        const auto routingEntry = routingTable.lookup(participantId);
        if (routingEntry) {
            destAddress = routingEntry->address;
            addresses.insert(destAddress);
        }
    }
    return addresses;
}

std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>>
AbstractMessageRouter::getDestinationAddresses(const ImmutableMessage& message)
{
    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> addresses;
    if (message.getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        const std::string& multicastId = message.getRecipient();

        // lookup local multicast receivers
        std::unordered_set<std::string> multicastReceivers =
                multicastReceiverDirectory.getReceivers(multicastId);
        addresses = lookupAddresses(multicastReceivers);

        // add global transport address if message is NOT received from global
        // AND provider is globally visible
        if (!message.isReceivedFromGlobal() && addressCalculator && publishToGlobal(message)) {
            std::shared_ptr<const joynr::system::RoutingTypes::Address> globalTransport =
                    addressCalculator->compute(message);
            if (globalTransport) {
                addresses.insert(std::move(globalTransport));
            }
        }
    } else {
        const std::string& destinationPartId = message.getRecipient();
        const auto routingEntry = routingTable.lookup(destinationPartId);
        if (routingEntry) {
            addresses.insert(routingEntry->address);
        }
    }
    return addresses;
}

void AbstractMessageRouter::checkExpiryDate(const ImmutableMessage& message)
{
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    if (now > message.getExpiryDate()) {
        std::string errorMessage("Received expired message. Dropping the message (ID: " +
                                 message.getId() + ").");
        JOYNR_LOG_WARN(logger, errorMessage);
        throw exceptions::JoynrMessageNotSentException(errorMessage);
    }
}

void AbstractMessageRouter::route(std::shared_ptr<ImmutableMessage> message, std::uint32_t tryCount)
{
    assert(messagingStubFactory);
    assert(message);
    checkExpiryDate(*message);
    routeInternal(std::move(message), tryCount);
}

void AbstractMessageRouter::sendMessages(
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
                            item->getContent(), std::move(messagingStub), address, *this, tryCount),
                    std::chrono::milliseconds(0));
        } catch (const exceptions::JoynrMessageNotSentException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Message with Id {} could not be sent. Error: {}",
                            item->getContent()->getId(),
                            e.getMessage());
        }
    }
}

void AbstractMessageRouter::scheduleMessage(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        std::uint32_t tryCount,
        std::chrono::milliseconds delay)
{
    auto stub = messagingStubFactory->create(destAddress);
    if (stub) {
        messageScheduler.schedule(new MessageRunnable(std::move(message),
                                                      std::move(stub),
                                                      std::move(destAddress),
                                                      *this,
                                                      tryCount),
                                  delay);
    } else {
        JOYNR_LOG_WARN(
                logger,
                "Message with id {} could not be send to {}. Stub creation failed. => Queueing "
                "message.",
                message->getId(),
                destAddress->toString());
        // save the message for later delivery
        queueMessage(std::move(message));
    }
}

void AbstractMessageRouter::activateMessageCleanerTimer()
{
    messageQueueCleanerTimer.expiresFromNow(messageQueueCleanerTimerPeriodMs);
    messageQueueCleanerTimer.asyncWait(std::bind(
            &AbstractMessageRouter::onMessageCleanerTimerExpired, this, std::placeholders::_1));
}

void AbstractMessageRouter::onMessageCleanerTimerExpired(const boost::system::error_code& errorCode)
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

void AbstractMessageRouter::queueMessage(std::shared_ptr<ImmutableMessage> message)
{
    JOYNR_LOG_TRACE(logger, "message queued: {}", message->toLogMessage());
    messageQueue->queueMessage(std::move(message));
}

void AbstractMessageRouter::loadRoutingTable(std::string fileName)
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

void AbstractMessageRouter::saveRoutingTable()
{
    WriteLocker lock(routingTableLock);
    try {
        joynr::util::saveStringToFile(
                routingTableFileName, joynr::serializer::serializeToJson(routingTable));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger, ex.what());
    }
}

void AbstractMessageRouter::addToRoutingTable(std::string participantId,
                                              std::shared_ptr<RoutingEntry> routingEntry)
{
    {
        WriteLocker lock(routingTableLock);
        routingTable.add(participantId, std::move(routingEntry));
    }
    saveRoutingTable();
}

/**
 * IMPLEMENTATION of MessageRunnable class
 */

INIT_LOGGER(MessageRunnable);

MessageRunnable::MessageRunnable(
        std::shared_ptr<ImmutableMessage> message,
        std::shared_ptr<IMessagingStub> messagingStub,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
        AbstractMessageRouter& messageRouter,
        std::uint32_t tryCount)
        : Runnable(true),
          ObjectWithDecayTime(message->getExpiryDate()),
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
        // TODO is it safe to capture (this) here? rather capture members by value!
        auto onFailure = [this](const exceptions::JoynrRuntimeException& e) {
            try {
                exceptions::JoynrDelayMessageException& delayException =
                        dynamic_cast<exceptions::JoynrDelayMessageException&>(
                                const_cast<exceptions::JoynrRuntimeException&>(e));
                std::chrono::milliseconds delay = delayException.getDelayMs();

                JOYNR_LOG_TRACE(logger,
                                "Rescheduling message after error: messageId: {}, new delay {}ms, "
                                "reason: {}",
                                message->getId(),
                                delay.count(),
                                e.getMessage());
                messageRouter.scheduleMessage(message, destAddress, tryCount + 1, delay);
            } catch (const std::bad_cast&) {
                JOYNR_LOG_ERROR(logger,
                                "Message with ID {} could not be sent! reason: {}",
                                message->getId(),
                                e.getMessage());
            }
        };
        messagingStub->transmit(message, onFailure);
    } else {
        JOYNR_LOG_ERROR(logger, "Message with ID {}  expired: dropping!", message->getId());
    }
}

} // namespace joynr

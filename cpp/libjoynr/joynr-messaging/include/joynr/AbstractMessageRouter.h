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

#ifndef ABSTRACTMESSAGEROUTER_H
#define ABSTRACTMESSAGEROUTER_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/IMessageRouter.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/RoutingTable.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/Runnable.h"
#include "joynr/SteadyTimer.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace boost
{
namespace system
{
class error_code;
} // namespace system
} // namespace boost

namespace joynr
{

template <typename T>
class MessageQueue;

class IMessageSender;
class IMessagingStub;
class IMessagingStubFactory;
class IMulticastAddressCalculator;
class ITransportStatus;
class ImmutableMessage;
class ThreadPoolDelayedScheduler;

/**
  * Common implementation of functionalities of a message router object.
  */
class JOYNR_EXPORT AbstractMessageRouter
        : public joynr::IMessageRouter,
          public std::enable_shared_from_this<AbstractMessageRouter>
{

public:
    ~AbstractMessageRouter() override;
    void addProvisionedNextHop(std::string participantId,
                               std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                               bool isGloballyVisible);
    virtual void setToKnown(const std::string& participantId) override;

    virtual void init();
    std::uint64_t getNumberOfRoutedMessages() const;
    void removeRoutingEntries(std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    void route(std::shared_ptr<ImmutableMessage> message, std::uint32_t tryCount = 0) final;
    virtual void shutdown();

    void setMessageSender(std::weak_ptr<IMessageSender> messageSender);

    friend class MessageRunnable;
    friend class ConsumerPermissionCallback;

protected:
    struct AddressEqual
    {
    public:
        bool operator()(std::shared_ptr<const joynr::system::RoutingTypes::Address> address1,
                        std::shared_ptr<const joynr::system::RoutingTypes::Address> address2) const
        {
            return (*address1 == *address2);
        }
    };

    struct AddressHash
    {
    public:
        size_t operator()(std::shared_ptr<const joynr::system::RoutingTypes::Address> address) const
        {
            return address->hashCode();
        }
    };

    // use specialized set with custom comparator to prevent duplicate insertion
    // of elements with logically equivalent content
    using AddressUnorderedSet =
            std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>,
                               AddressHash,
                               AddressEqual>;

    // Instantiation of this class only possible through its child classes.
    AbstractMessageRouter(MessagingSettings& messagingSettings,
                          std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
                          boost::asio::io_service& ioService,
                          std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
                          std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
                          std::unique_ptr<MessageQueue<std::string>> messageQueue,
                          std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>>
                                  transportNotAvailableQueue,
                          const std::vector<std::string>& knownGbids = {});

    virtual bool publishToGlobal(const ImmutableMessage& message) = 0;
    AddressUnorderedSet getDestinationAddresses(const ImmutableMessage& message,
                                                const ReadLocker& messageQueueRetryReadLock);

    virtual void routeInternal(std::shared_ptr<ImmutableMessage> message,
                               std::uint32_t tryCount) = 0;

    void sendQueuedMessages(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address) final;

    void sendQueuedMessages(const std::string& destinationPartId,
                            std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                            WriteLocker&& messageQueueRetryWriteLock);

    virtual void sendMessage(std::shared_ptr<ImmutableMessage> message,
                             std::shared_ptr<const system::RoutingTypes::Address> destAddress,
                             std::uint32_t tryCount = 0) = 0;

    void scheduleMessage(std::shared_ptr<ImmutableMessage> message,
                         std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
                         std::uint32_t tryCount = 0,
                         std::chrono::milliseconds delay = std::chrono::milliseconds(0));

    virtual bool isValidForRoutingTable(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address) = 0;

    virtual bool allowRoutingEntryUpdate(const routingtable::RoutingEntry& oldEntry,
                                         const system::RoutingTypes::Address& newAddress) = 0;

    bool addToRoutingTable(std::string participantId,
                           bool isGloballyVisible,
                           std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                           const std::int64_t expiryDateMs,
                           const bool isSticky);

    void activateMessageCleanerTimer();
    void activateRoutingTableCleanerTimer();
    void registerTransportStatusCallbacks();
    void rescheduleQueuedMessagesForTransport(std::shared_ptr<ITransportStatus> transportStatus);
    void onMessageCleanerTimerExpired(std::shared_ptr<AbstractMessageRouter> thisSharedptr,
                                      const boost::system::error_code& errorCode);
    void onRoutingTableCleanerTimerExpired(const boost::system::error_code& errorCode);

    virtual void queueMessage(std::shared_ptr<ImmutableMessage> message,
                              ReadLocker& messageQueueRetryReadLock);
    void onMsgsDropped(std::deque<std::shared_ptr<ImmutableMessage>>& droppedMessages);
    /*
     * return always true in libjoynr and result accessControlChecked for the CCMessageRouter
     */
    virtual bool canMessageBeTransmitted(std::shared_ptr<ImmutableMessage> message) const = 0;

    /*
     * AbstractMessageRouter provides default implementation and CcMessageRouter overrrides it
     */
    virtual void removeUnreachableMulticastReceivers(
            const std::string& multicastId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
            const std::string& providerParticipantId);

    virtual void stopSubscription(std::shared_ptr<ImmutableMessage> message);

    boost::optional<routingtable::RoutingEntry> getRoutingEntry(const std::string& participantId);

    std::chrono::milliseconds createDelayWithExponentialBackoff(
            std::uint32_t sendMsgRetryIntervalMs,
            std::uint32_t tryCount) const;
    RoutingTable _routingTable;
    ReadWriteLock _routingTableLock;
    MulticastReceiverDirectory _multicastReceiverDirectory;
    MessagingSettings _messagingSettings;
    std::shared_ptr<IMessagingStubFactory> _messagingStubFactory;
    std::shared_ptr<ThreadPoolDelayedScheduler> _messageScheduler;
    std::unique_ptr<MessageQueue<std::string>> _messageQueue;
    std::weak_ptr<IMessageSender> _messageSender;
    // MessageQueue ReadLocker is required to protect calls to queueMessage and
    // getDestinationAddresses:
    // The routing table must not be modified between getDestinationAddresses and queueMessage.
    // MessageQueue WriterLocker is required to protect calls to addNextHop/addProvisionedNextHop
    // and sendMessages:
    // Routing table look ups and insertions to the messageQueue must not be done between addNextHop
    // and sendMessages calls.
    ReadWriteLock _messageQueueRetryLock;
    std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> _transportNotAvailableQueue;
    std::mutex _transportAvailabilityMutex;
    std::unique_ptr<IMulticastAddressCalculator> _addressCalculator;
    SteadyTimer _messageQueueCleanerTimer;
    const std::chrono::milliseconds _messageQueueCleanerTimerPeriodMs;
    SteadyTimer _routingTableCleanerTimer;
    std::vector<std::shared_ptr<ITransportStatus>> _transportStatuses;
    bool _printRoutedMessages;
    std::uint32_t _routedMessagePrintIntervalS;

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractMessageRouter);
    ADD_LOGGER(AbstractMessageRouter)

    void checkExpiryDate(const ImmutableMessage& message);
    AddressUnorderedSet lookupAddresses(const std::unordered_set<std::string>& participantIds);
    std::atomic<bool> _isShuttingDown;
    std::atomic<std::uint64_t> _numberOfRoutedMessages;
    const std::uint64_t _maxAclRetryIntervalMs;
    std::uint32_t _messageCleaningCycleCounter;
};

/**
 * Class to send message
 */
class JOYNR_EXPORT MessageRunnable : public Runnable, public ObjectWithDecayTime
{
public:
    MessageRunnable(std::shared_ptr<ImmutableMessage> message,
                    std::shared_ptr<IMessagingStub> messagingStub,
                    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
                    std::weak_ptr<AbstractMessageRouter> messageRouter,
                    std::uint32_t tryCount);
    void shutdown() override;
    void run() override;

private:
    std::shared_ptr<ImmutableMessage> _message;
    std::shared_ptr<IMessagingStub> _messagingStub;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _destAddress;
    std::weak_ptr<AbstractMessageRouter> _messageRouter;
    std::uint32_t _tryCount;

    ADD_LOGGER(MessageRunnable)
};

} // namespace joynr
#endif // ABSTRACTMESSAGEROUTER_H

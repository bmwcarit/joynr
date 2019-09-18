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

#ifndef ABSTRACTMESSAGEROUTER_H
#define ABSTRACTMESSAGEROUTER_H

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

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
#include "joynr/ThreadPoolDelayedScheduler.h"
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
class ITransportStatus;

class IMessagingStub;
class IMessagingStubFactory;
class ImmutableMessage;
class IMulticastAddressCalculator;

namespace system
{
class RoutingProxy;
} // namespace system

/**
  * Common implementation of functionalities of a message router object.
  */
class JOYNR_EXPORT AbstractMessageRouter
        : public joynr::IMessageRouter,
          public std::enable_shared_from_this<AbstractMessageRouter>
{

public:
    virtual ~AbstractMessageRouter();
    void addProvisionedNextHop(std::string participantId,
                               std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                               bool isGloballyVisible);
    virtual void setToKnown(const std::string& participantId) override;

    virtual void init();
    void saveRoutingTable();
    void loadRoutingTable(std::string fileName);
    std::uint64_t getNumberOfRoutedMessages() const;

    void route(std::shared_ptr<ImmutableMessage> message, std::uint32_t tryCount = 0) final;
    virtual void shutdown();

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
                          bool persistRoutingTable,
                          std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
                          std::unique_ptr<MessageQueue<std::string>> messageQueue,
                          std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>>
                                  transportNotAvailableQueue);

    virtual bool publishToGlobal(const ImmutableMessage& message) = 0;
    AddressUnorderedSet getDestinationAddresses(const ImmutableMessage& message,
                                                const ReadLocker& messageQueueRetryReadLock);

    void registerGlobalRoutingEntryIfRequired(const ImmutableMessage& message);
    virtual void routeInternal(std::shared_ptr<ImmutableMessage> message,
                               std::uint32_t tryCount) = 0;

    virtual void sendQueuedMessages(
            const std::string& destinationPartId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
            const WriteLocker& messageQueueRetryWriteLock) = 0;

    void sendQueuedMessages(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address) final;

    virtual bool isValidForRoutingTable(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address) = 0;

    virtual bool allowRoutingEntryUpdate(const routingtable::RoutingEntry& oldEntry,
                                         const system::RoutingTypes::Address& newAddress) = 0;

    void addToRoutingTable(std::string participantId,
                           bool isGloballyVisible,
                           std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                           const std::int64_t expiryDateMs,
                           const bool isSticky);

    virtual void doAccessControlCheckOrScheduleMessage(
            std::shared_ptr<ImmutableMessage> message,
            std::shared_ptr<const system::RoutingTypes::Address> destAddress,
            std::uint32_t tryCount = 0);

    void scheduleMessage(std::shared_ptr<ImmutableMessage> message,
                         std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
                         std::uint32_t tryCount = 0,
                         std::chrono::milliseconds delay = std::chrono::milliseconds(0));

    void activateMessageCleanerTimer();
    void activateRoutingTableCleanerTimer();
    void registerTransportStatusCallbacks();
    void rescheduleQueuedMessagesForTransport(std::shared_ptr<ITransportStatus> transportStatus);
    void onMessageCleanerTimerExpired(std::shared_ptr<AbstractMessageRouter> thisSharedptr,
                                      const boost::system::error_code& errorCode);
    void onRoutingTableCleanerTimerExpired(const boost::system::error_code& errorCode);

    virtual void queueMessage(std::shared_ptr<ImmutableMessage> message,
                              const ReadLocker& messageQueueRetryReadLock);
    /*
     * return always true in libjoynr and result accessControlChecked for the CCMessageRouter
     */
    virtual bool canMessageBeTransmitted(std::shared_ptr<ImmutableMessage> message) const = 0;

    /*
     * AbstractMessageRouter provides default implementation and CcMessageRouter overrrides it
     */
    virtual void removeMulticastReceiver(
            const std::string& multicastId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
            const std::string& providerParticipantId);

    std::chrono::milliseconds createDelayWithExponentialBackoff(
            std::uint32_t sendMsgRetryIntervalMs,
            std::uint32_t tryCount) const;
    RoutingTable routingTable;
    ReadWriteLock routingTableLock;
    MulticastReceiverDirectory multicastReceiverDirectory;
    MessagingSettings messagingSettings;
    bool persistRoutingTable;
    std::shared_ptr<IMessagingStubFactory> messagingStubFactory;
    std::shared_ptr<ThreadPoolDelayedScheduler> messageScheduler;
    std::unique_ptr<MessageQueue<std::string>> messageQueue;
    // MessageQueue ReadLocker is required to protect calls to queueMessage and
    // getDestinationAddresses:
    // The routing table must not be modified between getDestinationAddresses and queueMessage.
    // MessageQueue WriterLocker is required to protect calls to addNextHop/addProvisionedNextHop
    // and sendMessages:
    // Routing table look ups and insertions to the messageQueue must not be done between addNextHop
    // and sendMessages calls.
    ReadWriteLock messageQueueRetryLock;
    std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> transportNotAvailableQueue;
    std::mutex transportAvailabilityMutex;
    std::string routingTableFileName;
    std::unique_ptr<IMulticastAddressCalculator> addressCalculator;
    SteadyTimer messageQueueCleanerTimer;
    const std::chrono::milliseconds messageQueueCleanerTimerPeriodMs;
    SteadyTimer routingTableCleanerTimer;
    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractMessageRouter);
    ADD_LOGGER(AbstractMessageRouter)

    void checkExpiryDate(const ImmutableMessage& message);
    AddressUnorderedSet lookupAddresses(const std::unordered_set<std::string>& participantIds);
    std::atomic<bool> isShuttingDown;
    std::atomic<std::uint64_t> numberOfRoutedMessages;
    const std::uint64_t maxAclRetryIntervalMs;
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
    std::shared_ptr<ImmutableMessage> message;
    std::shared_ptr<IMessagingStub> messagingStub;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress;
    std::weak_ptr<AbstractMessageRouter> messageRouter;
    std::uint32_t tryCount;

    ADD_LOGGER(MessageRunnable)
};

} // namespace joynr
#endif // ABSTRACTMESSAGEROUTER_H

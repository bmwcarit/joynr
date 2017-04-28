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

#include <chrono>
#include <memory>
#include <string>
#include <unordered_set>

#include "joynr/Directory.h"
#include "joynr/IMessageRouter.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MessageQueue.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/Runnable.h"
#include "joynr/SteadyTimer.h"
#include "joynr/ThreadPoolDelayedScheduler.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
namespace system
{
class error_code;
} // namespace system
} // namespace boost

namespace joynr
{

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
class JOYNR_EXPORT AbstractMessageRouter : public joynr::IMessageRouter
{
public:
    virtual ~AbstractMessageRouter();

    void addProvisionedNextHop(std::string participantId,
                               std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    void saveRoutingTable();
    void loadRoutingTable(std::string fileName);

    friend class MessageRunnable;
    friend class ConsumerPermissionCallback;

protected:
    // Instantiation of this class only possible through its child classes.
    AbstractMessageRouter(
            std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
            boost::asio::io_service& ioService,
            std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
            int maxThreads = 1,
            std::unique_ptr<MessageQueue> messageQueue = std::make_unique<MessageQueue>());

    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>>
    getDestinationAddresses(const ImmutableMessage& message);

    std::unordered_set<std::shared_ptr<const joynr::system::RoutingTypes::Address>> lookupAddresses(
            const std::unordered_set<std::string>& participantIds);

    void sendMessages(const std::string& destinationPartId,
                      std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    void addToRoutingTable(std::string participantId,
                           std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    void scheduleMessage(std::shared_ptr<ImmutableMessage> message,
                         std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
                         std::uint32_t tryCount = 0,
                         std::chrono::milliseconds delay = std::chrono::milliseconds(0));

    void activateMessageCleanerTimer();
    void onMessageCleanerTimerExpired(const boost::system::error_code& errorCode);

    using RoutingTable = Directory<std::string, const joynr::system::RoutingTypes::Address>;
    RoutingTable routingTable;
    ReadWriteLock routingTableLock;
    MulticastReceiverDirectory multicastReceiverDirectory;
    std::shared_ptr<IMessagingStubFactory> messagingStubFactory;
    ThreadPoolDelayedScheduler messageScheduler;
    std::unique_ptr<MessageQueue> messageQueue;
    std::string routingTableFileName;
    std::unique_ptr<IMulticastAddressCalculator> addressCalculator;
    SteadyTimer messageQueueCleanerTimer;
    const std::chrono::milliseconds messageQueueCleanerTimerPeriodMs;
    void queueMessage(std::shared_ptr<ImmutableMessage> message) override;

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractMessageRouter);
    ADD_LOGGER(AbstractMessageRouter);

    bool routingTableSaveFilterFunc(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress);
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
                    AbstractMessageRouter& messageRouter,
                    std::uint32_t tryCount);
    void shutdown() override;
    void run() override;

private:
    std::shared_ptr<ImmutableMessage> message;
    std::shared_ptr<IMessagingStub> messagingStub;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress;
    AbstractMessageRouter& messageRouter;
    std::uint32_t tryCount;

    ADD_LOGGER(MessageRunnable);
};

} // namespace joynr
#endif // ABSTRACTMESSAGEROUTER_H

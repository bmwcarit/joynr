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

#ifndef CHILDMESSAGEROUTER_H
#define CHILDMESSAGEROUTER_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/AbstractMessageRouter.h"
#include "joynr/JoynrExport.h"
#include "joynr/MessagingSettings.h"
#include "joynr/PrivateCopyAssign.h"

#include "joynr/Logger.h"

namespace joynr
{

template <typename T>
class MessageQueue;
class ITransportStatus;

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class IMessagingStubFactory;
class IMulticastAddressCalculator;

namespace system
{
class Address;
class RoutingProxy;
} // namespace system

/**
  * Class MessageRouter receives incoming JoynrMessages on the ClusterController
  * and forwards them either to a remote ClusterController or to a LibJoynr on the machine.
  *
  *  1 extracts the destination participant ID and looks up the EndpointAddress in the
  *MessagingEndpointDirectory
  *  2 creates a <Middleware>MessagingStub by calling MessagingStubFactory.create(EndpointAddress
  *addr)
  *  3 forwards the message using the <Middleware>MessagingStub.send(JoynrMessage msg)
  *
  *  In sending, a ThreadPool of default size 6 is used with a 500ms default retry interval.
  */

class JOYNR_EXPORT LibJoynrMessageRouter : public joynr::AbstractMessageRouter
{
public:
    LibJoynrMessageRouter(
            MessagingSettings& messagingSettings,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> incomingAddress,
            std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
            boost::asio::io_service& ioService,
            std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
            bool persistRoutingTable,
            std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
            std::unique_ptr<MessageQueue<std::string>> messageQueue,
            std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>>
                    transportNotAvailableQueue);

    ~LibJoynrMessageRouter() override;

    void routeInternal(std::shared_ptr<ImmutableMessage> message, std::uint32_t tryCount = 0) final;

    /*
     * Implement methods from IMessageRouter
     */
    void addNextHop(const std::string& participantId,
                    const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address,
                    bool isGloballyVisible,
                    const std::int64_t expiryDateMs,
                    const bool isSticky,
                    std::function<void()> onSuccess = nullptr,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                            onError = nullptr) final;

    void removeNextHop(
            const std::string& participantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void removeMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void setParentAddress(
            std::string parentParticipantId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> parentAddress);

    void setToKnown(const std::string& participantId) override;

    /*
     * Method specific to LibJoynrMessageRouter
     */
    void setParentRouter(std::shared_ptr<joynr::system::RoutingProxy> parentRouter,
                         std::function<void()> onSuccess = nullptr,
                         std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                                 onError = nullptr);

    bool publishToGlobal(const ImmutableMessage& message) final;

    /*
     * Method specific to LibJoynrMessageRouter,
     * removes parentRouter shared ptr in order to break cyclic dependency
     * SubscriptionManager -> LibJoynrMessageRouter -> RoutingProxy -> SubscriptionManager
     */
    void shutdown() final;

    friend class MessageRunnable;

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrMessageRouter);
    ADD_LOGGER(LibJoynrMessageRouter)

    void sendQueuedMessages(const std::string& destinationPartId,
                            std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                            const WriteLocker& messageQueueRetryWriteLock) final;

    bool isParentMessageRouterSet();
    void addNextHopToParent(std::string participantId,
                            bool isGloballyVisible,
                            std::function<void(void)> onSuccess = nullptr,
                            std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                                    onError = nullptr);
    bool isValidForRoutingTable(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address) final;
    bool allowRoutingEntryUpdate(const routingtable::RoutingEntry& oldEntry,
                                 const system::RoutingTypes::Address& newAddress) final;

    std::shared_ptr<joynr::system::RoutingProxy> _parentRouter;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _parentAddress;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _incomingAddress;
    std::unordered_set<std::string> _runningParentResolves;
    mutable std::mutex _parentResolveMutex;

    bool canMessageBeTransmitted(std::shared_ptr<ImmutableMessage> message) const final;

    void removeRunningParentResolvers(const std::string& destinationPartId);

    std::mutex _parentClusterControllerReplyToAddressMutex;
    std::string _parentClusterControllerReplyToAddress;
    const bool _DEFAULT_IS_GLOBALLY_VISIBLE;
};

} // namespace joynr
#endif // CHILDMESSAGEROUTER_H

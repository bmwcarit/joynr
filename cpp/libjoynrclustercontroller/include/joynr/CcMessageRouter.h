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

#ifndef CCMESSAGEROUTER_H
#define CCMESSAGEROUTER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "joynr/AbstractMessageRouter.h"
#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/system/RoutingAbstractProvider.h"

namespace joynr
{

template <typename T>
class MessageQueue;

class ClusterControllerSettings;
class IAccessController;
class IMessageSender;
class IMessagingStubFactory;
class IMulticastAddressCalculator;
class IPlatformSecurityManager;
class ITransportStatus;
class ImmutableMessage;
class MessagingSettings;
class MulticastMessagingSkeletonDirectory;

namespace exceptions
{
class JoynrRuntimeException;
class ProviderRuntimeException;
}

namespace system
{
class MessageNotificationProvider;
} // namespace system

class CcMessageNotificationProvider;

/**
  * MessageRouter specialization for cluster-controller. It receives incoming ImmutableMessages
  * on the ClusterController and forwards them either to a remote ClusterController or
  * to a LibJoynr on the machine.
  *
  *  1 extracts the destination participant ID and looks up the EndpointAddress in the
  *     MessagingEndpointDirectory
  *  2 creates a <Middleware>MessagingStub by calling MessagingStubFactory.create(EndpointAddress
  *addr)
  *  3 forwards the message using the <Middleware>MessagingStub.transmit(ImmutableMessage msg)
  *
  *  In sending, a ThreadPool of default size 1 is used.
  */

class JOYNR_EXPORT CcMessageRouter : public joynr::AbstractMessageRouter,
                                     public joynr::system::RoutingAbstractProvider
{
public:
    // TODO: change shared_ptr to unique_ptr once JoynrClusterControllerRuntime is refactored
    CcMessageRouter(MessagingSettings& messagingSettings,
                    ClusterControllerSettings& clusterControllerSettings,
                    std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
                    std::shared_ptr<MulticastMessagingSkeletonDirectory>
                            multicastMessagingSkeletonDirectory,
                    std::unique_ptr<IPlatformSecurityManager> securityManager,
                    boost::asio::io_service& ioService,
                    std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
                    const std::string& globalClusterControllerAddress,
                    const std::string& messageNotificationProviderParticipantId,
                    bool persistRoutingTable,
                    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses,
                    std::unique_ptr<MessageQueue<std::string>> messageQueue,
                    std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>>
                            transportNotAvailableQueue,
                    const system::RoutingTypes::Address& ownGlobalAddress,
                    const std::vector<std::string>& knownGbids);

    ~CcMessageRouter() override;

    void routeInternal(std::shared_ptr<ImmutableMessage> message, std::uint32_t tryCount) final;

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

    /*
     * Implement methods from RoutingAbstractProvider
     */
    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::ChannelAddress& channelAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::MqttAddress& mqttAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::BrowserAddress& browserAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketAddress& webSocketAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::BinderAddress& binderAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void removeNextHop(const std::string& participantId,
                       std::function<void()> onSuccess = nullptr,
                       std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                               onError = nullptr) final;

    void resolveNextHop(
            const std::string& participantId,
            std::function<void(const bool& resolved)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void getGlobalAddress(
            std::function<void(const std::string&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void getReplyToAddress(
            std::function<void(const std::string&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    /*
     * used by AbstractMessageRouter
     */
    void removeMulticastReceiver(
            const std::string& multicastId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
            const std::string& providerParticipantId) final;

    void stopSubscription(std::shared_ptr<ImmutableMessage> message) final;

    /*
     * Implement both IMessageRouter and RoutingAbstractProvider
     */
    void removeMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    /*
     * Public methods specific to CcMessageRouter
     */
    bool publishToGlobal(const ImmutableMessage& message) final;
    void setAccessController(std::weak_ptr<IAccessController> accessController);
    void setMessageSender(std::weak_ptr<IMessageSender> messageSender);
    void saveMulticastReceiverDirectory() const;
    void loadMulticastReceiverDirectory(std::string filename);
    std::shared_ptr<joynr::system::MessageNotificationProvider> getMessageNotificationProvider()
            const;
    friend class MessageRunnable;
    friend class ConsumerPermissionCallback;

private:
    bool isValidForRoutingTable(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address) final;
    bool allowRoutingEntryUpdate(const routingtable::RoutingEntry& oldEntry,
                                 const system::RoutingTypes::Address& newAddress) final;

    void reestablishMulticastSubscriptions();
    void registerMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError);

    void sendQueuedMessages(const std::string& destinationPartId,
                            std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                            const WriteLocker& messageQueueRetryWriteLock) final;

    void doAccessControlCheckOrScheduleMessage(
            std::shared_ptr<ImmutableMessage> message,
            std::shared_ptr<const system::RoutingTypes::Address> destAddress,
            std::uint32_t tryCount = 0) final;
    void queueMessage(std::shared_ptr<ImmutableMessage> message,
                      const ReadLocker& messageQueueRetryReadLock) final;

    bool canMessageBeTransmitted(std::shared_ptr<ImmutableMessage> message) const final;

    DISALLOW_COPY_AND_ASSIGN(CcMessageRouter);
    ADD_LOGGER(CcMessageRouter)

    std::shared_ptr<MulticastMessagingSkeletonDirectory> _multicastMessagingSkeletonDirectory;
    std::unique_ptr<IPlatformSecurityManager> _securityManager;
    std::weak_ptr<IAccessController> _accessController;
    std::string _multicastReceiverDirectoryFilename;
    const std::string _globalClusterControllerAddress;
    std::shared_ptr<CcMessageNotificationProvider> _messageNotificationProvider;
    const std::string _messageNotificationProviderParticipantId;
    ClusterControllerSettings& _clusterControllerSettings;
    const bool _multicastReceiverDirectoryPersistencyEnabled;
    const system::RoutingTypes::Address& _ownGlobalAddress;
    std::weak_ptr<IMessageSender> _messageSender;
};

} // namespace joynr
#endif // CCMESSAGEROUTER_H

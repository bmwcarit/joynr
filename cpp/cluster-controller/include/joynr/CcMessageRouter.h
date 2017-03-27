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

#include "joynr/AbstractMessageRouter.h"
#include "joynr/system/RoutingAbstractProvider.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MessageQueue.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/Runnable.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{

class IAccessController;
class IMessagingStubFactory;
class IMulticastAddressCalculator;
class IPlatformSecurityManager;
class JoynrMessage;
class MulticastMessagingSkeletonDirectory;

namespace system
{
class Address;
class MessageNotificationProvider;
} // namespace system

class CcMessageNotificationProvider;

/**
  * MessageRouter specialization for cluster-controller. It receives incoming JoynrMessages
  * on the ClusterController and forwards them either to a remote ClusterController or
  * to a LibJoynr on the machine.
  *
  *  1 extracts the destination participant ID and looks up the EndpointAddress in the
  *     MessagingEndpointDirectory
  *  2 creates a <Middleware>MessagingStub by calling MessagingStubFactory.create(EndpointAddress
  *addr)
  *  3 forwards the message using the <Middleware>MessagingStub.send(JoynrMessage msg)
  *
  *  In sending, a ThreadPool of default size 1 is used.
  */

class JOYNR_EXPORT CcMessageRouter : public joynr::AbstractMessageRouter,
                                     public joynr::system::RoutingAbstractProvider
{
public:
    // TODO: change shared_ptr to unique_ptr once JoynrClusterControllerRuntime is refactored
    CcMessageRouter(std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
                    std::shared_ptr<MulticastMessagingSkeletonDirectory>
                            multicastMessagingSkeletonDirectory,
                    std::unique_ptr<IPlatformSecurityManager> securityManager,
                    boost::asio::io_service& ioService,
                    std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
                    const std::string& globalClusterControllerAddress,
                    int maxThreads = 1,
                    std::unique_ptr<MessageQueue> messageQueue = std::make_unique<MessageQueue>());

    ~CcMessageRouter() override;

    /*
     * Implement methods from IMessageRouter
     */
    void route(JoynrMessage& message, std::uint32_t tryCount = 0) final;

    void addNextHop(const std::string& participantId,
                    const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address,
                    std::function<void()> onSuccess = nullptr,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                            onError = nullptr) final;

    void queueMessage(const JoynrMessage& message) final;

    /*
     * Implement methods from RoutingAbstractProvider
     */
    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::ChannelAddress& channelAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::MqttAddress& mqttAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::CommonApiDbusAddress& commonApiDbusAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::BrowserAddress& browserAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketAddress& webSocketAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) final;

    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
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
    void setAccessController(std::shared_ptr<IAccessController> accessController);
    void saveMulticastReceiverDirectory() const;
    void loadMulticastReceiverDirectory(std::string filename);
    std::shared_ptr<joynr::system::MessageNotificationProvider> getMessageNotificationProvider()
            const;
    friend class MessageRunnable;
    friend class ConsumerPermissionCallback;

private:
    void reestablishMulticastSubscriptions();
    void registerMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError);

    DISALLOW_COPY_AND_ASSIGN(CcMessageRouter);
    ADD_LOGGER(CcMessageRouter);

    std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory;
    std::unique_ptr<IPlatformSecurityManager> securityManager;
    std::shared_ptr<IAccessController> accessController;
    std::string multicastReceveiverDirectoryFilename;
    const std::string globalClusterControllerAddress;
    std::shared_ptr<CcMessageNotificationProvider> messageNotificationProvider;
};

} // namespace joynr
#endif // CCMESSAGEROUTER_H

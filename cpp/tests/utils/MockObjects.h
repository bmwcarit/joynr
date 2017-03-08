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

#ifndef MOCKOBJECTS_H_
#define MOCKOBJECTS_H_

#include <memory>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <boost/any.hpp>
#include <boost/asio.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "PrettyPrint.h"
#include "LibJoynrMockObjects.h"

#include "joynr/access-control/IAccessController.h"
#include "cluster-controller/access-control/LocalDomainAccessController.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/testRequestCaller.h"
#include "joynr/vehicle/DefaultGpsProvider.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/Runnable.h"
#include "joynr/vehicle/GpsRequestCaller.h"
#include "joynr/IMessageReceiver.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessaging.h"
#include "joynr/ReplyCaller.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/AbstractMessageRouter.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/JoynrMessageSender.h"

#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/vehicle/GpsProvider.h"

#include "joynr/IMessagingStubFactory.h"
#include "joynr/IMessagingMulticastSubscriber.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"

#include "joynr/ClusterControllerDirectories.h"

#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "common/in-process/InProcessMessagingSkeleton.h"
#include "joynr/InProcessConnectorFactory.h"
#include "cluster-controller/http-communication-manager/HttpReceiver.h"

#include "joynr/infrastructure/GlobalDomainAccessControllerProxy.h"

#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/MessagingSettings.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/PublicationManager.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/IMessageSender.h"
#include "joynr/BrokerUrl.h"
#include "joynr/Settings.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/IProxyBuilder.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/types/Version.h"
#include "joynr/exceptions/JoynrException.h"

#include "libjoynr/websocket/IWebSocketPpClient.h"
#include "joynr/IWebSocketSendInterface.h"
#include "libjoynr/websocket/WebSocketSettings.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

#include "joynr/MulticastPublication.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastSubscriptionQos.h"

#include "joynr/OneWayRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/Request.h"

#include "joynr/IMulticastAddressCalculator.h"

namespace joynr
{
class JoynrMessage;
class RequestCaller;
class SubscriptionReply;
class BroadcastSubscriptionRequest;

namespace exceptions
{
class JoynrException;
class JoynrRuntimeException;
class ProviderRuntimeException;
class DiscoveryException;
} // namespace exceptions
} // namespace joynr

using ::testing::A;
using ::testing::_;
using ::testing::A;
using ::testing::Eq;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Return;

// Disable VC++ warnings due to google mock
// http://code.google.com/p/googlemock/wiki/FrequentlyAskedQuestions#MSVC_gives_me_warning_C4301_or_C4373_when_I_define_a_mock_method
#ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable : 4373 )
#endif

// Disable compiler warnings.
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wreorder"

template<typename T>
class MockProxyBuilder : public joynr::IProxyBuilder<T>
{
public:
    MockProxyBuilder(){
        ON_CALL(*this, setCached(_)).WillByDefault(Return(this));
        ON_CALL(*this, setMessagingQos(_)).WillByDefault(Return(this));
        ON_CALL(*this, setDiscoveryQos(_)).WillByDefault(Return(this));
    }

    MOCK_METHOD1_T(setCached, joynr::IProxyBuilder<T>*(const bool cached));
    MOCK_METHOD1_T(setMessagingQos, joynr::IProxyBuilder<T>*(const joynr::MessagingQos& cached));
    MOCK_METHOD1_T(setDiscoveryQos, joynr::IProxyBuilder<T>*(const joynr::DiscoveryQos& cached));
    MOCK_METHOD0_T(build, std::unique_ptr<T>());
    MOCK_METHOD2_T(buildAsync, void(std::function<void(std::unique_ptr<T> proxy)> onSuccess,
                                    std::function<void(const joynr::exceptions::DiscoveryException&)>));
};

class MockCapabilitiesClient : public joynr::ICapabilitiesClient {
public:
    MOCK_METHOD3(add, void(const joynr::types::GlobalDiscoveryEntry& entry,
                           std::function<void()> onSuccess,
                           std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));

    MOCK_METHOD1(remove, void(std::vector<std::string> participantIdList));
    MOCK_METHOD1(remove, void(const std::string& participantId));
    MOCK_METHOD3(lookup, std::vector<joynr::types::GlobalDiscoveryEntry>(const std::vector<std::string>& domain, const std::string& interfaceName, const std::int64_t messagingTtl));
    MOCK_METHOD5(lookup, void(
                     const std::vector<std::string>& domain,
                     const std::string& interfaceName,
                     const std::int64_t messagingTtl,
                     std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD3(lookup, void(
                     const std::string& participantId,
                     std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)> callbackFct,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD3(touch, void(const std::string& clusterControllerId,
                     std::function<void()> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));

    void setProxyBuilder(std::unique_ptr<joynr::IProxyBuilder<joynr::infrastructure::GlobalCapabilitiesDirectoryProxy>> input) {
        std::ignore = input;
    }
};

class MockInProcessMessagingSkeleton : public joynr::InProcessMessagingSkeleton
{
public:
    MOCK_METHOD2(transmit, void(joynr::JoynrMessage& message, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure));
};

class MockDelayedScheduler : public joynr::DelayedScheduler
{
public:
    MockDelayedScheduler(boost::asio::io_service& ioService)
        : DelayedScheduler([](joynr::Runnable*){ assert(false); }, ioService, std::chrono::milliseconds::zero())
    {
    }

    void shutdown() { joynr::DelayedScheduler::shutdown(); }
    MOCK_METHOD1(unschedule, void (joynr::DelayedScheduler::RunnableHandle));
    MOCK_METHOD2(schedule, DelayedScheduler::RunnableHandle (joynr::Runnable*, std::chrono::milliseconds delay));
};

class MockRunnable : public joynr::Runnable
{
public:
    MockRunnable(bool deleteMe) : Runnable(deleteMe)
    {
    }

    MOCK_CONST_METHOD0(dtorCalled, void ());
    ~MockRunnable()
    {
        dtorCalled();
    }

    MOCK_METHOD0(shutdown, void ());
    MOCK_METHOD0(run, void ());
};

class MockRunnableWithAccuracy : public joynr::Runnable
{
public:
    static const std::uint64_t timerAccuracyTolerance_ms = 5U;

    MockRunnableWithAccuracy(bool deleteMe,
                             const std::uint64_t delay);

    MOCK_CONST_METHOD0(dtorCalled, void ());
    ~MockRunnableWithAccuracy();

    MOCK_METHOD0(shutdown, void ());

    MOCK_CONST_METHOD0(runCalled, void());
    MOCK_CONST_METHOD0(runCalledInTime, void());
    void run() override;

private:
    const std::uint64_t est_ms;
    static joynr::Logger logger;
};

class MockRunnableBlocking : public joynr::Runnable
{
public:
    MockRunnableBlocking(bool deleteMe = false)
        : Runnable(deleteMe),
          mutex(),
          wait()
    {
    }

    MOCK_CONST_METHOD0(dtorCalled, void ());
    ~MockRunnableBlocking() { dtorCalled(); }

    MOCK_METHOD0(shutdownCalled, void ());
    void shutdown()
    {
        wait.notify_all();
        shutdownCalled();
    }

    void manualShutdown()
    {
        wait.notify_all();
    }

    MOCK_CONST_METHOD0(runEntry, void ());
    MOCK_CONST_METHOD0(runExit, void ());
    void run()
    {
        runEntry();
        std::unique_lock<std::mutex> lock(mutex);
        wait.wait(lock);
        runExit();
    }

private:
    std::mutex mutex;
    std::condition_variable wait;
};

class MockInProcessConnectorFactory : public joynr::InProcessConnectorFactory {
public:

    MockInProcessConnectorFactory()
        : InProcessConnectorFactory(nullptr,nullptr,nullptr,nullptr) {
    }

    MOCK_METHOD1(canBeCreated, bool(const std::shared_ptr<const joynr::system::RoutingTypes::Address> address));
};

class MockDispatcher : public joynr::IDispatcher {
public:
    MOCK_METHOD3(addReplyCaller, void(const std::string& requestReplyId,
                                      std::shared_ptr<joynr::IReplyCaller> replyCaller,
                                      const joynr::MessagingQos& qosSettings));
    MOCK_METHOD1(removeReplyCaller, void(const std::string& requestReplyId));
    MOCK_METHOD2(addRequestCaller, void(const std::string& participantId, std::shared_ptr<joynr::RequestCaller> requestCaller));
    MOCK_METHOD1(removeRequestCaller, void(const std::string& participantId));
    MOCK_METHOD1(receive, void(const joynr::JoynrMessage& message));
    MOCK_METHOD1(registerSubscriptionManager, void(std::shared_ptr<joynr::ISubscriptionManager> subscriptionManager));
    MOCK_METHOD1(registerPublicationManager,void(joynr::PublicationManager* publicationManager));
};

class MockInProcessDispatcher : public MockDispatcher , public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, std::shared_ptr<joynr::RequestCaller>(const std::string& participantId));
    MOCK_METHOD1(containsRequestCaller, bool(const std::string& participantId));
};

class MockMessaging : public joynr::IMessaging {
public:
  MOCK_METHOD2(transmit, void(joynr::JoynrMessage& message, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure));
  MOCK_METHOD2(test1, void(int a0, int a1));
};

class MockMessagingStubFactory : public joynr::IMessagingStubFactory {
public:
    MOCK_METHOD1(create, std::shared_ptr<joynr::IMessaging>(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&));
    MOCK_METHOD1(remove, void(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&));
    MOCK_METHOD1(contains, bool(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&));
};

class MockMessageRouter : public joynr::AbstractMessageRouter {
public:
    void invokeAddNextHopOnSuccessFct(const std::string& participantId,
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& inprocessAddress,
            std::function<void()> onSuccess) {
        if (onSuccess) {
            onSuccess();
        }
    }
    void invokeRemoveNextHopOnSuccessFct(const std::string& participantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) {
        if (onSuccess) {
            onSuccess();
        }
    }

    MockMessageRouter(boost::asio::io_service& ioService):
        AbstractMessageRouter(std::make_shared<MockMessagingStubFactory>(),
                      ioService,
                      nullptr,
                      0)
    {
        EXPECT_CALL(
                *this,
                addNextHop(_,_,_)
        )
                .WillRepeatedly(testing::Invoke(this, &MockMessageRouter::invokeAddNextHopOnSuccessFct));
        EXPECT_CALL(
                *this,
                removeNextHop(_,_,_)
        )
                .WillRepeatedly(testing::Invoke(this, &MockMessageRouter::invokeRemoveNextHopOnSuccessFct));
    }

    MOCK_METHOD2(route, void(const joynr::JoynrMessage& message, std::uint32_t tryCount));

    MOCK_METHOD6(registerMulticastReceiver, void(const std::string& multicastId,
                                                 const std::string& subscriberParticipantId,
                                                 const std::string& providerParticipantId,
                                                 std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress,
                                                 std::function<void()> onSuccess,
                                                 std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError));

    MOCK_METHOD3(addNextHop, void(
            const std::string& participantId,
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& inprocessAddress,
            std::function<void()> onSuccess));

    MOCK_METHOD3(removeNextHop, void(
            const std::string& participantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD5(addMulticastReceiver, void(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD5(removeMulticastReceiver, void(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD1(queueMessage, void(const joynr::JoynrMessage& message));
};

class MockJoynrMessageSender : public joynr::IJoynrMessageSender {
public:

    MOCK_METHOD1(
            registerDispatcher,
            void(joynr::IDispatcher* dispatcher)
    );

    MOCK_METHOD5(
            sendRequest,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::Request& request,
                std::shared_ptr<joynr::IReplyCaller> callback
            )
    );
    
    MOCK_METHOD4(
            sendOneWayRequest,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::OneWayRequest& request
            )
    );

    MOCK_METHOD4(
            sendReply,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::Reply& reply
            )
    );

    MOCK_METHOD4(
            sendSubscriptionRequest,
            void(
                const std::string &senderParticipantId,
                const std::string &receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionRequest& subscriptionRequest
            )
    );

    MOCK_METHOD4(
            sendBroadcastSubscriptionRequest,
            void(
                const std::string &senderParticipantId,
                const std::string &receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::BroadcastSubscriptionRequest& subscriptionRequest
            )
    );

    MOCK_METHOD4(
            sendSubscriptionReply,
            void(
                const std::string &senderParticipantId,
                const std::string &receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionReply& subscriptionReply
            )
    );

    MOCK_METHOD4(
            sendSubscriptionStop,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionStop& subscriptionStop
            )
    );

    MOCK_METHOD4(
            sendSubscriptionPublicationMock,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionPublication& subscriptionPublication
            )
    );

    MOCK_METHOD3(
            sendMulticast,
            void (const std::string& fromParticipantId,
                  const joynr::MulticastPublication& multicastPublication,
                  const joynr::MessagingQos& messagingQos
            )
   );

    MOCK_METHOD4(
            sendMulticastSubscriptionRequest,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::MulticastSubscriptionRequest& subscriptionRequest
            )
    );

    void sendSubscriptionPublication(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const joynr::MessagingQos& qos,
        joynr::SubscriptionPublication&& subscriptionPublication
    ){
        sendSubscriptionPublicationMock(senderParticipantId,receiverParticipantId,qos,subscriptionPublication);
    }
};

template <typename T>
class MockReplyCaller : public joynr::ReplyCaller<T> {
public:
    MockReplyCaller(std::function<void(const T& returnValue)> callbackFct,
                    std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>& error)> errorFct) : joynr::ReplyCaller<T>(callbackFct, errorFct) {}
    MOCK_METHOD1_T(returnValue, void(const T& payload));
    MOCK_METHOD0_T(timeOut, void());
    MOCK_CONST_METHOD0_T(getType, std::string());
};

class MockGpsFloatSubscriptionListener
        : public joynr::ISubscriptionListener<joynr::types::Localisation::GpsLocation, float> {
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(onReceive, void(const joynr::types::Localisation::GpsLocation& value, const float&));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

class MockPublicationSender : public joynr::IPublicationSender {
public:
    MOCK_METHOD4(
            sendSubscriptionPublicationMock,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionPublication& subscriptionPublication
            )
    );

    MOCK_METHOD4(
            sendSubscriptionReply,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionReply& subscriptionReply
            )
    );

    void sendSubscriptionPublication(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const joynr::MessagingQos& qos,
        joynr::SubscriptionPublication&& subscriptionPublication
    ){
        sendSubscriptionPublicationMock(senderParticipantId,receiverParticipantId,qos,subscriptionPublication);
    }
};

class MockDiscovery : public joynr::system::IDiscovery {
public:
    MOCK_METHOD1(
            add,
            void(
                const joynr::types::DiscoveryEntry& entry
            )
    );
    MOCK_METHOD2(
            lookup,
            void(
                joynr::types::DiscoveryEntryWithMetaInfo& result,
                const std::string& participantId
            )
    );
    MOCK_METHOD4(
            lookup,
            void(
                std::vector<joynr::types::DiscoveryEntryWithMetaInfo> & result,
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos
            )
    );
    MOCK_METHOD1(
            remove,
            void(
                const std::string& participantId
            )
    );
    MOCK_METHOD3(
            addAsync,
            std::shared_ptr<joynr::Future<void>>(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
    MOCK_METHOD3(
            lookupAsync,
            std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
    MOCK_METHOD5(
            lookupAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>(
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
    MOCK_METHOD3(
            removeAsync,
            std::shared_ptr<joynr::Future<void>>(
                const std::string& participantId,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
};

class IMockProviderInterface {
public:
    virtual ~IMockProviderInterface() = default;
    static const std::string& INTERFACE_NAME();
};


class MockProvider : public joynr::AbstractJoynrProvider, public IMockProviderInterface {
public:
    static const std::uint32_t MAJOR_VERSION;
    static const std::uint32_t MINOR_VERSION;
    MOCK_CONST_METHOD0(getProviderQos, joynr::types::ProviderQos());
    MOCK_CONST_METHOD0(getParticipantId, std::string());
    ~MockProvider() override = default;
    std::string getInterfaceName() const override;
};

namespace joynr
{
template <>
inline std::shared_ptr<RequestCaller> RequestCallerFactory::create<MockProvider>(std::shared_ptr<MockProvider> provider)
{
    std::ignore = provider;
    return std::shared_ptr<RequestCaller>(nullptr);
}
} // namespace joynr;

class MockMessageReceiver : public joynr::IMessageReceiver
{
public:
    MockMessageReceiver() = default;
    MOCK_METHOD0(init, void());
    MOCK_CONST_METHOD0(getGlobalClusterControllerAddress, std::string&());
    MOCK_METHOD0(startReceiveQueue, void());
    MOCK_METHOD0(stopReceiveQueue, void());
    MOCK_METHOD0(updateSettings, void());
    MOCK_METHOD0(tryToDeleteChannel, bool());
    MOCK_METHOD1(registerReceiveCallback, void(std::function<void(const std::string&)> onTextMessageReceived));
    MOCK_METHOD0(isConnected, bool());
};

class MockMessageSender : public joynr::IMessageSender
{
public:
    MOCK_METHOD3(sendMessage,void(const joynr::system::RoutingTypes::Address&, const joynr::JoynrMessage&, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&));
    MOCK_METHOD1(init,void(const joynr::MessagingSettings& settings));
    MOCK_METHOD1(registerReceiver, void(std::shared_ptr<joynr::IMessageReceiver> receiver));
};

class MockMessagingStub : public joynr::IMessaging {
public:
    MOCK_METHOD2(transmit, void(joynr::JoynrMessage& message, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure));
};

class GlobalCapabilitiesMock {
public:
    MOCK_METHOD1(capabilitiesReceived, void(const std::vector<joynr::types::GlobalDiscoveryEntry>& results));
};

class MockRoutingProxy : public virtual joynr::system::RoutingProxy {
public:
    MockRoutingProxy() :
        RoutingProxy(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                "domain",
                joynr::MessagingQos()),
        ProxyBase(
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingProxyBase(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingSyncProxy(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingAsyncProxy(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                "domain",
                joynr::MessagingQos())
    { }

    MOCK_METHOD3(resolveNextHopAsync,
                 std::shared_ptr<joynr::Future<bool>>(
                     const std::string& participantId,
                     std::function<void(const bool& resolved)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD5(addMulticastReceiverAsync,
        std::shared_ptr<joynr::Future<void>> (
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD5(removeMulticastReceiverAsync,
        std::shared_ptr<joynr::Future<void>> (
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));
};

class MockMessagingMulticastSubscriber : public joynr::IMessagingMulticastSubscriber
{
public:

    MOCK_METHOD1(registerMulticastSubscription, void(const std::string& multicastId));
    MOCK_METHOD1(unregisterMulticastSubscription, void(const std::string& multicastId));
    MOCK_METHOD2(transmit, void (joynr::JoynrMessage& message, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure));
};

class MockGpsProvider : public joynr::vehicle::DefaultGpsProvider
{
public:
    MockGpsProvider() : joynr::vehicle::DefaultGpsProvider()
    {
    }

    ~MockGpsProvider()
    {
        JOYNR_LOG_DEBUG(logger, "I am being destroyed");
    }

    MOCK_METHOD1(getLocation, void(joynr::types::Localisation::GpsLocation& result) );
    MOCK_METHOD1(setLocation, void(joynr::types::Localisation::GpsLocation gpsLocation));

    std::string getParticipantId() const
    {
        return "Fake_ParticipantId_vehicle/DefaultGpsProvider";
    }
private:
    ADD_LOGGER(MockGpsProvider);
};

class MockTestRequestCaller : public joynr::tests::testRequestCaller {
public:
    void invokeLocationOnSuccessFct(std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess,
                            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError) {
        joynr::types::Localisation::GpsLocation location;
        onSuccess(location);
    }

    void invokeListOfStringsOnSuccessFct(std::function<void(const std::vector<std::string>&)> onSuccess,
                            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError) {
        std::vector<std::string> listOfStrings;
        listOfStrings.push_back("firstString");
        onSuccess(listOfStrings);
    }

    void invokeGetterOnErrorFunctionWithProviderRuntimeException(std::function<void(const std::int32_t&)> onSuccess,
            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError) {
        onError(std::make_shared<joynr::exceptions::ProviderRuntimeException>(providerRuntimeExceptionTestMsg));
    }

    void invokeMethodOnErrorFunctionWithProviderRuntimeException(std::function<void()> onSuccess,
            std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)> onError) {
        onError(std::make_shared<joynr::exceptions::ProviderRuntimeException>(providerRuntimeExceptionTestMsg));
    }

    void invokeMapParametersOnSuccessFct(const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
                                         std::function<void(const joynr::types::TestTypes::TStringKeyMap&)> onSuccess,
                                         std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)>) {
        onSuccess(tStringMapIn);
    }

    const joynr::types::Version& getProviderVersion() const {
        return providerVersion;
    }

    MockTestRequestCaller() :
            providerVersion(47, 11),
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>())
    {
        ON_CALL(
                *this,
                getLocationMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeLocationOnSuccessFct));
        ON_CALL(
                *this,
                getListOfStringsMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeListOfStringsOnSuccessFct));
        ON_CALL(
                *this,
                getAttributeWithProviderRuntimeExceptionMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeGetterOnErrorFunctionWithProviderRuntimeException));
        ON_CALL(
                *this,
                methodWithProviderRuntimeExceptionMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeMethodOnErrorFunctionWithProviderRuntimeException));
        ON_CALL(
                *this,
                mapParametersMock(_,_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeMapParametersOnSuccessFct));

    }
    MockTestRequestCaller(testing::Cardinality getLocationCardinality) :
            providerVersion(47, 11),
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>())
    {
        EXPECT_CALL(
                *this,
                getLocationMock(_,_)
        )
                .Times(getLocationCardinality)
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeLocationOnSuccessFct));
        EXPECT_CALL(
                *this,
                getListOfStringsMock(_,_)
        )
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeListOfStringsOnSuccessFct));
    }

    // GoogleMock does not support mocking functions with r-value references as parameters
    MOCK_METHOD2(getLocationMock,
                 void(std::function<void(const joynr::types::Localisation::GpsLocation& location)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>));

    void getLocation(
                std::function<void(const joynr::types::Localisation::GpsLocation&)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError
        ) override
    {
        getLocationMock(onSuccess, onError);
    }

    MOCK_METHOD3(mapParametersMock,
                 void(const joynr::types::TestTypes::TStringKeyMap&,
                      std::function<void(const joynr::types::TestTypes::TStringKeyMap&)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)>));

    void mapParameters(
                const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
                std::function<void(const joynr::types::TestTypes::TStringKeyMap& tStringMapOut)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)> onError
        ) override
    {
        mapParametersMock(tStringMapIn, onSuccess, onError);
    }

    MOCK_METHOD2(getListOfStringsMock,
                 void(std::function<void(const std::vector<std::string>& listOfStrings)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>));

    void getListOfStrings(
                std::function<void(const std::vector<std::string>&)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError
        ) override
    {
        getListOfStringsMock(onSuccess, onError);
    }

    MOCK_METHOD2(getAttributeWithProviderRuntimeExceptionMock,
                 void(std::function<void(const std::int32_t&)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>));

    void getAttributeWithProviderRuntimeException(
                std::function<void(const std::int32_t&)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError
        ) override
    {
        getAttributeWithProviderRuntimeExceptionMock(onSuccess, onError);
    }

    MOCK_METHOD2(methodWithProviderRuntimeExceptionMock,
                 void(std::function<void()>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)>));

    void methodWithProviderRuntimeException(
                std::function<void()>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)> onError
        ) override
    {
        methodWithProviderRuntimeExceptionMock(onSuccess, onError);
    }

    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, joynr::SubscriptionAttributeListener* attributeListener));
    MOCK_METHOD2(registerBroadcastListener, void(const std::string& broadcastName, joynr::UnicastBroadcastListener* broadcastListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, joynr::SubscriptionAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterBroadcastListener, void(const std::string& broadcastName, joynr::UnicastBroadcastListener* broadcastListener));

    std::string providerRuntimeExceptionTestMsg = "ProviderRuntimeExceptionTestMessage";

private:
    joynr::types::Version providerVersion;
};

class MockGpsRequestCaller : public joynr::vehicle::GpsRequestCaller {
public:
    MockGpsRequestCaller() : joynr::vehicle::GpsRequestCaller(std::make_shared<MockGpsProvider>() ) {}
    MOCK_METHOD2(getLocationMock, void(std::function<void(const joynr::types::Localisation::GpsLocation&)>,
                                       std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>));
    
    void getLocation(
                std::function<void(const joynr::types::Localisation::GpsLocation&)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError
        ) override
    {
        getLocationMock(onSuccess, onError);
    }

    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, joynr::SubscriptionAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, joynr::SubscriptionAttributeListener* attributeListener));
};


class MockIRequestCallerDirectory : public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, std::shared_ptr<joynr::RequestCaller>(const std::string& participantId));
    MOCK_METHOD1(containsRequestCaller, bool(const std::string& participantId));
};

class MockSubscriptionManager : public joynr::SubscriptionManager {
public:
    using SubscriptionManager::SubscriptionManager;

    MOCK_METHOD1(getSubscriptionCallback,std::shared_ptr<joynr::ISubscriptionCallback>(const std::string& subscriptionId));
    MOCK_METHOD5(registerSubscription,void(const std::string& subscribeToName,
                                           std::shared_ptr<joynr::ISubscriptionCallback> subscriptionCaller,
                                           std::shared_ptr<joynr::ISubscriptionListenerBase> subscriptionListener,
                                           std::shared_ptr<joynr::SubscriptionQos> qos,
                                           joynr::SubscriptionRequest& subscriptionRequest));
    MOCK_METHOD10(registerSubscription,void(const std::string& subscribeToName,
                                           const std::string& subscriberParticipantId,
                                           const std::string& providerParticipantId,
                                           const std::vector<std::string>& partitions,
                                           std::shared_ptr<joynr::ISubscriptionCallback> subscriptionCaller,
                                           std::shared_ptr<joynr::ISubscriptionListenerBase> subscriptionListener,
                                           std::shared_ptr<joynr::SubscriptionQos> qos,
                                           joynr::MulticastSubscriptionRequest& subscriptionRequest,
                                           std::function<void()> onSuccess,
                                           std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));
    MOCK_METHOD1(unregisterSubscription, void(const std::string& subscriptionId));
    MOCK_METHOD1(touchSubscriptionState,void(const std::string& subscriptionId));
    MOCK_METHOD1(
        getMulticastSubscriptionCallback,
        std::shared_ptr<joynr::ISubscriptionCallback>(const std::string& multicastId)
    );
    MOCK_METHOD1(
        getSubscriptionListener,
        std::shared_ptr<joynr::ISubscriptionListenerBase>(
                const std::string& subscriptionId
        )
    );
    MOCK_METHOD1(
        getMulticastSubscriptionListeners,
        std::forward_list<std::shared_ptr<joynr::ISubscriptionListenerBase>>(
                const std::string& multicastId
        )
    );
};

class MockSubscriptionCallback : public joynr::ISubscriptionCallback {
public:
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
    MOCK_METHOD1(executePublication, void(joynr::BasePublication& publication));
    MOCK_METHOD1(execute, void(const joynr::SubscriptionReply& subscriptionReply));

    void execute(joynr::BasePublication&& subscriptionPublication) override {
        executePublication(subscriptionPublication);
    }
};

class MockParticipantIdStorage : public joynr::ParticipantIdStorage {
public:
    MockParticipantIdStorage() : ParticipantIdStorage(std::string("mock filename")) {

    }
    MOCK_METHOD2(getProviderParticipantId, std::string(const std::string& domain, const std::string& interfaceName));
    MOCK_METHOD3(getProviderParticipantId, std::string(const std::string& domain, const std::string& interfaceName, const std::string& defaultValue));
};

class MockLocationUpdatedSelectiveFilter : public joynr::tests::TestLocationUpdateSelectiveBroadcastFilter {
public:
    MOCK_METHOD2(filter,
                 bool(
                     const joynr::types::Localisation::GpsLocation &location,
                     const joynr::tests::TestLocationUpdateSelectiveBroadcastFilterParameters &filterParameters));
};
class MockGlobalDomainAccessControllerProxy : public virtual joynr::infrastructure::GlobalDomainAccessControllerProxy {
public:
    MockGlobalDomainAccessControllerProxy() :
        GlobalDomainAccessControllerProxy(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                  "domain",
                joynr::MessagingQos()),
        ProxyBase(
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainAccessControllerProxyBase(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainAccessControllerSyncProxy(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainAccessControllerAsyncProxy(
                std::make_shared<const joynr::system::RoutingTypes::Address>(),
                nullptr,
                "domain",
                joynr::MessagingQos())
    {
    }

    MOCK_METHOD3(
            getDomainRolesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>& domainRoleEntries
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

    MOCK_METHOD4(
            getMasterAccessControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& masterAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

    MOCK_METHOD4(
            getMediatorAccessControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& mediatorAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

    MOCK_METHOD4(
            getOwnerAccessControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>& ownerAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

};

class MockLocalDomainAccessController : public joynr::LocalDomainAccessController {
public:
    using joynr::LocalDomainAccessController::LocalDomainAccessController;

    MOCK_METHOD5(getConsumerPermission,
                 void(
                     const std::string& userId,
                     const std::string& domain,
                     const std::string& interfaceName,
                     joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                     std::shared_ptr<joynr::LocalDomainAccessController::IGetConsumerPermissionCallback> callback));

    MOCK_METHOD5(getConsumerPermission,
                 joynr::infrastructure::DacTypes::Permission::Enum(
                     const std::string& userId,
                     const std::string& domain,
                     const std::string& interfaceName,
                     const std::string& operation,
                     joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel));
};

class MockMessagingSettings : public joynr::MessagingSettings {
public:
    MockMessagingSettings(joynr::Settings& settings):
        MessagingSettings(settings){}
    MOCK_METHOD0(
            getDiscoveryDirectoriesDomain,
            std::string());
    MOCK_METHOD0(
            getCapabilitiesDirectoryParticipantId,
            std::string());
};

class MockLocalCapabilitiesDirectory : public joynr::LocalCapabilitiesDirectory {
public:
    MockLocalCapabilitiesDirectory(MockMessagingSettings& messagingSettings, joynr::Settings& settings, boost::asio::io_service& ioService):
        messageRouter(ioService),
        libjoynrMockSettings(settings),
        LocalCapabilitiesDirectory(messagingSettings, nullptr, "localAddress", messageRouter, libjoynrMockSettings, ioService, "clusterControllerId"){}

    MOCK_METHOD3(
            lookup,
            void(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
                std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError
            ));

private:
    MockMessageRouter messageRouter;
    joynr::LibjoynrSettings libjoynrMockSettings;
};

class MockConsumerPermissionCallback : public joynr::IAccessController::IHasConsumerPermissionCallback
{
public:
    MOCK_METHOD1(hasConsumerPermission, void(bool hasPermission));
};

class MockWebSocketClient : public joynr::IWebSocketPpClient
{
public:

    MockWebSocketClient(joynr::WebSocketSettings wsSettings, boost::asio::io_service& ioService)
    {}
    MOCK_METHOD0(dtorCalled, void());
    ~MockWebSocketClient() override
    {
        dtorCalled();
    }

    MOCK_METHOD1(registerConnectCallback, void(std::function<void()>));
    MOCK_METHOD1(registerReconnectCallback, void(std::function<void()>));
    MOCK_METHOD1(registerReceiveCallback, void(std::function<void(const std::string&)>));

    void registerDisconnectCallback(std::function<void()> callback) override
    {
        onConnectionClosedCallback = callback;
    }

    void signalDisconnect()
    {
        onConnectionClosedCallback();
    }

    MOCK_METHOD1(connect, void(const joynr::system::RoutingTypes::WebSocketAddress&));
    MOCK_METHOD0(close, void());

    MOCK_CONST_METHOD0(isConnected, bool());

    MOCK_METHOD2(send, void(
            const std::string&,
            const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&));

    MOCK_CONST_METHOD0(getSender, std::shared_ptr<joynr::IWebSocketSendInterface>());

private:
    std::function<void()> onConnectionClosedCallback;
};

class MockWebSocketSendInterface : public joynr::IWebSocketSendInterface {
public:
    MOCK_METHOD2(send, void (const std::string& message,
                             const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure));
    MOCK_CONST_METHOD0(isInitialized, bool ());
    MOCK_CONST_METHOD0(isConnected, bool ());
};

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif // MOCKOBJECTS_H_

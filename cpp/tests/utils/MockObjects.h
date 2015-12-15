/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "PrettyPrint.h"
#include "LibJoynrMockObjects.h"

#include "cluster-controller/access-control/IAccessController.h"
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
#include "joynr/IClientCache.h"
#include "joynr/ReplyCaller.h"
#include "joynr/ISubscriptionListener.h"
#include "cluster-controller/capabilities-client/IGlobalCapabilitiesCallback.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MessageRouter.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/JoynrMessageSender.h"

#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/BroadcastSubscriptionRequest.h"

#include "joynr/RequestCallerFactory.h"
#include "joynr/vehicle/GpsProvider.h"

#include "joynr/IMessagingStubFactory.h"
#include "joynr/IRequestCallerDirectory.h"

#include "joynr/ClusterControllerDirectories.h"

#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "common/in-process/InProcessMessagingSkeleton.h"
#include "joynr/InProcessConnectorFactory.h"
#include "cluster-controller/http-communication-manager/HttpReceiver.h"

#include "joynr/infrastructure/ChannelUrlDirectoryProxy.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerProxy.h"

#include "joynr/MessageRouter.h"

#include "joynr/ILocalChannelUrlDirectory.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/MessagingSettings.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/PublicationManager.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ChannelUrlInformation.h"
#include "joynr/IMessageSender.h"
#include "joynr/BrokerUrl.h"
#include "joynr/Directory.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Variant.h"
#include "joynr/Settings.h"
#include "joynr/Logger.h"

#include "libjoynr/websocket/WebSocketClient.h"
#include "runtimes/cluster-controller-runtime/websocket/QWebSocketSendWrapper.h"

using ::testing::A;
using ::testing::_;
using ::testing::A;
using ::testing::Eq;
using ::testing::AllOf;
using ::testing::Property;

// Disable VC++ warnings due to google mock
// http://code.google.com/p/googlemock/wiki/FrequentlyAskedQuestions#MSVC_gives_me_warning_C4301_or_C4373_when_I_define_a_mock_method
#ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable : 4373 )
#endif

// Disable compiler warnings.
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wreorder"


class MockCapabilitiesClient : public joynr::ICapabilitiesClient {
public:
    MOCK_METHOD1(add, void(std::vector<joynr::types::CapabilityInformation> capabilitiesInformationList));
    MOCK_METHOD1(remove, void(std::vector<std::string> participantIdList));
    MOCK_METHOD1(remove, void(const std::string& participantId));
    MOCK_METHOD2(lookup, std::vector<joynr::types::CapabilityInformation>(const std::string& domain, const std::string& interfaceName));
    MOCK_METHOD4(lookup, void(
                     const std::string& domain,
                     const std::string& interfaceName,
                     std::function<void(const std::vector<joynr::types::CapabilityInformation>& capabilities)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD3(lookup, void(
                     const std::string& participantId,
                     std::function<void(const std::vector<joynr::types::CapabilityInformation>& capabilities)> callbackFct,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD0(getLocalChannelId, std::string());

};

class MockInProcessMessagingSkeleton : public joynr::InProcessMessagingSkeleton
{
public:
    MOCK_METHOD1(transmit, void(joynr::JoynrMessage& message));
};

class MockDelayedScheduler : public joynr::DelayedScheduler
{
public:
    MockDelayedScheduler()
        : DelayedScheduler([](joynr::Runnable*){ assert(false); }, std::chrono::milliseconds::zero())
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

    MOCK_METHOD1(canBeCreated, bool(const std::shared_ptr<joynr::system::RoutingTypes::Address> address));
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
    MOCK_METHOD1(registerSubscriptionManager, void(joynr::ISubscriptionManager* subscriptionManager));
    MOCK_METHOD1(registerPublicationManager,void(joynr::PublicationManager* publicationManager));
};

class MockInProcessDispatcher : public MockDispatcher , public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, std::shared_ptr<joynr::RequestCaller>(const std::string& participantId));
    MOCK_METHOD1(containsRequestCaller, bool(const std::string& participantId));
};

class MockMessaging : public joynr::IMessaging {
public:
  MOCK_METHOD1(transmit, void(joynr::JoynrMessage& message));
  MOCK_METHOD2(test1, void(int a0, int a1));
};

class MockMessageRouter : public joynr::MessageRouter {
public:
    MockMessageRouter():
        MessageRouter(nullptr, nullptr, 0) {

    }
    MOCK_METHOD1(route, void(const joynr::JoynrMessage& message));
    MOCK_METHOD2(addNextHop, void(std::string participantId, std::shared_ptr<joynr::system::RoutingTypes::Address> inprocessAddress));
    MOCK_METHOD1(removeNextHop, void(std::string participantId));
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
            sendSubscriptionPublication,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionPublication& subscriptionPublication
            )
    );
};

template <typename T>
class MockReplyCaller : public joynr::ReplyCaller<T> {
public:
    MockReplyCaller(std::function<void(const joynr::RequestStatus& status, const T& returnValue)> callbackFct,
                    std::function<void(const joynr::RequestStatus& status, const joynr::exceptions::JoynrException& error)> errorFct) : joynr::ReplyCaller<T>(callbackFct, errorFct) {}
    MOCK_METHOD1_T(returnValue, void(const T& payload));
    MOCK_METHOD0_T(timeOut, void());
    MOCK_CONST_METHOD0_T(getType, std::string());
};

class MockGpsFloatSubscriptionListener
        : public joynr::ISubscriptionListener<joynr::types::Localisation::GpsLocation, float> {
public:
    MOCK_METHOD2(onReceive, void(const joynr::types::Localisation::GpsLocation& value, const float&));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

class MockPublicationSender : public joynr::IPublicationSender {
public:
    MOCK_METHOD4(
            sendSubscriptionPublication,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionPublication& subscriptionPublication
            )
    );

};

class MockClientCache : public joynr::IClientCache {
public:
   MOCK_METHOD1(lookUp, joynr::Variant(const std::string& attributeId));
   MOCK_METHOD2(insert, void(std::string attributeId, joynr::Variant value));
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
                joynr::types::DiscoveryEntry& result,
                const std::string& participantId
            )
    );
    MOCK_METHOD4(
            lookup,
            void(
                std::vector<joynr::types::DiscoveryEntry> & result,
                const std::string& domain,
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
            std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntry>>(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntry& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
    MOCK_METHOD5(
            lookupAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                std::function<void(const std::vector<joynr::types::DiscoveryEntry>& result)>
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
    MOCK_CONST_METHOD0(getProviderQos, joynr::types::ProviderQos());
    MOCK_CONST_METHOD0(getParticipantId, std::string());
    ~MockProvider() override = default;
    std::string getInterfaceName() const override;
};

namespace joynr {

template<>
class RequestCallerFactoryHelper<MockProvider> {
public:
    std::shared_ptr<RequestCaller> create(std::shared_ptr<MockProvider> provider) {
        return std::shared_ptr<RequestCaller>(nullptr);
    }
};
} // namespace joynr

class MockMessageReceiver : public joynr::IMessageReceiver
{
public:
    MockMessageReceiver() = default;
    MOCK_METHOD1(init, void(std::shared_ptr<joynr::ILocalChannelUrlDirectory> channelUrlDirectory));
    MOCK_CONST_METHOD0(getReceiveChannelId, std::string&());
    MOCK_METHOD0(startReceiveQueue, void());
    MOCK_METHOD0(stopReceiveQueue, void());
    MOCK_METHOD0(waitForReceiveQueueStarted, void());
    MOCK_METHOD0(updateSettings, void());
    MOCK_METHOD0(tryToDeleteChannel, bool());
};

class MockMessageSender : public joynr::IMessageSender
{
public:
    MOCK_METHOD2(sendMessage,void(const std::string&, const joynr::JoynrMessage&));
    MOCK_METHOD2(init,void(std::shared_ptr<joynr::ILocalChannelUrlDirectory> channelUrlDirectory,const joynr::MessagingSettings& settings));
};

/*
 * Typed Callbacks
 */
template <typename ... Ts>
class MockCallback{
public:
    MOCK_METHOD1_T(onSuccess, void(const Ts&... result));
    MOCK_METHOD1_T(onError, void(const joynr::exceptions::JoynrException& error));
};

template<>
class MockCallback<void> {
public:
    MOCK_METHOD0(onSuccess, void(void));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrException& error));
};

template <typename T, typename ErrorEnum>
class MockCallbackWithApplicationError {
public:
    MOCK_METHOD1_T(onSuccess, void(const T& result));
    MOCK_METHOD1_T(onApplicationError, void(const ErrorEnum& errorEnum));
    MOCK_METHOD1_T(onRuntimeError, void(const joynr::exceptions::JoynrRuntimeException& runtimeError));
};

template <typename ErrorEnum>
class MockCallbackWithApplicationError<void, ErrorEnum> {
public:
    MOCK_METHOD0_T(onSuccess, void(void));
    MOCK_METHOD1_T(onApplicationError, void(const ErrorEnum& errorEnum));
    MOCK_METHOD1_T(onRuntimeError, void(const joynr::exceptions::JoynrRuntimeException& runtimeError));
};

template <typename ... Ts>
class MockCallbackWithOnErrorHavingRequestStatus{
public:
    MOCK_METHOD1_T(onSuccess, void(const Ts&... result));
    MOCK_METHOD2_T(onError, void(const joynr::RequestStatus& status,
                const joynr::exceptions::JoynrException& error));
};

template<>
class MockCallbackWithOnErrorHavingRequestStatus<void> {

public:
    MOCK_METHOD0(onSuccess, void(void));
    MOCK_METHOD2(onError, void(const joynr::RequestStatus& status,
            const joynr::exceptions::JoynrException& error));
};

class MockMessagingStubFactory : public joynr::IMessagingStubFactory {
public:
    MOCK_METHOD1(create, std::shared_ptr<joynr::IMessaging>(const joynr::system::RoutingTypes::Address& destEndpointAddress));
    MOCK_METHOD1(remove, void(const joynr::system::RoutingTypes::Address& destEndpointAddress));
    MOCK_METHOD1(contains, bool(const joynr::system::RoutingTypes::Address& destEndpointAddress));
};

class GlobalCapabilitiesMock {
public:
    MOCK_METHOD1(capabilitiesReceived, void(const std::vector<joynr::types::CapabilityInformation>& results));
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
                            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) {
        joynr::types::Localisation::GpsLocation location;
        onSuccess(location);
    }

    void invokeListOfStringsOnSuccessFct(std::function<void(const std::vector<std::string>&)> onSuccess,
                            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) {
        std::vector<std::string> listOfStrings;
        listOfStrings.push_back("firstString");
        onSuccess(listOfStrings);
    }

    void invokeGetterOnErrorFunctionWithProviderRuntimeException(std::function<void(const std::int32_t&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) {
        onError(joynr::exceptions::ProviderRuntimeException(providerRuntimeExceptionTestMsg));
    }

    void invokeMethodOnErrorFunctionWithProviderRuntimeException(std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrException&)> onError) {
        onError(joynr::exceptions::ProviderRuntimeException(providerRuntimeExceptionTestMsg));
    }

    void invokeMapParametersOnSuccessFct(const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
                                         std::function<void(const joynr::types::TestTypes::TStringKeyMap&)> onSuccess,
                                         std::function<void(const joynr::exceptions::JoynrException&)>) {
        onSuccess(tStringMapIn);
    }

    MockTestRequestCaller() :
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>())
    {
        ON_CALL(
                *this,
                getLocation(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeLocationOnSuccessFct));
        ON_CALL(
                *this,
                getListOfStrings(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeListOfStringsOnSuccessFct));
        ON_CALL(
                *this,
                getAttributeWithProviderRuntimeException(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeGetterOnErrorFunctionWithProviderRuntimeException));
        ON_CALL(
                *this,
                methodWithProviderRuntimeException(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeMethodOnErrorFunctionWithProviderRuntimeException));
        ON_CALL(
                *this,
                mapParameters(_,_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeMapParametersOnSuccessFct));

    }
    MockTestRequestCaller(testing::Cardinality getLocationCardinality) :
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>())
    {
        EXPECT_CALL(
                *this,
                getLocation(_,_)
        )
                .Times(getLocationCardinality)
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeLocationOnSuccessFct));
        EXPECT_CALL(
                *this,
                getListOfStrings(_,_)
        )
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeListOfStringsOnSuccessFct));
    }

    MOCK_METHOD2(getLocation,
                 void(std::function<void(const joynr::types::Localisation::GpsLocation& location)>,
                      std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>));
    MOCK_METHOD3(mapParameters,
                 void(const joynr::types::TestTypes::TStringKeyMap&,
                      std::function<void(const joynr::types::TestTypes::TStringKeyMap&)>,
                      std::function<void(const joynr::exceptions::JoynrException&)>));
    MOCK_METHOD2(getListOfStrings,
                 void(std::function<void(const std::vector<std::string>& listOfStrings)>,
                      std::function<void(const joynr::exceptions::JoynrException& exception)>));
    MOCK_METHOD2(getAttributeWithProviderRuntimeException,
                 void(std::function<void(const std::int32_t&)>,
                      std::function<void(const joynr::exceptions::ProviderRuntimeException&)>));
    MOCK_METHOD2(methodWithProviderRuntimeException,
                 void(std::function<void()>,
                      std::function<void(const joynr::exceptions::JoynrException&)>));
    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(registerBroadcastListener, void(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterBroadcastListener, void(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener));

    std::string providerRuntimeExceptionTestMsg = "ProviderRuntimeExceptionTestMessage";
};

class MockGpsRequestCaller : public joynr::vehicle::GpsRequestCaller {
public:
    MockGpsRequestCaller() : joynr::vehicle::GpsRequestCaller(std::make_shared<MockGpsProvider>() ) {}
    MOCK_METHOD2(getLocation, void(std::function<void(const joynr::types::Localisation::GpsLocation& location)>,
                                   std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>));
    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
};


class MockIRequestCallerDirectory : public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, std::shared_ptr<joynr::RequestCaller>(const std::string& participantId));
    MOCK_METHOD1(containsRequestCaller, bool(const std::string& participantId));
};


template <typename Key, typename T>
class MockDirectory : public joynr::IDirectory<Key, T> {
public:
    MOCK_METHOD1_T(lookup, std::shared_ptr< T >(const Key& keyId));
    MOCK_METHOD1_T(contains, bool(const Key& keyId));

    MOCK_METHOD2_T(add, void(const Key &keyId, T* value));
    MOCK_METHOD2_T(add, void(const Key& keyId, std::shared_ptr < T > value));

    MOCK_METHOD3_T(add, void(const Key &keyId, T* value, std::int64_t ttl_ms));
    MOCK_METHOD3_T(add, void(const Key& keyId, std::shared_ptr < T > value, std::int64_t ttl_ms));
    MOCK_METHOD1_T(remove, void(const Key& keyId));
};

typedef MockDirectory<std::string, joynr::system::RoutingTypes::Address> MockMessagingEndpointDirectory;




class MockSubscriptionManager : public joynr::SubscriptionManager {
public:
    MOCK_METHOD1(getSubscriptionCallback,std::shared_ptr<joynr::ISubscriptionCallback>(const std::string& subscriptionId));
    MOCK_METHOD4(registerSubscription,void(const std::string& subscribeToName,
                                                    std::shared_ptr<joynr::ISubscriptionCallback> subscriptionCaller, // SubMgr gets ownership of ptr
                                                    const joynr::Variant& qosVariant,
                                                    joynr::SubscriptionRequest& subscriptionRequest));
    MOCK_METHOD1(unregisterSubscription, void(const std::string& subscriptionId));
    MOCK_METHOD1(touchSubscriptionState,void(const std::string& subscriptionId));
};


class MockPublicationManager : public joynr::PublicationManager {
public:
    MOCK_METHOD2(attributeValueChanged, void(const std::string& subscriptionId, const joynr::Variant& value));
};

class MockChannelUrlDirectoryProxy : public virtual joynr::infrastructure::ChannelUrlDirectoryProxy {
public:
    MockChannelUrlDirectoryProxy() :
        ChannelUrlDirectoryProxy(std::make_shared<joynr::system::RoutingTypes::Address>(), nullptr, nullptr, "domain", joynr::MessagingQos(), false),
        ProxyBase(nullptr, nullptr, "domain", joynr::MessagingQos(), false),
        ChannelUrlDirectoryProxyBase(std::make_shared<joynr::system::RoutingTypes::Address>(), nullptr, nullptr, "domain", joynr::MessagingQos(), false),
        ChannelUrlDirectorySyncProxy(std::make_shared<joynr::system::RoutingTypes::Address>(), nullptr, nullptr, "domain", joynr::MessagingQos(), false),
        ChannelUrlDirectoryAsyncProxy(std::make_shared<joynr::system::RoutingTypes::Address>(), nullptr, nullptr, "domain", joynr::MessagingQos(), false){}

    MOCK_METHOD3(getUrlsForChannelAsync,
                 std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> (
                     const std::string& channelId,
                     std::function<void(const joynr::types::ChannelUrlInformation& urls)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
                 )
    );

    MOCK_METHOD4(registerChannelUrlsAsync,
                 std::shared_ptr<joynr::Future<void> >(
                     const std::string& channelId,
                     const joynr::types::ChannelUrlInformation& channelUrlInformation,
                     std::function<void(void)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
                 )
    );

    MOCK_METHOD3(unregisterChannelUrlsAsync,
                 std::shared_ptr<joynr::Future<void>>(
                     const std::string& channelId,
                     std::function<void(void)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
                 )
    );
};


class MockLocalChannelUrlDirectory : public joynr::ILocalChannelUrlDirectory {
public:
    MOCK_METHOD4(registerChannelUrlsAsync,
                 std::shared_ptr<joynr::Future<void>>(
                     const std::string& channelId,
                     joynr::types::ChannelUrlInformation channelUrlInformation,
                     std::function<void(void)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError
                 )
    );

    MOCK_METHOD3(unregisterChannelUrlsAsync,
                 std::shared_ptr<joynr::Future<void>>(
                     const std::string& channelId,
                     std::function<void(void)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError
                 )
    );

    MOCK_METHOD4(getUrlsForChannelAsync,
                 std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>>(
                     const std::string& channelId,
                     const std::chrono::milliseconds timeout,
                     std::function<void(const joynr::types::ChannelUrlInformation&)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError
                 )
    );
};

class MockParticipantIdStorage : public joynr::ParticipantIdStorage {
public:
    MockParticipantIdStorage() : ParticipantIdStorage(std::string("mock filename")) {

    }
    MOCK_METHOD3(getProviderParticipantId, std::string(const std::string& domain, const std::string& interfaceName, const std::string& authenticationToken));
    MOCK_METHOD4(getProviderParticipantId, std::string(const std::string& domain, const std::string& interfaceName, const std::string& defaultValue, const std::string& authenticationToken));
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
                std::shared_ptr<joynr::system::RoutingTypes::Address> (new joynr::system::RoutingTypes::Address()),
                nullptr,
                nullptr,
                "domain",
                joynr::MessagingQos(),
                false),
        ProxyBase(
                nullptr,
                nullptr,
                "domain",
                joynr::MessagingQos(),
                false),
        GlobalDomainAccessControllerProxyBase(
                std::shared_ptr<joynr::system::RoutingTypes::Address> (new joynr::system::RoutingTypes::Address()),
                nullptr,
                nullptr,
                "domain",
                joynr::MessagingQos(),
                false),
        GlobalDomainAccessControllerSyncProxy(
                std::shared_ptr<joynr::system::RoutingTypes::Address> (new joynr::system::RoutingTypes::Address()),
                nullptr,
                nullptr,
                "domain",
                joynr::MessagingQos(),
                false),
        GlobalDomainAccessControllerAsyncProxy(
                std::shared_ptr<joynr::system::RoutingTypes::Address> (new joynr::system::RoutingTypes::Address()),
                nullptr,
                nullptr,
                "domain",
                joynr::MessagingQos(),
                false)
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

    MOCK_METHOD3(subscribeToDomainRoleEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerDomainRoleEntryChangedBroadcastFilterParameters,
                     std::shared_ptr<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::DomainRoleEntry,
                                                                 joynr::infrastructure::DacTypes::ChangeType::Enum>>,
                     std::shared_ptr<joynr::OnChangeSubscriptionQos>));
    MOCK_METHOD3(subscribeToOwnerAccessControlEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerOwnerAccessControlEntryChangedBroadcastFilterParameters,
                     std::shared_ptr<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::OwnerAccessControlEntry,
                                                                 joynr::infrastructure::DacTypes::ChangeType::Enum>>,
                     std::shared_ptr<joynr::OnChangeSubscriptionQos>));
    MOCK_METHOD3(subscribeToMediatorAccessControlEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerMediatorAccessControlEntryChangedBroadcastFilterParameters,
                     std::shared_ptr<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::ChangeType::Enum,
                                                                 joynr::infrastructure::DacTypes::MasterAccessControlEntry>>,
                     std::shared_ptr<joynr::OnChangeSubscriptionQos>));
    MOCK_METHOD3(subscribeToMasterAccessControlEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerMasterAccessControlEntryChangedBroadcastFilterParameters,
                     std::shared_ptr<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::ChangeType::Enum,
                                                                 joynr::infrastructure::DacTypes::MasterAccessControlEntry>>,
                     std::shared_ptr<joynr::OnChangeSubscriptionQos>));

};

class MockLocalDomainAccessController : public joynr::LocalDomainAccessController {
public:
    MockLocalDomainAccessController(joynr::LocalDomainAccessStore* store):
        LocalDomainAccessController(store){}

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
    MockLocalCapabilitiesDirectory(MockMessagingSettings& messagingSettings):
        messageRouter(),
        LocalCapabilitiesDirectory(messagingSettings,nullptr, messageRouter){}

    MOCK_METHOD3(
            lookup,
            void(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntry&)> onSuccess,
                std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError
            ));

private:
    MockMessageRouter messageRouter;
};

class MockConsumerPermissionCallback : public joynr::IAccessController::IHasConsumerPermissionCallback
{
public:
    MOCK_METHOD1(hasConsumerPermission, void(bool hasPermission));
};

class MockWebSocketClient : public joynr::WebSocketClient
{
public:
    MockWebSocketClient()
        : WebSocketClient([](const std::string& message){},
                                 [](WebSocket* webSocket){})
    {
    }

    MOCK_METHOD0(dtorCalled, void());
    ~MockWebSocketClient() override
    {
        dtorCalled();
    }

    MOCK_METHOD1(connect, void (const joynr::system::RoutingTypes::WebSocketAddress&));

    MOCK_METHOD1(send , void (const std::string& message));

    MOCK_CONST_METHOD0(isConnected, bool ());

    MOCK_METHOD2(onWebSocketWriteable, void (joynr::WebSocketContext::WebSocketConnectionHandle,
                              std::function<int(const std::string&)>));

    MOCK_METHOD3(onNewConnection, void (joynr::WebSocketContext::WebSocketConnectionHandle handle,
                         const std::string& host,
                         const std::string& name));

    void signalDisconnect() {
        onWebSocketDisconnected();
    }
};

class MockQWebSocketSendWrapper : public joynr::QWebSocketSendWrapper {
public:

    MockQWebSocketSendWrapper(QWebSocket* websocket)
        : QWebSocketSendWrapper(websocket)
    {
        registerDisconnectCallback(std::bind(&MockQWebSocketSendWrapper::onConnectionClosed, this));
    }

    MOCK_METHOD1(send, void (const std::string& message));

    MOCK_CONST_METHOD0(isConnected, bool ());

    MOCK_METHOD0(onConnectionClosed, void ());
};

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif // MOCKOBJECTS_H_

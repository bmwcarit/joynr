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

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <boost/any.hpp>
#include <boost/asio.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <mococrw/key.h>
#include <mococrw/x509.h>

#include <websocketpp/common/connection_hdl.hpp>

#include "joynr/access-control/IAccessController.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerProxy.h"
#include "joynr/infrastructure/GlobalDomainRoleControllerProxy.h"
#include "joynr/IClusterControllerSignalHandler.h"
#include "joynr/IDispatcher.h"
#include "joynr/IKeychain.h"
#include "joynr/IMessageRouter.h"
#include "joynr/IMessageSender.h"
#include "joynr/IMessagingMulticastSubscriber.h"
#include "joynr/IMessagingStub.h"
#include "joynr/IMessagingStubFactory.h"
#include "joynr/IMqttMessagingSkeleton.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/IProxyBuilder.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/ITransportMessageReceiver.h"
#include "joynr/ITransportMessageSender.h"
#include "joynr/IWebSocketSendInterface.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/Logger.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MessagingQos.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/PublicationManager.h"
#include "joynr/ReplyCaller.h"
#include "joynr/Request.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/Runnable.h"
#include "joynr/Settings.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"
#include "joynr/tests/testRequestCaller.h"
#include "joynr/types/Version.h"
#include "joynr/vehicle/DefaultGpsProvider.h"
#include "joynr/vehicle/GpsProvider.h"
#include "joynr/vehicle/GpsRequestCaller.h"

#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/websocket/IWebSocketPpClient.h"

#include "libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "libjoynrclustercontroller/capabilities-client/ICapabilitiesClient.h"
#include "libjoynrclustercontroller/http-communication-manager/HttpReceiver.h"
#include "libjoynrclustercontroller/include/joynr/ITransportStatus.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

#include "tests/PrettyPrint.h"
#include "tests/mock/LibJoynrMockObjects.h"

namespace joynr
{
class BroadcastSubscriptionRequest;
class ClusterControllerSettings;
class DiscoveryQos;
class MessagingSettings;
class MulticastPublication;
class MulticastSubscriptionQos;
class OneWayRequest;
class RequestCaller;
class Settings;
class SubscriptionReply;
class WebSocketSettings;

namespace types {
class DiscoveryQos;
} // namespace types
} // namespace joynr

using ::testing::_;
using ::testing::Eq;
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

class MockJoynrRuntime : public joynr::JoynrRuntime
{
public:
    MockJoynrRuntime(joynr::Settings& settings) : joynr::JoynrRuntime(settings) {
    }
    MockJoynrRuntime(std::unique_ptr<joynr::Settings> settings) : joynr::JoynrRuntime(*settings) {
    }
    MOCK_METHOD0(getMessageRouter, std::shared_ptr<joynr::IMessageRouter>());
};

class MockInProcessMessagingSkeleton : public joynr::InProcessMessagingSkeleton
{
public:
    MockInProcessMessagingSkeleton(std::weak_ptr<joynr::IDispatcher> dispatcher) : InProcessMessagingSkeleton(dispatcher){}
    MOCK_METHOD2(transmit, void(std::shared_ptr<joynr::ImmutableMessage> message, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure));
};

class MockDelayedScheduler : public joynr::DelayedScheduler
{
public:
    MockDelayedScheduler(boost::asio::io_service& ioService)
        : DelayedScheduler([](std::shared_ptr<joynr::Runnable>){ assert(false); }, ioService, std::chrono::milliseconds::zero())
    {
    }

    void shutdown() { joynr::DelayedScheduler::shutdown(); }
    MOCK_METHOD1(unschedule, void (joynr::DelayedScheduler::RunnableHandle));
    MOCK_METHOD2(schedule, DelayedScheduler::RunnableHandle (std::shared_ptr<joynr::Runnable>, std::chrono::milliseconds delay));
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

class MockTransportMessageReceiver : public joynr::ITransportMessageReceiver
{
public:
    MockTransportMessageReceiver() = default;
    MOCK_METHOD0(init, void());
    MOCK_CONST_METHOD0(getGlobalClusterControllerAddress, std::string&());
    MOCK_METHOD0(startReceiveQueue, void());
    MOCK_METHOD0(stopReceiveQueue, void());
    MOCK_METHOD0(updateSettings, void());
    MOCK_METHOD0(tryToDeleteChannel, bool());
    MOCK_METHOD1(registerReceiveCallback, void(std::function<void(smrf::ByteVector&&)> onMessageReceived));
    MOCK_METHOD0(isConnected, bool());
};

class MockTransportMessageSender : public joynr::ITransportMessageSender
{
public:
    MOCK_METHOD3(sendMessage,void(const joynr::system::RoutingTypes::Address&, std::shared_ptr<joynr::ImmutableMessage>, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&));
    MOCK_METHOD1(init,void(const joynr::MessagingSettings& settings));
};

class GlobalCapabilitiesMock {
public:
    MOCK_METHOD1(capabilitiesReceived, void(const std::vector<joynr::types::GlobalDiscoveryEntry>& results));
};

class MockRoutingProxy : public virtual joynr::system::RoutingProxy {
public:
    MockRoutingProxy(std::weak_ptr<joynr::JoynrRuntime> runtime) :
        RoutingProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        ProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingSyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingAsyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos())
    { }

    MOCK_METHOD5(addNextHopAsync, std::shared_ptr<joynr::Future<void>>(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError));

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
};

class MockGpsProvider : public joynr::vehicle::DefaultGpsProvider
{
public:
    MockGpsProvider() : joynr::vehicle::DefaultGpsProvider()
    {
    }

    ~MockGpsProvider()
    {
        JOYNR_LOG_DEBUG(logger(), "I am being destroyed");
    }

    MOCK_METHOD1(getLocation, void(joynr::types::Localisation::GpsLocation& result) );
    MOCK_METHOD1(setLocation, void(joynr::types::Localisation::GpsLocation gpsLocation));

    std::string getParticipantId() const
    {
        return "Fake_ParticipantId_vehicle/DefaultGpsProvider";
    }
private:
    ADD_LOGGER(MockGpsProvider)
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

    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, std::shared_ptr<joynr::SubscriptionAttributeListener> attributeListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, std::shared_ptr<joynr::SubscriptionAttributeListener> attributeListener));
};

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif // MOCKOBJECTS_H_

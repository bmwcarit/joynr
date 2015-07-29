/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "PrettyPrint.h"

#include "cluster-controller/access-control/IAccessController.h"
#include "cluster-controller/access-control/LocalDomainAccessController.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/testProvider.h"
#include "joynr/tests/testRequestCaller.h"
#include "joynr/vehicle/DefaultGpsProvider.h"
#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"
#include "QtCore"
#include "utils/TestQString.h"
#include <string>
#include <vector>
#include "utils/QThreadSleep.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/vehicle/GpsRequestCaller.h"
#include "joynr/types/QtGpsLocation.h"
#include "joynr/types/QtTrip.h"
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

#include "joynr/system/QtAddress.h"
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
#include "joynr/types/QtGpsLocation.h"
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
#include "joynr/BounceProxyUrl.h"
#include "joynr/Directory.h"

using ::testing::A;
using ::testing::_;
using ::testing::A;
using ::testing::Eq;
using ::testing::NotNull;
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
    MOCK_METHOD3(lookup, void(
                     const std::string& domain,
                     const std::string& interfaceName,
                     std::function<void(const joynr::RequestStatus& status, const std::vector<joynr::types::CapabilityInformation>& capabilities)> callbackFct));
    MOCK_METHOD2(lookup, void(
                     const std::string& participantId,
                     std::function<void(const joynr::RequestStatus& status, const std::vector<joynr::types::CapabilityInformation>& capabilities)> callbackFct));
    MOCK_METHOD0(getLocalChannelId, std::string());

};

class MockInProcessMessagingSkeleton : public joynr::InProcessMessagingSkeleton
{
public:
    MOCK_METHOD1(transmit, void(joynr::JoynrMessage& message));
};

class MockDelayedScheduler : public joynr::SingleThreadedDelayedScheduler
{
public:
    MockDelayedScheduler(QString name) : SingleThreadedDelayedScheduler(name){};
    MOCK_METHOD1(executeRunnable, void(QRunnable* runnable));
    MOCK_METHOD1(stopRunnable, void(QRunnable* runnable));
    MOCK_METHOD2(schedule, quint32(QRunnable* runnable, int delay_ms));
};

class MockInProcessConnectorFactory : public joynr::InProcessConnectorFactory {
public:

    MockInProcessConnectorFactory()
        : InProcessConnectorFactory(NULL,NULL,NULL,NULL) {
    }

    MOCK_METHOD1(canBeCreated, bool(const QSharedPointer<joynr::system::QtAddress> address));
};

class MockDispatcher : public joynr::IDispatcher {
public:
    MOCK_METHOD3(addReplyCaller, void(const std::string& requestReplyId,
                                      QSharedPointer<joynr::IReplyCaller> replyCaller,
                                      const joynr::MessagingQos& qosSettings));
    MOCK_METHOD1(removeReplyCaller, void(const std::string& requestReplyId));
    MOCK_METHOD2(addRequestCaller, void(const std::string& participantId, QSharedPointer<joynr::RequestCaller> requestCaller));
    MOCK_METHOD1(removeRequestCaller, void(const std::string& participantId));
    MOCK_METHOD1(receive, void(const joynr::JoynrMessage& message));
    MOCK_METHOD1(registerSubscriptionManager, void(joynr::ISubscriptionManager* subscriptionManager));
    MOCK_METHOD1(registerPublicationManager,void(joynr::PublicationManager* publicationManager));
};

class MockInProcessDispatcher : public MockDispatcher , public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, QSharedPointer<joynr::RequestCaller>(const std::string& participantId));
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
        MessageRouter(NULL, NULL, 0) {

    }
    MOCK_METHOD1(route, void(const joynr::JoynrMessage& message));
    MOCK_METHOD2(addNextHop, void(std::string participantId, QSharedPointer<joynr::system::QtAddress> inprocessAddress));
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
                QSharedPointer<joynr::IReplyCaller> callback
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
                    std::function<void(const joynr::RequestStatus& status)> errorFct) : joynr::ReplyCaller<T>(callbackFct, errorFct) {}
    MOCK_METHOD1_T(returnValue, void(const T& payload));
    MOCK_METHOD0_T(timeOut, void());
    MOCK_CONST_METHOD0_T(getType, QString());
};

// GMock doesn't support mocking variadic template functions directly.
// Workaround: Mock exactly the functions with the number of arguments used in the tests.
template <typename T>
class MockSubscriptionListenerOneType : public joynr::ISubscriptionListener<T> {
public:
     MOCK_METHOD1_T(onReceive, void( const T& value));
     MOCK_METHOD0(onError, void());
};

template <typename T1, typename T2, typename... Ts>
class MockSubscriptionListenerTwoTypes : public joynr::ISubscriptionListener<T1, T2, Ts...> {
public:
     MOCK_METHOD2_T(onReceive, void( const T1& value1, const T2& value2, const Ts&... values));
     MOCK_METHOD0(onError, void());
};

class MockGpsSubscriptionListener : public joynr::ISubscriptionListener<joynr::types::Localisation::GpsLocation> {
public:
    MOCK_METHOD1(onReceive, void(const joynr::types::Localisation::GpsLocation& value));
    MOCK_METHOD0(onError, void());
};

class MockGpsFloatSubscriptionListener
        : public joynr::ISubscriptionListener<joynr::types::Localisation::GpsLocation, float> {
public:
    MOCK_METHOD2(onReceive, void(const joynr::types::Localisation::GpsLocation& value, const float&));
    MOCK_METHOD0(onError, void());
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
   MOCK_METHOD1(lookUp, QVariant(const QString& attributeId));
   MOCK_METHOD2(insert, void(QString attributeId, QVariant value));
};

class MockDiscovery : public joynr::system::IDiscovery {
public:
    MOCK_METHOD2(
            add,
            void(
                joynr::RequestStatus& joynrInternalStatus,
                const joynr::types::DiscoveryEntry& entry
            )
    );
    MOCK_METHOD5(
            lookup,
            void(
                joynr::RequestStatus& joynrInternalStatus,
                std::vector<joynr::types::DiscoveryEntry> & result,
                const std::string& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos
            )
    );
    MOCK_METHOD3(
            lookup,
            void(
                joynr::RequestStatus& joynrInternalStatus,
                joynr::types::DiscoveryEntry& result,
                const std::string& participantId
            )
    );
    MOCK_METHOD2(
            remove,
            void(
                joynr::RequestStatus& joynrInternalStatus,
                const std::string& participantId
            )
    );
    MOCK_METHOD2(
            add,
            std::shared_ptr<joynr::Future<void>>(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                std::function<void(const joynr::RequestStatus& status)> callbackFct
            )
    );
    MOCK_METHOD2(
            lookup,
            std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntry>>(
                const std::string& participantId,
                std::function<void(const joynr::RequestStatus& status, const joynr::types::DiscoveryEntry& result)>
                        callbackFct
            )
    );
    MOCK_METHOD4(
            lookup,
            std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                std::function<void(const joynr::RequestStatus& status, const std::vector<joynr::types::DiscoveryEntry>& result)>
                        callbackFct
            )
    );
    MOCK_METHOD2(
            remove,
            std::shared_ptr<joynr::Future<void>>(
                const std::string& participantId,
                std::function<void(const joynr::RequestStatus& status)> callbackFct
            )
    );
};

class IMockProviderInterface {
public:
    virtual ~IMockProviderInterface(){}
    static const std::string& INTERFACE_NAME();
};


class MockProvider : public joynr::AbstractJoynrProvider, public IMockProviderInterface {
public:
    MOCK_CONST_METHOD0(getProviderQos, joynr::types::ProviderQos());
    MOCK_CONST_METHOD0(getParticipantId, std::string());
    virtual ~MockProvider(){}
    virtual std::string getInterfaceName() const;
};

namespace joynr {

template<>
class RequestCallerFactoryHelper<MockProvider> {
public:
    QSharedPointer<RequestCaller> create(std::shared_ptr<MockProvider> provider) {
        return QSharedPointer<RequestCaller>(NULL);
    }
};
} // namespace joynr

class MockMessageReceiver : public joynr::IMessageReceiver
{
public:
    MockMessageReceiver(){};
    MOCK_METHOD1(init, void(QSharedPointer<joynr::ILocalChannelUrlDirectory> channelUrlDirectory));
    MOCK_CONST_METHOD0(getReceiveChannelId, QString&());
    MOCK_METHOD0(startReceiveQueue, void());
    MOCK_METHOD0(stopReceiveQueue, void());
    MOCK_METHOD0(waitForReceiveQueueStarted, void());
    MOCK_METHOD0(updateSettings, void());
    MOCK_METHOD0(tryToDeleteChannel, bool());
};

class MockMessageSender : public joynr::IMessageSender
{
public:
    MOCK_METHOD2(sendMessage,void(const QString&, const joynr::JoynrMessage&));
    MOCK_METHOD2(init,void(QSharedPointer<joynr::ILocalChannelUrlDirectory> channelUrlDirectory,const joynr::MessagingSettings& settings));
};

/*
 * Typed Callbacks
 */
template <typename ... Ts>
class MockCallback{
public:
    MOCK_METHOD2_T(callbackFct, void(const joynr::RequestStatus& status, const Ts&... result));
    MOCK_METHOD1_T(errorFct, void(const joynr::RequestStatus& status));
};

template<>
class MockCallback<void> {

public:
    MOCK_METHOD1(callbackFct, void(const joynr::RequestStatus& status));
    MOCK_METHOD1(errorFct, void(const joynr::RequestStatus& status));
};

class MockMessagingStubFactory : public joynr::IMessagingStubFactory {
public:
    MOCK_METHOD2(create, QSharedPointer<joynr::IMessaging>(std::string destParticipantId, const joynr::system::QtAddress& destEndpointAddress));
    MOCK_METHOD1(remove, void(std::string destParticipantId));
    MOCK_METHOD1(contains, bool(std::string destPartId));
};

class GlobalCapabilitiesMock {
public:
    MOCK_METHOD2(capabilitiesReceived, void(const joynr::RequestStatus& status, const std::vector<joynr::types::CapabilityInformation>& results));
};

class MockGpsProvider : public joynr::vehicle::DefaultGpsProvider
{
    public:
    MockGpsProvider() : joynr::vehicle::DefaultGpsProvider()
    {
    };
    ~MockGpsProvider()
    {
        qDebug() << "I am being destroyed_ MockProvider";
    };
    /*void getLocation(RequestStatus& status, QtGpsLocation& result)
    {
        result = QtGpsLocation(QtGpsFixEnum::MODE2D, 4,5,6,7);
    }
    */
    MOCK_METHOD2(getLocation, void(joynr::RequestStatus& status, joynr::types::Localisation::GpsLocation& result) );
    MOCK_METHOD2(setLocation, void(joynr::RequestStatus& status, joynr::types::Localisation::GpsLocation gpsLocation));
    //MOCK_METHOD2(calculateAvailableSatellites,void (RequestStatus& status, int32_t& result));
    //MOCK_METHOD2(restartWithRetries, void (RequestStatus& status, int32_t gpsFix));

    void  restartWithRetries(joynr::RequestStatus& status, int32_t gpsfix ) {

        status.setCode(joynr::RequestStatusCode::OK);
    }

    void  calculateAvailableSatellites(joynr::RequestStatus& status, int32_t& result) {
        result = 42;
        status.setCode(joynr::RequestStatusCode::OK);
    }


    std::string getParticipantId() const{
        return "Fake_ParticipantId_vehicle/DefaultGpsProvider";
    }
};

class MockTestProvider : public joynr::tests::DefaulttestProvider
{
public:
    MockTestProvider() :
        joynr::tests::DefaulttestProvider()
    {
        EXPECT_CALL(*this, getLocation(_))
                .WillRepeatedly(testing::Invoke(this, &MockTestProvider::invokeOnSuccess));
    }
    MockTestProvider(joynr::types::ProviderQos qos) :
        DefaulttestProvider()
    {
        providerQos = qos;
        EXPECT_CALL(*this, getLocation(_))
                .WillRepeatedly(testing::Invoke(this, &MockTestProvider::invokeOnSuccess));
    }
    ~MockTestProvider()
    {
    };

    void invokeOnSuccess(std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess) {
        joynr::types::Localisation::GpsLocation location;
        onSuccess(location);
    }

    void fireLocationUpdateSelective(const joynr::types::Localisation::GpsLocation& location) {
        joynr::tests::testAbstractProvider::fireLocationUpdateSelective(location);
    }

    MOCK_METHOD1(
            getLocation,
            void(
                    std::function<void(const joynr::types::Localisation::GpsLocation& result)> onSuccess
            )
    );
    MOCK_METHOD2(
            setLocation,
            void(
                    const joynr::types::Localisation::GpsLocation& gpsLocation,
                    std::function<void()> onSuccess
            )
    );

    void sumInts(
            const std::vector<int32_t>& ints,
            std::function<void(const int32_t& result)> onSuccess)
    {
        int32_t result = 0;
        int32_t j;
        foreach ( j, ints) {
            result += j;
        }
        onSuccess(result);
    }
    void returnPrimeNumbers(
            const int32_t &upperBound,
            std::function<void(
                const std::vector<int32_t>& result)> onSuccess)
    {
        std::vector<int32_t> result;
        assert(upperBound<7);
        result.clear();
        result.push_back(2);
        result.push_back(3);
        result.push_back(5);
        onSuccess(result);
    }
    void optimizeTrip(
            const joynr::types::Localisation::Trip& input,
            std::function<void(
                const joynr::types::Localisation::Trip& result)> onSuccess)
    {
         onSuccess(input);
    }
    void optimizeLocationList(
            const std::vector<joynr::types::Localisation::GpsLocation>& inputList,
            std::function<void(
                const std::vector<joynr::types::Localisation::GpsLocation>& result)> onSuccess)

    {
         onSuccess(inputList);
    }

    void overloadedOperation(
            const joynr::tests::testTypes::DerivedStruct& input,
            std::function<void(
                const std::string& result)> onSuccess)
    {
        std::string result("QtDerivedStruct");
        onSuccess(result);
    }

    void overloadedOperation(
            const joynr::tests::testTypes::AnotherDerivedStruct& input,
            std::function<void(
                const std::string& result)> onSuccess)
    {
        std::string result("QtAnotherDerivedStruct");
        onSuccess(result);
    }
};

class MockTestRequestCaller : public joynr::tests::testRequestCaller {
public:
    void invokeOnSuccessFct(std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess) {
        joynr::types::Localisation::GpsLocation location;
        onSuccess(location);
    }

    MockTestRequestCaller() :
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>())
    {
        EXPECT_CALL(
                *this,
                getLocation(_)
        )
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeOnSuccessFct));
    }
    MockTestRequestCaller(testing::Cardinality getLocationCardinality) :
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>())
    {
        EXPECT_CALL(
                *this,
                getLocation(_)
        )
                .Times(getLocationCardinality)
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeOnSuccessFct));
    }
    MOCK_METHOD1(getLocation, void(std::function<void(const joynr::types::Localisation::GpsLocation& location)>));
    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(registerBroadcastListener, void(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterBroadcastListener, void(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener));

};

class MockGpsRequestCaller : public joynr::vehicle::GpsRequestCaller {
public:
    MockGpsRequestCaller() : joynr::vehicle::GpsRequestCaller(std::make_shared<MockGpsProvider>() ) {}
    MOCK_METHOD1(getLocation, void(std::function<void(const joynr::RequestStatus& status, const joynr::types::Localisation::GpsLocation& location)>));
    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, joynr::IAttributeListener* attributeListener));
};


class MockIRequestCallerDirectory : public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, QSharedPointer<joynr::RequestCaller>(const std::string& participantId));
    MOCK_METHOD1(containsRequestCaller, bool(const std::string& participantId));
};

class MockEndpointAddress : public joynr::system::QtAddress {

};



template <typename Key, typename T>
class MockDirectory : public joynr::IDirectory<Key, T> {
public:
    MOCK_METHOD1_T(lookup, QSharedPointer< T >(const Key& keyId));
    MOCK_METHOD1_T(contains, bool(const Key& keyId));

    MOCK_METHOD2_T(add, void(const Key &keyId, T* value));
    MOCK_METHOD2_T(add, void(const Key& keyId, QSharedPointer < T > value));

    MOCK_METHOD3_T(add, void(const Key &keyId, T* value, qint64 ttl_ms));
    MOCK_METHOD3_T(add, void(const Key& keyId, QSharedPointer < T > value, qint64 ttl_ms));
    MOCK_METHOD1_T(remove, void(const Key& keyId));
};

typedef MockDirectory<QString, joynr::system::QtAddress> MockMessagingEndpointDirectory;




class MockSubscriptionManager : public joynr::SubscriptionManager {
public:
    MOCK_METHOD1(getSubscriptionCallback,QSharedPointer<joynr::ISubscriptionCallback>(const QString& subscriptionId));
    MOCK_METHOD4(registerSubscription,void(const QString& subscribeToName,
                                                    QSharedPointer<joynr::ISubscriptionCallback> subscriptionCaller, // SubMgr gets ownership of ptr
                                                    QSharedPointer<joynr::QtSubscriptionQos> qos,
                                                    joynr::SubscriptionRequest& subscriptionRequest));
    MOCK_METHOD1(unregisterSubscription, void(const QString& subscriptionId));
    MOCK_METHOD1(touchSubscriptionState,void(const QString& subscriptionId));
};


class MockPublicationManager : public joynr::PublicationManager {
public:
    MOCK_METHOD2(attributeValueChanged, void(const QString& subscriptionId, const QVariant& value));
};

//virtual public IChannelUrlDirectory, virtual public ChannelUrlDirectorySyncProxy, virtual public ChannelUrlDirectoryAsyncProxy
class MockChannelUrlDirectoryProxy : public virtual joynr::infrastructure::ChannelUrlDirectoryProxy {
public:
    MockChannelUrlDirectoryProxy() :
        ChannelUrlDirectoryProxy(QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()), NULL, NULL, "domain", joynr::MessagingQos(), false),
        joynr::ProxyBase(NULL, NULL, "domain", "INTERFACE_NAME", joynr::MessagingQos(), false),
        ChannelUrlDirectoryProxyBase(QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()), NULL, NULL, "domain", joynr::MessagingQos(), false),
        ChannelUrlDirectorySyncProxy(QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()), NULL, NULL, "domain", joynr::MessagingQos(), false),
        ChannelUrlDirectoryAsyncProxy(QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()), NULL, NULL, "domain", joynr::MessagingQos(), false){}


    MOCK_METHOD2(getUrlsForChannel,std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> (const std::string& channelId, std::function<void(const joynr::RequestStatus& status, const joynr::types::ChannelUrlInformation& urls)> callbackFct));

    MOCK_METHOD3(registerChannelUrls, std::shared_ptr<joynr::Future<void> >(
                                           const std::string& channelId,
                                           const joynr::types::ChannelUrlInformation& channelUrlInformation,
                                           std::function<void(const joynr::RequestStatus&)> callbackFct));

    MOCK_METHOD2(unregisterChannelUrls, std::shared_ptr<joynr::Future<void> >(const std::string& channelId, std::function<void(const joynr::RequestStatus&)> callbackFct));
};


class MockLocalChannelUrlDirectory : public joynr::ILocalChannelUrlDirectory {
public:
    MOCK_METHOD3(registerChannelUrls, std::shared_ptr<joynr::Future<void>>(
                     const std::string& channelId,
                     joynr::types::ChannelUrlInformation channelUrlInformation,
                     std::function<void(const joynr::RequestStatus&)> callbackFct));

    MOCK_METHOD2(unregisterChannelUrls, std::shared_ptr<joynr::Future<void>>(
                    const std::string& channelId,
                    std::function<void(const joynr::RequestStatus&)> callbackFct));

    MOCK_METHOD3(getUrlsForChannel, std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>>(
                    const std::string& channelId,
                    const qint64& timeout_ms,
                    std::function<void(const joynr::RequestStatus&, const joynr::types::ChannelUrlInformation&)>
                                         callbackFct));
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
                QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()),
                NULL,
                NULL,
                "domain",
                joynr::MessagingQos(),
                false),
        joynr::ProxyBase(
                NULL,
                NULL,
                "domain",
                "INTERFACE_NAME",
                joynr::MessagingQos(),
                false),
        GlobalDomainAccessControllerProxyBase(
                QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()),
                NULL,
                NULL,
                "domain",
                joynr::MessagingQos(),
                false),
        GlobalDomainAccessControllerSyncProxy(
                QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()),
                NULL,
                NULL,
                "domain",
                joynr::MessagingQos(),
                false),
        GlobalDomainAccessControllerAsyncProxy(
                QSharedPointer<joynr::system::QtAddress> (new joynr::system::QtAddress()),
                NULL,
                NULL,
                "domain",
                joynr::MessagingQos(),
                false)
    {
    }

    MOCK_METHOD2(
            getDomainRoles,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>>>(
                    const std::string& uid,
                    std::function<void(
                        const joynr::RequestStatus& status,
                        const std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>& domainRoleEntries
                    )> callbackFct
            )
    );

    MOCK_METHOD3(
            getMasterAccessControlEntries,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                    const std::string& domain,
                    const std::string& interfaceName,
                    std::function<void(
                        const joynr::RequestStatus& status,
                        const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& masterAces
                    )> callbackFct
            )
    );

    MOCK_METHOD3(
            getMediatorAccessControlEntries,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                    const std::string& domain,
                    const std::string& interfaceName,
                    std::function<void(
                        const joynr::RequestStatus& status,
                        const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& mediatorAces
                    )> callbackFct
            )
    );

    MOCK_METHOD3(
            getOwnerAccessControlEntries,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>>>(
                    const std::string& domain,
                    const std::string& interfaceName,
                    std::function<void(
                        const joynr::RequestStatus& status,
                        const std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>& ownerAces
                    )> callbackFct
            )
    );

    MOCK_METHOD3(subscribeToDomainRoleEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerDomainRoleEntryChangedBroadcastFilterParameters,
                     QSharedPointer<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::DomainRoleEntry,
                                                                 joynr::infrastructure::DacTypes::ChangeType::Enum>>,
                     QSharedPointer<joynr::QtOnChangeSubscriptionQos>));
    MOCK_METHOD3(subscribeToOwnerAccessControlEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerOwnerAccessControlEntryChangedBroadcastFilterParameters,
                     QSharedPointer<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::OwnerAccessControlEntry,
                                                                 joynr::infrastructure::DacTypes::ChangeType::Enum>>,
                     QSharedPointer<joynr::QtOnChangeSubscriptionQos>));
    MOCK_METHOD3(subscribeToMediatorAccessControlEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerMediatorAccessControlEntryChangedBroadcastFilterParameters,
                     QSharedPointer<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::ChangeType::Enum,
                                                                 joynr::infrastructure::DacTypes::MasterAccessControlEntry>>,
                     QSharedPointer<joynr::QtOnChangeSubscriptionQos>));
    MOCK_METHOD3(subscribeToMasterAccessControlEntryChangedBroadcast,
                 std::string(
                     joynr::infrastructure::GlobalDomainAccessControllerMasterAccessControlEntryChangedBroadcastFilterParameters,
                     QSharedPointer<joynr::ISubscriptionListener<joynr::infrastructure::DacTypes::ChangeType::Enum,
                                                                 joynr::infrastructure::DacTypes::MasterAccessControlEntry>>,
                     QSharedPointer<joynr::QtOnChangeSubscriptionQos>));

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
                     QSharedPointer<joynr::LocalDomainAccessController::IGetConsumerPermissionCallback> callback));

    MOCK_METHOD5(getConsumerPermission,
                 joynr::infrastructure::QtPermission::Enum(
                     const std::string& userId,
                     const std::string& domain,
                     const std::string& interfaceName,
                     const std::string& operation,
                     joynr::infrastructure::DacTypes::TrustLevel::Enum trustLevel));
};

class MockMessagingSettings : public joynr::MessagingSettings {
public:
    MockMessagingSettings(QSettings& settings):
        MessagingSettings(settings){}
    MOCK_METHOD0(
            getDiscoveryDirectoriesDomain,
            QString());
    MOCK_METHOD0(
            getCapabilitiesDirectoryParticipantId,
            QString());
};

class MockLocalCapabilitiesDirectory : public joynr::LocalCapabilitiesDirectory {
public:
    MockLocalCapabilitiesDirectory(MockMessagingSettings& messagingSettings):
        messageRouter(),
        LocalCapabilitiesDirectory(messagingSettings,NULL, messageRouter){}

    MOCK_METHOD2(
            lookup,
            void(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntry&)> lookupCallback
            ));

private:
    MockMessageRouter messageRouter;
};

class MockConsumerPermissionCallback : public joynr::IAccessController::IHasConsumerPermissionCallback
{
public:
    MOCK_METHOD1(hasConsumerPermission, void(bool hasPermission));
};

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif /* MOCKOBJECTS_H_ */

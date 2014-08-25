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
#include "PrettyPrint.h"

#include "joynr/tests/DefaultTestProvider.h"
#include "joynr/tests/TestProvider.h"
#include "joynr/tests/TestRequestCaller.h"
#include "joynr/vehicle/DefaultGpsProvider.h"
#include "QtCore"
#include "utils/TestQString.h"
#include "utils/QThreadSleep.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/vehicle/GpsRequestCaller.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/types/Trip.h"
#include "joynr/IMessageReceiver.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessaging.h"
#include "joynr/IClientCache.h"
#include "joynr/ReplyCaller.h"
#include "joynr/ISubscriptionListener.h"
#include "cluster-controller/capabilities-client/IGlobalCapabilitiesCallback.h"
#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/JoynrMessageSender.h"

#include "joynr/system/Address.h"
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
#include "joynr/types/GpsLocation.h"
#include "cluster-controller/http-communication-manager/HttpReceiver.h"

#include "joynr/infrastructure/ChannelUrlDirectoryProxy.h"

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
    MOCK_METHOD1(add, void(QList<joynr::types::CapabilityInformation> capabilitiesInformationList));
    MOCK_METHOD1(remove, void(QList<QString> participantIdList));
    MOCK_METHOD1(remove, void(const QString& participantId));
    MOCK_METHOD2(lookup, QList<joynr::types::CapabilityInformation>(const QString& domain, const QString& interfaceName));
    MOCK_METHOD3(lookup, void(const QString& domain, const QString& interfaceName, QSharedPointer<joynr::IGlobalCapabilitiesCallback> callback));
    MOCK_METHOD2(lookup, void(const QString& participantId, QSharedPointer<joynr::IGlobalCapabilitiesCallback> callback));
    MOCK_METHOD0(getLocalChannelId, QString());

};


class MockGpsCallBack : public joynr::ICallback<joynr::types::GpsLocation> {
public:
    MockGpsCallBack(){};
    MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
    MOCK_METHOD2(onSuccess, void(const joynr::RequestStatus status, joynr::types::GpsLocation result));
    MOCK_METHOD0(test,void());
};

class MockInProcessMessagingSkeleton : public joynr::InProcessMessagingSkeleton
{
public:
    MOCK_METHOD2(transmit, void(joynr::JoynrMessage& message, const joynr::MessagingQos& qoS));
};

class MockDelayedScheduler : public joynr::SingleThreadedDelayedScheduler
{
public:
    MockDelayedScheduler(QString name) : SingleThreadedDelayedScheduler(name){};
    MOCK_METHOD1(executeRunnable, void(QRunnable* runnable));
    MOCK_METHOD2(schedule, void(QRunnable* runnable, int delay_ms));
};

class MockInProcessConnectorFactory : public joynr::InProcessConnectorFactory {
public:

    MockInProcessConnectorFactory()
        : InProcessConnectorFactory(NULL,NULL,NULL,NULL) {
    }

    MOCK_METHOD1(canBeCreated, bool(const QSharedPointer<joynr::system::Address> address));
};

class MockDispatcher : public joynr::IDispatcher {
public:
    MOCK_METHOD3(addReplyCaller, void(const QString& requestReplyId,
                                      QSharedPointer<joynr::IReplyCaller> replyCaller,
                                      const joynr::MessagingQos& qosSettings));
    MOCK_METHOD1(removeReplyCaller, void(const QString& requestReplyId));
    MOCK_METHOD2(addRequestCaller, void(const QString& participantId, QSharedPointer<joynr::RequestCaller> requestCaller));
    MOCK_METHOD1(removeRequestCaller, void(const QString& participantId));
    MOCK_METHOD2(receive, void(const joynr::JoynrMessage& message, const joynr::MessagingQos& qosSettings));
    MOCK_METHOD1(registerSubscriptionManager, void(joynr::SubscriptionManager* subscriptionManager));
    MOCK_METHOD1(registerPublicationManager,void(joynr::PublicationManager* publicationManager));
};

class MockInProcessDispatcher : public MockDispatcher , public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, QSharedPointer<joynr::RequestCaller>(const QString& participantId));
    MOCK_METHOD1(containsRequestCaller, bool(const QString& participantId));
};

class MockMessaging : public joynr::IMessaging {
public:
  MOCK_METHOD2(transmit, void(joynr::JoynrMessage& message, const joynr::MessagingQos& qos));
  MOCK_METHOD2(test1, void(int a0, int a1));
};

class MockMessageRouter : public joynr::MessageRouter {
public:
    MockMessageRouter():
        MessageRouter(NULL, 500, 0){

    }
    MOCK_METHOD2(route, void(const joynr::JoynrMessage& message, const joynr::MessagingQos& qos));
    MOCK_METHOD2(addNextHop, void(QString participantId, QSharedPointer<joynr::system::Address> inprocessAddress));
    MOCK_METHOD2(removeNextHop, void(joynr::RequestStatus& joynrInternalStatus, QString participantId));
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
                const QString& senderParticipantId,
                const QString& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::Request& request,
                QSharedPointer<joynr::IReplyCaller> callback
            )
    );

    MOCK_METHOD4(
            sendReply,
            void(
                const QString& senderParticipantId,
                const QString& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::Reply& reply
            )
    );

    MOCK_METHOD4(
            sendSubscriptionRequest,
            void(
                const QString &senderParticipantId,
                const QString &receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionRequest& subscriptionRequest
            )
    );

    MOCK_METHOD4(
            sendBroadcastSubscriptionRequest,
            void(
                const QString &senderParticipantId,
                const QString &receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::BroadcastSubscriptionRequest& subscriptionRequest
            )
    );

    MOCK_METHOD4(
            sendSubscriptionReply,
            void(
                const QString &senderParticipantId,
                const QString &receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionReply& subscriptionReply
            )
    );

    MOCK_METHOD4(
            sendSubscriptionStop,
            void(
                const QString& senderParticipantId,
                const QString& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionStop& subscriptionStop
            )
    );

    MOCK_METHOD4(
            sendSubscriptionPublication,
            void(
                const QString& senderParticipantId,
                const QString& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionPublication& subscriptionPublication
            )
    );
};

template <typename T>
class MockReplyCaller : public joynr::ReplyCaller<T> {
public:
    MockReplyCaller(QSharedPointer <joynr::ICallback<T> > callback) : joynr::ReplyCaller<T>(callback) {}
    MOCK_METHOD1_T(returnValue, void(const T& payload));
    MOCK_METHOD0_T(timeOut, void());
    MOCK_CONST_METHOD0_T(getType, QString());
};


template <typename T>
class MockCallback : public joynr::ICallback<T> {
public:
     MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
     MOCK_METHOD2_T(onSuccess, void(const joynr::RequestStatus status, T result));
};

template <typename T>
class MockSubscriptionListener : public joynr::ISubscriptionListener<T> {
public:
     MOCK_METHOD1_T(receive, void( T value));
     MOCK_METHOD0(publicationMissed, void());
};

class MockGpsSubscriptionListener : public joynr::ISubscriptionListener<joynr::types::GpsLocation> {
public:
    MOCK_METHOD1(receive, void(joynr::types::GpsLocation value));
    MOCK_METHOD0(publicationMissed, void());
};

class MockPublicationSender : public joynr::IPublicationSender {
public:
    MOCK_METHOD4(
            sendSubscriptionPublication,
            void(
                const QString& senderParticipantId,
                const QString& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionPublication& subscriptionPublication
            )
    );

};

class MockClientCache : public joynr::IClientCache {
public:
   MOCK_METHOD2(lookUp, QVariant(const QString& attributeId, qint64 maxAcceptedAgeInMs));
   MOCK_METHOD2(insert, void(QString attributeId, QVariant value));
};

class MockDiscovery : public joynr::system::IDiscovery {
public:
    MOCK_METHOD2(
            add,
            void(
                joynr::RequestStatus& joynrInternalStatus ,
                joynr::system::DiscoveryEntry entry
            )
    );
    MOCK_METHOD5(
            lookup,
            void(
                joynr::RequestStatus& joynrInternalStatus,
                QList<joynr::system::DiscoveryEntry> & result,
                QString domain,
                QString interfaceName,
                joynr::system::DiscoveryQos discoveryQos
            )
    );
    MOCK_METHOD3(
            lookup,
            void(
                joynr::RequestStatus& joynrInternalStatus,
                joynr::system::DiscoveryEntry& result,
                QString participantId
            )
    );
    MOCK_METHOD2(
            remove,
            void(
                joynr::RequestStatus& joynrInternalStatus,
                QString participantId
            )
    );
    MOCK_METHOD2(
            add,
            void(
                QSharedPointer<joynr::ICallback<void> > callback,
                joynr::system::DiscoveryEntry entry
            )
    );
    MOCK_METHOD3(
            add,
            void(
                QSharedPointer<joynr::Future<void> > future,
                QSharedPointer<joynr::ICallback<void> > callback,
                joynr::system::DiscoveryEntry entry
            )
    );
    MOCK_METHOD2(
            add,
            void(
                QSharedPointer<joynr::Future<void> > future,
                joynr::system::DiscoveryEntry entry
            )
    );
    MOCK_METHOD4(
            lookup,
            void(
                QSharedPointer<joynr::ICallback<QList<joynr::system::DiscoveryEntry> > > callback,
                QString domain,
                QString interfaceName,
                joynr::system::DiscoveryQos discoveryQos
            )
    );
    MOCK_METHOD5(
            lookup,
            void(
                QSharedPointer<joynr::Future<QList<joynr::system::DiscoveryEntry> > > future,
                QSharedPointer<joynr::ICallback<QList<joynr::system::DiscoveryEntry> > > callback,
                QString domain,
                QString interfaceName,
                joynr::system::DiscoveryQos discoveryQos
            )
    );
    MOCK_METHOD4(
            lookup,
            void(
                QSharedPointer<joynr::Future<QList<joynr::system::DiscoveryEntry> > > future,
                QString domain,
                QString interfaceName,
                joynr::system::DiscoveryQos discoveryQos
            )
    );
    MOCK_METHOD2(
            lookup,
            void(
                QSharedPointer<joynr::ICallback<joynr::system::DiscoveryEntry> > callback,
                QString participantId
            )
    );
    MOCK_METHOD3(
            lookup,
            void(
                QSharedPointer<joynr::Future<joynr::system::DiscoveryEntry> > future,
                QSharedPointer<joynr::ICallback<joynr::system::DiscoveryEntry> > callback,
                QString participantId
            )
    );
    MOCK_METHOD2(
            lookup,
            void(
                QSharedPointer<joynr::Future<joynr::system::DiscoveryEntry> > future,
                QString participantId
            )
    );
    MOCK_METHOD2(
            remove,
            void(
                QSharedPointer<joynr::ICallback<void> > callback,
                QString participantId
            )
    );
    MOCK_METHOD3(
            remove,
            void(
                QSharedPointer<joynr::Future<void> > future,
                QSharedPointer<joynr::ICallback<void> > callback,
                QString participantId
            )
    );
    MOCK_METHOD2(
            remove,
            void(
                QSharedPointer<joynr::Future<void> > future,
                QString participantId
            )
    );
};

class IMockProviderInterface {
public:
    virtual ~IMockProviderInterface(){}
    static const QString getInterfaceName();
};


class MockProvider : public joynr::Provider, public IMockProviderInterface {
public:
    MOCK_CONST_METHOD0(getProviderQos, joynr::types::ProviderQos());
    MOCK_CONST_METHOD0(getParticipantId, QString());
    virtual ~MockProvider(){}
};

namespace joynr {

template<>
class RequestCallerFactoryHelper<MockProvider> {
public:
    QSharedPointer<RequestCaller> create(QSharedPointer<MockProvider> provider) {
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
    MOCK_METHOD3(sendMessage,void(const QString&, const QDateTime&, const joynr::JoynrMessage&));
    MOCK_METHOD2(init,void(QSharedPointer<joynr::ILocalChannelUrlDirectory> channelUrlDirectory,const joynr::MessagingSettings& settings));
};

/*
 * Typed Callbacks
 */

class MockGpsLocationCallback : public joynr::ICallback<joynr::types::GpsLocation> {
public:
    MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
    MOCK_METHOD2(onSuccess, void(const joynr::RequestStatus status, joynr::types::GpsLocation result));
};

class MockIntCallback : public joynr::ICallback<int> {
public:
    MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
    MOCK_METHOD2(onSuccess, void(const joynr::RequestStatus status, int result));
};

class MockQInt64Callback : public joynr::ICallback<qint64> {
public:
    MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
    MOCK_METHOD2(onSuccess, void(const joynr::RequestStatus status, qint64 result));
};

class MockQInt8Callback : public joynr::ICallback<qint8> {
public:
    MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
    MOCK_METHOD2(onSuccess, void(const joynr::RequestStatus status, qint8 result));
};

class MockVoidCallback : public joynr::ICallback<void> {

public:
    MOCK_METHOD1(onSuccess, void(const joynr::RequestStatus status));
    MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
};

class MockMessagingStubFactory : public joynr::IMessagingStubFactory {
public:
    MOCK_METHOD2(create, QSharedPointer<joynr::IMessaging>(QString destParticipantId, const joynr::system::Address& destEndpointAddress));
    MOCK_METHOD1(remove, void(QString destParticipantId));
    MOCK_METHOD1(contains, bool(QString destPartId));
};


class IGlobalCapabilitiesCallbackMock : public joynr::IGlobalCapabilitiesCallback {
public:
    MOCK_METHOD1(capabilitiesReceived, void(QList<joynr::types::CapabilityInformation> results) );
};



class MockGpsProvider : public joynr::vehicle::DefaultGpsProvider
{
    public:
    MockGpsProvider() : joynr::vehicle::DefaultGpsProvider(joynr::types::ProviderQos(QList<joynr::types::CustomParameter>(),1,1,joynr::types::ProviderScope::GLOBAL,false))
    {
    };
    ~MockGpsProvider()
    {
        qDebug() << "I am being destroyed_ MockProvider";
    };
    /*void getLocation(RequestStatus& status, GpsLocation& result)
    {
        result = GpsLocation(GpsFixEnum::MODE2D, 4,5,6,7);
    }
    */
    MOCK_METHOD2(getLocation, void(joynr::RequestStatus& status, joynr::types::GpsLocation& result) );
    MOCK_METHOD2(setLocation, void(joynr::RequestStatus& status, joynr::types::GpsLocation gpsLocation));
    //MOCK_METHOD2(calculateAvailableSatellites,void (RequestStatus& status, int& result));
    //MOCK_METHOD2(restartWithRetries, void (RequestStatus& status, int gpsFix));

    void  restartWithRetries(joynr::RequestStatus& status, int gpsfix ) {

        status.setCode(joynr::RequestStatusCode::OK);
    }

    void  calculateAvailableSatellites(joynr::RequestStatus& status, int& result) {
        result = 42;
        status.setCode(joynr::RequestStatusCode::OK);
    }


    QString getParticipantId() const{
        return QString("Fake_ParticipantId_vehicle/DefaultGpsProvider");
    }
};

class MockTestProvider : public joynr::tests::DefaultTestProvider
{
public:
    MockTestProvider() :
        joynr::tests::DefaultTestProvider(joynr::types::ProviderQos(QList<joynr::types::CustomParameter>(),1,1,joynr::types::ProviderScope::GLOBAL,false))
    {
    };
    MockTestProvider(joynr::types::ProviderQos qos) :
        DefaultTestProvider(qos)
    {
    };
    ~MockTestProvider()
    {
    };

    MOCK_METHOD2(getLocation, void(joynr::RequestStatus& status, joynr::types::GpsLocation& result) );
    MOCK_METHOD2(setLocation, void(joynr::RequestStatus& status, joynr::types::GpsLocation gpsLocation));

    void  sumInts(joynr::RequestStatus& status, int& result, QList<int>  ints) {
        result = 0;
        int j;
        foreach ( j, ints) {
            result += j;
        }
        status.setCode(joynr::RequestStatusCode::OK);
    }
    void  returnPrimeNumbers(joynr::RequestStatus& status, QList<int> & result, int upperBound) {
        assert(upperBound<7);
        result.clear();
        result << 2 << 3 << 5;
        status.setCode(joynr::RequestStatusCode::OK);
    }
    void  optimizeTrip(joynr::RequestStatus& status, joynr::types::Trip& result, joynr::types::Trip input) {
         result = input; //just copy the trip and return it.
         status.setCode(joynr::RequestStatusCode::OK);
    }
    void optimizeLocationList(joynr::RequestStatus &status, QList<joynr::types::GpsLocation> &result, QList<joynr::types::GpsLocation> inputList)
    {
        result = inputList; //just copy the trip and return it.
        status.setCode(joynr::RequestStatusCode::OK);
    }

    void overloadedOperation(joynr::RequestStatus& status,
                             QString& result,
                             joynr::tests::DerivedStruct input) {
        result = "DerivedStruct";
        status.setCode(joynr::RequestStatusCode::OK);
    }

    void overloadedOperation(joynr::RequestStatus& status,
                             QString& result,
                             joynr::tests::AnotherDerivedStruct input) {
        result = "AnotherDerivedStruct";
        status.setCode(joynr::RequestStatusCode::OK);
    }
};

class MockTestRequestCaller : public joynr::tests::TestRequestCaller {
public:
//    MockTestRequestCaller() : joynr::vehicle::GpsRequestCaller(QSharedPointer<joynr::vehicle::GpsProvider>(new MockGpsProvider()) ) {}
    MockTestRequestCaller() : joynr::tests::TestRequestCaller(QSharedPointer<joynr::tests::TestProvider>(new MockTestProvider()) ) {}
    MOCK_METHOD2(getLocation, void(joynr::RequestStatus& status, joynr::types::GpsLocation& location));
    MOCK_METHOD2(registerAttributeListener, void(const QString& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const QString& attributeName, joynr::IAttributeListener* attributeListener));
};

class MockGpsRequestCaller : public joynr::vehicle::GpsRequestCaller {
public:
    MockGpsRequestCaller() : joynr::vehicle::GpsRequestCaller(QSharedPointer<joynr::vehicle::GpsProvider>(new MockGpsProvider()) ) {}
    MOCK_METHOD2(getLocation, void(joynr::RequestStatus& status, joynr::types::GpsLocation& location));
    MOCK_METHOD2(registerAttributeListener, void(const QString& attributeName, joynr::IAttributeListener* attributeListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const QString& attributeName, joynr::IAttributeListener* attributeListener));
};


class MockIRequestCallerDirectory : public joynr::IRequestCallerDirectory {
public:
    MOCK_METHOD1(lookupRequestCaller, QSharedPointer<joynr::RequestCaller>(const QString& participantId));
    MOCK_METHOD1(containsRequestCaller, bool(const QString& participantId));
};

class MockEndpointAddress : public joynr::system::Address {

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

typedef MockDirectory<QString, joynr::system::Address> MockMessagingEndpointDirectory;




class MockSubscriptionManager : joynr::SubscriptionManager {
public:
    MOCK_METHOD1(getSubscriptionCallback,QSharedPointer<joynr::ISubscriptionCallback>(const QString& subscriptionId));
    MOCK_METHOD6(registerSubscription,void(const QString &proxyId,const QString &providerId,
                                                    const QString &attributeName,
                                                    joynr::ISubscriptionCallback * attributeSubscriptionCaller, // SubMgr gets ownership of ptr
                                                    const joynr::SubscriptionQos &qos,
                                                    joynr::SubscriptionRequest& subscriptionReques));
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
        ChannelUrlDirectoryProxy(QSharedPointer<joynr::system::Address> (new joynr::system::Address()), NULL, NULL, "domain",joynr::ProxyQos(), joynr::MessagingQos(), false),
        joynr::ProxyBase(NULL, NULL, "domain", "INTERFACE_NAME", joynr::ProxyQos(), joynr::MessagingQos(), false),
        ChannelUrlDirectoryProxyBase(QSharedPointer<joynr::system::Address> (new joynr::system::Address()), NULL, NULL, "domain", joynr::ProxyQos(), joynr::MessagingQos(), false),
        ChannelUrlDirectorySyncProxy(QSharedPointer<joynr::system::Address> (new joynr::system::Address()), NULL, NULL, "domain", joynr::ProxyQos(), joynr::MessagingQos(), false),
        ChannelUrlDirectoryAsyncProxy(QSharedPointer<joynr::system::Address> (new joynr::system::Address()), NULL, NULL, "domain", joynr::ProxyQos(), joynr::MessagingQos(), false){}


    MOCK_METHOD2(getUrlsForChannel,void (QSharedPointer<joynr::Future<joynr::types::ChannelUrlInformation> > future,
                                         QString channelId));

    MOCK_METHOD3(registerChannelUrls, void(QSharedPointer<joynr::Future<void> > future,
                                           QString channelId,
                                           joynr::types::ChannelUrlInformation channelUrlInformation));

    MOCK_METHOD2(unregisterChannelUrls, void(QSharedPointer<joynr::Future<void> > future, QString channelId));
};


class MockLocalChannelUrlDirectory : public joynr::ILocalChannelUrlDirectory {
public:
    MOCK_METHOD3(registerChannelUrls, void(
                     QSharedPointer<joynr::Future<void> > future,
                     const QString& channelId,
                     joynr::types::ChannelUrlInformation channelUrlInformation));

    MOCK_METHOD2(unregisterChannelUrls, void(
            QSharedPointer<joynr::Future<void> > future,
            const QString& channelId));

    MOCK_METHOD3(getUrlsForChannel, void(
            QSharedPointer<joynr::Future<joynr::types::ChannelUrlInformation> > future,
            const QString& channelId,
            const qint64& timeout_ms));
};

class MockVoidOperationCallback : public joynr::ICallback<void> {
public:
    ~MockVoidOperationCallback() {}
    MOCK_METHOD1(onSuccess, void(const joynr::RequestStatus status));
    MOCK_METHOD1(onFailure, void(const joynr::RequestStatus status));
};

class MockParticipantIdStorage : public joynr::ParticipantIdStorage {
public:
    MockParticipantIdStorage() : ParticipantIdStorage(QString("mock filename")) {

    }
    MOCK_METHOD3(getProviderParticipantId, QString(const QString& domain, const QString& interfaceName, const QString& authenticationToken));
    MOCK_METHOD4(getProviderParticipantId, QString(const QString& domain, const QString& interfaceName, const QString& authenticationToken, const QString& defaultValue));
};

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif /* MOCKOBJECTS_H_ */

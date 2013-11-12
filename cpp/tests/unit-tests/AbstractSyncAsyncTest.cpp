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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/IClientCache.h"
#include "joynr/ProxyQos.h"
#include "joynr/IMessaging.h"
#include "joynr/vehicle/GpsJoynrMessagingConnector.h"
#include "joynr/IJoynrMessageSender.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/Dispatcher.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/ReplyCaller.h"
#include "joynr/IReplyCaller.h"
#include "joynr/Dispatcher.h"
#include "tests/utils/MockObjects.h"
#include <QVariant>

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Invoke;
using ::testing::Unused;
using namespace joynr;

/**
 * These tests test the communication from X through to the JoynrMessageSender.
 * Some methods are defined here, that are used in ProxyTest and GpsJoynrMessagingConnectorTest
 */

class CallBackActions {
public:
    // for test: sync_setAttributeNotCached
    void executeCallBackVoidResult(
            Unused, // sender participant ID
            Unused, // receiver participant ID
            Unused, // messaging QoS
            Unused, // request object to send
            QSharedPointer<IReplyCaller> callback // reply caller to notify when reply is received
    ) {
        (callback.dynamicCast<ReplyCaller<void> >())->returnValue();
    }

    // related to test: sync_getAttributeNotCached
    void executeCallBackGpsLocationResult(
            Unused, // sender participant ID
            Unused, // receiver participant ID
            Unused, // messaging QoS
            Unused, // request object to send
            QSharedPointer<IReplyCaller> callback // reply caller to notify when reply is received
    ) {

        types::GpsLocation location(types::GpsFixEnum::MODE3D, 1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0, 0, 0);
        (callback.dynamicCast<ReplyCaller<types::GpsLocation> >())->returnValue(location);
    }

    // related to test: sync_OperationWithNoArguments
    void executeCallBackIntResult(
            Unused, // sender participant ID
            Unused, // receiver participant ID
            Unused, // messaging QoS
            Unused, // request object to send
            QSharedPointer<IReplyCaller> callback // reply caller to notify when reply is received
    ) {

        callback.dynamicCast<ReplyCaller<int> >()->returnValue(9);
    }
};


/**
 * @brief Fixutre.
 */
class AbstractSyncAsyncTest : public ::testing::Test {
public:
    AbstractSyncAsyncTest():
        callBackActions(),
        qosSettings(),
        mockDispatcher(),
        mockMessagingStub(),
        callBack(),
        mockJoynrMessageSender(),
        proxyParticipantId(),
        providerParticipantId(),
        mockClientCache(),
        endPointAddress(),
        gpsLocationResult(),
        asyncGpsFixture(NULL)
    {}
    virtual ~AbstractSyncAsyncTest(){}
    void SetUp(){
        qosSettings = MessagingQos(456000);
        endPointAddress = QSharedPointer<JoynrMessagingEndpointAddress>(new JoynrMessagingEndpointAddress("endPointAddress"));
        proxyParticipantId = "participantId";
        providerParticipantId = "providerParticipantId";
        gpsLocationResult = types::GpsLocation(types::GpsFixEnum::MODE3D, 12.0, 14.0, 16.0, 0,0,0,0,0,0);
        mockJoynrMessageSender = new MockJoynrMessageSender();
        // asyncGpsFixture must be created after derived objects have run Setup()
    }

    void TearDown(){
        delete mockJoynrMessageSender;
        delete asyncGpsFixture;
    }



    // sets the expectations on the call expected on the MessageSender from the connector
    virtual testing::internal::TypedExpectation<void(
            const QString&,
            const QString&,
            const MessagingQos&,
            const Request&,
            QSharedPointer<IReplyCaller>
    )>& setExpectationsForSendRequestCall(QString expectedType, QString methodName) = 0;

    virtual vehicle::IGps* createFixture(bool cacheEnabled)=0;

    void testAsync_getAttributeNotCached() {
        asyncGpsFixture = createFixture(false);

        MockGpsLocationCallback* callback = new MockGpsLocationCallback();
        QSharedPointer<ICallback<joynr::types::GpsLocation> > spCallback = QSharedPointer<ICallback<joynr::types::GpsLocation> > (callback);

        setExpectationsForSendRequestCall("joynr__types__GpsLocation", "getLocation");
        asyncGpsFixture->getLocation(spCallback);
    }



    void testSync_setAttributeNotCached() {
        vehicle::IGps* gpsFixture = createFixture(false);

        EXPECT_CALL(
                    *mockJoynrMessageSender,
                    sendRequest(
                        _, //Eq(proxyParticipantId), // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _, // messaging QoS
                        AllOf(
                            Property(&Request::getMethodName, Eq("setLocation")),
                            Property(&Request::getParams, (Property(&QList<QVariant>::size, Eq(1))))
                        ), // request object to send
                        Property(
                            &QSharedPointer<IReplyCaller>::data,
                            AllOf(NotNull(), Property(&IReplyCaller::getTypeName, Eq("void")))
                        ) // reply caller to notify when reply is received
                    )
        ).WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackVoidResult));

        types::GpsLocation location(types::GpsFixEnum::MODE3D, 1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0, 0, 0);
        RequestStatus status;
        gpsFixture->setLocation(status, location);
        delete gpsFixture;
    }


    void testSync_getAttributeNotCached() {
        vehicle::IGps* gpsFixture = createFixture(false);
        setExpectationsForSendRequestCall("joynr__types__GpsLocation", "getLocation")
                .WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackGpsLocationResult));

        RequestStatus status;
        types::GpsLocation gpsLocation;
        gpsFixture->getLocation(status, gpsLocation);
        types::GpsLocation location(types::GpsFixEnum::MODE3D, 1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0, 0, 0);
        EXPECT_EQ(gpsLocation, location);
        EXPECT_TRUE(status.successful());
        delete gpsFixture;
    }

    void testAsync_getAttributeCached() {
        asyncGpsFixture = createFixture(true);

        MockGpsLocationCallback* callback = new MockGpsLocationCallback();
        QSharedPointer<ICallback<types::GpsLocation> > spCallback = QSharedPointer<ICallback<types::GpsLocation> > (callback);

        setExpectationsForSendRequestCall("joynr__types__GpsLocation", "getLocation").Times(0);

        types::GpsLocation location(types::GpsFixEnum::MODE3D, 1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0, 0, 0);
        QVariant qvariant;
        qvariant.setValue(location);

        ON_CALL(mockClientCache, lookUp(_,_)).WillByDefault(Return(qvariant));

        asyncGpsFixture->getLocation(spCallback);
    }

    void testSync_getAttributeCached() {
        vehicle::IGps* gpsFixture = createFixture(true);

        setExpectationsForSendRequestCall("joynr__types__GpsLocation", "getLocation").Times(0);

        types::GpsLocation location(types::GpsFixEnum::MODE3D, 1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0, 0, 0);
        QVariant qvariant;
        qvariant.setValue(location);
        ON_CALL(mockClientCache, lookUp(_,_)).WillByDefault(Return(qvariant));

        RequestStatus status;
        types::GpsLocation gpsLocation;
        gpsFixture->getLocation(status, gpsLocation);
        EXPECT_EQ(gpsLocation, location);
        EXPECT_TRUE(status.successful());
        delete gpsFixture;
    }

    void testAsync_OperationWithNoArguments() {
        asyncGpsFixture = createFixture(false);

        MockIntCallback* callback = new MockIntCallback();
        QSharedPointer<ICallback<int> > spCallback = QSharedPointer<ICallback<int> > (callback);

        setExpectationsForSendRequestCall("int", "calculateAvailableSatellites");

        asyncGpsFixture->calculateAvailableSatellites(spCallback);
    }

    void testSync_OperationWithNoArguments() {
        vehicle::IGps* gpsFixture = createFixture(false);
        setExpectationsForSendRequestCall("int", "calculateAvailableSatellites")
                .WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackIntResult));

        RequestStatus requestStatus;
        int result;
        gpsFixture->calculateAvailableSatellites(requestStatus, result);
        EXPECT_EQ(result, 9);
        EXPECT_TRUE(requestStatus.successful());
        delete gpsFixture;
    }

    void testSubscribeToAttribute() {
        //EXPECT_CALL(*mockJoynrMessageSender,
        //            sendSubscriptionRequest(_,_,_,_)).Times(1);

        QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    new MockGpsSubscriptionListener());
        //TODO uncomment once the connector has the correct signature!
        //vehicle::IGps* gpsFixture = createFixture(false);
        //SubscriptionQos  subscriptionQos(100, 200, true, 80, 80);
        //gpsFixture->subscribeToLocation(subscriptionListener, subscriptionQos);
        //delete gpsFixture;
    }

protected:
    CallBackActions callBackActions;
    MessagingQos qosSettings;
    MockDispatcher mockDispatcher;
    MockMessaging mockMessagingStub;
    QSharedPointer<IReplyCaller> callBack;
    MockJoynrMessageSender* mockJoynrMessageSender;
    QString proxyParticipantId;
    QString providerParticipantId;
    MockClientCache mockClientCache;
    QSharedPointer<JoynrMessagingEndpointAddress> endPointAddress;
    types::GpsLocation gpsLocationResult;
    vehicle::IGps *asyncGpsFixture;
private:
    DISALLOW_COPY_AND_ASSIGN(AbstractSyncAsyncTest);
};

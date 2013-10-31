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
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/ICommunicationManager.h"
#include "joynr/HttpCommunicationManager.h"
#include "joynr/MessagingSettings.h"
#include "common/SettingsMerger.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/Future.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

#include "joynr/vehicle/IGps.h"
#include "joynr/vehicle/GpsProvider.h"

#include "joynr/tests/ITest.h"
#include "joynr/tests/TestProvider.h"
#include "joynr/tests/TestProxy.h"
#include "joynr/vehicle/GpsProxy.h"

//using testing::Return;
//using testing::ReturnRef;
//using testing::ByRef;
//using testing::SetArgReferee;
//using testing::AtLeast;
using namespace ::testing;
using namespace joynr;

using testing::Return;
using testing::ReturnRef;
using testing::ByRef;
using testing::SetArgReferee;
using testing::AtLeast;

class CombinedRunTimeTest : public ::testing::Test {
public:
    QString settingsFilename;
    QString libjoynrSettingsFilename;
    JoynrClusterControllerRuntime* runtime;
    ICommunicationManager* mockCommunicationManager; //will be deleted when runtime is deleted.
    QSettings settings;
    MessagingSettings* messagingSettings;
    CombinedRunTimeTest()
        : settingsFilename("resources/integrationtest.settings"),
          libjoynrSettingsFilename("resources/libjoynrintegrationtest.settings"),
          runtime(NULL),
          mockCommunicationManager( new MockCommunicationManager() ),
          settings(settingsFilename, QSettings::IniFormat),
          messagingSettings(new MessagingSettings(settings))
    {
        QString str("CombinedRunTimeTestChannelId");
		assert(str.isEmpty() == false);
        EXPECT_CALL(*(dynamic_cast<MockCommunicationManager*>(mockCommunicationManager)), getReceiveChannelId() )
                .WillRepeatedly(::testing::ReturnRefOfCopy(str));
        //runtime can only be created, after MockCommunicationManager has been told to return
        //a channelId for getReceiveChannelId.
        QSettings* settings = SettingsMerger::mergeSettings(settingsFilename);
        SettingsMerger::mergeSettings(libjoynrSettingsFilename, settings);
        runtime = new JoynrClusterControllerRuntime(NULL, settings, mockCommunicationManager );

    }

    ~CombinedRunTimeTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
    }
private:
    DISALLOW_COPY_AND_ASSIGN(CombinedRunTimeTest);
};


void SetUp(){
}

void TearDown(){
    QFile::remove("SubscriptionRequests.persist");
    QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
}

TEST_F(CombinedRunTimeTest, communicationManagerMockReturnsChannelId){
    ASSERT_NE(mockCommunicationManager->getReceiveChannelId(), "");
}

TEST_F(CombinedRunTimeTest, instantiate_Runtime)
{
    ASSERT_TRUE(runtime != NULL);
}

TEST_F(CombinedRunTimeTest, startMessaging_does_Not_Throw)
{
    EXPECT_CALL(*(dynamic_cast<MockCommunicationManager*>(mockCommunicationManager)), startReceiveQueue() )
            .Times(1);
    EXPECT_CALL(*(dynamic_cast<MockCommunicationManager*>(mockCommunicationManager)), stopReceiveQueue() )
            .Times(2);

    runtime->startMessaging();
    ASSERT_TRUE(runtime != NULL);
    runtime->stopMessaging();
}

TEST_F(CombinedRunTimeTest, register_and_use_local_Provider)
{
    QString domain = "testDomain0";
    QSharedPointer<MockGpsProvider> mockProvider(new MockGpsProvider());
    types::GpsLocation gpsLocation1(types::GpsFixEnum::Mode2D, 1.1, 2.2, 3.3, 0.0, 0.0, 0.0, 0, 0, 444);
    EXPECT_CALL(*mockProvider, getLocation(A<RequestStatus&>(), A<types::GpsLocation&>()))
           .WillOnce(DoAll(SetArgReferee<0>(RequestStatusCode::OK), SetArgReferee<1>(gpsLocation1)));

   runtime->startMessaging();
   QString participantId = runtime->registerCapability<vehicle::GpsProvider>(domain, mockProvider, QString());

   ProxyBuilder<vehicle::GpsProxy>* gpsProxyBuilder =
           runtime->getProxyBuilder<vehicle::GpsProxy>(domain);

   DiscoveryQos discoveryQos(1000);
   discoveryQos.addCustomParameter("fixedParticipantId", participantId);
   discoveryQos.setDiscoveryTimeout(50);
   discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_CHANNEL);


   vehicle::GpsProxy* gpsProxy = gpsProxyBuilder
                                       ->setRuntimeQos(MessagingQos(5000))
                                       ->setCached(false)
                                       ->setDiscoveryQos(discoveryQos)
                                       ->build();

   QSharedPointer<Future<types::GpsLocation> > future( new Future<types::GpsLocation>() );
   gpsProxy->getLocation(future);
   future->waitForFinished(500);
   EXPECT_EQ(gpsProxy->getInterfaceName(), "vehicle/gps");
   ASSERT_EQ(RequestStatusCode::OK.toString(), future->getStatus().getCode().toString());
   EXPECT_EQ(gpsLocation1, future->getValue());
   delete gpsProxy;
   delete gpsProxyBuilder;

}



  // This test is similar to register_and_use_local, but uses the IDL Test
  // It also tests lists as input/output-parameters of methods and lists as attributes.


TEST_F(CombinedRunTimeTest, DISABLED_register_and_use_local_TestProvider)
{

      // This test is disabled, because currently the inprocess-connectors are not implemented.


   QString domain = "testDomain";
   QString channelId = "testChannel";
   RequestStatus rs;
   types::GpsLocation location;

   QSharedPointer<MockTestProvider> mockProvider ( new MockTestProvider(types::ProviderQos()));
   mockProvider->getLocation(rs, location);
// Does not work yet, because CapabilitiesStubWrapper is not finished yet.
//   EXPECT_CALL(mockProvider, getLocation(_,_))
//           .Times(1);
   runtime->startMessaging();
   runtime->registerCapability<tests::TestProvider>(domain, mockProvider, QString());

   ProxyBuilder<tests::TestProxy>* testProxyBuilder =
           runtime->getProxyBuilder<tests::TestProxy>(domain);

   // NOTE: this method is not available any more
   QString participantId; // = mockProvider->getParticipantId();
   DiscoveryQos discoveryQos(1000);
   discoveryQos.addCustomParameter("fixedParticipantId", participantId);
   discoveryQos.setDiscoveryTimeout(50);
   discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_CHANNEL);


   tests::TestProxy* testProxy = testProxyBuilder
                                       ->setRuntimeQos(MessagingQos(5000))
                                       ->setCached(false)
                                       ->setDiscoveryQos(discoveryQos)
                                       ->build();
   QSharedPointer<Future<int> > future( new Future<int>() );
   QList<int> list;
   list << 4 << 6 << 12;
   testProxy->sumInts(future,list);
   future->waitForFinished(500);
   EXPECT_EQ(testProxy->getInterfaceName(), "tests/Test");
   ASSERT_EQ(RequestStatusCode::OK.toString(), future->getStatus().getCode().toString());
   EXPECT_EQ(22,future->getValue());

}


/**
  * Tests if subscription works locally (polling attribute values). Needs to be enabled once the
  * signature of the subscribeTo... method is corrected to expect a listener, not a callback
  *
  * Tests if subscription works locally (polling attribute values).
  */
TEST_F(CombinedRunTimeTest, register_and_subscribe_to_local_Provider) {
    QFile::remove("SubscriptionRequests.persist");
    QString domain = "testDomain";
    QSharedPointer<MockGpsProvider> mockProvider (new MockGpsProvider());
    types::GpsLocation gpsLocation1(types::GpsFixEnum::Mode2D, 1.1, 2.2, 3.3, 0, 0, 0, 0, 0, 444);
    EXPECT_CALL(*mockProvider, getLocation(A<RequestStatus&>(), A<types::GpsLocation&>()))
           .Times(::testing::Between(1, 2))
           .WillRepeatedly(SetArgReferee<1>(gpsLocation1));

    runtime->startMessaging();
    QString participantId = runtime->registerCapability<vehicle::GpsProvider>(domain, mockProvider, QString());

    ProxyBuilder<vehicle::GpsProxy>* gpsProxyBuilder =
            runtime->getProxyBuilder<vehicle::GpsProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_CHANNEL);


    vehicle::GpsProxy* gpsProxy = gpsProxyBuilder
                                        ->setRuntimeQos(MessagingQos(5000))
                                        ->setCached(false)
                                        ->setDiscoveryQos(discoveryQos)
                                        ->build();

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                new MockGpsSubscriptionListener());

    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(480, 200, 200, 100));
    QString subScriptionId = gpsProxy->subscribeToLocation(subscriptionListener, subscriptionQos);
    QThreadSleep::msleep(250);
    gpsProxy->unsubscribeFromLocation(subScriptionId);
    delete gpsProxy;
    delete gpsProxyBuilder;
}


TEST_F(CombinedRunTimeTest, unsubscribe_from_local_Provider) {
    QFile::remove("SubscriptionRequests.persist");
    QString domain = "testDomain2";
    QSharedPointer<MockGpsProvider> mockProvider (new MockGpsProvider());
    types::GpsLocation gpsLocation1(types::GpsFixEnum::Mode2D, 1.1, 2.2, 3.3, 0, 0, 0, 0, 0, 444);
    EXPECT_CALL(*mockProvider, getLocation(A<RequestStatus&>(), A<types::GpsLocation&>()))
            .Times(AtLeast(2))
            .WillRepeatedly(SetArgReferee<1>(gpsLocation1));

    runtime->startMessaging();
    QString participantId = runtime->registerCapability<vehicle::GpsProvider>(domain, mockProvider, QString());

    ProxyBuilder<vehicle::GpsProxy>* gpsProxyBuilder =
            runtime->getProxyBuilder<vehicle::GpsProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_CHANNEL);


    vehicle::GpsProxy* gpsProxy = gpsProxyBuilder
                                        ->setRuntimeQos(MessagingQos(5000))
                                        ->setCached(false)
                                        ->setDiscoveryQos(discoveryQos)
                                        ->build();

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                new MockGpsSubscriptionListener());

    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(800, 200, 200, 10000));
    QString subscriptionId = gpsProxy->subscribeToLocation(subscriptionListener, subscriptionQos);
    QThreadSleep::msleep(600);
    gpsProxy->unsubscribeFromLocation(subscriptionId);
    QThreadSleep::msleep(600);
    delete gpsProxyBuilder;
    delete gpsProxy;
}


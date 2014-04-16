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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "PrettyPrint.h"
#include "joynr/PrivateCopyAssign.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/SettingsMerger.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/Future.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

#include "joynr/tests/ITest.h"
#include "joynr/tests/TestProvider.h"
#include "joynr/tests/TestProxy.h"

using namespace ::testing;
using namespace joynr;

using testing::Return;
using testing::ReturnRef;
using testing::ByRef;
using testing::SetArgReferee;
using testing::AtLeast;

class JoynrClusterControllerRuntimeTest : public ::testing::Test {
public:
    QString settingsFilename;
    QString libjoynrSettingsFilename;
    JoynrClusterControllerRuntime* runtime;
    MockMessageReceiver* mockMessageReceiver; // will be deleted when runtime is deleted.
    MockMessageSender* mockMessageSender;
    QSettings settings;

    JoynrClusterControllerRuntimeTest() :
            settingsFilename("test-resources/integrationtest.settings"),
            libjoynrSettingsFilename("test-resources/libjoynrintegrationtest.settings"),
            runtime(NULL),
            mockMessageReceiver(new MockMessageReceiver()),
            mockMessageSender(new MockMessageSender()),
            settings(settingsFilename, QSettings::IniFormat)
    {
        QString channelId("JoynrClusterControllerRuntimeTest.ChannelId");

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        QSettings* settings = SettingsMerger::mergeSettings(settingsFilename);
        SettingsMerger::mergeSettings(libjoynrSettingsFilename, settings);
        runtime = new JoynrClusterControllerRuntime(NULL, settings, mockMessageReceiver, mockMessageSender);
    }

    ~JoynrClusterControllerRuntimeTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
    }
private:
    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntimeTest);
};


void SetUp(){
}

void TearDown(){
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCIPTIONREQUEST_STORAGE_FILENAME());
    QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
}

TEST_F(JoynrClusterControllerRuntimeTest, instantiateRuntime)
{
    ASSERT_TRUE(runtime != NULL);
}

TEST_F(JoynrClusterControllerRuntimeTest, startMessagingDoesNotThrow)
{
    EXPECT_CALL(*mockMessageReceiver, startReceiveQueue())
            .Times(1);
    EXPECT_CALL(*mockMessageReceiver, stopReceiveQueue())
            .Times(2);

    ASSERT_TRUE(runtime != NULL);
    runtime->startMessaging();
    runtime->stopMessaging();
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProvider)
{
    QString domain("JoynrClusterControllerRuntimeTest.Domain.A");
    QString authenticationToken("JoynrClusterControllerRuntimeTest.AuthenticationToken.A");
    QSharedPointer<MockTestProvider> mockTestProvider(new MockTestProvider());

    types::GpsLocation gpsLocation(
                1.1,                        // longitude
                2.2,                        // latitude
                3.3,                        // altitude
                types::GpsFixEnum::MODE2D,  // gps fix
                0.0,                        // heading
                0.0,                        // quality
                0.0,                        // elevation
                0.0,                        // bearing
                444,                        // gps time
                444,                        // device time
                444                         // time
    );
    RequestStatus requestStatus;
    requestStatus.setCode(RequestStatusCode::OK);
    EXPECT_CALL(*mockTestProvider, getLocation(A<RequestStatus&>(), A<types::GpsLocation&>()))
            .WillOnce(DoAll(SetArgReferee<0>(requestStatus), SetArgReferee<1>(gpsLocation)));

    runtime->startMessaging();
    QString participantId = runtime->registerCapability<tests::TestProvider>(
                domain,
                mockTestProvider,
                authenticationToken
    );

    ProxyBuilder<tests::TestProxy>* testProxyBuilder =
            runtime->getProxyBuilder<tests::TestProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);


    tests::TestProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<Future<types::GpsLocation> > future(new Future<types::GpsLocation>());
    testProxy->getLocation(future);
    future->waitForFinished(500);

    EXPECT_EQ(tests::TestProxy::getInterfaceName(), testProxy->getInterfaceName());
    ASSERT_EQ(RequestStatusCode::OK, future->getStatus().getCode());
    EXPECT_EQ(gpsLocation, future->getValue());
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProviderWithListArguments)
{
    QString domain("JoynrClusterControllerRuntimeTest.Domain.A");
    QString authenticationToken("JoynrClusterControllerRuntimeTest.AuthenticationToken.A");
    QSharedPointer<MockTestProvider> mockTestProvider(new MockTestProvider());

    QList<int> ints;
    ints << 4 << 6 << 12;
    int sum = 22;
    RequestStatus requestStatus;
    requestStatus.setCode(RequestStatusCode::OK);

    runtime->startMessaging();
    QString participantId = runtime->registerCapability<tests::TestProvider>(
                domain,
                mockTestProvider,
                authenticationToken
    );

    ProxyBuilder<tests::TestProxy>* testProxyBuilder =
            runtime->getProxyBuilder<tests::TestProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);


    tests::TestProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<Future<int> > future(new Future<int>());
    testProxy->sumInts(future, ints);
    future->waitForFinished(500);

    EXPECT_EQ(tests::TestProxy::getInterfaceName(), testProxy->getInterfaceName());
    ASSERT_EQ(RequestStatusCode::OK, future->getStatus().getCode());
    EXPECT_EQ(sum, future->getValue());
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndSubscribeToLocalProvider) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCIPTIONREQUEST_STORAGE_FILENAME());
    QString domain("JoynrClusterControllerRuntimeTest.Domain.A");
    QString authenticationToken("JoynrClusterControllerRuntimeTest.AuthenticationToken.A");
    QSharedPointer<MockTestProvider> mockTestProvider(new MockTestProvider());

    types::GpsLocation gpsLocation(
                1.1,                        // longitude
                2.2,                        // latitude
                3.3,                        // altitude
                types::GpsFixEnum::MODE2D,  // gps fix
                0.0,                        // heading
                0.0,                        // quality
                0.0,                        // elevation
                0.0,                        // bearing
                444,                        // gps time
                444,                        // device time
                444                         // time
    );
    RequestStatus requestStatus;
    requestStatus.setCode(RequestStatusCode::OK);
    EXPECT_CALL(*mockTestProvider, getLocation(A<RequestStatus&>(), A<types::GpsLocation&>()))
            .Times(Between(1, 2))
            .WillRepeatedly(DoAll(SetArgReferee<0>(requestStatus), SetArgReferee<1>(gpsLocation)));

    runtime->startMessaging();
    QString participantId = runtime->registerCapability<tests::TestProvider>(
                domain,
                mockTestProvider,
                authenticationToken
    );

    ProxyBuilder<tests::TestProxy>* testProxyBuilder =
            runtime->getProxyBuilder<tests::TestProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    tests::TestProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<MockGpsSubscriptionListener> mockSubscriptionListener(
                new MockGpsSubscriptionListener()
    );
    EXPECT_CALL(*mockSubscriptionListener, receive(gpsLocation))
            .Times(Between(1, 2));


    QSharedPointer<SubscriptionQos> subscriptionQos = QSharedPointer<SubscriptionQos>(
                new OnChangeWithKeepAliveSubscriptionQos(
                    480, // validity
                    200, // min interval
                    200, // max interval
                    100  // alert after interval
                )
    );
    QString subscriptionId = testProxy->subscribeToLocation(mockSubscriptionListener, subscriptionQos);
    QThreadSleep::msleep(250);
    testProxy->unsubscribeFromLocation(subscriptionId);
    delete testProxy;
    delete testProxyBuilder;
}


TEST_F(JoynrClusterControllerRuntimeTest, unsubscribeFromLocalProvider) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCIPTIONREQUEST_STORAGE_FILENAME());
    QString domain("JoynrClusterControllerRuntimeTest.Domain.A");
    QString authenticationToken("JoynrClusterControllerRuntimeTest.AuthenticationToken.A");
    QSharedPointer<MockTestProvider> mockTestProvider(new MockTestProvider());

    types::GpsLocation gpsLocation(
                1.1,                        // longitude
                2.2,                        // latitude
                3.3,                        // altitude
                types::GpsFixEnum::MODE2D,  // gps fix
                0.0,                        // heading
                0.0,                        // quality
                0.0,                        // elevation
                0.0,                        // bearing
                444,                        // gps time
                444,                        // device time
                444                         // time
    );
    RequestStatus requestStatus;
    requestStatus.setCode(RequestStatusCode::OK);
    EXPECT_CALL(*mockTestProvider, getLocation(A<RequestStatus&>(), A<types::GpsLocation&>()))
            .Times(Between(3, 4))
            .WillRepeatedly(DoAll(SetArgReferee<0>(requestStatus), SetArgReferee<1>(gpsLocation)));

    runtime->startMessaging();
    QString participantId = runtime->registerCapability<tests::TestProvider>(
                domain,
                mockTestProvider,
                authenticationToken
    );

    ProxyBuilder<tests::TestProxy>* testProxyBuilder =
            runtime->getProxyBuilder<tests::TestProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    tests::TestProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<MockGpsSubscriptionListener> mockSubscriptionListener(
                new MockGpsSubscriptionListener()
    );
    EXPECT_CALL(*mockSubscriptionListener, receive(gpsLocation))
            .Times(AtMost(3));

    QSharedPointer<SubscriptionQos> subscriptionQos = QSharedPointer<SubscriptionQos>(
                new OnChangeWithKeepAliveSubscriptionQos(
                    800,   // validity
                    200,   // min interval
                    200,   // max interval
                    10000  // alert after interval
                )
    );
    QString subscriptionId = testProxy->subscribeToLocation(mockSubscriptionListener, subscriptionQos);
    QThreadSleep::msleep(500);
    testProxy->unsubscribeFromLocation(subscriptionId);
    QThreadSleep::msleep(600);
    delete testProxyBuilder;
    delete testProxy;
}


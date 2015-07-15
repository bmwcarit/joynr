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
#include <memory>
#include <string>
#include "PrettyPrint.h"
#include "joynr/PrivateCopyAssign.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/Future.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/TypeUtil.h"

#include "joynr/tests/Itest.h"
#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"

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
    JoynrClusterControllerRuntime* runtime;
    joynr::types::GpsLocation gpsLocation;
    MockMessageReceiver* mockMessageReceiver; // will be deleted when runtime is deleted.
    MockMessageSender* mockMessageSender;

    JoynrClusterControllerRuntimeTest() :
            settingsFilename("test-resources/integrationtest.settings"),
            runtime(NULL),
            gpsLocation(
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
            ),
            mockMessageReceiver(new MockMessageReceiver()),
            mockMessageSender(new MockMessageSender())
    {
        QString channelId("JoynrClusterControllerRuntimeTest.ChannelId");

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        runtime = new JoynrClusterControllerRuntime(
                    NULL,
                    new QSettings(settingsFilename, QSettings::IniFormat),
                    mockMessageReceiver,
                    mockMessageSender
        );
    }

    ~JoynrClusterControllerRuntimeTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
    }

    void invokeCallbackWithGpsLocation(
            std::function<void(
                    const joynr::RequestStatus& status,
                    const joynr::types::GpsLocation location)> callbackFct) {
        callbackFct(joynr::RequestStatus(joynr::RequestStatusCode::OK), gpsLocation);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntimeTest);
};


void SetUp(){
}

void TearDown(){
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
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
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    EXPECT_CALL(*mockTestProvider,
                getLocation(A<std::function<void(const joynr::RequestStatus&,
                                                 const types::GpsLocation&)>>()))
            .WillOnce(Invoke(this,
                             &JoynrClusterControllerRuntimeTest::invokeCallbackWithGpsLocation));

    runtime->startMessaging();
    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", TypeUtil::convertStdStringtoQString(participantId));
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);


    tests::testProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<Future<types::GpsLocation> > future(testProxy->getLocation());
    future->waitForFinished(500);

    EXPECT_EQ(tests::testProxy::getInterfaceName(), testProxy->getInterfaceName());
    ASSERT_EQ(RequestStatusCode::OK, future->getStatus().getCode());
    joynr::types::GpsLocation actualValue;
    future->getValues(actualValue);
    EXPECT_EQ(gpsLocation, actualValue);
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProviderWithListArguments)
{
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    QList<int> ints;
    ints << 4 << 6 << 12;
    int sum = 22;
    RequestStatus requestStatus;
    requestStatus.setCode(RequestStatusCode::OK);

    runtime->startMessaging();
    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", TypeUtil::convertStdStringtoQString(participantId));
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);


    tests::testProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<Future<int> > future(testProxy->sumInts(ints));
    future->waitForFinished(500);

    EXPECT_EQ(tests::testProxy::getInterfaceName(), testProxy->getInterfaceName());
    ASSERT_EQ(RequestStatusCode::OK, future->getStatus().getCode());
    int actualValue;
    future->getValues(actualValue);
    EXPECT_EQ(sum, actualValue);
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndSubscribeToLocalProvider) {
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    EXPECT_CALL(*mockTestProvider,
                getLocation(A<std::function<void(const joynr::RequestStatus&,
                                                 const types::GpsLocation&)>>()))
            .Times(Between(1, 2))
            .WillRepeatedly(Invoke(this,
                                   &JoynrClusterControllerRuntimeTest::invokeCallbackWithGpsLocation));

    runtime->startMessaging();
    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", TypeUtil::convertStdStringtoQString(participantId));
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    tests::testProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<MockGpsSubscriptionListener> mockSubscriptionListener(
                new MockGpsSubscriptionListener()
    );
    EXPECT_CALL(*mockSubscriptionListener, onReceive(gpsLocation))
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
    QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    EXPECT_CALL(*mockTestProvider, getLocation(A<std::function<void(const joynr::RequestStatus&, const types::GpsLocation&)>>()))
            .Times(Between(3, 4))
            .WillRepeatedly(Invoke(this, &JoynrClusterControllerRuntimeTest::invokeCallbackWithGpsLocation));

    runtime->startMessaging();
    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", TypeUtil::convertStdStringtoQString(participantId));
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    tests::testProxy* testProxy = testProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    QSharedPointer<MockGpsSubscriptionListener> mockSubscriptionListener(
                new MockGpsSubscriptionListener()
    );
    EXPECT_CALL(*mockSubscriptionListener, onReceive(gpsLocation))
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


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
#include <vector>

#include "joynr/PrivateCopyAssign.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/Future.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/TypeUtil.h"
#include "joynr/Settings.h"
#include "joynr/ThreadUtil.h"

#include "joynr/tests/Itest.h"
#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"
#include "QSemaphore"

using namespace ::testing;
using namespace joynr;

using testing::Return;
using testing::ReturnRef;
using testing::ByRef;
using testing::SetArgReferee;
using testing::AtLeast;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->release(1);
}

class JoynrClusterControllerRuntimeTest : public ::testing::Test {
public:
    std::string settingsFilename;
    JoynrClusterControllerRuntime* runtime;
    joynr::types::Localisation::GpsLocation gpsLocation;
    MockMessageReceiver* mockMessageReceiver; // will be deleted when runtime is deleted.
    MockMessageSender* mockMessageSender;
    QSemaphore semaphore;

    JoynrClusterControllerRuntimeTest() :
            settingsFilename("test-resources/integrationtest.settings"),
            runtime(NULL),
            gpsLocation(
                1.1,                        // longitude
                2.2,                        // latitude
                3.3,                        // altitude
                types::Localisation::GpsFixEnum::MODE2D,  // gps fix
                0.0,                        // heading
                0.0,                        // quality
                0.0,                        // elevation
                0.0,                        // bearing
                444,                        // gps time
                444,                        // device time
                444                         // time
            ),
            mockMessageReceiver(new MockMessageReceiver()),
            mockMessageSender(new MockMessageSender()),
            semaphore(0)
    {
        QString channelId("JoynrClusterControllerRuntimeTest.ChannelId");

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        runtime = new JoynrClusterControllerRuntime(
                    NULL,
                    new Settings(settingsFilename),
                    mockMessageReceiver,
                    mockMessageSender
        );
    }

    ~JoynrClusterControllerRuntimeTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
    }

    void invokeOnSuccessWithGpsLocation(
            std::function<void(const joynr::types::Localisation::GpsLocation location)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError
    ) {
        onSuccess(gpsLocation);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntimeTest);
};


void SetUp(){
}

void TearDown(){
    QFile::remove(
                TypeUtil::toQt(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()));
    QFile::remove(
                TypeUtil::toQt(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME()));
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
            .Times(1);

    ASSERT_TRUE(runtime != NULL);
    runtime->startMessaging();
    runtime->stopMessaging();
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProvider)
{
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    EXPECT_CALL(
            *mockTestProvider,
            getLocation(A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                        A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
    )
            .WillOnce(Invoke(
                      this,
                      &JoynrClusterControllerRuntimeTest::invokeOnSuccessWithGpsLocation
            ));

    runtime->startMessaging();
    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);


    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    std::shared_ptr<Future<types::Localisation::GpsLocation> > future(testProxy->getLocationAsync());
    future->wait(500);

    EXPECT_EQ(tests::testProxy::INTERFACE_NAME(), testProxy->INTERFACE_NAME());
    ASSERT_EQ(RequestStatusCode::OK, future->getStatus().getCode());
    joynr::types::Localisation::GpsLocation actualValue;
    future->get(actualValue);
    EXPECT_EQ(gpsLocation, actualValue);
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProviderWithListArguments)
{
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");

    std::vector<int> ints;
    ints.push_back(4);
    ints.push_back(6);
    ints.push_back(12);
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
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);


    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    std::shared_ptr<Future<int> > future(testProxy->sumIntsAsync(ints));
    future->wait(500);

    EXPECT_EQ(tests::testProxy::INTERFACE_NAME(), testProxy->INTERFACE_NAME());
    ASSERT_EQ(RequestStatusCode::OK, future->getStatus().getCode());
    int actualValue;
    future->get(actualValue);
    EXPECT_EQ(sum, actualValue);
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndSubscribeToLocalProvider) {
    QFile::remove(TypeUtil::toQt(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()));
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    EXPECT_CALL(
            *mockTestProvider,
            getLocation(A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                        A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
    )
            .Times(AtLeast(1))
            .WillRepeatedly(Invoke(
                    this,
                    &JoynrClusterControllerRuntimeTest::invokeOnSuccessWithGpsLocation
            ));

    runtime->startMessaging();
    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener(
                new MockGpsSubscriptionListener()
    );
    EXPECT_CALL(*mockSubscriptionListener, onReceive(gpsLocation))
            .Times(AtLeast(1));


    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                    480, // validity
                    200, // min interval
                    200, // max interval
                    200  // alert after interval
                );
    std::string subscriptionId = testProxy->subscribeToLocation(mockSubscriptionListener, subscriptionQos);
    ThreadUtil::sleepForMillis(250);
    testProxy->unsubscribeFromLocation(subscriptionId);
    delete testProxy;
    delete testProxyBuilder;
}


TEST_F(JoynrClusterControllerRuntimeTest, unsubscribeFromLocalProvider) {
    QFile::remove(TypeUtil::toQt(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()));
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    EXPECT_CALL(
            *mockTestProvider,
            getLocation(A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                        A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
    )
            .WillRepeatedly(Invoke(
                    this,
                    &JoynrClusterControllerRuntimeTest::invokeOnSuccessWithGpsLocation
            ));

    runtime->startMessaging();
    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    std::shared_ptr<MockGpsSubscriptionListener> mockSubscriptionListener(
                new MockGpsSubscriptionListener()
    );

    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                    2000,   // validity
                    100,   // min interval
                    1000,   // max interval
                    10000  // alert after interval
                );
    ON_CALL(*mockSubscriptionListener, onReceive(Eq(gpsLocation)))
            .WillByDefault(ReleaseSemaphore(&semaphore));

    std::string subscriptionId = testProxy->subscribeToLocation(mockSubscriptionListener, subscriptionQos);

    ASSERT_TRUE(semaphore.tryAcquire(1, 1000));

    testProxy->unsubscribeFromLocation(subscriptionId);

    ThreadUtil::sleepForMillis(300);

    ASSERT_FALSE(semaphore.tryAcquire(1, 1000));

    delete testProxyBuilder;
    delete testProxy;
}


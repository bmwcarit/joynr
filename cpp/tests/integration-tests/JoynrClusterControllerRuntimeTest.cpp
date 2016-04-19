/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "joynr/LibjoynrSettings.h"

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
    std::string settingsFilenameMqttWithHttpBackend;
    std::string settingsFilenameHttp;
    JoynrClusterControllerRuntime* runtime;
    joynr::types::Localisation::GpsLocation gpsLocation;
    MockMessageReceiver* mockHttpMessageReceiver; // will be deleted when runtime is deleted.
    MockMessageSender* mockHttpMessageSender;
    MockMessageReceiver* mockMqttMessageReceiver;
    MockMessageSender* mockMqttMessageSender;
    QSemaphore semaphore;

    JoynrClusterControllerRuntimeTest() :
        settingsFilenameMqttWithHttpBackend("test-resources/MqttWithHttpBackendJoynrClusterControllerRuntimeTest.settings"),
        settingsFilenameHttp("test-resources/HttpJoynrClusterControllerRuntimeTest.settings"),
            runtime(nullptr),
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
            mockHttpMessageReceiver(new MockMessageReceiver()),
            mockHttpMessageSender(new MockMessageSender()),
            mockMqttMessageReceiver(new MockMessageReceiver()),
            mockMqttMessageSender(new MockMessageSender()),
            semaphore(0)
    {
        std::string httpChannelId("JoynrClusterControllerRuntimeTest.HttpChannelId");
        std::string mqttChannelId("JoynrClusterControllerRuntimeTest.MqttChannelId");


        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        ON_CALL(*mockHttpMessageReceiver, getReceiveChannelId())
                .WillByDefault(::testing::ReturnRefOfCopy(httpChannelId));
        ON_CALL(*mockMqttMessageReceiver, getReceiveChannelId())
                .WillByDefault(::testing::ReturnRefOfCopy(mqttChannelId));


    }

    ~JoynrClusterControllerRuntimeTest(){
        if (runtime) {
            runtime->deleteChannel();
            runtime->stopMessaging();
            delete runtime;
        }
    }

    void createRuntimeMqttWithHttpBackend() {
        EXPECT_CALL(*mockHttpMessageReceiver, getReceiveChannelId())
                .Times(1);
        EXPECT_CALL(*mockMqttMessageReceiver, getReceiveChannelId())
                .Times(1);

        runtime = new JoynrClusterControllerRuntime(
                    nullptr,
                    new Settings(settingsFilenameMqttWithHttpBackend),
                    mockHttpMessageReceiver,
                    mockHttpMessageSender,
                    mockMqttMessageReceiver,
                    mockMqttMessageSender
        );
    }

    void createRuntimeHttp() {
        EXPECT_CALL(*mockHttpMessageReceiver, getReceiveChannelId())
                .Times(1);

        runtime = new JoynrClusterControllerRuntime(
                    nullptr,
                    new Settings(settingsFilenameHttp),
                    mockHttpMessageReceiver,
                    mockHttpMessageSender,
                    mockMqttMessageReceiver,
                    mockMqttMessageSender
        );
    }

    void startMessagingDoesNotThrow();

    void invokeOnSuccessWithGpsLocation(
            std::function<void(const joynr::types::Localisation::GpsLocation location)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError
    ) {
        onSuccess(gpsLocation);
    }

    void TearDown() override{
        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntimeTest);
};

TEST_F(JoynrClusterControllerRuntimeTest, instantiateRuntimeMqttWithHttpBackend)
{
    createRuntimeMqttWithHttpBackend();
    ASSERT_TRUE(runtime != nullptr);
}

TEST_F(JoynrClusterControllerRuntimeTest, instantiateRuntimeHttp)
{
    createRuntimeHttp();
    ASSERT_TRUE(runtime != nullptr);
}

void JoynrClusterControllerRuntimeTest::startMessagingDoesNotThrow() {
    EXPECT_CALL(*mockHttpMessageReceiver, startReceiveQueue())
            .Times(1);
    EXPECT_CALL(*mockHttpMessageReceiver, stopReceiveQueue())
            .Times(1);

    ASSERT_TRUE(runtime != nullptr);
    runtime->startMessaging();
    runtime->stopMessaging();
}

TEST_F(JoynrClusterControllerRuntimeTest, startMessagingHttpDoesNotThrow)
{
    createRuntimeHttp();
    startMessagingDoesNotThrow();
}

TEST_F(JoynrClusterControllerRuntimeTest, startMessagingMqttWithHttpBackendDoesNotThrow)
{
    createRuntimeMqttWithHttpBackend();
    EXPECT_CALL(*mockMqttMessageReceiver, startReceiveQueue())
            .Times(1);
    EXPECT_CALL(*mockMqttMessageReceiver, stopReceiveQueue())
            .Times(1);
    startMessagingDoesNotThrow();
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProvider)
{
    createRuntimeMqttWithHttpBackend();
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    auto mockTestProvider = std::make_shared<MockTestProvider>();

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
    ASSERT_EQ(StatusCodeEnum::SUCCESS, future->getStatus());
    joynr::types::Localisation::GpsLocation actualValue;
    future->get(actualValue);
    EXPECT_EQ(gpsLocation, actualValue);
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProviderWithListArguments)
{
    createRuntimeMqttWithHttpBackend();
    auto mockTestProvider = std::make_shared<MockTestProvider>();
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");

    std::vector<int> ints;
    ints.push_back(4);
    ints.push_back(6);
    ints.push_back(12);
    int sum = 22;

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
    ASSERT_EQ(StatusCodeEnum::SUCCESS, future->getStatus());
    int actualValue;
    future->get(actualValue);
    EXPECT_EQ(sum, actualValue);
    delete testProxy;
    delete testProxyBuilder;
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndSubscribeToLocalProvider) {
    createRuntimeMqttWithHttpBackend();
    std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    auto mockTestProvider = std::make_shared<MockTestProvider>();

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

    auto mockSubscriptionListener = std::make_shared<MockGpsSubscriptionListener>();
    EXPECT_CALL(*mockSubscriptionListener, onReceive(gpsLocation))
            .Times(AtLeast(1));


    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                    480, // validity
                    200, // min interval
                    200, // max interval
                    200  // alert after interval
                );
    std::string subscriptionId = testProxy->subscribeToLocation(mockSubscriptionListener, subscriptionQos);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    testProxy->unsubscribeFromLocation(subscriptionId);
    delete testProxy;
    delete testProxyBuilder;
}


TEST_F(JoynrClusterControllerRuntimeTest, unsubscribeFromLocalProvider) {
    createRuntimeMqttWithHttpBackend();
    std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    auto mockTestProvider = std::make_shared<MockTestProvider>();

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

    auto mockSubscriptionListener = std::make_shared<MockGpsSubscriptionListener>();

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

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    ASSERT_FALSE(semaphore.tryAcquire(1, 1000));

    delete testProxyBuilder;
    delete testProxy;
}


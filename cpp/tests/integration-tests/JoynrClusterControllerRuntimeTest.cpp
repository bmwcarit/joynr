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
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "joynr/BrokerUrl.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Future.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderScope.h"

#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockJoynrClusterControllerMqttConnectionData.h"
#include "tests/mock/MockMosquittoConnection.h"
#include "tests/mock/MockMqttMessagingSkeleton.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockTestProvider.h"
#include "tests/mock/MockTransportMessageReceiver.h"
#include "tests/mock/MockTransportMessageSender.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

using testing::AtLeast;
using testing::ByRef;
using testing::Return;
using testing::SetArgReferee;

class JoynrClusterControllerRuntimeTest : public ::testing::Test
{
public:
    std::string settingsFilenameMqtt;
    std::string settingsFilenameMqttMultipleBackends;
    std::string settingsFilenameMqttMultipleBackendsMisconfigured;
    std::string settingsFilenameMqttMultipleBackendsEmptyDefaultGbid;
    std::string settingsFilenameHttp;
    std::string settingsFilenameMultipleAclRclFiles;
    Settings testSettings;
    ClusterControllerSettings ccSettings;
    std::string settingsFilenameMqttTlsOnNoCertificates;
    std::shared_ptr<JoynrClusterControllerRuntime> runtime;
    joynr::types::Localisation::GpsLocation gpsLocation;
    std::shared_ptr<MockTransportMessageReceiver> mockHttpMessageReceiver;
    std::shared_ptr<MockTransportMessageSender> mockHttpMessageSender;
    std::shared_ptr<MockTransportMessageReceiver> mockMqttMessageReceiver;
    std::shared_ptr<MockTransportMessageSender> mockMqttMessageSender;
    std::shared_ptr<MockMosquittoConnection> mockMosquittoConnection;
    std::vector<std::shared_ptr<JoynrClusterControllerMqttConnectionData>>
            mockMqttMultipleConnections;
    Semaphore semaphore;
    std::string serializedChannelAddress;
    std::string serializedMqttAddress;

    JoynrClusterControllerRuntimeTest()
            : settingsFilenameMqtt("test-resources/MqttJoynrClusterControllerRuntimeTest.settings"),
              settingsFilenameMqttMultipleBackends(
                      "test-resources/"
                      "MqttMB_JoynrClusterControllerRuntimeTest.settings"),
              settingsFilenameMqttMultipleBackendsMisconfigured(
                      "test-resources/"
                      "MqttMB_JoynrClusterControllerRuntimeMisconfiguredTest.settings"),
              settingsFilenameMqttMultipleBackendsEmptyDefaultGbid(
                      "test-resources/"
                      "MqttMB_JoynrClusterControllerRuntimeEmptyGbid.settings"),
              settingsFilenameHttp("test-resources/HttpJoynrClusterControllerRuntimeTest.settings"),
              settingsFilenameMultipleAclRclFiles(
                      "test-resources/AclRclJoynrClusterControllerRuntimeTest.settings"),
              testSettings(settingsFilenameMqtt),
              ccSettings(testSettings),
              runtime(nullptr),
              gpsLocation(1.1,                                     // longitude
                          2.2,                                     // latitude
                          3.3,                                     // altitude
                          types::Localisation::GpsFixEnum::MODE2D, // gps fix
                          0.0,                                     // heading
                          0.0,                                     // quality
                          0.0,                                     // elevation
                          0.0,                                     // bearing
                          444,                                     // gps time
                          444,                                     // device time
                          444                                      // time
                          ),
              mockHttpMessageReceiver(std::make_shared<MockTransportMessageReceiver>()),
              mockHttpMessageSender(std::make_shared<MockTransportMessageSender>()),
              mockMqttMessageReceiver(std::make_shared<MockTransportMessageReceiver>()),
              mockMqttMessageSender(std::make_shared<MockTransportMessageSender>()),
              mockMosquittoConnection(
                      std::make_shared<MockMosquittoConnection>(ccSettings,
                                                                joynr::BrokerUrl("testBrokerUrl"),
                                                                std::chrono::seconds(1),
                                                                std::chrono::seconds(1),
                                                                std::chrono::seconds(1),
                                                                false,
                                                                "testClientId")),
              mockMqttMultipleConnections(),
              semaphore(0),
              globalMqttTopic("mqtt_JoynrClusterControllerRuntimeTest.topic"),
              globalMqttBrokerUrl("mqtt_JoynrClusterControllerRuntimeTest.brokerUrl"),
              globalHttpChannelId("http_JoynrClusterControllerRuntimeTest.ChannelId"),
              globalHttpEndPointUrl("http_JoynrClusterControllerRuntimeTest.endPointUrl"),
              mqttGlobalAddress(globalMqttBrokerUrl, globalMqttTopic),
              httpGlobalAddress(globalHttpEndPointUrl, globalHttpChannelId)
    {
        serializedChannelAddress = joynr::serializer::serializeToJson(httpGlobalAddress);
        serializedMqttAddress = joynr::serializer::serializeToJson(mqttGlobalAddress);
    }

    ~JoynrClusterControllerRuntimeTest()
    {
        if (runtime) {
            runtime->deleteChannel();
            runtime->stopExternalCommunication();
            runtime->shutdown();
            test::util::resetAndWaitUntilDestroyed(runtime);
        }
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockHttpMessageReceiver.get()));
        EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMqttMessageReceiver.get()));
    }

    void createRuntimeMqtt()
    {
        // runtime can only be created, after MockMessageReceiver has been told to return
        // a channelId for getReceiveChannelId.
        EXPECT_CALL(*mockHttpMessageReceiver, getSerializedGlobalClusterControllerAddress())
                .Times(0);

        mockMqttMultipleConnections.push_back(
                std::make_shared<MockJoynrClusterControllerMqttConnectionData>());
        auto connectionData =
                std::dynamic_pointer_cast<MockJoynrClusterControllerMqttConnectionData>(
                        mockMqttMultipleConnections[0]);

        EXPECT_CALL(*connectionData, getMqttMessageReceiver())
                .WillRepeatedly(Return(mockMqttMessageReceiver));

        EXPECT_CALL(*connectionData, getMqttMessageSender())
                .WillRepeatedly(Return(mockMqttMessageSender));

        EXPECT_CALL(*connectionData, getMosquittoConnection())
                .WillRepeatedly(Return(mockMosquittoConnection));

        EXPECT_CALL(*mockMqttMessageReceiver, getSerializedGlobalClusterControllerAddress())
                .WillOnce(::testing::Return(serializedMqttAddress));
        EXPECT_CALL(*mockMqttMessageReceiver, getGlobalClusterControllerAddress())
                .WillOnce(::testing::ReturnRef(mqttGlobalAddress));

        runtime = std::make_shared<JoynrClusterControllerRuntime>(
                std::make_unique<Settings>(settingsFilenameMqtt),
                failOnFatalRuntimeError,
                nullptr,
                nullptr,
                mockHttpMessageReceiver,
                mockHttpMessageSender,
                mockMqttMultipleConnections);
        runtime->init();
    }

    void createRuntimeMqttWithAdditionalGbids()
    {
        EXPECT_CALL(*mockHttpMessageReceiver, getSerializedGlobalClusterControllerAddress())
                .Times(0);

        std::vector<std::shared_ptr<MockJoynrClusterControllerMqttConnectionData>>
                connectionDataVector;
        for (std::uint8_t i = 0; i < 3; i++) {
            mockMqttMultipleConnections.push_back(
                    std::make_shared<MockJoynrClusterControllerMqttConnectionData>());

            auto connectionData =
                    std::dynamic_pointer_cast<MockJoynrClusterControllerMqttConnectionData>(
                            mockMqttMultipleConnections[i]);
            connectionDataVector.push_back(connectionData);

            EXPECT_CALL(*connectionDataVector[i], getMqttMessageReceiver())
                    .WillRepeatedly(Return(mockMqttMessageReceiver));

            EXPECT_CALL(*connectionDataVector[i], getMqttMessageSender()).Times(3);

            EXPECT_CALL(*connectionDataVector[i], getMosquittoConnection())
                    .WillRepeatedly(Return(mockMosquittoConnection));
        }

        EXPECT_CALL(*mockMqttMessageReceiver, getSerializedGlobalClusterControllerAddress())
                .WillOnce(::testing::Return(serializedMqttAddress));
        EXPECT_CALL(*mockMqttMessageReceiver, getGlobalClusterControllerAddress())
                .WillOnce(::testing::ReturnRef(mqttGlobalAddress));

        runtime = std::make_shared<JoynrClusterControllerRuntime>(
                std::make_unique<Settings>(settingsFilenameMqttMultipleBackends),
                failOnFatalRuntimeError,
                nullptr,
                nullptr,
                mockHttpMessageReceiver,
                mockHttpMessageSender,
                mockMqttMultipleConnections);
        runtime->init();
    }

    void createRuntimeHttp()
    {
        mockMqttMultipleConnections.push_back(
                std::make_shared<MockJoynrClusterControllerMqttConnectionData>());
        auto connectionData =
                std::dynamic_pointer_cast<MockJoynrClusterControllerMqttConnectionData>(
                        mockMqttMultipleConnections[0]);

        EXPECT_CALL(*connectionData, getMqttMessageReceiver())
                .WillRepeatedly(Return(mockMqttMessageReceiver));

        // runtime can only be created, after MockMessageReceiver has been told to return
        // a channelId for getReceiveChannelId.
        EXPECT_CALL(*mockHttpMessageReceiver, getSerializedGlobalClusterControllerAddress())
                .WillOnce(::testing::Return(serializedChannelAddress));
        EXPECT_CALL(*mockHttpMessageReceiver, getGlobalClusterControllerAddress())
                .WillOnce(::testing::ReturnRef(httpGlobalAddress));
        EXPECT_CALL(*mockMqttMessageReceiver, getSerializedGlobalClusterControllerAddress())
                .Times(0);

        runtime = std::make_shared<JoynrClusterControllerRuntime>(
                std::make_unique<Settings>(settingsFilenameHttp),
                failOnFatalRuntimeError,
                nullptr,
                nullptr,
                mockHttpMessageReceiver,
                mockHttpMessageSender,
                mockMqttMultipleConnections);
        runtime->init();
    }

    void startExternalCommunicationDoesNotThrow();

    void invokeOnSuccessWithGpsLocation(
            std::function<void(const joynr::types::Localisation::GpsLocation location)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    /*onError*/)
    {
        onSuccess(gpsLocation);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrClusterControllerRuntimeTest);
    const std::string globalMqttTopic;
    const std::string globalMqttBrokerUrl;
    const std::string globalHttpChannelId;
    const std::string globalHttpEndPointUrl;

protected:
    const system::RoutingTypes::MqttAddress mqttGlobalAddress;
    const system::RoutingTypes::ChannelAddress httpGlobalAddress;
};

TEST_F(JoynrClusterControllerRuntimeTest, loadMultipleAclRclFiles)
{
    // runtime can only be created, after MockMessageReceiver has been told to return
    // a channelId for getReceiveChannelId.
    EXPECT_CALL(*mockHttpMessageReceiver, getSerializedGlobalClusterControllerAddress())
            .WillOnce(::testing::Return(serializedChannelAddress));
    EXPECT_CALL(*mockMqttMessageReceiver, getSerializedGlobalClusterControllerAddress()).Times(0);
    EXPECT_CALL(*mockHttpMessageReceiver, getGlobalClusterControllerAddress())
            .WillOnce(::testing::ReturnRef(httpGlobalAddress));

    runtime = std::make_shared<JoynrClusterControllerRuntime>(
            std::make_unique<Settings>(settingsFilenameMultipleAclRclFiles),
                failOnFatalRuntimeError,
            nullptr,
            nullptr,
            mockHttpMessageReceiver,
            mockHttpMessageSender,
            mockMqttMultipleConnections);

    runtime->init();

    ASSERT_TRUE(runtime != nullptr);
}

TEST_F(JoynrClusterControllerRuntimeTest, instantiateRuntimeMqtt)
{
    createRuntimeMqtt();
    ASSERT_TRUE(runtime != nullptr);
}

TEST_F(JoynrClusterControllerRuntimeTest, instantiateRuntimeHttp)
{
    createRuntimeHttp();
    ASSERT_TRUE(runtime != nullptr);
}

TEST_F(JoynrClusterControllerRuntimeTest, mqttTlsOnButNoCertificates)
{
    runtime = std::make_shared<JoynrClusterControllerRuntime>(
            std::make_unique<Settings>(settingsFilenameMqttTlsOnNoCertificates), failOnFatalRuntimeError);

    ASSERT_TRUE(runtime != nullptr);

    runtime->init();

    runtime->startExternalCommunication();
    runtime->stopExternalCommunication();
}
TEST_F(JoynrClusterControllerRuntimeTest, injectCustomMqttMessagingSkeleton)
{
    auto mockMqttMessagingSkeleton = std::make_shared<MockMqttMessagingSkeleton>();

    auto mockMqttMessagingSkeletonFactory = [mockMqttMessagingSkeleton](
                                                    std::weak_ptr<IMessageRouter> messageRouter,
                                                    std::shared_ptr<MqttReceiver> mqttReceiver,
                                                    const std::string& gbid,
                                                    const std::string& multicastTopicPrefix,
                                                    std::uint64_t ttlUplift) {
        std::ignore = messageRouter;
        std::ignore = mqttReceiver;
        std::ignore = multicastTopicPrefix;
        std::ignore = gbid;
        std::ignore = ttlUplift;
        return mockMqttMessagingSkeleton;
    };

    smrf::ByteVector msg;
    auto registerReceivedCallbackHelper =
            [msg](std::function<void(smrf::ByteVector &&)> onMessageReceived) mutable {
                onMessageReceived(std::move(msg));
            };

    mockMqttMultipleConnections.push_back(
            std::make_shared<MockJoynrClusterControllerMqttConnectionData>());
    auto connectionData = std::dynamic_pointer_cast<MockJoynrClusterControllerMqttConnectionData>(
            mockMqttMultipleConnections[0]);

    EXPECT_CALL(*connectionData, getMqttMessageReceiver())
            .WillRepeatedly(Return(mockMqttMessageReceiver));

    EXPECT_CALL(*connectionData, getMqttMessageSender()).Times(3);
    EXPECT_CALL(*connectionData, getMosquittoConnection()).Times(1);

    EXPECT_CALL(*mockMqttMessageReceiver, registerReceiveCallback(_))
            .WillOnce(Invoke(registerReceivedCallbackHelper));
    EXPECT_CALL(*mockMqttMessageReceiver, getSerializedGlobalClusterControllerAddress())
            .WillOnce(::testing::Return(serializedMqttAddress));
    EXPECT_CALL(*mockMqttMessageReceiver, getGlobalClusterControllerAddress())
            .WillOnce(::testing::ReturnRef(mqttGlobalAddress));

    EXPECT_CALL(*mockMqttMessagingSkeleton, onMessageReceivedMock(msg));

    runtime = std::make_shared<JoynrClusterControllerRuntime>(
            std::make_unique<Settings>(settingsFilenameMqtt),
                failOnFatalRuntimeError,
            nullptr,
            mockMqttMessagingSkeletonFactory,
            mockHttpMessageReceiver,
            mockHttpMessageSender,
            mockMqttMultipleConnections);
    runtime->init();
}

void JoynrClusterControllerRuntimeTest::startExternalCommunicationDoesNotThrow()
{
    ASSERT_TRUE(runtime != nullptr);
    runtime->startExternalCommunication();
    runtime->stopExternalCommunication();
}

TEST_F(JoynrClusterControllerRuntimeTest, startExternalCommunicationHttpDoesNotThrow)
{
    EXPECT_CALL(*mockHttpMessageReceiver, startReceiveQueue()).Times(1);
    EXPECT_CALL(*mockHttpMessageReceiver, stopReceiveQueue()).Times(1);

    createRuntimeHttp();
    startExternalCommunicationDoesNotThrow();
}

TEST_F(JoynrClusterControllerRuntimeTest, startExternalCommunicationMqttDoesNotThrow)
{
    EXPECT_CALL(*mockHttpMessageReceiver, startReceiveQueue()).Times(0);
    EXPECT_CALL(*mockHttpMessageReceiver, stopReceiveQueue()).Times(0);

    createRuntimeMqtt();
    startExternalCommunicationDoesNotThrow();
}

TEST_F(JoynrClusterControllerRuntimeTest, startExternalCommunicationWithAdditionalBackends)
{
    EXPECT_CALL(*mockHttpMessageReceiver, startReceiveQueue()).Times(0);
    EXPECT_CALL(*mockHttpMessageReceiver, stopReceiveQueue()).Times(0);

    createRuntimeMqttWithAdditionalGbids();
    startExternalCommunicationDoesNotThrow();
}

TEST_F(JoynrClusterControllerRuntimeTest,
       runtimeConstructionThrowsDueToMissconfiguredMultipleBrokersConfig)
{
    EXPECT_THROW(
            JoynrClusterControllerRuntime(
                    std::make_unique<Settings>(settingsFilenameMqttMultipleBackendsMisconfigured),
                    failOnFatalRuntimeError,
                    nullptr,
                    nullptr,
                    mockHttpMessageReceiver,
                    mockHttpMessageSender,
                    mockMqttMultipleConnections),
            exceptions::JoynrRuntimeException);
}

TEST_F(JoynrClusterControllerRuntimeTest, runtimeAllowsEmptyGbid)
{
    EXPECT_CALL(*mockHttpMessageReceiver, getSerializedGlobalClusterControllerAddress()).Times(0);

    std::vector<std::shared_ptr<MockJoynrClusterControllerMqttConnectionData>> connectionDataVector;
    mockMqttMultipleConnections.push_back(
            std::make_shared<MockJoynrClusterControllerMqttConnectionData>());

    auto connectionData = std::dynamic_pointer_cast<MockJoynrClusterControllerMqttConnectionData>(
            mockMqttMultipleConnections[0]);
    connectionDataVector.push_back(connectionData);

    EXPECT_CALL(*connectionDataVector[0], getMqttMessageReceiver())
            .Times(5)
            .WillRepeatedly(Return(mockMqttMessageReceiver));

    EXPECT_CALL(*connectionDataVector[0], getMqttMessageSender()).Times(3);

    EXPECT_CALL(*connectionDataVector[0], getMosquittoConnection())
            .Times(3)
            .WillRepeatedly(Return(mockMosquittoConnection));

    EXPECT_CALL(*mockMqttMessageReceiver, getSerializedGlobalClusterControllerAddress())
            .WillOnce(::testing::Return(serializedMqttAddress));
    EXPECT_CALL(*mockMqttMessageReceiver, getGlobalClusterControllerAddress())
            .WillOnce(::testing::ReturnRef(mqttGlobalAddress));

    runtime = std::make_shared<JoynrClusterControllerRuntime>(
            std::make_unique<Settings>(settingsFilenameMqttMultipleBackendsEmptyDefaultGbid),
                failOnFatalRuntimeError,
            nullptr,
            nullptr,
            mockHttpMessageReceiver,
            mockHttpMessageSender,
            mockMqttMultipleConnections);
    runtime->init();
    runtime->startExternalCommunication();

    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    auto mockTestProvider = std::make_shared<MockTestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    EXPECT_CALL(
            *mockTestProvider,
            getLocation(
                    A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                    A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>()))
            .WillOnce(Invoke(
                    this, &JoynrClusterControllerRuntimeTest::invokeOnSuccessWithGpsLocation));

    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, mockTestProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeoutMs(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(5000))
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    std::shared_ptr<Future<types::Localisation::GpsLocation>> future(testProxy->getLocationAsync());
    future->wait(500);

    EXPECT_EQ(tests::testProxy::INTERFACE_NAME(), testProxy->INTERFACE_NAME());
    ASSERT_EQ(StatusCodeEnum::SUCCESS, future->getStatus());
    joynr::types::Localisation::GpsLocation actualValue;
    future->get(actualValue);
    EXPECT_EQ(gpsLocation, actualValue);
    runtime->unregisterProvider(participantId);
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProvider)
{
    createRuntimeMqtt();
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    auto mockTestProvider = std::make_shared<MockTestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    EXPECT_CALL(
            *mockTestProvider,
            getLocation(
                    A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                    A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>()))
            .WillOnce(Invoke(
                    this, &JoynrClusterControllerRuntimeTest::invokeOnSuccessWithGpsLocation));

    runtime->startExternalCommunication();
    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, mockTestProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeoutMs(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(5000))
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    std::shared_ptr<Future<types::Localisation::GpsLocation>> future(testProxy->getLocationAsync());
    future->wait(500);

    EXPECT_EQ(tests::testProxy::INTERFACE_NAME(), testProxy->INTERFACE_NAME());
    ASSERT_EQ(StatusCodeEnum::SUCCESS, future->getStatus());
    joynr::types::Localisation::GpsLocation actualValue;
    future->get(actualValue);
    EXPECT_EQ(gpsLocation, actualValue);
    runtime->unregisterProvider(participantId);
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndUseLocalProviderWithListArguments)
{
    createRuntimeMqtt();
    auto mockTestProvider = std::make_shared<MockTestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");

    std::vector<int> ints;
    ints.push_back(4);
    ints.push_back(6);
    ints.push_back(12);
    int sum = 22;

    runtime->startExternalCommunication();
    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, mockTestProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeoutMs(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(5000))
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    std::shared_ptr<Future<int>> future(testProxy->sumIntsAsync(ints));
    future->wait(500);

    EXPECT_EQ(tests::testProxy::INTERFACE_NAME(), testProxy->INTERFACE_NAME());
    ASSERT_EQ(StatusCodeEnum::SUCCESS, future->getStatus());
    int actualValue;
    future->get(actualValue);
    EXPECT_EQ(sum, actualValue);
    runtime->unregisterProvider(participantId);
}

TEST_F(JoynrClusterControllerRuntimeTest, registerAndSubscribeToLocalProvider)
{
    createRuntimeMqtt();
    std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    auto mockTestProvider = std::make_shared<MockTestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    EXPECT_CALL(
            *mockTestProvider,
            getLocation(
                    A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                    A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>()))
            .Times(AtLeast(1))
            .WillRepeatedly(Invoke(
                    this, &JoynrClusterControllerRuntimeTest::invokeOnSuccessWithGpsLocation));

    runtime->startExternalCommunication();
    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, mockTestProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeoutMs(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(5000))
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    auto mockSubscriptionListener = std::make_shared<MockGpsSubscriptionListener>();
    EXPECT_CALL(*mockSubscriptionListener, onReceive(gpsLocation)).Times(AtLeast(1));

    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(480,  // validity
                                                                   1000, // publication ttl
                                                                   200,  // min interval
                                                                   200,  // max interval
                                                                   200   // alert after interval
            );
    auto future = testProxy->subscribeToLocation(mockSubscriptionListener, subscriptionQos);
    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW({ future->get(5000, subscriptionId); });
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    testProxy->unsubscribeFromLocation(subscriptionId);
    runtime->unregisterProvider(participantId);
}

TEST_F(JoynrClusterControllerRuntimeTest, unsubscribeFromLocalProvider)
{
    createRuntimeMqtt();
    std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
    std::string domain("JoynrClusterControllerRuntimeTest.Domain.A");
    auto mockTestProvider = std::make_shared<MockTestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    EXPECT_CALL(
            *mockTestProvider,
            getLocation(
                    A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                    A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>()))
            .WillRepeatedly(Invoke(
                    this, &JoynrClusterControllerRuntimeTest::invokeOnSuccessWithGpsLocation));

    runtime->startExternalCommunication();
    std::string participantId =
            runtime->registerProvider<tests::testProvider>(domain, mockTestProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeoutMs(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);

    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(5000))
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    auto mockSubscriptionListener = std::make_shared<MockGpsSubscriptionListener>();

    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(2000, // validity
                                                                   1000, // publication ttl
                                                                   100,  // min interval
                                                                   1000, // max interval
                                                                   10000 // alert after interval
            );
    ON_CALL(*mockSubscriptionListener, onReceive(Eq(gpsLocation)))
            .WillByDefault(ReleaseSemaphore(&semaphore));

    auto future = testProxy->subscribeToLocation(mockSubscriptionListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW({ future->get(5000, subscriptionId); });
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));

    testProxy->unsubscribeFromLocation(subscriptionId);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(1)));
    runtime->unregisterProvider(participantId);
}

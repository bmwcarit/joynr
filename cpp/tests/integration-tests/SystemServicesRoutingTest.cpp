/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2020 BMW Car IT GmbH
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
#include <memory>
#include <string>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/serializer/Serializer.h"

#include "tests/JoynrTest.h"
#include "tests/utils/PtrUtils.h"
#include "tests/mock/MockTransportMessageSender.h"
#include "tests/mock/MockTransportMessageReceiver.h"

using namespace joynr;
using ::testing::Mock;

class SystemServicesRoutingTest : public ::testing::Test
{
public:
    SystemServicesRoutingTest()
            : settingsFilename("test-resources/SystemServicesRoutingTest.settings"),
              settings(std::make_unique<Settings>(settingsFilename)),
              routingDomain(),
              routingProviderParticipantId(),
              runtime(nullptr),
              mockMessageReceiverMqtt(std::make_shared<MockTransportMessageReceiver>()),
              discoveryQos(),
              routingProxyBuilder(nullptr),
              routingProxy(nullptr),
              globalMqttTopic("mqtt_SystemServicesRoutingTest.topic"),
              globalMqttBrokerUrl("mqtt_SystemServicesRoutingTest.brokerUrl"),
              mqttGlobalAddress()
    {
        SystemServicesSettings systemSettings(*settings);
        systemSettings.printSettings();
        routingDomain = systemSettings.getDomain();
        routingProviderParticipantId = systemSettings.getCcRoutingProviderParticipantId();

        discoveryQos.setCacheMaxAgeMs(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", routingProviderParticipantId);
        discoveryQos.setDiscoveryTimeoutMs(50);

        std::string serializedMqttAddress = joynr::serializer::serializeToJson(mqttGlobalAddress);

        EXPECT_CALL(*(std::dynamic_pointer_cast<MockTransportMessageReceiver>(
                              mockMessageReceiverMqtt).get()),
                    getSerializedGlobalClusterControllerAddress())
                .WillRepeatedly(::testing::Return(serializedMqttAddress));
        EXPECT_CALL(*(std::dynamic_pointer_cast<MockTransportMessageReceiver>(
                              mockMessageReceiverMqtt).get()),
                    getGlobalClusterControllerAddress())
                .WillRepeatedly(::testing::ReturnRef(mqttGlobalAddress));

        // runtime can only be created, after MockMessageReceiver has been told to return
        // a channelId for getReceiveChannelId.
        runtime = std::make_unique<JoynrClusterControllerRuntime>(std::move(settings),
                                                                  failOnFatalRuntimeError,
                                                                  nullptr,
                                                                  nullptr);
        // routing provider is normally registered in JoynrClusterControllerRuntime::create
        runtime->init();
    }

    ~SystemServicesRoutingTest()
    {
        runtime->stopExternalCommunication();
        runtime->shutdown();
        runtime.reset();

        EXPECT_TRUE(Mock::VerifyAndClearExpectations(
                std::dynamic_pointer_cast<MockTransportMessageReceiver>(mockMessageReceiverMqtt)
                        .get()));

        test::util::resetAndWaitUntilDestroyed(runtime);
        test::util::resetAndWaitUntilDestroyed(mockMessageReceiverMqtt);

        std::remove(settingsFilename.c_str());

        // Delete persisted files
        std::remove(ClusterControllerSettings::
                            DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

    void SetUp()
    {
        participantId = util::createUuid();
        isGloballyVisible = true;
        routingProxyBuilder =
                runtime->createProxyBuilder<joynr::system::RoutingProxy>(routingDomain);
    }

protected:
    std::string settingsFilename;
    std::unique_ptr<Settings> settings;
    std::string routingDomain;
    std::string routingProviderParticipantId;
    std::shared_ptr<JoynrClusterControllerRuntime> runtime;
    std::shared_ptr<ITransportMessageReceiver> mockMessageReceiverMqtt;
    DiscoveryQos discoveryQos;
    std::shared_ptr<ProxyBuilder<joynr::system::RoutingProxy>> routingProxyBuilder;
    std::shared_ptr<joynr::system::RoutingProxy> routingProxy;
    std::string participantId;
    bool isGloballyVisible;

private:
    DISALLOW_COPY_AND_ASSIGN(SystemServicesRoutingTest);
    const std::string globalMqttTopic;
    const std::string globalMqttBrokerUrl;
    const system::RoutingTypes::MqttAddress mqttGlobalAddress;
};

TEST_F(SystemServicesRoutingTest, routingProviderIsAvailable)
{
    JOYNR_EXPECT_NO_THROW(routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(5000))
                                                 ->setDiscoveryQos(discoveryQos)
                                                 ->build());
}

TEST_F(SystemServicesRoutingTest, unknowParticipantIsNotResolvable)
{
    routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(5000))
                           ->setDiscoveryQos(discoveryQos)
                           ->build();

    bool isResolvable = false;
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}

TEST_F(SystemServicesRoutingTest, addNextHopMqtt)
{
    routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(5000))
                           ->setDiscoveryQos(discoveryQos)
                           ->build();

    joynr::system::RoutingTypes::MqttAddress address(
            "brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address, isGloballyVisible);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);
}

TEST_F(SystemServicesRoutingTest, removeNextHopMqtt)
{
    routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(5000))
                           ->setDiscoveryQos(discoveryQos)
                           ->build();

    joynr::system::RoutingTypes::MqttAddress address(
            "brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address, isGloballyVisible);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);

    try {
        routingProxy->removeNextHop(participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "removeNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}

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
#include <string>
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/TypeUtil.h"
#include "joynr/Settings.h"
#include "joynr/LibjoynrSettings.h"

#include "joynr/system/RoutingProxy.h"

using namespace joynr;

class SystemServicesRoutingTest : public ::testing::Test {
public:
    SystemServicesRoutingTest() :
            settingsFilename("test-resources/SystemServicesRoutingTest.settings"),
            settings(new Settings(settingsFilename)),
            routingDomain(),
            routingProviderParticipantId(),
            runtime(nullptr),
            mockMessageReceiverHttp(new MockMessageReceiver()),
            mockMessageReceiverMqtt(new MockMessageReceiver()),
            mockMessageSender(new MockMessageSender()),
            discoveryQos(),
            routingProxyBuilder(nullptr),
            routingProxy(nullptr)
    {
        SystemServicesSettings systemSettings(*settings);
        systemSettings.printSettings();
        routingDomain = systemSettings.getDomain();
        routingProviderParticipantId = systemSettings.getCcRoutingProviderParticipantId();

        discoveryQos.setCacheMaxAge(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", routingProviderParticipantId);
        discoveryQos.setDiscoveryTimeout(50);

        std::string channelIdHttp("SystemServicesRoutingTest.ChannelId");
        std::string channelIdMqtt("mqtt_SystemServicesRoutingTest.ChannelId");
        EXPECT_CALL(*(dynamic_cast<MockMessageReceiver*>(mockMessageReceiverHttp)), getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelIdHttp));
        EXPECT_CALL(*(dynamic_cast<MockMessageReceiver*>(mockMessageReceiverMqtt)), getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelIdMqtt));

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        runtime = new JoynrClusterControllerRuntime(
                nullptr,
                settings,
                mockMessageReceiverHttp,
                mockMessageSender,
                mockMessageReceiverMqtt,
                mockMessageSender);
        // routing provider is normally registered in JoynrClusterControllerRuntime::create
        runtime->registerRoutingProvider();
    }

    ~SystemServicesRoutingTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
        std::remove(settingsFilename.c_str());
    }

    void SetUp(){
        participantId = util::createUuid();
        routingProxyBuilder = runtime
                ->createProxyBuilder<joynr::system::RoutingProxy>(routingDomain);
    }

    void TearDown(){
        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
        delete routingProxy;
        delete routingProxyBuilder;
    }
    
protected:
    std::string settingsFilename;
    Settings* settings;
    std::string routingDomain;
    std::string routingProviderParticipantId;
    JoynrClusterControllerRuntime* runtime;
    IMessageReceiver* mockMessageReceiverHttp;
    IMessageReceiver* mockMessageReceiverMqtt;
    MockMessageSender* mockMessageSender;
    DiscoveryQos discoveryQos;
    ProxyBuilder<joynr::system::RoutingProxy>* routingProxyBuilder;
    joynr::system::RoutingProxy* routingProxy;
    std::string participantId;
    
private:
    DISALLOW_COPY_AND_ASSIGN(SystemServicesRoutingTest);
};


TEST_F(SystemServicesRoutingTest, routingProviderIsAvailable)
{
    EXPECT_NO_THROW(
        routingProxy = routingProxyBuilder
                ->setMessagingQos(MessagingQos(5000))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();
    );
}

TEST_F(SystemServicesRoutingTest, unknowParticipantIsNotResolvable)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    bool isResolvable = false;
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}


TEST_F(SystemServicesRoutingTest, addNextHopHttp)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);
}

TEST_F(SystemServicesRoutingTest, removeNextHopHttp)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);

    try {
        routingProxy->removeNextHop(participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "removeNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}


TEST_F(SystemServicesRoutingTest, addNextHopMqtt)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::MqttAddress address("brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);
}

TEST_F(SystemServicesRoutingTest, removeNextHopMqtt)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::MqttAddress address("brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);

    try {
        routingProxy->removeNextHop(participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "removeNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}

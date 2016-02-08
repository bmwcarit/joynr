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
    std::string settingsFilename;
    Settings* settings;
    std::string routingDomain;
    std::string routingProviderParticipantId;
    JoynrClusterControllerRuntime* runtime;
    IMessageReceiver* mockMessageReceiver;
    MockMessageSender* mockMessageSender;
    DiscoveryQos discoveryQos;
    ProxyBuilder<joynr::system::RoutingProxy>* routingProxyBuilder;
    joynr::system::RoutingProxy* routingProxy;

    SystemServicesRoutingTest() :
            settingsFilename("test-resources/SystemServicesRoutingTest.settings"),
            settings(new Settings(settingsFilename)),
            routingDomain(),
            routingProviderParticipantId(),
            runtime(nullptr),
            mockMessageReceiver(new MockMessageReceiver()),
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

        std::string channelId("SystemServicesRoutingTest.ChannelId");
        EXPECT_CALL(*(dynamic_cast<MockMessageReceiver*>(mockMessageReceiver)), getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        runtime = new JoynrClusterControllerRuntime(nullptr, settings, mockMessageReceiver, mockMessageSender);
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
        routingProxyBuilder = runtime
                ->createProxyBuilder<joynr::system::RoutingProxy>(routingDomain);
    }

    void TearDown(){
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
        delete routingProxy;
        delete routingProxyBuilder;
    }

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

    std::string participantId("SystemServicesRoutingTest.ParticipantId.A");
    bool isResolvable = false;
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
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

    std::string participantId("SystemServicesRoutingTest.ParticipantId.A");
    joynr::system::RoutingTypes::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
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

    std::string participantId("SystemServicesRoutingTest.ParticipantId.A");
    joynr::system::RoutingTypes::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);

    try {
        routingProxy->removeNextHop(participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "removeNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
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

    std::string participantId("SystemServicesRoutingTest.ParticipantId.A");
    joynr::system::RoutingTypes::MqttAddress address("brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
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

    std::string participantId("SystemServicesRoutingTest.ParticipantId.A");
    joynr::system::RoutingTypes::MqttAddress address("brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);

    try {
        routingProxy->removeNextHop(participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "removeNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}

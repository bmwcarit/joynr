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
#include "joynr/LibjoynrSettings.h"

#include "joynr/system/DiscoveryProxy.h"
#include "joynr/Settings.h"
#include "joynr/types/Version.h"
#include "joynr/JsonSerializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"

using namespace joynr;

class SystemServicesDiscoveryTest : public ::testing::Test {
public:
    std::string settingsFilename;
    Settings* settings;
    std::string discoveryDomain;
    std::string discoveryProviderParticipantId;
    JoynrClusterControllerRuntime* runtime;
    IMessageReceiver* mockMessageReceiverHttp;
    IMessageReceiver* mockMessageReceiverMqtt;
    DiscoveryQos discoveryQos;
    ProxyBuilder<joynr::system::DiscoveryProxy>* discoveryProxyBuilder;
    joynr::system::DiscoveryProxy* discoveryProxy;
    std::int64_t lastSeenDateMs;
    std::int64_t expiryDateMs;

    SystemServicesDiscoveryTest() :
        settingsFilename("test-resources/SystemServicesDiscoveryTest.settings"),
        settings(new Settings(settingsFilename)),
        discoveryDomain(),
        discoveryProviderParticipantId(),
        runtime(nullptr),
        mockMessageReceiverHttp(new MockMessageReceiver()),
        mockMessageReceiverMqtt(new MockMessageReceiver()),
        discoveryQos(),
        discoveryProxyBuilder(nullptr),
        discoveryProxy(nullptr),
        lastSeenDateMs(-1),
        expiryDateMs(-1)
    {
        SystemServicesSettings systemSettings(*settings);
        systemSettings.printSettings();
        discoveryDomain = systemSettings.getDomain();
        discoveryProviderParticipantId = systemSettings.getCcDiscoveryProviderParticipantId();

        discoveryQos.setCacheMaxAge(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", discoveryProviderParticipantId);
        discoveryQos.setDiscoveryTimeout(50);

        std::string httpChannelId("http_SystemServicesDiscoveryTest.ChannelId");
        std::string httpEndPointUrl("http_SystemServicesRoutingTest.endPointUrl");
        std::string mqttTopic("mqtt_SystemServicesRoutingTest.topic");
        std::string mqttBrokerUrl("mqtt_SystemServicesRoutingTest.brokerUrl");

        using system::RoutingTypes::ChannelAddress;
        using system::RoutingTypes::MqttAddress;

        std::string serializedChannelAddress = JsonSerializer::serialize(ChannelAddress(httpEndPointUrl, httpChannelId));
        std::string serializedMqttAddress = JsonSerializer::serialize(MqttAddress(mqttBrokerUrl, mqttTopic));

        EXPECT_CALL(*(dynamic_cast<MockMessageReceiver*>(mockMessageReceiverHttp)), getGlobalClusterControllerAddress())
                .WillRepeatedly(::testing::ReturnRefOfCopy(serializedChannelAddress));
        EXPECT_CALL(*(dynamic_cast<MockMessageReceiver*>(mockMessageReceiverMqtt)), getGlobalClusterControllerAddress())
                .WillRepeatedly(::testing::ReturnRefOfCopy(serializedMqttAddress));

        //runtime can only be created, after MockCommunicationManager has been told to return
        //a channelId for getReceiveChannelId.
        runtime = new JoynrClusterControllerRuntime(
                nullptr,
                settings,
                mockMessageReceiverHttp,
                nullptr,
                mockMessageReceiverMqtt);
        // discovery provider is normally registered in JoynrClusterControllerRuntime::create
        runtime->registerDiscoveryProvider();
    }

    ~SystemServicesDiscoveryTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
        std::remove(settingsFilename.c_str());
    }

    void SetUp(){
        discoveryProxyBuilder = runtime
                ->createProxyBuilder<joynr::system::DiscoveryProxy>(discoveryDomain);
    }

    void TearDown(){
        std::remove(
                    LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
        std::remove(
                    LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
        delete discoveryProxy;
        delete discoveryProxyBuilder;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SystemServicesDiscoveryTest);
};


TEST_F(SystemServicesDiscoveryTest, discoveryProviderIsAvailable)
{
    EXPECT_NO_THROW(
        discoveryProxy = discoveryProxyBuilder
                ->setMessagingQos(MessagingQos(5000))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();
    );
}

TEST_F(SystemServicesDiscoveryTest, lookupUnknowParticipantReturnsEmptyResult)
{
    discoveryProxy = discoveryProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    std::vector<joynr::types::DiscoveryEntry> result;
    std::string domain("SystemServicesDiscoveryTest.Domain.A");
    std::string interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    joynr::types::DiscoveryQos discoveryQos(
                5000,                                      // max cache age
                joynr::types::DiscoveryScope::LOCAL_ONLY, // discovery scope
                false                                      // provider must support on change subscriptions
    );

    try {
        discoveryProxy->lookup(result, domain, interfaceName, discoveryQos);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "lookup was not successful";
    }
    EXPECT_TRUE(result.empty());
}


TEST_F(SystemServicesDiscoveryTest, add)
{
    discoveryProxy = discoveryProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    std::vector<joynr::types::DiscoveryEntry> result;
    std::string domain("SystemServicesDiscoveryTest.Domain.A");
    std::string interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    std::string participantId("SystemServicesDiscoveryTest.ParticipantID.A");
    joynr::types::DiscoveryQos discoveryQos(
                5000,                                      // max cache age
                joynr::types::DiscoveryScope::LOCAL_ONLY, // discovery scope
                false                                      // provider must support on change subscriptions
    );
    joynr::types::ProviderQos providerQos(
                std::vector<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // priority
                joynr::types::ProviderScope::LOCAL,     // scope for provider registration
                false                                   // provider supports on change subscriptions
    );
    joynr::types::Version providerVersion(47, 11);
    std::vector<joynr::types::DiscoveryEntry> expectedResult;
    joynr::types::DiscoveryEntry discoveryEntry(
                providerVersion,
                domain,
                interfaceName,
                participantId,
                providerQos,
                lastSeenDateMs,
                expiryDateMs
    );
    expectedResult.push_back(discoveryEntry);

    try {
        discoveryProxy->lookup(result, domain, interfaceName, discoveryQos);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "lookup was not successful";
    }
    EXPECT_TRUE(result.empty());

    try {
        discoveryProxy->add(discoveryEntry);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "add was not successful";
    }

    try {
        discoveryProxy->lookup(result, domain, interfaceName, discoveryQos);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "lookup was not successful";
    }
    EXPECT_EQ(expectedResult, result);
}

TEST_F(SystemServicesDiscoveryTest, remove)
{
    discoveryProxy = discoveryProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    std::string domain("SystemServicesDiscoveryTest.Domain.A");
    std::string interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    std::string participantId("SystemServicesDiscoveryTest.ParticipantID.A");
    joynr::types::DiscoveryQos discoveryQos(
                5000,                                      // max cache age
                joynr::types::DiscoveryScope::LOCAL_ONLY, // discovery scope
                false                                      // provider must support on change subscriptions
    );
    joynr::types::ProviderQos providerQos(
                std::vector<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // priority
                joynr::types::ProviderScope::LOCAL,     // scope for provider registration
                false                                   // provider supports on change subscriptions
    );
    joynr::types::Version providerVersion(47, 11);
    std::vector<joynr::types::DiscoveryEntry> expectedResult;
    joynr::types::DiscoveryEntry discoveryEntry(
                providerVersion,
                domain,
                interfaceName,
                participantId,
                providerQos,
                lastSeenDateMs,
                expiryDateMs
    );
    expectedResult.push_back(discoveryEntry);

    try {
        discoveryProxy->add(discoveryEntry);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "add was not successful";
    }

    std::vector<joynr::types::DiscoveryEntry> result;
    try {
        discoveryProxy->lookup(result, domain, interfaceName, discoveryQos);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "lookup was not successful";
    }
    EXPECT_EQ(expectedResult, result);

    try {
        discoveryProxy->remove(participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "remove was not successful";
    }

    result.clear();
    try {
        discoveryProxy->lookup(result, domain, interfaceName, discoveryQos);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "lookup was not successful";
    }
    EXPECT_TRUE(result.empty());
}

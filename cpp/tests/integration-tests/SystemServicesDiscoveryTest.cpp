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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/DiscoveryQos.h"
#include "joynr/LCDUtil.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/Settings.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/Version.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockTransportMessageReceiver.h"
#include "tests/mock/MockTransportMessageSender.h"
#include "tests/mock/TestJoynrClusterControllerRuntime.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

class SystemServicesDiscoveryTest : public ::testing::Test
{
public:
    std::string settingsFilename;
    std::unique_ptr<Settings> settings;
    std::string discoveryDomain;
    std::string discoveryProviderParticipantId;
    std::shared_ptr<TestJoynrClusterControllerRuntime> runtime;
    std::shared_ptr<ITransportMessageReceiver> mockMessageReceiverMqtt;
    std::shared_ptr<ITransportMessageSender> mockMessageSenderMqtt;
    DiscoveryQos discoveryQos;
    std::shared_ptr<ProxyBuilder<joynr::system::DiscoveryProxy>> discoveryProxyBuilder;
    std::shared_ptr<joynr::system::DiscoveryProxy> discoveryProxy;
    std::int64_t lastSeenDateMs;
    std::int64_t expiryDateMs;
    std::string publicKeyId;

    SystemServicesDiscoveryTest()
            : settingsFilename("test-resources/SystemServicesDiscoveryTest.settings"),
              settings(std::make_unique<Settings>(settingsFilename)),
              discoveryDomain(),
              discoveryProviderParticipantId(),
              runtime(),
              mockMessageReceiverMqtt(std::make_shared<MockTransportMessageReceiver>()),
              mockMessageSenderMqtt(std::make_shared<MockTransportMessageSender>()),
              discoveryQos(),
              discoveryProxyBuilder(nullptr),
              discoveryProxy(nullptr),
              lastSeenDateMs(TimePoint::now().toMilliseconds()),
              expiryDateMs(-1),
              publicKeyId(""),
              globalMqttTopic("mqtt_SystemServicesDiscoveryTest.topic"),
              globalMqttBrokerUrl("mqtt_SystemServicesDiscoveryTest.brokerUrl"),
              mqttGlobalAddress(globalMqttBrokerUrl, globalMqttTopic)
    {
        SystemServicesSettings systemSettings(*settings);
        systemSettings.printSettings();
        discoveryDomain = systemSettings.getDomain();
        discoveryProviderParticipantId = systemSettings.getCcDiscoveryProviderParticipantId();

        discoveryQos.setCacheMaxAgeMs(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", discoveryProviderParticipantId);
        discoveryQos.setDiscoveryTimeoutMs(1000);

        std::string serializedMqttAddress = joynr::serializer::serializeToJson(mqttGlobalAddress);

        EXPECT_CALL(
                *(std::dynamic_pointer_cast<MockTransportMessageReceiver>(mockMessageReceiverMqtt)),
                getSerializedGlobalClusterControllerAddress())
                .WillRepeatedly(Return(serializedMqttAddress));
        EXPECT_CALL(
                *(std::dynamic_pointer_cast<MockTransportMessageReceiver>(mockMessageReceiverMqtt)),
                getGlobalClusterControllerAddress())
                .WillRepeatedly(ReturnRef(mqttGlobalAddress));

        // runtime can only be created, after MockCommunicationManager has been told to return
        // a channelId for getReceiveChannelId.
        runtime = std::make_shared<TestJoynrClusterControllerRuntime>(
                std::move(settings), failOnFatalRuntimeError, nullptr, nullptr);
        // discovery provider is normally registered in JoynrClusterControllerRuntime::create
        runtime->init();

        discoveryProxyBuilder =
                runtime->createProxyBuilder<joynr::system::DiscoveryProxy>(discoveryDomain);
    }

    ~SystemServicesDiscoveryTest()
    {
        discoveryProxy.reset();
        discoveryProxyBuilder.reset();
        runtime->stopExternalCommunication();

        runtime->shutdown();
        test::util::resetAndWaitUntilDestroyed(runtime);
        test::util::resetAndWaitUntilDestroyed(mockMessageReceiverMqtt);
        test::util::resetAndWaitUntilDestroyed(mockMessageSenderMqtt);
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

protected:
    void genericAddLookupRemoveDiscoveryEntryTest(
            std::vector<joynr::types::DiscoveryEntryWithMetaInfo> result,
            std::string domain,
            std::string interfaceName,
            joynr::types::DiscoveryQos discoveryQosLocal)
    {
        discoveryProxy = discoveryProxyBuilder->setMessagingQos(MessagingQos(5000))
                                 ->setDiscoveryQos(discoveryQos)
                                 ->build();

        std::string participantId("SystemServicesDiscoveryTest.ParticipantID.A");
        joynr::types::ProviderQos providerQos(
                std::vector<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                            // priority
                joynr::types::ProviderScope::LOCAL,           // scope for provider registration
                false // provider supports on change subscriptions
        );
        joynr::types::Version providerVersion(47, 11);
        joynr::types::DiscoveryEntry discoveryEntry(providerVersion,
                                                    domain,
                                                    interfaceName,
                                                    participantId,
                                                    providerQos,
                                                    lastSeenDateMs,
                                                    expiryDateMs,
                                                    publicKeyId);
        std::vector<joynr::types::DiscoveryEntryWithMetaInfo> expectedResult;
        auto expectedEntry = LCDUtil::convert(true, discoveryEntry);
        expectedResult.push_back(expectedEntry);

        try {
            discoveryProxy->lookup(result, {domain}, interfaceName, discoveryQosLocal);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "lookup was not successful";
        }
        EXPECT_TRUE(result.empty());

        try {
            discoveryProxy->add(discoveryEntry);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "add was not successful";
        }

        try {
            discoveryProxy->lookup(result, {domain}, interfaceName, discoveryQosLocal);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "lookup was not successful";
        }
        EXPECT_EQ(expectedResult.size(), result.size());
        EXPECT_EQ(expectedResult[0].getProviderVersion(), result[0].getProviderVersion());
        EXPECT_EQ(expectedResult[0].getDomain(), result[0].getDomain());
        EXPECT_EQ(expectedResult[0].getInterfaceName(), result[0].getInterfaceName());
        EXPECT_EQ(expectedResult[0].getParticipantId(), result[0].getParticipantId());
        EXPECT_EQ(expectedResult[0].getQos(), result[0].getQos());
        EXPECT_LE(expectedResult[0].getLastSeenDateMs(), result[0].getLastSeenDateMs());
        EXPECT_LE(result[0].getLastSeenDateMs(), (expectedResult[0].getLastSeenDateMs() + 5000));
        EXPECT_EQ(expectedResult[0].getExpiryDateMs(), result[0].getExpiryDateMs());
        EXPECT_EQ(expectedResult[0].getPublicKeyId(), result[0].getPublicKeyId());

        // cleanup after test
        try {
            discoveryProxy->remove(participantId);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "remove was not successful";
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SystemServicesDiscoveryTest);
    const std::string globalMqttTopic;
    const std::string globalMqttBrokerUrl;
    const system::RoutingTypes::MqttAddress mqttGlobalAddress;
};

TEST_F(SystemServicesDiscoveryTest, discoveryProviderIsAvailable)
{
    JOYNR_EXPECT_NO_THROW(discoveryProxy =
                                  discoveryProxyBuilder->setMessagingQos(MessagingQos(5000))
                                          ->setDiscoveryQos(discoveryQos)
                                          ->build(););
}

TEST_F(SystemServicesDiscoveryTest, lookupUnknownParticipantReturnsEmptyResult)
{
    discoveryProxy = discoveryProxyBuilder->setMessagingQos(MessagingQos(5000))
                             ->setDiscoveryQos(discoveryQos)
                             ->build();

    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> result;
    std::string domain("SystemServicesDiscoveryTest.Domain.A");
    std::string interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    joynr::types::DiscoveryQos discoveryQos(
            5000,                                     // max cache age
            5000,                                     // discovery ttl
            joynr::types::DiscoveryScope::LOCAL_ONLY, // discovery scope
            false // provider must support on change subscriptions
    );

    try {
        discoveryProxy->lookup(result, {domain}, interfaceName, discoveryQos);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "lookup was not successful";
    }
    EXPECT_TRUE(result.empty());
}

TEST_F(SystemServicesDiscoveryTest, add)
{
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> result;
    std::string domain("SystemServicesDiscoveryTest.Domain.A");
    std::string interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    joynr::types::DiscoveryQos discoveryQosLocal(
            5000,                                     // max cache age
            5000,                                     // discovery ttl
            joynr::types::DiscoveryScope::LOCAL_ONLY, // discovery scope
            false // provider must support on change subscriptions
    );
    genericAddLookupRemoveDiscoveryEntryTest(result, domain, interfaceName, discoveryQosLocal);
}

TEST_F(SystemServicesDiscoveryTest, remove)
{
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> result;
    std::string domain("SystemServicesDiscoveryTest.Domain.A");
    std::string interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    joynr::types::DiscoveryQos discoveryQosLocal(
            5000,                                     // max cache age
            5000,                                     // discovery ttl
            joynr::types::DiscoveryScope::LOCAL_ONLY, // discovery scope
            false // provider must support on change subscriptions
    );
    genericAddLookupRemoveDiscoveryEntryTest(result, domain, interfaceName, discoveryQosLocal);

    result.clear();
    try {
        discoveryProxy->lookup(result, {domain}, interfaceName, discoveryQosLocal);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE() << "lookup was not successful";
    }
    EXPECT_TRUE(result.empty());
}

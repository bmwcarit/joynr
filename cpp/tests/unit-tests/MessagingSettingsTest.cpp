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
#include <cstdio>
#include <limits>

#include "tests/utils/Gtest.h"

#include "joynr/BrokerUrl.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Settings.h"
#include "tests/PrettyPrint.h"

using namespace joynr;

class MessagingSettingsTest : public testing::Test
{
public:
    MessagingSettingsTest()
            : testSettingsFileNameNonExistent(
                      "test-resources/MessagingSettingsTest-nonexistent.settings"),
              testSettingsFileNameMqtt("test-resources/MqttMessagingSettingsTest.settings"),
              testSettingsFileNameMqttWithGbid(
                      "test-resources/MqttMessagingSettingsWithGbidTest.settings")
    {
    }

protected:
    ADD_LOGGER(MessagingSettingsTest)
    const std::string testSettingsFileNameNonExistent;
    const std::string testSettingsFileNameMqtt;
    const std::string testSettingsFileNameMqttWithGbid;
};

TEST_F(MessagingSettingsTest, intializedWithDefaultSettings)
{
    Settings testSettings(testSettingsFileNameNonExistent);

    // file is not loaded because it intentionally does not exist
    // defaults will be loaded from another file instead
    EXPECT_FALSE(testSettings.isLoaded());

    MessagingSettings messagingSettings(testSettings);

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BROKER_URL()));

    EXPECT_TRUE(
            messagingSettings.contains(MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));

    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));

    EXPECT_TRUE(
            messagingSettings.contains(MessagingSettings::SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS()));
    EXPECT_EQ(messagingSettings.getMqttKeepAliveTimeSeconds().count(),
              MessagingSettings::DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS().count());
    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS()));
    EXPECT_EQ(messagingSettings.getMqttReconnectDelayTimeSeconds().count(),
              MessagingSettings::DEFAULT_MQTT_RECONNECT_DELAY_TIME_SECONDS().count());
    EXPECT_TRUE(
            messagingSettings.contains(MessagingSettings::SETTING_MQTT_CONNECTION_TIMEOUT_MS()));
    EXPECT_EQ(messagingSettings.getMqttConnectionTimeoutMs().count(),
              MessagingSettings::DEFAULT_MQTT_CONNECTION_TIMEOUT_MS().count());
    EXPECT_EQ(messagingSettings.getTtlUpliftMs(), MessagingSettings::DEFAULT_TTL_UPLIFT_MS());
    EXPECT_TRUE(
            messagingSettings.contains(MessagingSettings::SETTING_ROUTING_TABLE_GRACE_PERIOD_MS()));
    EXPECT_EQ(messagingSettings.getRoutingTableGracePeriodMs(),
              MessagingSettings::DEFAULT_ROUTING_TABLE_GRACE_PERIOD_MS());
    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS()));
    EXPECT_EQ(messagingSettings.getRoutingTableCleanupIntervalMs(),
              MessagingSettings::DEFAULT_ROUTING_TABLE_CLEANUP_INTERVAL_MS());
    EXPECT_EQ(messagingSettings.getDiscardUnroutableRepliesAndPublications(),
              MessagingSettings::DEFAULT_DISCARD_UNROUTABLE_REPLIES_AND_PUBLICATIONS());
}

TEST_F(MessagingSettingsTest, overrideDefaultSettings)
{
    std::string expectedBrokerUrl("mqtt://custom-broker-host:1883/");
    std::int64_t expectedRoutingTableGracePeriodMs = 5000;
    std::int64_t expectedRoutingTableCleanupIntervalMs = 6000;
    Settings testSettings(testSettingsFileNameNonExistent);

    testSettings.set(MessagingSettings::SETTING_BROKER_URL(), expectedBrokerUrl);
    testSettings.set(MessagingSettings::SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(),
                     expectedRoutingTableGracePeriodMs);
    testSettings.set(MessagingSettings::SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(),
                     expectedRoutingTableCleanupIntervalMs);
    MessagingSettings messagingSettings(testSettings);

    std::string brokerUrl = messagingSettings.getBrokerUrlString();
    EXPECT_EQ(expectedBrokerUrl, brokerUrl);
    std::int64_t routingTableGracePeriodMs = messagingSettings.getRoutingTableGracePeriodMs();
    EXPECT_EQ(expectedRoutingTableGracePeriodMs, routingTableGracePeriodMs);
    std::int64_t routingTableCleanupIntervalMs =
            messagingSettings.getRoutingTableCleanupIntervalMs();
    EXPECT_EQ(expectedRoutingTableCleanupIntervalMs, routingTableCleanupIntervalMs);
}

void checkBrokerSettings(MessagingSettings messagingSettings, std::string expectedBrokerUrl)
{
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BROKER_URL()));

    std::string brokerUrl = messagingSettings.getBrokerUrlString();
    EXPECT_EQ(expectedBrokerUrl, brokerUrl);
}

void checkGbidSettings(MessagingSettings messagingSettings, std::string expectedGbid)
{
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_GBID()));

    std::string gbid = messagingSettings.getGbid();
    EXPECT_EQ(expectedGbid, gbid);
}

void checkAdditionalBackendUrlSettings(MessagingSettings messagingSettings,
                                       std::string expectedBrokerUrl,
                                       std::uint8_t index)
{
    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_BROKER_URL(index)));

    std::string brokerUrl = messagingSettings.getAdditionalBackendBrokerUrlString(index);
    EXPECT_EQ(expectedBrokerUrl, brokerUrl);
}

void checkAdditionalBackendGbidSettings(MessagingSettings messagingSettings,
                                        std::string expectedGbid,
                                        std::uint8_t index)
{
    EXPECT_TRUE(
            messagingSettings.contains(MessagingSettings::SETTING_ADDITIONAL_BACKEND_GBID(index)));

    std::string gbid = messagingSettings.getAdditionalBackendGbid(index);

    EXPECT_EQ(expectedGbid, gbid);
}
void checkAdditionalBackendMqttConnectionTimeout(
        MessagingSettings messagingSettings,
        std::chrono::milliseconds expectedBrokerMqttConnectionTimeout,
        std::uint8_t index)
{
    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(index)));

    std::chrono::milliseconds brokerConnectionTimeoutMs =
            messagingSettings.getAdditionalBackendMqttConnectionTimeoutMs(index);

    EXPECT_EQ(expectedBrokerMqttConnectionTimeout.count(), brokerConnectionTimeoutMs.count());
}
void checkAdditionalBackendMqttKeepAliveTimeSeconds(
        MessagingSettings messagingSettings,
        std::chrono::seconds expectedBrokerMqttKeepAliveTimeSeconds,
        std::uint8_t index)
{
    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(index)));

    std::chrono::seconds brokerKeepAliveTimeSeconds =
            messagingSettings.getAdditionalBackendMqttKeepAliveTimeSeconds(index);

    EXPECT_EQ(expectedBrokerMqttKeepAliveTimeSeconds.count(), brokerKeepAliveTimeSeconds.count());
}
void checkDiscoveryDirectorySettings(MessagingSettings messagingSettings,
                                     std::string expectedCapabilitiesDirectoryChannelId)
{
    EXPECT_TRUE(messagingSettings.contains(
            MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));

    std::string capabilitiesDirectoryChannelId =
            messagingSettings.getCapabilitiesDirectoryChannelId();
    EXPECT_EQ(expectedCapabilitiesDirectoryChannelId, capabilitiesDirectoryChannelId);
}

TEST_F(MessagingSettingsTest, mqttOnly)
{
    std::string expectedBrokerUrl("mqtt://custom-broker-host:1883/");
    std::string expectedCapabilitiesDirectoryChannelId("mqtt_discoverydirectory_channelid");

    Settings testSettings(testSettingsFileNameMqtt);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    checkBrokerSettings(messagingSettings, expectedBrokerUrl);

    checkDiscoveryDirectorySettings(messagingSettings, expectedCapabilitiesDirectoryChannelId);
}

TEST_F(MessagingSettingsTest, discoveryEntryExpiryIntervalMsStores64BitValue)
{
    const std::string fileName =
            "test-resources/MessagingSettingsDiscoveryEntryExpiryIntervalMs.settings";
    Settings testSettings(fileName);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    constexpr std::int64_t int64Max = std::numeric_limits<std::int64_t>::max();
    EXPECT_EQ(int64Max, messagingSettings.getDiscoveryEntryExpiryIntervalMs());
}

TEST_F(MessagingSettingsTest, discardUnroutableRepliesAndPublicationsValue)
{
    const std::string fileName =
            "test-resources/MessagingSettingsDiscardUnroutableRepliesAndPublications.settings";
    Settings testSettings(fileName);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    EXPECT_EQ(true, messagingSettings.getDiscardUnroutableRepliesAndPublications());

    bool expectedValue = false;
    messagingSettings.setDiscardUnroutableRepliesAndPublications(expectedValue);
    EXPECT_EQ(expectedValue, messagingSettings.getDiscardUnroutableRepliesAndPublications());

    expectedValue = true;
    messagingSettings.setDiscardUnroutableRepliesAndPublications(expectedValue);
    EXPECT_EQ(expectedValue, messagingSettings.getDiscardUnroutableRepliesAndPublications());
}

TEST_F(MessagingSettingsTest, mqttWithGbid)
{
    std::string expectedBrokerUrl("mqtt://custom-broker-host:1883/");
    std::string expectedGbid("defaultGbid");

    Settings testSettings(testSettingsFileNameMqttWithGbid);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    checkBrokerSettings(messagingSettings, expectedBrokerUrl);
    checkGbidSettings(messagingSettings, expectedGbid);
}

TEST_F(MessagingSettingsTest, mqttWithAdditionalBackends)
{
    std::string expectedBrokerUrl("mqtt://custom-broker-host:1883/");
    std::string expectedGbid("defaultGbid");

    std::string expectedAdditionalBackend0Url("mqtt://additional-backend-host-0:1883/");
    std::string expectedAdditionalBackend0Gbid("additional-gbid-0");
    std::chrono::seconds expectedAdditionalBackend0KeepAliveSeconds(10);
    std::chrono::milliseconds expectedAdditionalBackend0ConnectionTimeoutMs(20);

    std::string expectedAdditionalBackend1Url("mqtt://additional-backend-host-1:1883/");
    std::string expectedAdditionalBackend1Gbid("additional-gbid-1");
    std::chrono::seconds expectedAdditionalBackend1KeepAliveSeconds(30);
    std::chrono::milliseconds expectedAdditionalBackend1ConnectionTimeoutMs(40);

    Settings testSettings(testSettingsFileNameMqttWithGbid);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    checkAdditionalBackendUrlSettings(messagingSettings, expectedAdditionalBackend0Url, 0);
    checkAdditionalBackendGbidSettings(messagingSettings, expectedAdditionalBackend0Gbid, 0);
    checkAdditionalBackendMqttConnectionTimeout(
            messagingSettings, expectedAdditionalBackend0ConnectionTimeoutMs, 0);
    checkAdditionalBackendMqttKeepAliveTimeSeconds(
            messagingSettings, expectedAdditionalBackend0KeepAliveSeconds, 0);

    checkAdditionalBackendUrlSettings(messagingSettings, expectedAdditionalBackend1Url, 1);
    checkAdditionalBackendGbidSettings(messagingSettings, expectedAdditionalBackend1Gbid, 1);
    checkAdditionalBackendMqttConnectionTimeout(
            messagingSettings, expectedAdditionalBackend1ConnectionTimeoutMs, 1);
    checkAdditionalBackendMqttKeepAliveTimeSeconds(
            messagingSettings, expectedAdditionalBackend1KeepAliveSeconds, 1);
}

TEST_F(MessagingSettingsTest, mqttWithAdditionalBackendsWithoutDefaultGbid)
{
    Settings testSettingsWithoutDefaultGbid(testSettingsFileNameMqtt);
    std::string expectedAdditionalBackend0Url("mqtt://additional-backend-host-0:1883/");
    std::string expectedAdditionalBackend0Gbid("additional-gbid-0");
    testSettingsWithoutDefaultGbid.set(MessagingSettings::SETTING_ADDITIONAL_BACKEND_BROKER_URL(0),
                                       expectedAdditionalBackend0Url);
    testSettingsWithoutDefaultGbid.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_GBID(0), expectedAdditionalBackend0Gbid);
    EXPECT_THROW(
            MessagingSettings{testSettingsWithoutDefaultGbid}, exceptions::JoynrRuntimeException);
}

TEST_F(MessagingSettingsTest, mqttWithAdditionalBackendsWithoutGbidForAdditionalBackend)
{
    Settings testSettingsAdditionalBrokerNoGbid(testSettingsFileNameMqttWithGbid);
    std::string expectedAdditionalBackend2Url("mqtt://additional-backend-host-2:1883/");
    testSettingsAdditionalBrokerNoGbid.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_BROKER_URL(2),
            expectedAdditionalBackend2Url);
    EXPECT_TRUE(testSettingsAdditionalBrokerNoGbid.isLoaded());
    EXPECT_THROW(MessagingSettings{testSettingsAdditionalBrokerNoGbid},
                 exceptions::JoynrRuntimeException);
}

TEST_F(MessagingSettingsTest, mqttWithAdditionalBackendsWithoutUrlForAdditionalBackend)
{
    Settings testSettingsAdditionalBrokerNoUrl(testSettingsFileNameMqttWithGbid);
    std::string expectedAdditionalBackend2Gbid("additional-gbid-2");
    testSettingsAdditionalBrokerNoUrl.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_GBID(2), expectedAdditionalBackend2Gbid);
    EXPECT_TRUE(testSettingsAdditionalBrokerNoUrl.isLoaded());
    EXPECT_THROW(MessagingSettings{testSettingsAdditionalBrokerNoUrl},
                 exceptions::JoynrRuntimeException);
}

TEST_F(MessagingSettingsTest, mqttWithAdditionalBackendsWithoutDefaultMqttInfo)
{
    Settings testSettingsAdditionalBroker(testSettingsFileNameMqttWithGbid);
    std::string expectedAdditionalBackend2Url("mqtt://additional-backend-host-2:1883/");
    std::string expectedAdditionalBackend2Gbid("additional-gbid-2");
    std::chrono::seconds expectedAdditionalBackend2KeepAliveSeconds(
            MessagingSettings::DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS());
    std::chrono::milliseconds expectedAdditionalBackend2ConnectionTimeoutMs(
            MessagingSettings::DEFAULT_MQTT_CONNECTION_TIMEOUT_MS());

    std::string expectedAdditionalBackend3Url("mqtt://additional-backend-host-3:1883/");
    std::string expectedAdditionalBackend3Gbid("additional-gbid-3");
    std::chrono::seconds expectedAdditionalBackend3KeepAliveSeconds(20);
    std::chrono::milliseconds expectedAdditionalBackend3ConnectionTimeoutMs(
            MessagingSettings::DEFAULT_MQTT_CONNECTION_TIMEOUT_MS());

    std::string expectedAdditionalBackend4Url("mqtt://additional-backend-host-4:1883/");
    std::string expectedAdditionalBackend4Gbid("additional-gbid-4");
    std::chrono::seconds expectedAdditionalBackend4KeepAliveSeconds(
            MessagingSettings::DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS());
    std::chrono::milliseconds expectedAdditionalBackend4ConnectionTimeoutMs(500);

    testSettingsAdditionalBroker.set(MessagingSettings::SETTING_ADDITIONAL_BACKEND_BROKER_URL(2),
                                     expectedAdditionalBackend2Url);
    testSettingsAdditionalBroker.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_GBID(2), expectedAdditionalBackend2Gbid);

    testSettingsAdditionalBroker.set(MessagingSettings::SETTING_ADDITIONAL_BACKEND_BROKER_URL(3),
                                     expectedAdditionalBackend3Url);
    testSettingsAdditionalBroker.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_GBID(3), expectedAdditionalBackend3Gbid);
    testSettingsAdditionalBroker.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_MQTT_KEEP_ALIVE_TIME_SECONDS(3),
            expectedAdditionalBackend3KeepAliveSeconds.count());

    testSettingsAdditionalBroker.set(MessagingSettings::SETTING_ADDITIONAL_BACKEND_BROKER_URL(4),
                                     expectedAdditionalBackend4Url);
    testSettingsAdditionalBroker.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_GBID(4), expectedAdditionalBackend4Gbid);
    testSettingsAdditionalBroker.set(
            MessagingSettings::SETTING_ADDITIONAL_BACKEND_MQTT_CONNECTION_TIMEOUT_MS(4),
            expectedAdditionalBackend4ConnectionTimeoutMs.count());

    MessagingSettings messagingSettings(testSettingsAdditionalBroker);
    EXPECT_TRUE(testSettingsAdditionalBroker.isLoaded());

    checkAdditionalBackendMqttConnectionTimeout(
            messagingSettings, expectedAdditionalBackend2ConnectionTimeoutMs, 2);
    checkAdditionalBackendMqttKeepAliveTimeSeconds(
            messagingSettings, expectedAdditionalBackend2KeepAliveSeconds, 2);

    checkAdditionalBackendMqttConnectionTimeout(
            messagingSettings, expectedAdditionalBackend3ConnectionTimeoutMs, 3);
    checkAdditionalBackendMqttKeepAliveTimeSeconds(
            messagingSettings, expectedAdditionalBackend3KeepAliveSeconds, 3);

    checkAdditionalBackendMqttConnectionTimeout(
            messagingSettings, expectedAdditionalBackend4ConnectionTimeoutMs, 4);
    checkAdditionalBackendMqttKeepAliveTimeSeconds(
            messagingSettings, expectedAdditionalBackend4KeepAliveSeconds, 4);
}

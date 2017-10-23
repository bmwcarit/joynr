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

#include <gtest/gtest.h>

#include "PrettyPrint.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Settings.h"
#include "joynr/BrokerUrl.h"

using namespace joynr;

class MessagingSettingsTest : public testing::Test {
public:
    MessagingSettingsTest() :
        testSettingsFileNameNonExistent("test-resources/MessagingSettingsTest-nonexistent.settings"),
        testSettingsFileNameHttp("test-resources/HttpMessagingSettingsTest.settings"),
        testSettingsFileNameMqtt("test-resources/MqttMessagingSettingsTest.settings")
    {
    }

protected:
    ADD_LOGGER(MessagingSettingsTest)
    const std::string testSettingsFileNameNonExistent;
    const std::string testSettingsFileNameHttp;
    const std::string testSettingsFileNameMqtt;
};

TEST_F(MessagingSettingsTest, intializedWithDefaultSettings) {
    Settings testSettings(testSettingsFileNameNonExistent);

    // file is not loaded because it intentionally does not exist
    // defaults will be loaded from another file instead
    EXPECT_FALSE(testSettings.isLoaded());

    MessagingSettings messagingSettings(testSettings);

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BROKER_URL()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_URL()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_MQTT_KEEP_ALIVE_TIME_SECONDS()));
    EXPECT_EQ(messagingSettings.getMqttKeepAliveTimeSeconds().count(), MessagingSettings::DEFAULT_MQTT_KEEP_ALIVE_TIME_SECONDS().count());
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_MQTT_RECONNECT_DELAY_TIME_SECONDS()));
    EXPECT_EQ(messagingSettings.getMqttReconnectDelayTimeSeconds().count(), MessagingSettings::DEFAULT_MQTT_RECONNECT_DELAY_TIME_SECONDS().count());
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_MQTT_CONNECTION_TIMEOUT_MS()));
    EXPECT_EQ(messagingSettings.getMqttConnectionTimeoutMs().count(), MessagingSettings::DEFAULT_MQTT_CONNECTION_TIMEOUT_MS().count());
    EXPECT_EQ(messagingSettings.getTtlUpliftMs(), MessagingSettings::DEFAULT_TTL_UPLIFT_MS());
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_ROUTING_TABLE_GRACE_PERIOD_MS()));
    EXPECT_EQ(messagingSettings.getRoutingTableGracePeriodMs(), MessagingSettings::DEFAULT_ROUTING_TABLE_GRACE_PERIOD_MS());
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS()));
    EXPECT_EQ(messagingSettings.getRoutingTableCleanupIntervalMs(), MessagingSettings::DEFAULT_ROUTING_TABLE_CLEANUP_INTERVAL_MS());
}

TEST_F(MessagingSettingsTest, overrideDefaultSettings) {
    std::string expectedBrokerUrl("http://custom-bounceproxy-host:8080/bounceproxy/MessagingSettingsTest-overrideDefaultSettings/");
    std::int64_t expectedRoutingTableGracePeriodMs = 5000;
    std::int64_t expectedRoutingTableCleanupIntervalMs = 6000;
    Settings testSettings(testSettingsFileNameNonExistent);

    testSettings.set(MessagingSettings::SETTING_BROKER_URL(), expectedBrokerUrl);
    testSettings.set(MessagingSettings::SETTING_ROUTING_TABLE_GRACE_PERIOD_MS(), expectedRoutingTableGracePeriodMs);
    testSettings.set(MessagingSettings::SETTING_ROUTING_TABLE_CLEANUP_INTERVAL_MS(), expectedRoutingTableCleanupIntervalMs);
    MessagingSettings messagingSettings(testSettings);

    std::string brokerUrl = messagingSettings.getBrokerUrlString();
    EXPECT_EQ(expectedBrokerUrl, brokerUrl);
    std::int64_t routingTableGracePeriodMs = messagingSettings.getRoutingTableGracePeriodMs();
    EXPECT_EQ(expectedRoutingTableGracePeriodMs, routingTableGracePeriodMs);
    std::int64_t routingTableCleanupIntervalMs = messagingSettings.getRoutingTableCleanupIntervalMs();
    EXPECT_EQ(expectedRoutingTableCleanupIntervalMs, routingTableCleanupIntervalMs);
}

void checkBrokerSettings(
        MessagingSettings messagingSettings,
        std::string expectedBrokerUrl) {
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BROKER_URL()));

    std::string brokerUrl = messagingSettings.getBrokerUrlString();
    EXPECT_EQ(expectedBrokerUrl, brokerUrl);
}

void checkDiscoveryDirectorySettings(
        MessagingSettings messagingSettings,
        std::string expectedCapabilitiesDirectoryChannelId) {
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));

    std::string capabilitiesDirectoryChannelId = messagingSettings.getCapabilitiesDirectoryChannelId();
    EXPECT_EQ(expectedCapabilitiesDirectoryChannelId, capabilitiesDirectoryChannelId);
}

TEST_F(MessagingSettingsTest, httpOnly) {
    std::string expectedBrokerUrl("http://custom-bounceproxy-host:8080/bounceproxy/");
    std::string expectedCapabilitiesDirectoryChannelId("discoverydirectory_channelid");

    Settings testSettings(testSettingsFileNameHttp);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    checkBrokerSettings(messagingSettings, expectedBrokerUrl);

    checkDiscoveryDirectorySettings(messagingSettings, expectedCapabilitiesDirectoryChannelId);
}

TEST_F(MessagingSettingsTest, mqttOnly) {
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
    const std::string fileName = "test-resources/MessagingSettingsDiscoveryEntryExpiryIntervalMs.settings";
    Settings testSettings(fileName);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    constexpr std::int64_t int64Max = std::numeric_limits<std::int64_t>::max();
    EXPECT_EQ(int64Max, messagingSettings.getDiscoveryEntryExpiryIntervalMs());
}

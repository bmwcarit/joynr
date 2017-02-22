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
#include <gtest/gtest.h>
#include <cstdio>
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
        testSettingsFileNameMqtt("test-resources/MqttMessagingSettingsTest.settings"),
        testSettingsFileNameMqttWithHttpBackend("test-resources/MqttWithHttpBackendMessagingSettingsTest.settings"),
        testSettingsFileNameAccessControl("test-resources/MessagingWithAccessControlEnabled.settings")
    {
    }

protected:
    ADD_LOGGER(MessagingSettingsTest);
    const std::string testSettingsFileNameNonExistent;
    const std::string testSettingsFileNameHttp;
    const std::string testSettingsFileNameMqtt;
    const std::string testSettingsFileNameMqttWithHttpBackend;
    const std::string testSettingsFileNameAccessControl;
};

INIT_LOGGER(MessagingSettingsTest);

TEST_F(MessagingSettingsTest, intializedWithDefaultSettings) {
    Settings testSettings(testSettingsFileNameNonExistent);

    // file is not loaded because it intentionally does not exist
    // defaults will be loaded from another file instead
    EXPECT_FALSE(testSettings.isLoaded());

    MessagingSettings messagingSettings(testSettings);

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BROKER_URL()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BOUNCE_PROXY_URL()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_URL()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_MQTT_KEEP_ALIVE_TIME()));
    EXPECT_EQ(messagingSettings.getMqttKeepAliveTime().count(), MessagingSettings::DEFAULT_MQTT_KEEP_ALIVE_TIME().count());
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_MQTT_RECONNECT_SLEEP_TIME()));
    EXPECT_EQ(messagingSettings.getMqttReconnectSleepTime().count(), MessagingSettings::DEFAULT_MQTT_RECONNECT_SLEEP_TIME().count());
    EXPECT_EQ(messagingSettings.getTtlUpliftMs(), MessagingSettings::DEFAULT_TTL_UPLIFT_MS());
}

TEST_F(MessagingSettingsTest, overrideDefaultSettings) {
    std::string expectedBrokerUrl("http://custom-bounceproxy-host:8080/bounceproxy/MessagingSettingsTest-overrideDefaultSettings/");
    Settings testSettings(testSettingsFileNameNonExistent);

    testSettings.set(MessagingSettings::SETTING_BROKER_URL(), expectedBrokerUrl);
    MessagingSettings messagingSettings(testSettings);

    std::string brokerUrl = messagingSettings.getBrokerUrlString();
    EXPECT_EQ(expectedBrokerUrl, brokerUrl);
}

void checkBrokerSettings(
        MessagingSettings messagingSettings,
        std::string expectedBrokerUrl,
        std::string expectedBounceProxyUrl) {
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BROKER_URL()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BOUNCE_PROXY_URL()));

    std::string brokerUrl = messagingSettings.getBrokerUrlString();
    EXPECT_EQ(expectedBrokerUrl, brokerUrl);

    std::string bounceProxyUrl = messagingSettings.getBounceProxyUrlString();
    EXPECT_EQ(expectedBounceProxyUrl, bounceProxyUrl);
}

void checkDiscoveryDirectorySettings(
        MessagingSettings messagingSettings,
        std::string expectedCapabilitiesDirectoryChannelId) {
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));

    std::string capabilitiesDirectoryChannelId = messagingSettings.getCapabilitiesDirectoryChannelId();
    EXPECT_EQ(expectedCapabilitiesDirectoryChannelId, capabilitiesDirectoryChannelId);
}

TEST_F(MessagingSettingsTest, mqttWithHttpBackend) {
    std::string expectedBrokerUrl("mqtt://custom-broker-host:1883/");
    std::string expectedBounceProxyUrl("http://custom-bounceproxy-host:8080/bounceproxy/");
    std::string expectedCapabilitiesDirectoryChannelId("discoverydirectory_channelid");

    Settings testSettings(testSettingsFileNameMqttWithHttpBackend);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    // the file contains different settings for brokerUrl and bounceProxyUrl
    checkBrokerSettings(messagingSettings, expectedBrokerUrl, expectedBounceProxyUrl);

    checkDiscoveryDirectorySettings(messagingSettings, expectedCapabilitiesDirectoryChannelId);
}

TEST_F(MessagingSettingsTest, writeAccessControlToSettings) {
    // write new settings
    {
        Settings testSettings(testSettingsFileNameAccessControl);
        ASSERT_TRUE(testSettings.isLoaded());

        MessagingSettings messagingSettings(testSettings);

        // in the loaded setting file the access control is set to false
        EXPECT_FALSE(messagingSettings.enableAccessController());

        messagingSettings.setEnableAccessController(true);
        EXPECT_TRUE(messagingSettings.enableAccessController());

        testSettings.sync();
    }

    // load and check
    {
        Settings testSettings(testSettingsFileNameAccessControl);
        ASSERT_TRUE(testSettings.isLoaded());

        MessagingSettings messagingSettings(testSettings);

        // in the loaded setting file the access control is set now to true
        EXPECT_TRUE(messagingSettings.enableAccessController());

        // revert changes to setting file
        messagingSettings.setEnableAccessController(false);
        testSettings.sync();
    }
}

/*
 * This test does not work anymore, as http only can not be configured by setting the broker url to a http url
 * Before re-activating this patch, a parameter should be added to the settings file which explicitely enable/disables http/mqtt
 */
TEST_F(MessagingSettingsTest, DISABLED_httpOnly) {
    std::string expectedBrokerUrl("http://custom-bounceproxy-host:8080/bounceproxy/");
    std::string expectedCapabilitiesDirectoryChannelId("discoverydirectory_channelid");

    Settings testSettings(testSettingsFileNameHttp);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    // since only brokerUrl is present, bounceProxyUrl is setup identically
    checkBrokerSettings(messagingSettings, expectedBrokerUrl, expectedBrokerUrl);

    checkDiscoveryDirectorySettings(messagingSettings, expectedCapabilitiesDirectoryChannelId);
}

/*
 * This test does not work anymore, as mqtt only can not be configured by setting the broker url to a mqtt url
 * Before re-activating this patch, a parameter should be added to the settings file which explicitely enable/disables http/mqtt
 */
TEST_F(MessagingSettingsTest, DISABLED_mqttOnly) {
    std::string expectedBrokerUrl("mqtt://custom-broker-host:1883/");
    std::string defaultBounceProxyUrl("http://localhost:8080/bounceproxy/");
    std::string expectedCapabilitiesDirectoryChannelId("mqtt_discoverydirectory_channelid");

    Settings testSettings(testSettingsFileNameMqtt);
    EXPECT_TRUE(testSettings.isLoaded());
    MessagingSettings messagingSettings(testSettings);

    // since only brokerUrl is present, bounceProxyUrl is setup identically
    checkBrokerSettings(messagingSettings, expectedBrokerUrl, defaultBounceProxyUrl);

    checkDiscoveryDirectorySettings(messagingSettings, expectedCapabilitiesDirectoryChannelId);
}

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
#include <cstdio>
#include "PrettyPrint.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Settings.h"
#include "joynr/TypeUtil.h"
#include "joynr/BounceProxyUrl.h"

using namespace joynr;

class MessagingSettingsTest : public testing::Test {
public:
    MessagingSettingsTest() :
        testSettingsFileName("MessagingSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown() {
        std::remove(testSettingsFileName.c_str());
    }

protected:
    ADD_LOGGER(MessagingSettingsTest);
    std::string testSettingsFileName;
};

INIT_LOGGER(MessagingSettingsTest);

TEST_F(MessagingSettingsTest, intializedWithDefaultSettings) {
    Settings testSettings(testSettingsFileName);

    EXPECT_FALSE(testSettings.isLoaded());

    MessagingSettings messagingSettings(testSettings);

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_BOUNCE_PROXY_URL()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_DISCOVERY_DIRECTORIES_DOMAIN()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_URL()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_CHANNELID()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CHANNEL_URL_DIRECTORY_PARTICIPANTID()));

    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_URL()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_CHANNELID()));
    EXPECT_TRUE(messagingSettings.contains(MessagingSettings::SETTING_CAPABILITIES_DIRECTORY_PARTICIPANTID()));
}

TEST_F(MessagingSettingsTest, overrideDefaultSettings) {
    std::string expectedBounceProxyUrl("http://localhost:8080/bounceproxy/MessagingSettingsTest-overrideDefaultSettings/");
    Settings testSettings(testSettingsFileName);
    testSettings.set(MessagingSettings::SETTING_BOUNCE_PROXY_URL(), expectedBounceProxyUrl);
    MessagingSettings messagingSettings(testSettings);

    std::string bounceProxyUrl = messagingSettings.getBounceProxyUrlString();
    EXPECT_EQ(expectedBounceProxyUrl, bounceProxyUrl);
}

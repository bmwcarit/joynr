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
#include <QSettings>
#include <QFile>
#include "PrettyPrint.h"
#include "joynr/MessagingSettings.h"

using namespace joynr;

class MessagingSettingsTest : public testing::Test {
public:
    MessagingSettingsTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "MessagingSettingsTest")),
        testSettingsFileName("MessagingSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown() {
        QFile::remove(testSettingsFileName);
    }

protected:
    joynr_logging::Logger* logger;
    QString testSettingsFileName;
};

TEST_F(MessagingSettingsTest, intializedWithDefaultSettings) {
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
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
    QString expectedBounceProxyUrl("http://localhost:8080/bounceproxy/MessagingSettingsTest-overrideDefaultSettings/");
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    testSettings.setValue(MessagingSettings::SETTING_BOUNCE_PROXY_URL(), expectedBounceProxyUrl);
    MessagingSettings messagingSettings(testSettings);

    QString bounceProxyUrl = messagingSettings.value(MessagingSettings::SETTING_BOUNCE_PROXY_URL()).toString();
    EXPECT_EQ_QSTRING(expectedBounceProxyUrl, bounceProxyUrl);
}

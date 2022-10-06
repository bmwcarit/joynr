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

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Settings.h"

using namespace joynr;

TEST(ClusterControllerSettingsTest, accessControlIsEnabled)
{
    Settings testSettings("test-resources/CCSettingsWithAccessControlEnabled.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    // In the loaded setting file the access control is set to true
    EXPECT_TRUE(clusterControllerSettings.enableAccessController());
}

TEST(ClusterControllerSettingsTest, accessControlIsDisabled)
{
    Settings testSettings("test-resources/CCSettingsWithAccessControlDisabled.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    // In the loaded setting file the access control is set to false
    EXPECT_FALSE(clusterControllerSettings.enableAccessController());
}

TEST(ClusterControllerSettingsTest, clusterControllerAclEntriesPathSet)
{
    Settings testSettings(
            "test-resources/CCSettingsWithAccessControlEnabledAndAclFilePathSet.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);
    EXPECT_EQ("test-resources", clusterControllerSettings.getAclEntriesDirectory());
}

TEST(ClusterControllerSettingsTest, initializedWithDefaultSettings)
{
    Settings testSettings("test-resources/CCSettings-nonexistent.settings");

    ASSERT_FALSE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.getMessageQueueLimit(),
              ClusterControllerSettings::DEFAULT_MESSAGE_QUEUE_LIMIT());
}

// check specific non-default settings

TEST(ClusterControllerSettingsTest, messageQueueLimitIsSet)
{
    Settings testSettings("test-resources/CCSettingsWithMessageQueueLimit.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.getMessageQueueLimit(), std::uint64_t(10));
}

TEST(ClusterControllerSettingsTest, perParticipantIdMessageQueueLimitIsSet)
{
    Settings testSettings("test-resources/CCSettingsWithMessageQueueLimit.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.getPerParticipantIdMessageQueueLimit(), std::uint64_t(5));
}

TEST(ClusterControllerSettingsTest, transportNotAvailableQueueLimitIsSet)
{
    Settings testSettings("test-resources/CCSettingsWithMessageQueueLimit.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.getTransportNotAvailableQueueLimit(), std::uint64_t(10));
}

TEST(ClusterControllerSettingsTest, messageQueueLimitBytesIsSet)
{
    Settings testSettings("test-resources/CCSettingsWithMessageQueueLimit.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.getMessageQueueLimitBytes(), std::uint64_t(104857600));
}

TEST(ClusterControllerSettingsTest, transportNotAvailableQueueLimitBytesIsSet)
{
    Settings testSettings("test-resources/CCSettingsWithMessageQueueLimit.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.getTransportNotAvailableQueueLimitBytes(),
              std::uint64_t(52428800));
}

TEST(ClusterControllerSettingsTest, globalCapabilitiesDirectoryCompressedMessagesEnabledIsSet)
{
    Settings testSettings("test-resources/CCSettingsWithGlobalDiscovery.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.isGlobalCapabilitiesDirectoryCompressedMessagesEnabled(),
              true);
}

TEST(ClusterControllerSettingsTest, webSocketDisabledFromSettingsButModifiable)
{
    Settings testSettings("test-resources/CCSettingsWithWebSocketDisabled.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.isWebSocketEnabled(), false);

    clusterControllerSettings.setWebSocketEnabled(true);
    EXPECT_EQ(clusterControllerSettings.isWebSocketEnabled(), true);
}

TEST(ClusterControllerSettingsTest, udsDisabledFromSettingsButModifiable)
{
    Settings testSettings("test-resources/CCSettingsWithUdsDisabled.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    EXPECT_EQ(clusterControllerSettings.isUdsEnabled(), false);

    clusterControllerSettings.setUdsEnabled(true);
    EXPECT_EQ(clusterControllerSettings.isUdsEnabled(), true);
}

// check default values

TEST(ClusterControllerSettingsTest, defaultMessageQueueLimitIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.getMessageQueueLimit(),
              ClusterControllerSettings::DEFAULT_MESSAGE_QUEUE_LIMIT());
}

TEST(ClusterControllerSettingsTest, defaultPerParticipantIdMessageQueueLimitIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.getPerParticipantIdMessageQueueLimit(),
              ClusterControllerSettings::DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT());
}

TEST(ClusterControllerSettingsTest, defaultTransportNotAvailableQueueLimitIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.getTransportNotAvailableQueueLimit(),
              ClusterControllerSettings::DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT());
}

TEST(ClusterControllerSettingsTest, defaultMessageQueueLimitBytesIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.getMessageQueueLimitBytes(),
              ClusterControllerSettings::DEFAULT_MESSAGE_QUEUE_LIMIT_BYTES());
}

TEST(ClusterControllerSettingsTest, defaultTransportNotAvailableQueueLimitBytesIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.getTransportNotAvailableQueueLimitBytes(),
              ClusterControllerSettings::DEFAULT_TRANSPORT_NOT_AVAILABLE_QUEUE_LIMIT_BYTES());
}

TEST(ClusterControllerSettingsTest,
     defaultGlobalCapabilitiesDirectoryCompressedMessagesEnabledIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.isGlobalCapabilitiesDirectoryCompressedMessagesEnabled(),
              ClusterControllerSettings::
                      DEFAULT_GLOBAL_CAPABILITIES_DIRECTORY_COMPRESSED_MESSAGES_ENABLED());
}

TEST(ClusterControllerSettingsTest, defaultWebSocketEnabledIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.isWebSocketEnabled(), true);
}

TEST(ClusterControllerSettingsTest, defaultUdsEnabledIsSet)
{
    Settings settings;
    ClusterControllerSettings clusterControllerSettings(settings);

    EXPECT_EQ(clusterControllerSettings.isUdsEnabled(), true);
}

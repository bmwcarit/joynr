/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include <chrono>
#include <cstdio>

#include "joynr/UdsSettings.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/Settings.h"

#include "tests/PrettyPrint.h"

using namespace joynr;

class UdsSettingsTest : public testing::Test
{
public:
    UdsSettingsTest() : testSettingsFileName("UdsSettingsTest-nonexistent.settings")
    {
    }

    virtual void TearDown()
    {
        std::remove(testSettingsFileName.c_str());
    }

protected:
    ADD_LOGGER(UdsSettingsTest)
    std::string testSettingsFileName;
};

TEST_F(UdsSettingsTest, intializedWithDefaultSettings)
{
    Settings testSettings(testSettingsFileName);
    UdsSettings udsSettings(testSettings);

    EXPECT_TRUE(udsSettings.contains(UdsSettings::SETTING_SOCKET_PATH()));
    EXPECT_TRUE(udsSettings.contains(UdsSettings::SETTING_RECONNECT_SLEEP_TIME_MS()));
    EXPECT_TRUE(udsSettings.contains(UdsSettings::SETTING_CLIENT_ID()));

    EXPECT_EQ(udsSettings.getSocketPath(), joynr::UdsSettings::DEFAULT_SOCKET_PATH());
    EXPECT_EQ(udsSettings.getReconnectSleepTimeMs(), joynr::UdsSettings::DEFAULT_RECONNECT_SLEEP_TIME_MS());
    EXPECT_NE(udsSettings.getClientId(), "");
}

TEST_F(UdsSettingsTest, overrideDefaultSettings)
{
    const std::string expectedSocketPath("/tmp/test-path");
    EXPECT_NE(expectedSocketPath, joynr::UdsSettings::DEFAULT_SOCKET_PATH());

    const std::chrono::milliseconds expectedReconnectSleepTimeMs(1024);
    EXPECT_NE(expectedReconnectSleepTimeMs, joynr::UdsSettings::DEFAULT_RECONNECT_SLEEP_TIME_MS());

    const std::string expectedClientId("testClientId");

    Settings testSettings(testSettingsFileName);
    UdsSettings udsSettings(testSettings);
    udsSettings.setSocketPath(expectedSocketPath);
    udsSettings.setReconnectSleepTimeMs(expectedReconnectSleepTimeMs);
    udsSettings.setClientId(expectedClientId);

    const std::string socketPath = udsSettings.getSocketPath();
    EXPECT_EQ(expectedSocketPath, socketPath);

    const std::chrono::milliseconds reconnectSleepTimeMs = udsSettings.getReconnectSleepTimeMs();
    EXPECT_EQ(expectedReconnectSleepTimeMs, reconnectSleepTimeMs);

    const std::string clientId = udsSettings.getClientId();
    EXPECT_EQ(expectedClientId, clientId);
}

TEST_F(UdsSettingsTest, createsUdsAddress)
{
    const std::string expectedSocketPath("/test-path");
    EXPECT_NE(expectedSocketPath, joynr::UdsSettings::DEFAULT_SOCKET_PATH());

    const joynr::system::RoutingTypes::UdsAddress expectedMessagingAddress(expectedSocketPath);

    Settings testSettings(testSettingsFileName);
    UdsSettings udsSettings(testSettings);
    udsSettings.setSocketPath(expectedSocketPath);

    const system::RoutingTypes::UdsAddress udsAddress =
            udsSettings.createClusterControllerMessagingAddress();
    EXPECT_EQ(expectedMessagingAddress, udsAddress);
}

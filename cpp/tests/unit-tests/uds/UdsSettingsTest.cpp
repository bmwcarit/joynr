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
#include <chrono>
#include <cstdio>

#include "tests/utils/Gtest.h"

#include "joynr/Settings.h"
#include "joynr/UdsSettings.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

#include "tests/PrettyPrint.h"

using namespace joynr;

class UdsSettingsTest : public testing::Test
{
public:
    UdsSettingsTest() : testSettingsFileName("UdsSettingsTest-nonexistent.settings")
    {
    }

    void TearDown() override
    {
        remove(testSettingsFileName.c_str());
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
    EXPECT_TRUE(udsSettings.contains(UdsSettings::SETTING_CONNECT_SLEEP_TIME_MS()));
    EXPECT_TRUE(udsSettings.contains(UdsSettings::SETTING_CLIENT_ID()));
    EXPECT_TRUE(udsSettings.contains(UdsSettings::SETTING_SENDING_QUEUE_SIZE()));

    EXPECT_EQ(udsSettings.getSocketPath(), joynr::UdsSettings::DEFAULT_SOCKET_PATH());
    EXPECT_EQ(udsSettings.getConnectSleepTimeMs(),
              joynr::UdsSettings::DEFAULT_CONNECT_SLEEP_TIME_MS());
    EXPECT_NE(udsSettings.getClientId(), "");
    EXPECT_EQ(udsSettings.getSendingQueueSize(), joynr::UdsSettings::DEFAULT_SENDING_QUEUE_SIZE());
}

TEST_F(UdsSettingsTest, overrideDefaultSettings)
{
    const std::string expectedSocketPath("/tmp/test-path");
    EXPECT_NE(expectedSocketPath, joynr::UdsSettings::DEFAULT_SOCKET_PATH());

    const std::chrono::milliseconds expectedConnectSleepTimeMs(1024);
    EXPECT_NE(expectedConnectSleepTimeMs, joynr::UdsSettings::DEFAULT_CONNECT_SLEEP_TIME_MS());

    const std::string expectedClientId("testClientId");

    Settings testSettings(testSettingsFileName);
    UdsSettings udsSettings(testSettings);
    udsSettings.setSocketPath(expectedSocketPath);
    udsSettings.setConnectSleepTimeMs(expectedConnectSleepTimeMs);
    udsSettings.setClientId(expectedClientId);

    const std::string socketPath = udsSettings.getSocketPath();
    EXPECT_EQ(expectedSocketPath, socketPath);

    const std::chrono::milliseconds connectSleepTimeMs = udsSettings.getConnectSleepTimeMs();
    EXPECT_EQ(expectedConnectSleepTimeMs, connectSleepTimeMs);

    const std::string clientId = udsSettings.getClientId();
    EXPECT_EQ(expectedClientId, clientId);

    const std::size_t expectedSendingQueueSize(42);
    EXPECT_NE(expectedSendingQueueSize, joynr::UdsSettings::DEFAULT_SENDING_QUEUE_SIZE());
    udsSettings.setSendingQueueSize(expectedSendingQueueSize);
    const auto sendingQueueSize = udsSettings.getSendingQueueSize();
    EXPECT_EQ(expectedSendingQueueSize, sendingQueueSize);
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

TEST_F(UdsSettingsTest, createClientMessagingAddress)
{
    const std::string expectedClientId("some-non-random-client-id");
    Settings testSettings(testSettingsFileName);
    UdsSettings udsSettings(testSettings);
    EXPECT_NE(expectedClientId, udsSettings.getClientId());

    const joynr::system::RoutingTypes::UdsClientAddress expectedAddress(expectedClientId);

    udsSettings.setClientId(expectedClientId);

    const auto createdAddress = udsSettings.createClientMessagingAddress();
    EXPECT_EQ(expectedAddress, createdAddress);
}

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
#include "tests/utils/Gtest.h"
#include <chrono>
#include <cstdio>

#include "joynr/WebSocketSettings.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/Settings.h"

#include "tests/PrettyPrint.h"

using namespace joynr;

class WebSocketSettingsTest : public testing::Test
{
public:
    WebSocketSettingsTest() : testSettingsFileName("WebSocketSettingsTest-nonexistent.settings")
    {
    }

    virtual void TearDown()
    {
        std::remove(testSettingsFileName.c_str());
    }

protected:
    ADD_LOGGER(WebSocketSettingsTest)
    std::string testSettingsFileName;
};

TEST_F(WebSocketSettingsTest, intializedWithDefaultSettings)
{
    Settings testSettings(testSettingsFileName);
    WebSocketSettings wsSettings(testSettings);

    EXPECT_TRUE(wsSettings.contains(WebSocketSettings::SETTING_CC_MESSAGING_URL()));
    EXPECT_TRUE(wsSettings.contains(WebSocketSettings::SETTING_RECONNECT_SLEEP_TIME_MS()));
}

TEST_F(WebSocketSettingsTest, overrideDefaultSettings)
{
    std::string expectedMessagingUrl("ws://test-host:42/test-path");
    std::chrono::milliseconds expectedReconnectSleepTimeMs(1024);
    Settings testSettings(testSettingsFileName);
    testSettings.set(WebSocketSettings::SETTING_CC_MESSAGING_URL(), expectedMessagingUrl);
    WebSocketSettings wsSettings(testSettings);
    wsSettings.setReconnectSleepTimeMs(expectedReconnectSleepTimeMs);

    std::string messagingUrl = wsSettings.getClusterControllerMessagingUrl();
    EXPECT_EQ(expectedMessagingUrl, messagingUrl);
    std::chrono::milliseconds reconnectSleepTimeMs = wsSettings.getReconnectSleepTimeMs();
    EXPECT_EQ(expectedReconnectSleepTimeMs, reconnectSleepTimeMs);
}

TEST_F(WebSocketSettingsTest, createsWebSocketAddress)
{
    std::string expectedMessagingUrl("ws://test-host:42/test-path");
    joynr::system::RoutingTypes::WebSocketAddress expectedMessagingAddress(
            joynr::system::RoutingTypes::WebSocketProtocol::WS, "test-host", 42, "/test-path");
    Settings testSettings(testSettingsFileName);
    testSettings.set(WebSocketSettings::SETTING_CC_MESSAGING_URL(), expectedMessagingUrl);
    WebSocketSettings wsSettings(testSettings);

    system::RoutingTypes::WebSocketAddress wsAddress =
            wsSettings.createClusterControllerMessagingAddress();
    EXPECT_EQ(expectedMessagingAddress, wsAddress);
}

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#include <QFile>
#include "PrettyPrint.h"
#include "libjoynr/websocket/WebSocketSettings.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/TypeUtil.h"
#include "joynr/Settings.h"

using namespace joynr;

class WebSocketSettingsTest : public testing::Test {
public:
    WebSocketSettingsTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "WebSocketSettingsTest")),
        testSettingsFileName("WebSocketSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown() {
        QFile::remove(TypeUtil::toQt(testSettingsFileName));
    }

protected:
    joynr_logging::Logger* logger;
    std::string testSettingsFileName;
};

TEST_F(WebSocketSettingsTest, intializedWithDefaultSettings) {
    Settings testSettings(testSettingsFileName);
    WebSocketSettings wsSettings(testSettings);

    EXPECT_TRUE(wsSettings.contains(WebSocketSettings::SETTING_CC_MESSAGING_URL()));
}

TEST_F(WebSocketSettingsTest, overrideDefaultSettings) {
    std::string expectedMessagingUrl("ws://test-host:42/test-path");
    Settings testSettings(testSettingsFileName);
    testSettings.set(WebSocketSettings::SETTING_CC_MESSAGING_URL(), expectedMessagingUrl);
    WebSocketSettings wsSettings(testSettings);

    std::string messagingUrl = wsSettings.getClusterControllerMessagingUrl();
    EXPECT_EQ(expectedMessagingUrl, messagingUrl);
}

TEST_F(WebSocketSettingsTest, createsWebSocketAddress) {
    std::string expectedMessagingUrl("ws://test-host:42/test-path");
    joynr::system::RoutingTypes::WebSocketAddress expectedMessagingAddress(
                joynr::system::RoutingTypes::WebSocketProtocol::WS,
                "test-host",
                42,
                "/test-path"
    );
    Settings testSettings(testSettingsFileName);
    testSettings.set(WebSocketSettings::SETTING_CC_MESSAGING_URL(), expectedMessagingUrl);
    WebSocketSettings wsSettings(testSettings);

    system::RoutingTypes::WebSocketAddress wsAddress = wsSettings.createClusterControllerMessagingAddress();
    EXPECT_EQ(expectedMessagingAddress, wsAddress);
}

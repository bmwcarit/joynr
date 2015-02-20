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
#include <QSettings>
#include <QFile>
#include "PrettyPrint.h"
#include "libjoynr/websocket/WebSocketSettings.h"
#include "joynr/system/WebSocketAddress.h"

using namespace joynr;

class WebSocketSettingsTest : public testing::Test {
public:
    WebSocketSettingsTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "WebSocketSettingsTest")),
        testSettingsFileName("WebSocketSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown() {
        QFile::remove(testSettingsFileName);
    }

protected:
    joynr_logging::Logger* logger;
    QString testSettingsFileName;
};

TEST_F(WebSocketSettingsTest, intializedWithDefaultSettings) {
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    WebSocketSettings wsSettings(testSettings);

    EXPECT_TRUE(wsSettings.contains(WebSocketSettings::SETTING_CC_MESSAGING_URL()));
}

TEST_F(WebSocketSettingsTest, overrideDefaultSettings) {
    QString expectedMessagingUrl("ws://test-host:42/test-path");
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    testSettings.setValue(WebSocketSettings::SETTING_CC_MESSAGING_URL(), expectedMessagingUrl);
    WebSocketSettings wsSettings(testSettings);

    QString messagingUrl = wsSettings.value(WebSocketSettings::SETTING_CC_MESSAGING_URL()).toString();
    EXPECT_EQ_QSTRING(expectedMessagingUrl, messagingUrl);
}

TEST_F(WebSocketSettingsTest, createsWebSocketAddress) {
    QString expectedMessagingUrl("ws://test-host:42/test-path");
    joynr::system::WebSocketAddress expectedMessagingAddress(
                joynr::system::WebSocketProtocol::WS,
                "test-host",
                42,
                "/test-path"
    );
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    testSettings.setValue(WebSocketSettings::SETTING_CC_MESSAGING_URL(), expectedMessagingUrl);
    WebSocketSettings wsSettings(testSettings);

    EXPECT_EQ(expectedMessagingAddress, wsSettings.createClusterControllerMessagingAddress());
}

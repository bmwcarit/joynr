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
#include "common/dbus/DbusSettings.h"
#include "joynr/Settings.h"

using namespace joynr;

class DbusSettingsTest : public testing::Test {
public:
    DbusSettingsTest() :
        testSettingsFileName("DbusSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown() {
        std::remove(testSettingsFileName.c_str());
    }

protected:
    std::string testSettingsFileName;
};

TEST_F(DbusSettingsTest, intializedWithDefaultSettings) {
    Settings testSettings{testSettingsFileName};
    DbusSettings dbusSettings(testSettings);

    EXPECT_FALSE(dbusSettings.getClusterControllerMessagingDomain().empty());
    EXPECT_FALSE(dbusSettings.getClusterControllerMessagingServiceName().empty());
    EXPECT_FALSE(dbusSettings.getClusterControllerMessagingParticipantId().empty());
}

TEST_F(DbusSettingsTest, overrideDefaultSettings) {
    std::string expectedMessagingDomain("test-domain");
    Settings testSettings{testSettingsFileName};
    testSettings.set(DbusSettings::SETTING_CC_MESSAGING_DOMAIN(), expectedMessagingDomain);
    DbusSettings dbusSettings(testSettings);

    std::string messagingDomain = dbusSettings.getClusterControllerMessagingDomain();
    EXPECT_EQ(expectedMessagingDomain, messagingDomain);
}

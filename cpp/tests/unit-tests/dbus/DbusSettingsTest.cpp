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
#include "common/dbus/DbusSettings.h"

using namespace joynr;

class DbusSettingsTest : public testing::Test {
public:
    DbusSettingsTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "DbusSettingsTest")),
        testSettingsFileName("DbusSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown() {
        QFile::remove(testSettingsFileName);
    }

protected:
    joynr_logging::Logger* logger;
    QString testSettingsFileName;
};

TEST_F(DbusSettingsTest, intializedWithDefaultSettings) {
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    DbusSettings dbusSettings(testSettings);

    EXPECT_TRUE(dbusSettings.contains(DbusSettings::SETTING_CC_MESSAGING_DOMAIN()));
    EXPECT_TRUE(dbusSettings.contains(DbusSettings::SETTING_CC_MESSAGING_SERVICENAME()));
    EXPECT_TRUE(dbusSettings.contains(DbusSettings::SETTING_CC_MESSAGING_PARTICIPANTID()));
}

TEST_F(DbusSettingsTest, overrideDefaultSettings) {
    QString expectedMessagingDomain("test-domain");
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    testSettings.setValue(DbusSettings::SETTING_CC_MESSAGING_DOMAIN(), expectedMessagingDomain);
    DbusSettings dbusSettings(testSettings);

    QString messagingDomain = dbusSettings.value(DbusSettings::SETTING_CC_MESSAGING_DOMAIN()).toString();
    EXPECT_EQ_QSTRING(expectedMessagingDomain, messagingDomain);
}

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
#include "joynr/SystemServicesSettings.h"

using namespace joynr;

class SystemServicesSettingsTest : public testing::Test {
public:
    SystemServicesSettingsTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "SystemServicesSettingsTest")),
        testSettingsFileName("SystemServicesSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown() {
        QFile::remove(testSettingsFileName);
    }

protected:
    joynr_logging::Logger* logger;
    QString testSettingsFileName;
};

TEST_F(SystemServicesSettingsTest, intializedWithDefaultSettings) {
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    SystemServicesSettings systemSettings(testSettings);

    EXPECT_TRUE(systemSettings.contains(SystemServicesSettings::SETTING_DOMAIN()));
    EXPECT_TRUE(systemSettings.contains(SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()));
    EXPECT_TRUE(systemSettings.contains(SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()));
}

TEST_F(SystemServicesSettingsTest, overrideDefaultSettings) {
    QString expectedDomain("overridenDomain");
    QSettings testSettings(testSettingsFileName, QSettings::IniFormat);
    testSettings.setValue(SystemServicesSettings::SETTING_DOMAIN(), expectedDomain);
    SystemServicesSettings systemSettings(testSettings);

    QString domain = systemSettings.value(SystemServicesSettings::SETTING_DOMAIN()).toString();
    EXPECT_EQ_QSTRING(expectedDomain, domain);
}

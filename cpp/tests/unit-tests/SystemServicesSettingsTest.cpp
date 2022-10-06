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
#include <string>

#include "tests/utils/Gtest.h"

#include "joynr/Settings.h"
#include "joynr/SystemServicesSettings.h"

#include "tests/PrettyPrint.h"

using namespace joynr;

class SystemServicesSettingsTest : public testing::Test
{
public:
    SystemServicesSettingsTest()
            : testSettingsFileName("SystemServicesSettingsTest-testSettings.settings")
    {
    }

    virtual void TearDown()
    {
        std::remove(testSettingsFileName.c_str());
    }

protected:
    ADD_LOGGER(SystemServicesSettingsTest)
    std::string testSettingsFileName;
};

TEST_F(SystemServicesSettingsTest, intializedWithDefaultSettings)
{
    Settings testSettings(testSettingsFileName);
    SystemServicesSettings systemSettings(testSettings);

    EXPECT_TRUE(systemSettings.contains(SystemServicesSettings::SETTING_DOMAIN()));
    EXPECT_TRUE(systemSettings.contains(
            SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()));
    EXPECT_TRUE(systemSettings.contains(
            SystemServicesSettings::SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()));
    EXPECT_TRUE(systemSettings.contains(
            SystemServicesSettings::SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()));
}

TEST_F(SystemServicesSettingsTest, overrideDefaultSettings)
{
    std::string expectedDomain("overridenDomain");
    Settings testSettings(testSettingsFileName);
    testSettings.set(SystemServicesSettings::SETTING_DOMAIN(), expectedDomain);
    SystemServicesSettings systemSettings(testSettings);

    std::string domain = systemSettings.getDomain();
    EXPECT_EQ(expectedDomain, domain);
}

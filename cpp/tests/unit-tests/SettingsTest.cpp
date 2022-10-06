/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/Settings.h"

#include <boost/filesystem.hpp>
#include <tuple>

#include "tests/utils/Gtest.h"

class SettingsTest : public ::testing::TestWithParam<std::tuple<int, int, bool>>
{
public:
    SettingsTest()
            : settingsPath("settings-test.settings"),
              fileSystemSyncGracePeriod(500),
              firstValue(std::get<0>(GetParam())),
              secondValue(std::get<1>(GetParam())),
              settingsFileChanged(std::get<2>(GetParam()))
    {
    }

    void TearDown() final
    {
        boost::filesystem::remove(settingsPath);
    }

protected:
    const boost::filesystem::path settingsPath;
    const std::chrono::milliseconds fileSystemSyncGracePeriod;

    const int firstValue;
    const int secondValue;
    const bool settingsFileChanged;
};

TEST_P(SettingsTest, settingsFileUpdate)
{
    joynr::Settings settings(settingsPath.string());

    settings.set("value", firstValue);
    bool initialWritePerformed = settings.sync();

    settings.set("value", secondValue);
    bool secondWritePerformed = settings.sync();

    EXPECT_EQ(true, initialWritePerformed);
    EXPECT_EQ(settingsFileChanged, secondWritePerformed);
}

INSTANTIATE_TEST_SUITE_P(SettingsTest,
                         SettingsTest,
                         testing::Values(std::make_tuple(0, 0, false),
                                         std::make_tuple(0, 1, true)));

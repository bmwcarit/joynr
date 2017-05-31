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
#include <limits>

#include <gtest/gtest.h>

#include "joynr/Settings.h"
#include "joynr/ClusterControllerSettings.h"

using namespace joynr;

TEST(ClusterControllerSettingsTest, accessControlIsEnabled) {
    Settings testSettings("test-resources/CCSettingsWithAccessControlEnabled.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    // In the loaded setting file the access control is set to false
    EXPECT_TRUE(clusterControllerSettings.enableAccessController());
}

TEST(ClusterControllerSettingsTest, accessControlIsDisabled) {
    Settings testSettings("test-resources/CCSettingsWithAccessControlDisabled.settings");
    ASSERT_TRUE(testSettings.isLoaded());

    ClusterControllerSettings clusterControllerSettings(testSettings);

    // In the loaded setting file the access control is set to false
    EXPECT_FALSE(clusterControllerSettings.enableAccessController());
}

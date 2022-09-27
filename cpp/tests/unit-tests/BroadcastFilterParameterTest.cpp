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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/BroadcastFilterParameters.h"

using namespace joynr;

class BroadcastFilterParametersTest : public ::testing::Test
{
public:
    BroadcastFilterParametersTest()
    {
    }
};

TEST_F(BroadcastFilterParametersTest, compareFilterParameters)
{
    BroadcastFilterParameters param1;
    param1.setFilterParameter("firstEntry", "firstEntry");
    param1.setFilterParameter("secondentry", "sndEntry");

    BroadcastFilterParameters param2;
    param2.setFilterParameter("firstEntry", "firstEntry");
    param2.setFilterParameter("secondentry", "sndEntry2");

    EXPECT_FALSE(param1 == param2);

    BroadcastFilterParameters param3;
    param3.setFilterParameter("firstEntry", "firstEntry");
    param3.setFilterParameter("secondentry", "sndEntry");
    EXPECT_EQ(param3, param1);

    BroadcastFilterParameters param4;
    param4.setFilterParameter("secondentry", "sndEntry");
    param4.setFilterParameter("firstEntry", "firstEntry");
    EXPECT_EQ(param4, param1);
}

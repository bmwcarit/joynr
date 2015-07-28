/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "joynr/StdBroadcastFilterParameters.h"
#include "joynr/QtBroadcastFilterParameters.h"

using namespace joynr;

class BroadcastFilterParametersTest : public ::testing::Test {
public:
    BroadcastFilterParametersTest()
    {

    }
};


TEST_F(BroadcastFilterParametersTest, compareFilterParameters)
{
    StdBroadcastFilterParameters param1;
    param1.setFilterParameter("firstEntry", "firstEntry");
    param1.setFilterParameter("secondentry", "sndEntry");

    StdBroadcastFilterParameters param2;
    param2.setFilterParameter("firstEntry", "firstEntry");
    param2.setFilterParameter("secondentry", "sndEntry2");

    EXPECT_FALSE(param1 == param2);

    StdBroadcastFilterParameters param3;
    param3.setFilterParameter("firstEntry", "firstEntry");
    param3.setFilterParameter("secondentry", "sndEntry");
    EXPECT_EQ(param3, param1);

    StdBroadcastFilterParameters param4;
    param4.setFilterParameter("secondentry", "sndEntry");
    param4.setFilterParameter("firstEntry", "firstEntry");
    EXPECT_EQ(param4, param1);
}

TEST_F(BroadcastFilterParametersTest, correctConvertsionFromStdToQt)
{
    StdBroadcastFilterParameters stdParam;
    stdParam.setFilterParameter("firstEntry", "firstEntry");
    stdParam.setFilterParameter("secondentry", "sndEntry");

    QtBroadcastFilterParameters qtParam = QtBroadcastFilterParameters::createQt(stdParam);
    EXPECT_EQ(stdParam, QtBroadcastFilterParameters::createStd(qtParam));

    stdParam.setFilterParameter("aaaa", "b");
    qtParam = QtBroadcastFilterParameters::createQt(stdParam);
    EXPECT_EQ(stdParam, QtBroadcastFilterParameters::createStd(qtParam));
}

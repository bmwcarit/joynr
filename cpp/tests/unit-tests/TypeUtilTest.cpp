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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/TypeUtil.h"
#include <string>
#include <vector>

using namespace joynr;

class TypeUtilTest : public ::testing::Test {

};

TEST_F(TypeUtilTest, testUInt16ToIntConversion)
{

    std::uint16_t expectedValue(1);
    int testData = 1;

    std::uint16_t result = TypeUtil::toStdUInt16(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550;
    testData = 2550;
    result = TypeUtil::toStdUInt16(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToUInt16Conversion)
{

    int expectedValue(1);
    std::uint16_t testData = 1;

    int result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550;
    testData = 2550;
    result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testInt16ToIntConversion)
{

    std::int16_t expectedValue(-255);
    int testData = -255;

    std::int16_t result = TypeUtil::toStdInt16(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 3000;
    testData = 3000;
    result = TypeUtil::toStdInt16(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToInt16Conversion)
{

    int expectedValue(-11);
    std::int16_t testData = -11;

    int result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 3000;
    testData = 3000;
    result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testUInt32ToIntConversion)
{

    std::uint32_t expectedValue(1);
    int testData = 1;

    std::uint32_t result = TypeUtil::toStdUInt32(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550000;
    testData = 2550000;
    result = TypeUtil::toStdUInt32(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToUInt32Conversion)
{

    int expectedValue(1);
    std::uint32_t testData = 1;

    int result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550000;
    testData = 2550000;
    result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testInt32ToIntConversion)
{

    std::int32_t expectedValue(-255555);
    int testData = -255555;

    std::int32_t result = TypeUtil::toStdInt32(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255555;
    testData = 255555;
    result = TypeUtil::toStdInt32(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToInt32Conversion)
{

    int expectedValue(-2555551);
    std::int32_t testData = -2555551;

    int result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255555;
    testData = 255555;
    result = TypeUtil::toInt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testUInt64Toint64_tConversion)
{

    std::uint64_t expectedValue(1);
    std::int64_t testData = 1;

    std::uint64_t result = TypeUtil::toStdUInt64(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = TypeUtil::toStdUInt64(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testint64_tToUInt64Conversion)
{

    std::int64_t expectedValue(1);
    std::uint64_t testData = 1;

    std::int64_t result = TypeUtil::toStdInt64(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = TypeUtil::toStdInt64(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testInt64Toint64_tConversion)
{

    std::int64_t expectedValue(-1);
    std::int64_t testData = -1;

    std::int64_t result = TypeUtil::toStdInt64(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 10;
    testData = 10;
    result = TypeUtil::toStdInt64(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testFloatToDoubleConversion)
{

    float expectedValue(-1.03040);
    double testData = -1.03040;

    float result = TypeUtil::toStdFloat(testData);

    EXPECT_FLOAT_EQ(expectedValue, result);

    expectedValue = 10000000.1234;
    testData = 10000000.1234;
    result = TypeUtil::toStdFloat(testData);

    EXPECT_FLOAT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testDoubleToFloatConversion)
{

    double expectedValue(-1.03040);
    float testData = -1.03040;

    double result = TypeUtil::toDouble(testData);

    EXPECT_NEAR(expectedValue, result, 0.01);

    expectedValue = 1000000.1234;
    testData = 1000000.1234;
    result = TypeUtil::toDouble(testData);

    EXPECT_NEAR(expectedValue, result, 0.01);
}


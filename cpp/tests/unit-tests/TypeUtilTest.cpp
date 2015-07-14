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
#include "joynr/TypeUtil.h"
#include <QString>
#include <string>
#include <QByteArray>
#include <vector>

using namespace joynr;

class TypeUtilTest : public ::testing::Test {

};
TEST_F(TypeUtilTest, testQStringToStdStringConversion)
{

    std::string expectedValue("testData");
    QString testData(QString::fromStdString(expectedValue));

    std::string result = TypeUtil::toStd(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testQStringsToStdStringsConversion)
{

    std::string elem1("testData1");
    std::string elem2("testData2");

    QList<QString> testData;

    testData.append(QString::fromStdString(elem1));
    testData.append(QString::fromStdString(elem2));

    QList<std::string> result = TypeUtil::toStd(testData);

    EXPECT_TRUE(result.size() == 2);
    EXPECT_EQ(result.at(0), elem1);
    EXPECT_EQ(result.at(1), elem2);
}

TEST_F(TypeUtilTest, testStringToQStringConversion)
{

    QString expectedValue("testData");
    std::string testData(expectedValue.toStdString());

    QString result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testStdStringsToQStringsConversion)
{

    QString elem1("testData1");
    QString elem2("testData2");

    QList<std::string> testData;

    testData.append(elem1.toStdString());
    testData.append(elem2.toStdString());

    QList<QString> result = TypeUtil::toQt(testData);

    EXPECT_TRUE(result.size() == 2);
    EXPECT_EQ(result.at(0), elem1);
    EXPECT_EQ(result.at(1), elem2);
}

TEST_F(TypeUtilTest, testStringEnd2EndConversion)
{

    QString elem1("testData1");
    QString elem2("testData2");

    QList<QString> testData;

    testData.append(elem1);
    testData.append(elem2);

    QList<QString> results = TypeUtil::toQt(
                TypeUtil::toStd(testData));

    EXPECT_TRUE(results.size() == 2);
    EXPECT_EQ(results.at(0), elem1);
    EXPECT_EQ(results.at(1), elem2);

    QString result = TypeUtil::toQt(
                TypeUtil::toStd(elem1));

    EXPECT_EQ(result, elem1);
}

TEST_F(TypeUtilTest, testUInt8ToQInt8Conversion)
{

    uint8_t expectedValue(1);
    qint8 testData = 1;

    uint8_t result = TypeUtil::toStdUInt8(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = TypeUtil::toStdUInt8(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testQInt8ToUInt8Conversion)
{

    qint8 expectedValue(1);
    uint8_t testData = 1;

    qint8 result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testInt8ToQInt8Conversion)
{

    int8_t expectedValue(-1);
    qint8 testData = -1;

    int8_t result = TypeUtil::toStdInt8(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 10;
    testData = 10;
    result = TypeUtil::toStdInt8(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testQInt8ToInt8Conversion)
{

    qint8 expectedValue(-1);
    int8_t testData = -1;

    qint8 result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 10;
    testData = 10;

    result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testUInt16ToIntConversion)
{

    uint16_t expectedValue(1);
    int testData = 1;

    uint16_t result = TypeUtil::toStdUInt16(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550;
    testData = 2550;
    result = TypeUtil::toStdUInt16(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToUInt16Conversion)
{

    int expectedValue(1);
    uint16_t testData = 1;

    int result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550;
    testData = 2550;
    result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testInt16ToIntConversion)
{

    int16_t expectedValue(-255);
    int testData = -255;

    int16_t result = TypeUtil::toStdInt16(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 3000;
    testData = 3000;
    result = TypeUtil::toStdInt16(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToInt16Conversion)
{

    int expectedValue(-11);
    int16_t testData = -11;

    int result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 3000;
    testData = 3000;
    result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testUInt32ToIntConversion)
{

    uint32_t expectedValue(1);
    int testData = 1;

    uint32_t result = TypeUtil::toStdUInt32(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550000;
    testData = 2550000;
    result = TypeUtil::toStdUInt32(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToUInt32Conversion)
{

    int expectedValue(1);
    uint32_t testData = 1;

    int result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 2550000;
    testData = 2550000;
    result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testInt32ToIntConversion)
{

    int32_t expectedValue(-255555);
    int testData = -255555;

    int32_t result = TypeUtil::toStdInt32(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255555;
    testData = 255555;
    result = TypeUtil::toStdInt32(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testIntToInt32Conversion)
{

    int expectedValue(-2555551);
    int32_t testData = -2555551;

    int result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255555;
    testData = 255555;
    result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testUInt64ToQInt64Conversion)
{

    uint64_t expectedValue(1);
    qint64 testData = 1;

    uint64_t result = TypeUtil::toStdUInt64(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = TypeUtil::toStdUInt64(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testQInt64ToUInt64Conversion)
{

    qint64 expectedValue(1);
    uint64_t testData = 1;

    qint64 result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testInt64ToQInt64Conversion)
{

    int64_t expectedValue(-1);
    qint64 testData = -1;

    int64_t result = TypeUtil::toStdInt64(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 10;
    testData = 10;
    result = TypeUtil::toStdInt64(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testQInt64ToInt64Conversion)
{

    qint64 expectedValue(-1);
    int64_t testData = -1;

    qint64 result = TypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 10;
    testData = 10;
    result = TypeUtil::toQt(testData);

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

    double result = TypeUtil::toQt(testData);

    EXPECT_NEAR(expectedValue, result, 0.01);

    expectedValue = 1000000.1234;
    testData = 1000000.1234;
    result = TypeUtil::toQt(testData);

    EXPECT_NEAR(expectedValue, result, 0.01);
}

TEST_F(TypeUtilTest, testUInt8VectorToQByteArrayConversion)
{

    std::vector<uint8_t> expectedValue;

    expectedValue.push_back(3);
    expectedValue.push_back(2);
    expectedValue.push_back(1);

    QByteArray testData;

    testData.append(3);
    testData.append(2);
    testData.append(1);

    std::vector<uint8_t> result = TypeUtil::toStd(testData);
    EXPECT_EQ(expectedValue, result);
}

TEST_F(TypeUtilTest, testQByteArrayToUInt8VectorConversion)
{
    QByteArray expectedValue;

    expectedValue.append(3);
    expectedValue.append(2);
    expectedValue.append(1);

    std::vector<uint8_t> testData;

    testData.push_back(3);
    testData.push_back(2);
    testData.push_back(1);

    QByteArray result = TypeUtil::toQt(testData);
    EXPECT_EQ(expectedValue, result);
}

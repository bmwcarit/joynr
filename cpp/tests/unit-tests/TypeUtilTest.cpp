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

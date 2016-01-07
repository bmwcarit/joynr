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
#include "joynr/QtTypeUtil.h"
#include <QString>
#include <string>
#include <QByteArray>
#include <vector>

using namespace joynr;

class QtQtTypeUtilTest : public ::testing::Test {

};

TEST_F(QtQtTypeUtilTest, testQStringsToStdStringsConversion)
{

    std::string elem1("testData1");
    std::string elem2("testData2");

    QList<QString> testData;

    testData.append(QString::fromStdString(elem1));
    testData.append(QString::fromStdString(elem2));

    std::vector<std::string> result = QtTypeUtil::toStd(testData);

    EXPECT_TRUE(result.size() == 2);
    EXPECT_EQ(result.at(0), elem1);
    EXPECT_EQ(result.at(1), elem2);
}

TEST_F(QtQtTypeUtilTest, testStringToQStringConversion)
{

    QString expectedValue("testData");
    std::string testData(expectedValue.toStdString());

    QString result = QtTypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(QtQtTypeUtilTest, testStdStringsToQStringsConversion)
{

    QString elem1("testData1");
    QString elem2("testData2");

    std::vector<std::string> testData;

    testData.push_back(elem1.toStdString());
    testData.push_back(elem2.toStdString());

    QList<QString> result = QtTypeUtil::toQt(testData);

    EXPECT_TRUE(result.size() == 2);
    EXPECT_EQ(result.at(0), elem1);
    EXPECT_EQ(result.at(1), elem2);
}

TEST_F(QtQtTypeUtilTest, testStringEnd2EndConversion)
{

    QString elem1("testData1");
    QString elem2("testData2");

    QList<QString> testData;

    testData.append(elem1);
    testData.append(elem2);

    QList<QString> results = QtTypeUtil::toQt(
                QtTypeUtil::toStd(testData));

    EXPECT_TRUE(results.size() == 2);
    EXPECT_EQ(results.at(0), elem1);
    EXPECT_EQ(results.at(1), elem2);

    QString result = QtTypeUtil::toQt(
                QtTypeUtil::toStd(elem1));

    EXPECT_EQ(result, elem1);
}

TEST_F(QtQtTypeUtilTest, testUInt8ToQInt8Conversion)
{

    std::uint8_t expectedValue(1);
    qint8 testData = 1;

    std::uint8_t result = QtTypeUtil::toStdUInt8(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = QtTypeUtil::toStdUInt8(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(QtQtTypeUtilTest, testQInt8ToUInt8Conversion)
{

    qint8 expectedValue(1);
    std::uint8_t testData = 1;

    qint8 result = QtTypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 255;
    testData = 255;
    result = QtTypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(QtQtTypeUtilTest, testInt8ToQInt8Conversion)
{

    std::int8_t expectedValue(-1);
    qint8 testData = -1;

    std::int8_t result = QtTypeUtil::toStdInt8(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 10;
    testData = 10;
    result = QtTypeUtil::toStdInt8(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(QtQtTypeUtilTest, testQInt8ToInt8Conversion)
{

    qint8 expectedValue(-1);
    std::int8_t testData = -1;

    qint8 result = QtTypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);

    expectedValue = 10;
    testData = 10;

    result = QtTypeUtil::toQt(testData);

    EXPECT_EQ(expectedValue, result);
}

TEST_F(QtQtTypeUtilTest, testUInt8VectorToQByteArrayConversion)
{

    std::vector<std::uint8_t> expectedValue;

    expectedValue.push_back(3);
    expectedValue.push_back(2);
    expectedValue.push_back(1);

    QByteArray testData;

    testData.append(3);
    testData.append(2);
    testData.append(1);

    std::vector<std::uint8_t> result = QtTypeUtil::toStd(testData);
    EXPECT_EQ(expectedValue, result);
}

TEST_F(QtQtTypeUtilTest, testQByteArrayToUInt8VectorConversion)
{
    QByteArray expectedValue;

    expectedValue.append(3);
    expectedValue.append(2);
    expectedValue.append(1);

    std::vector<std::uint8_t> testData;

    testData.push_back(3);
    testData.push_back(2);
    testData.push_back(1);

    QByteArray result = QtTypeUtil::toQByteArray(testData);
    EXPECT_EQ(expectedValue, result);
}


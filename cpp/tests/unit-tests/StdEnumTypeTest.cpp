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
#include <gtest/gtest.h>

#include "joynr/tests/testTypes/TestEnum.h"
#include "joynr/tests/testTypes/TestEnumExtended.h"
#include "joynr/tests/EnumInsideInterface.h"
#include "joynr/tests/EnumInsideInterfaceWithoutVersion.h"
#include "joynr/types/TestTypesWithoutVersion/EnumInsideTypeCollectionWithoutVersion.h"

#include "tests/PrettyPrint.h"

using namespace joynr::tests::testTypes;

class StdEnumTypeTest : public testing::Test
{
public:
    StdEnumTypeTest()
            : testEnumZero(TestEnum::ZERO),
              testEnumZeroOther(TestEnum::ZERO),
              testEnumOne(TestEnum::ONE),
              testEnumOneOther(TestEnum::ONE),
              testEnumTwo(TestEnum::TWO)
    {
    }

    virtual ~StdEnumTypeTest() = default;

protected:
    TestEnum::Enum testEnumZero;
    TestEnum::Enum testEnumZeroOther;
    TestEnum::Enum testEnumOne;
    TestEnum::Enum testEnumOneOther;
    TestEnum::Enum testEnumTwo;
};

TEST_F(StdEnumTypeTest, createEnum)
{
    TestEnum::Enum testEnum(TestEnum::ONE);
    EXPECT_EQ(testEnum, TestEnum::ONE);
    EXPECT_TRUE(testEnum == TestEnum::ONE);
}

TEST_F(StdEnumTypeTest, assignEnum)
{
    TestEnum::Enum testEnum(TestEnum::ZERO);
    EXPECT_NE(testEnumOne, testEnum);
    testEnum = testEnumOne;
    EXPECT_EQ(testEnumOne, testEnum);
}

TEST_F(StdEnumTypeTest, assignEnumLiteral)
{
    TestEnum::Enum testEnum(TestEnum::ZERO);
    EXPECT_NE(TestEnum::ONE, testEnum);
    testEnum = TestEnum::ONE;
    EXPECT_EQ(TestEnum::ONE, testEnum);
}

TEST_F(StdEnumTypeTest, equalsOperator)
{
    EXPECT_EQ(testEnumZero, testEnumZeroOther);
    EXPECT_EQ(testEnumOne, testEnumOneOther);
}

TEST_F(StdEnumTypeTest, notEqualsOperator)
{
    EXPECT_NE(testEnumZero, testEnumOne);
}

TEST_F(StdEnumTypeTest, greaterThanOperator)
{
    EXPECT_GT(testEnumOne, testEnumZero);
}

TEST_F(StdEnumTypeTest, greaterEqualOperator)
{
    EXPECT_GE(testEnumOne, testEnumZero);
    EXPECT_GE(testEnumOne, testEnumOne);
}

TEST_F(StdEnumTypeTest, lessThanOperator)
{
    EXPECT_LT(testEnumZero, testEnumOne);
}

TEST_F(StdEnumTypeTest, lessEqualOperator)
{
    EXPECT_LE(testEnumZero, testEnumOne);
    EXPECT_LE(testEnumOne, testEnumOne);
}

TEST_F(StdEnumTypeTest, getEnum)
{
    std::string literal = TestEnum::getLiteral(TestEnum::ZERO);
    EXPECT_EQ(TestEnum::ZERO, TestEnum::getEnum(literal));
    std::uint32_t ordinal = TestEnum::getOrdinal(TestEnum::ZERO);
    EXPECT_EQ(ordinal, TestEnum::getEnum(literal));
}

TEST_F(StdEnumTypeTest, getTypeName)
{
    EXPECT_EQ("joynr.tests.testTypes.TestEnum", TestEnum::getTypeName());
}

TEST_F(StdEnumTypeTest, baseAndExtendedOrdinalsAreEqual)
{
    EXPECT_EQ(TestEnum::ZERO, TestEnumExtended::ZERO);
    TestEnumExtended::Enum exZero(TestEnumExtended::ZERO);
    EXPECT_EQ(testEnumZero, exZero);
// remember the current state of GCC diagnostics
#pragma GCC diagnostic push
// disable GCC enum-compare warning
// This test is to see if in principle comparison of base enums with extended enums is possible.
#pragma GCC diagnostic ignored "-Wenum-compare"
#pragma GCC diagnostic ignored "-Wsign-compare"
    EXPECT_TRUE(TestEnum::ZERO == TestEnumExtended::ZERO);
    EXPECT_TRUE(testEnumZero == exZero);
// restore previous state of GCC diagnostics
#pragma GCC diagnostic pop
}

TEST_F(StdEnumTypeTest, hash)
{
    std::hash<TestEnum::Enum> hashFunctionObj;
    EXPECT_EQ(hashFunctionObj(testEnumZero), hashFunctionObj(testEnumZeroOther));
    EXPECT_NE(hashFunctionObj(testEnumZero), hashFunctionObj(testEnumOne));
    EXPECT_NE(hashFunctionObj(testEnumZero), hashFunctionObj(testEnumTwo));
    EXPECT_EQ(hashFunctionObj(testEnumOne), hashFunctionObj(testEnumOneOther));
    EXPECT_NE(hashFunctionObj(testEnumOne), hashFunctionObj(testEnumTwo));
}

TEST_F(StdEnumTypeTest, versionIsSetInEnumInsideInterface)
{
    std::uint32_t expectedMajorVersion = 47;
    std::uint32_t expectedMinorVersion = 11;

    EXPECT_EQ(expectedMajorVersion, joynr::tests::EnumInsideInterface::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::tests::EnumInsideInterface::MINOR_VERSION);
}

TEST_F(StdEnumTypeTest, defaultVersionIsSetInEnumInsideInterfaceWithoutVersion)
{
    std::uint32_t expectedMajorVersion = 0;
    std::uint32_t expectedMinorVersion = 0;

    EXPECT_EQ(expectedMajorVersion, joynr::tests::EnumInsideInterfaceWithoutVersion::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::tests::EnumInsideInterfaceWithoutVersion::MINOR_VERSION);
}

TEST_F(StdEnumTypeTest, versionIsSetInEnumInsideTypeCollection)
{
    std::uint32_t expectedMajorVersion = 48;
    std::uint32_t expectedMinorVersion = 12;

    EXPECT_EQ(expectedMajorVersion, joynr::tests::testTypes::TestEnum::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::tests::testTypes::TestEnum::MINOR_VERSION);
}

TEST_F(StdEnumTypeTest, defaultVersionIsSetInEnumInsideTypeCollectionWithoutVersion)
{
    std::uint32_t expectedMajorVersion = 0;
    std::uint32_t expectedMinorVersion = 0;

    EXPECT_EQ(expectedMajorVersion,
              joynr::types::TestTypesWithoutVersion::EnumInsideTypeCollectionWithoutVersion::
                      MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion,
              joynr::types::TestTypesWithoutVersion::EnumInsideTypeCollectionWithoutVersion::
                      MINOR_VERSION);
}

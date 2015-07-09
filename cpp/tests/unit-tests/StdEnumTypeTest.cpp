/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "PrettyPrint.h"
#include "joynr/joynrlogging.h"

#include "joynr/tests/testTypes/StdTestEnum.h"
#include "joynr/tests/testTypes/StdTestEnumExtended.h"

using namespace joynr::tests::testTypes;

class StdEnumTypeTest : public testing::Test {
public:
    StdEnumTypeTest() :
            testEnumZero(StdTestEnum::ZERO),
            testEnumZeroOther(StdTestEnum::ZERO),
            testEnumOne(StdTestEnum::ONE),
            testEnumOneOther(StdTestEnum::ONE)
    {}

    virtual ~StdEnumTypeTest() {
    }

protected:
    static joynr::joynr_logging::Logger* logger;

    StdTestEnum::Enum testEnumZero;
    StdTestEnum::Enum testEnumZeroOther;
    StdTestEnum::Enum testEnumOne;
    StdTestEnum::Enum testEnumOneOther;
};

joynr::joynr_logging::Logger* StdEnumTypeTest::logger(
        joynr::joynr_logging::Logging::getInstance()->getLogger("TST", "StdEnumTypeTest")
);

TEST_F(StdEnumTypeTest, createEnum) {
    StdTestEnum::Enum testEnum(StdTestEnum::ONE);
    EXPECT_EQ(testEnum, StdTestEnum::ONE);
    EXPECT_TRUE(testEnum == StdTestEnum::ONE);
}

TEST_F(StdEnumTypeTest, assignEnum) {
    StdTestEnum::Enum testEnum(StdTestEnum::ZERO);
    EXPECT_NE(testEnumOne, testEnum);
    testEnum = testEnumOne;
    EXPECT_EQ(testEnumOne, testEnum);
}

TEST_F(StdEnumTypeTest, assignEnumLiteral) {
    StdTestEnum::Enum testEnum(StdTestEnum::ZERO);
    EXPECT_NE(StdTestEnum::ONE, testEnum);
    testEnum = StdTestEnum::ONE;
    EXPECT_EQ(StdTestEnum::ONE, testEnum);
}

TEST_F(StdEnumTypeTest, equalsOperator) {
    EXPECT_EQ(testEnumZero, testEnumZeroOther);
    EXPECT_EQ(testEnumOne, testEnumOneOther);
}

TEST_F(StdEnumTypeTest, notEqualsOperator) {
    EXPECT_NE(testEnumZero, testEnumOne);
}

TEST_F(StdEnumTypeTest, greaterThanOperator) {
    EXPECT_GT(testEnumOne, testEnumZero);
}

TEST_F(StdEnumTypeTest, greaterEqualOperator) {
    EXPECT_GE(testEnumOne, testEnumZero);
    EXPECT_GE(testEnumOne, testEnumOne);
}

TEST_F(StdEnumTypeTest, lessThanOperator) {
    EXPECT_LT(testEnumZero, testEnumOne);
}

TEST_F(StdEnumTypeTest, lessEqualOperator) {
    EXPECT_LE(testEnumZero, testEnumOne);
    EXPECT_LE(testEnumOne, testEnumOne);
}

TEST_F(StdEnumTypeTest, baseAndExtendedOrdinalsAreEqual) {
    EXPECT_EQ(StdTestEnum::ZERO, StdTestEnumExtended::ZERO);
    StdTestEnumExtended::Enum exZero(StdTestEnumExtended::ZERO);
    EXPECT_EQ(testEnumZero, exZero);
// remember the current state of GCC diagnostics
#pragma GCC diagnostic push
// disable GCC enum-compare warning
// This test is to see if in principle comparison of base enums with extended enums is possible.
#pragma GCC diagnostic ignored "-Wenum-compare"
    EXPECT_TRUE(StdTestEnum::ZERO == StdTestEnumExtended::ZERO);
    EXPECT_TRUE(testEnumZero == exZero);
// restore previous state of GCC diagnostics
#pragma GCC diagnostic pop
}

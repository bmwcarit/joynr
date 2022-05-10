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
#include <string>
#include <unordered_set>

#include <boost/math/special_functions/next.hpp>
#include "tests/utils/Gtest.h"

#include "joynr/ByteBuffer.h"
#include "joynr/types/TestTypes/Word.h"
#include "joynr/types/TestTypes/Vowel.h"
#include "joynr/types/TestTypes/TEverythingStruct.h"
#include "joynr/types/TestTypes/TEverythingExtendedStruct.h"
#include "joynr/types/TestTypes/TEverythingExtendedExtendedStruct.h"
#include "joynr/types/TestTypes/TEnum.h"
#include "joynr/tests/StructInsideInterface.h"
#include "joynr/tests/testTypes/ComplexTestType.h"
#include "joynr/tests/StructInsideInterfaceWithoutVersion.h"
#include "joynr/types/TestTypesWithoutVersion/StructInsideTypeCollectionWithoutVersion.h"

#include "tests/PrettyPrint.h"

using namespace joynr::types;

class StdComplexDataTypeTest : public testing::Test
{
public:
    StdComplexDataTypeTest()
            : tInt8(-20),
              tUInt8(0x42),
              tInt16(-32768),
              tUInt16(65535),
              tInt32(-2147483648),
              tUInt32(4294967295),
              tInt64(6294967295),
              tUInt64(-61474836480),
              tDouble(0.00000000001),
              tFloat(0.00001),
              tString("München"),
              tBoolean(true),
              tByteBuffer({0x01, 0x02, 0x03, 0x04, 0x05}),
              tUInt8Array({0xFF, 0xFE, 0xFD, 0xFC}),
              tEnum(TestTypes::TEnum::TLITERALA),
              tEnumArray({TestTypes::TEnum::TLITERALA, TestTypes::TEnum::TLITERALB}),
              tStringArray({"New York", "London", "Berlin", "Tokio"}),
              word(TestTypes::Word()),
              wordArray({TestTypes::Word()}),
              stringMap(),
              typeDefForTStruct(),
              tEverything1(tInt8,
                           tUInt8,
                           tInt16,
                           tUInt16,
                           tInt32,
                           tUInt32,
                           tInt64,
                           static_cast<std::uint64_t>(tUInt64),
                           tDouble,
                           tFloat,
                           tString,
                           tBoolean,
                           tByteBuffer,
                           tUInt8Array,
                           tEnum,
                           tEnumArray,
                           tStringArray,
                           word,
                           wordArray,
                           stringMap,
                           typeDefForTStruct),
              tBooleanExtended(false),
              tStringExtended("extended"),
              tEverythingExtended1(tInt8,
                                   tUInt8,
                                   tInt16,
                                   tUInt16,
                                   tInt32,
                                   tUInt32,
                                   tInt64,
                                   static_cast<std::uint64_t>(tUInt64),
                                   tDouble,
                                   tFloat,
                                   tString,
                                   tBoolean,
                                   tByteBuffer,
                                   tUInt8Array,
                                   tEnum,
                                   tEnumArray,
                                   tStringArray,
                                   word,
                                   wordArray,
                                   stringMap,
                                   typeDefForTStruct,
                                   tBooleanExtended,
                                   tStringExtended)
    {
    }

    virtual ~StdComplexDataTypeTest() = default;

protected:
    std::int8_t tInt8;
    std::uint8_t tUInt8;
    std::int16_t tInt16;
    std::uint16_t tUInt16;
    std::int32_t tInt32;
    std::uint32_t tUInt32;
    std::int64_t tInt64;
    std::int64_t tUInt64;
    double tDouble;
    float tFloat;
    std::string tString;
    bool tBoolean;
    joynr::ByteBuffer tByteBuffer;
    std::vector<std::uint8_t> tUInt8Array;
    TestTypes::TEnum::Enum tEnum;
    std::vector<joynr::types::TestTypes::TEnum::Enum> tEnumArray;
    std::vector<std::string> tStringArray;
    TestTypes::Word word;
    std::vector<TestTypes::Word> wordArray;
    TestTypes::TStringKeyMap stringMap;
    TestTypes::TypeDefForTStruct typeDefForTStruct;
    TestTypes::TEverythingStruct tEverything1;

    bool tBooleanExtended;
    std::string tStringExtended;
    TestTypes::TEverythingExtendedStruct tEverythingExtended1;
};

TEST_F(StdComplexDataTypeTest, createComplexDataType)
{
    EXPECT_EQ(tEverything1.getTBoolean(), tBoolean);
    EXPECT_EQ(tEverything1.getTByteBuffer(), tByteBuffer);
    EXPECT_EQ(tEverything1.getTDouble(), tDouble);
    EXPECT_EQ(tEverything1.getTEnum(), tEnum);
    EXPECT_EQ(tEverything1.getTEnumArray(), tEnumArray);
    EXPECT_EQ(tEverything1.getTEnumArray()[1], tEnumArray[1]);
    EXPECT_EQ(tEverything1.getTFloat(), tFloat);
    EXPECT_EQ(tEverything1.getTInt8(), tInt8);
    EXPECT_EQ(tEverything1.getTInt16(), tInt16);
    EXPECT_EQ(tEverything1.getTInt32(), tInt32);
    EXPECT_EQ(tEverything1.getTInt64(), tInt64);
    EXPECT_EQ(tEverything1.getTString(), tString);
    EXPECT_EQ(tEverything1.getTStringArray(), tStringArray);
    EXPECT_EQ(tEverything1.getTStringArray()[1], tStringArray[1]);
    EXPECT_EQ(tEverything1.getTUInt8(), tUInt8);
    EXPECT_EQ(tEverything1.getTUInt8Array(), tUInt8Array);
    EXPECT_EQ(tEverything1.getTUInt8Array()[1], tUInt8Array[1]);
    EXPECT_EQ(tEverything1.getTUInt16(), tUInt16);
    EXPECT_EQ(tEverything1.getTUInt32(), tUInt32);
    EXPECT_EQ(tEverything1.getTUInt64(), tUInt64);
}

TEST_F(StdComplexDataTypeTest, assignComplexDataType)
{
    joynr::types::TestTypes::TEverythingStruct tEverything2 = tEverything1;
    EXPECT_EQ(tEverything2, tEverything1);
    EXPECT_EQ(tEverything2.getTBoolean(), tBoolean);
    EXPECT_EQ(tEverything2.getTByteBuffer(), tByteBuffer);
    EXPECT_EQ(tEverything2.getTDouble(), tDouble);
    EXPECT_EQ(tEverything2.getTEnum(), tEnum);
    EXPECT_EQ(tEverything2.getTEnumArray(), tEnumArray);
    EXPECT_EQ(tEverything2.getTEnumArray()[1], tEnumArray[1]);
    EXPECT_EQ(tEverything2.getTFloat(), tFloat);
    EXPECT_EQ(tEverything2.getTInt8(), tInt8);
    EXPECT_EQ(tEverything2.getTInt16(), tInt16);
    EXPECT_EQ(tEverything2.getTInt32(), tInt32);
    EXPECT_EQ(tEverything2.getTInt64(), tInt64);
    EXPECT_EQ(tEverything2.getTString(), tString);
    EXPECT_EQ(tEverything2.getTStringArray(), tStringArray);
    EXPECT_EQ(tEverything2.getTStringArray()[1], tStringArray[1]);
    EXPECT_EQ(tEverything2.getTUInt8(), tUInt8);
    EXPECT_EQ(tEverything2.getTUInt8Array(), tUInt8Array);
    EXPECT_EQ(tEverything2.getTUInt8Array()[1], tUInt8Array[1]);
    EXPECT_EQ(tEverything2.getTUInt16(), tUInt16);
    EXPECT_EQ(tEverything2.getTUInt32(), tUInt32);
    EXPECT_EQ(tEverything2.getTUInt64(), tUInt64);
}

TEST_F(StdComplexDataTypeTest, copyConstructorComplexDataTypeCopiesValues)
{
    joynr::types::TestTypes::TEverythingStruct tEverything2(tEverything1);
    EXPECT_EQ(tEverything2, tEverything1);
    tEverything1.setTBoolean(!tEverything1.getTBoolean());
    EXPECT_NE(tEverything2, tEverything1);

    joynr::types::TestTypes::TEverythingStruct tEverything3(tEverything1);
    EXPECT_EQ(tEverything3, tEverything1);
    tEverything1.setTStringArray({"one", "two", "three"});
    EXPECT_NE(tEverything3, tEverything1);

    // make sure vectors are copied and not just assigned as references:
    // tEverything1 and tEverything4 should not have a reference or
    // pointer to changingVector
    std::vector<std::string> changingVector = {"one"};
    tEverything1.setTStringArray(changingVector);
    joynr::types::TestTypes::TEverythingStruct tEverything4(tEverything1);
    EXPECT_EQ(tEverything4, tEverything1);
    changingVector.push_back("two");
    tEverything4.setTStringArray(changingVector);
    EXPECT_NE(tEverything4, tEverything1);
}

TEST_F(StdComplexDataTypeTest, equalsOperator)
{
    EXPECT_EQ(tEverything1, tEverything1);

    joynr::types::TestTypes::TEverythingStruct tEverything2(tEverything1);
    EXPECT_EQ(tEverything2, tEverything1);
    EXPECT_EQ(tEverything2, tEverything1);
    EXPECT_EQ(tEverything2.getTBoolean(), tBoolean);
    EXPECT_EQ(tEverything2.getTByteBuffer(), tByteBuffer);
    EXPECT_EQ(tEverything2.getTDouble(), tDouble);
    EXPECT_EQ(tEverything2.getTEnum(), tEnum);
    EXPECT_EQ(tEverything2.getTEnumArray(), tEnumArray);
    EXPECT_EQ(tEverything2.getTEnumArray()[1], tEnumArray[1]);
    EXPECT_EQ(tEverything2.getTFloat(), tFloat);
    EXPECT_EQ(tEverything2.getTInt8(), tInt8);
    EXPECT_EQ(tEverything2.getTInt16(), tInt16);
    EXPECT_EQ(tEverything2.getTInt32(), tInt32);
    EXPECT_EQ(tEverything2.getTInt64(), tInt64);
    EXPECT_EQ(tEverything2.getTString(), tString);
    EXPECT_EQ(tEverything2.getTStringArray(), tStringArray);
    EXPECT_EQ(tEverything2.getTStringArray()[1], tStringArray[1]);
    EXPECT_EQ(tEverything2.getTUInt8(), tUInt8);
    EXPECT_EQ(tEverything2.getTUInt8Array(), tUInt8Array);
    EXPECT_EQ(tEverything2.getTUInt8Array()[1], tUInt8Array[1]);
    EXPECT_EQ(tEverything2.getTUInt16(), tUInt16);
    EXPECT_EQ(tEverything2.getTUInt32(), tUInt32);
    EXPECT_EQ(tEverything2.getTUInt64(), tUInt64);
}

TEST_F(StdComplexDataTypeTest, notEqualsOperator)
{
    joynr::types::TestTypes::TEverythingStruct tEverything2;
    EXPECT_NE(tEverything1, tEverything2);

    tEverything2 = tEverything1;
    EXPECT_EQ(tEverything1, tEverything2);

    tEverything1.setTBoolean(true);
    tEverything2.setTBoolean(false);
    EXPECT_NE(tEverything1, tEverything2);
}

TEST_F(StdComplexDataTypeTest, createExtendedComplexDataType)
{
    EXPECT_EQ(tEverythingExtended1.getTBoolean(), tBoolean);
    EXPECT_EQ(tEverythingExtended1.getTByteBuffer(), tByteBuffer);
    EXPECT_EQ(tEverythingExtended1.getTDouble(), tDouble);
    EXPECT_EQ(tEverythingExtended1.getTEnum(), tEnum);
    EXPECT_EQ(tEverythingExtended1.getTEnumArray(), tEnumArray);
    EXPECT_EQ(tEverythingExtended1.getTEnumArray()[1], tEnumArray[1]);
    EXPECT_EQ(tEverythingExtended1.getTFloat(), tFloat);
    EXPECT_EQ(tEverythingExtended1.getTInt8(), tInt8);
    EXPECT_EQ(tEverythingExtended1.getTInt16(), tInt16);
    EXPECT_EQ(tEverythingExtended1.getTInt32(), tInt32);
    EXPECT_EQ(tEverythingExtended1.getTInt64(), tInt64);
    EXPECT_EQ(tEverythingExtended1.getTString(), tString);
    EXPECT_EQ(tEverythingExtended1.getTStringArray(), tStringArray);
    EXPECT_EQ(tEverythingExtended1.getTStringArray()[1], tStringArray[1]);
    EXPECT_EQ(tEverythingExtended1.getTUInt8(), tUInt8);
    EXPECT_EQ(tEverythingExtended1.getTUInt8Array(), tUInt8Array);
    EXPECT_EQ(tEverythingExtended1.getTUInt8Array()[1], tUInt8Array[1]);
    EXPECT_EQ(tEverythingExtended1.getTUInt16(), tUInt16);
    EXPECT_EQ(tEverythingExtended1.getTUInt32(), tUInt32);
    EXPECT_EQ(tEverythingExtended1.getTUInt64(), tUInt64);
    EXPECT_EQ(tEverythingExtended1.getTUInt64(), tUInt64);
    EXPECT_EQ(tEverythingExtended1.getTBooleanExtended(), tBooleanExtended);
    EXPECT_EQ(tEverythingExtended1.getTStringExtended(), tStringExtended);
}

TEST_F(StdComplexDataTypeTest, equalsOperatorExtended)
{
    EXPECT_EQ(tEverythingExtended1, tEverythingExtended1);

    joynr::types::TestTypes::TEverythingExtendedStruct tEverythingExtended2(tEverythingExtended1);
    EXPECT_EQ(tEverythingExtended2, tEverythingExtended1);
    EXPECT_EQ(tEverythingExtended2.getTBoolean(), tBoolean);
    EXPECT_EQ(tEverythingExtended2.getTByteBuffer(), tByteBuffer);
    EXPECT_EQ(tEverythingExtended2.getTDouble(), tDouble);
    EXPECT_EQ(tEverythingExtended2.getTEnum(), tEnum);
    EXPECT_EQ(tEverythingExtended2.getTEnumArray(), tEnumArray);
    EXPECT_EQ(tEverythingExtended2.getTEnumArray()[1], tEnumArray[1]);
    EXPECT_EQ(tEverythingExtended2.getTFloat(), tFloat);
    EXPECT_EQ(tEverythingExtended2.getTInt8(), tInt8);
    EXPECT_EQ(tEverythingExtended2.getTInt16(), tInt16);
    EXPECT_EQ(tEverythingExtended2.getTInt32(), tInt32);
    EXPECT_EQ(tEverythingExtended2.getTInt64(), tInt64);
    EXPECT_EQ(tEverythingExtended2.getTString(), tString);
    EXPECT_EQ(tEverythingExtended2.getTStringArray(), tStringArray);
    EXPECT_EQ(tEverythingExtended2.getTStringArray()[1], tStringArray[1]);
    EXPECT_EQ(tEverythingExtended2.getTUInt8(), tUInt8);
    EXPECT_EQ(tEverythingExtended2.getTUInt8Array(), tUInt8Array);
    EXPECT_EQ(tEverythingExtended2.getTUInt8Array()[1], tUInt8Array[1]);
    EXPECT_EQ(tEverythingExtended2.getTUInt16(), tUInt16);
    EXPECT_EQ(tEverythingExtended2.getTUInt32(), tUInt32);
    EXPECT_EQ(tEverythingExtended2.getTUInt64(), tUInt64);
    EXPECT_EQ(tEverythingExtended2.getTBooleanExtended(), tBooleanExtended);
    EXPECT_EQ(tEverythingExtended2.getTStringExtended(), tStringExtended);

    tEverythingExtended2.setTBooleanExtended(!tEverythingExtended2.getTBooleanExtended());
    EXPECT_NE(tEverythingExtended2, tEverythingExtended1);
}

TEST_F(StdComplexDataTypeTest, equalsOperatorCompareSameClass)
{
    TestTypes::TEverythingExtendedStruct rhs = tEverythingExtended1;
    TestTypes::TEverythingExtendedStruct lhs = rhs;
    EXPECT_TRUE(lhs == rhs);
    EXPECT_FALSE(lhs != rhs);

    lhs.setTBoolean(!lhs.getTBoolean());
    EXPECT_FALSE(lhs == rhs);
    EXPECT_TRUE(lhs != rhs);
}

TEST_F(StdComplexDataTypeTest, equalsOperatorCompareWithReferenceToBase)
{
    const TestTypes::TEverythingExtendedStruct& rhs = tEverythingExtended1;
    const TestTypes::TEverythingStruct& lhs1 = rhs;
    EXPECT_TRUE(lhs1 == rhs);
    EXPECT_FALSE(lhs1 != rhs);

    TestTypes::TEverythingExtendedStruct tEverythingExtended2 = tEverythingExtended1;
    tEverythingExtended2.setTBoolean(!tEverythingExtended2.getTBoolean());
    const TestTypes::TEverythingStruct& lhs2 = tEverythingExtended2;
    EXPECT_FALSE(lhs2 == rhs);
    EXPECT_TRUE(lhs2 != rhs);
}

TEST_F(StdComplexDataTypeTest, equalsOperatorBaseCompareWithDerived)
{
    // intended object slicing:
    // only get those parts of TEverythingExtendedStruct which stem from TEverythingStruct
    TestTypes::TEverythingStruct rhs = tEverythingExtended1;
    TestTypes::TEverythingExtendedStruct lhs = tEverythingExtended1;
    EXPECT_FALSE(lhs == rhs);
    EXPECT_TRUE(lhs != rhs);
}

TEST_F(StdComplexDataTypeTest, assignExtendedComplexDataType)
{
    TestTypes::TEverythingExtendedStruct tEverythingExtended2;
    tEverythingExtended2 = tEverythingExtended1;
    EXPECT_EQ(tEverythingExtended1, tEverythingExtended2);
}

TEST_F(StdComplexDataTypeTest, copyExtendedComplexDataType)
{
    TestTypes::TEverythingExtendedStruct tEverythingExtended2(tEverythingExtended1);
    TestTypes::TEverythingExtendedStruct tEverythingExtended3 = tEverythingExtended2;
    EXPECT_EQ(tEverythingExtended1, tEverythingExtended2);
    EXPECT_EQ(tEverythingExtended2, tEverythingExtended3);
}

TEST_F(StdComplexDataTypeTest, equalsExtendedComplexDataTypeNotEqualBaseType)
{
    std::string tStringExtendedExtended("extendedextended");
    TestTypes::TEverythingExtendedExtendedStruct tEverythingExtendedExtended(
            tInt8,
            tUInt8,
            tInt16,
            tUInt16,
            tInt32,
            tUInt32,
            tInt64,
            static_cast<std::uint64_t>(tUInt64),
            tDouble,
            tFloat,
            tString,
            tBoolean,
            tByteBuffer,
            tUInt8Array,
            tEnum,
            tEnumArray,
            tStringArray,
            word,
            wordArray,
            stringMap,
            typeDefForTStruct,
            tBooleanExtended,
            tStringExtended,
            tStringExtendedExtended);

    EXPECT_NE(tEverything1, tEverythingExtended1);
    EXPECT_NE(tEverything1, tEverythingExtendedExtended);
    EXPECT_NE(tEverythingExtended1, tEverythingExtendedExtended);
    // currently a compile error
    //    EXPECT_NE(tEverythingExtendedExtended, tEverythingExtended1);
}

TEST_F(StdComplexDataTypeTest, hashCodeImplementation)
{
    std::unordered_set<TestTypes::TEverythingExtendedStruct> unorderedSet;

    auto returnedPair1 = unorderedSet.insert(tEverythingExtended1);
    EXPECT_TRUE(returnedPair1.second);

    auto returnedPair2 = unorderedSet.insert(tEverythingExtended1);
    EXPECT_FALSE(returnedPair2.second);
    EXPECT_EQ(unorderedSet.size(), 1);
}

TEST_F(StdComplexDataTypeTest, mapTypeListInitialization)
{
    TestTypes::TStringKeyMap map = {{"lorem", "ipsum"}, {"dolor", "sit"}};

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map["lorem"], "ipsum");
}

TEST_F(StdComplexDataTypeTest, versionIsSetInStructInsideInterface)
{
    std::uint32_t expectedMajorVersion = 47;
    std::uint32_t expectedMinorVersion = 11;

    EXPECT_EQ(expectedMajorVersion, joynr::tests::StructInsideInterface::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::tests::StructInsideInterface::MINOR_VERSION);
}

TEST_F(StdComplexDataTypeTest, defaultVersionIsSetInStructInsideInterfaceWithoutVersion)
{
    std::uint32_t expectedMajorVersion = 0;
    std::uint32_t expectedMinorVersion = 0;

    EXPECT_EQ(
            expectedMajorVersion, joynr::tests::StructInsideInterfaceWithoutVersion::MAJOR_VERSION);
    EXPECT_EQ(
            expectedMinorVersion, joynr::tests::StructInsideInterfaceWithoutVersion::MINOR_VERSION);
}

TEST_F(StdComplexDataTypeTest, versionIsSetInStructInsideTypeCollection)
{
    std::uint32_t expectedMajorVersion = 48;
    std::uint32_t expectedMinorVersion = 12;

    EXPECT_EQ(expectedMajorVersion, joynr::tests::testTypes::ComplexTestType::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, joynr::tests::testTypes::ComplexTestType::MINOR_VERSION);
}

TEST_F(StdComplexDataTypeTest, defaultVersionIsSetInStructInsideTypeCollectionWithoutVersion)
{
    std::uint32_t expectedMajorVersion = 0;
    std::uint32_t expectedMinorVersion = 0;

    EXPECT_EQ(expectedMajorVersion,
              joynr::types::TestTypesWithoutVersion::StructInsideTypeCollectionWithoutVersion::
                      MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion,
              joynr::types::TestTypesWithoutVersion::StructInsideTypeCollectionWithoutVersion::
                      MINOR_VERSION);
}

TEST_F(StdComplexDataTypeTest, compareFloatingPointValues)
{
    using namespace boost::math;
    TestTypes::TEverythingExtendedStruct struct1;
    const float floatValue1 = 1.0f;
    struct1.setTFloat(floatValue1);

    // 3 floating point values above floatValue1
    const float floatValue2 = float_next(float_next(float_next(floatValue1)));
    TestTypes::TEverythingExtendedStruct struct2;
    struct2.setTFloat(floatValue2);

    // operator== compares 4 ULPs
    EXPECT_TRUE(struct1 == struct2);

    // use a custom precision
    const std::size_t customMaxUlps = 2;
    EXPECT_FALSE(struct1.equals(struct2, customMaxUlps));
}

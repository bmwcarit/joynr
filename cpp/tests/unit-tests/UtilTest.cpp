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
#include <gmock/gmock.h>
#include "joynr/Util.h"
#include <vector>
#include <tuple>
#include <functional>
#include <string>
#include "joynr/types/TestTypes/TEverythingStruct.h"

using namespace joynr;

class UtilTest : public ::testing::Test {
protected:

    struct ExpandTuple {
        bool expandIntoThis(int arg1, float arg2, std::string arg3) {
            return arg1 == 23 && arg2 == 24.25 && arg3 == "Test";
        }
    };

    ExpandTuple expandTuple;
};

TEST_F(UtilTest, splitIntoJsonObjects)
{
    std::string inputStream;
    std::vector<std::string> result;

    inputStream = R"( not a valid Json )";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(0, result.size());

    inputStream = R"({"id":34})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(1, result.size());
    EXPECT_EQ(result.at(0), R"({"id":34})");

    inputStream = R"({"message":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"message":{one:two}})");
    EXPECT_EQ(result.at(1), R"({"id":35})");

    //payload may not contain { or } outside a string.
    inputStream = R"({"id":3{4})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(0, result.size());

    //  { within a string should be ok
    inputStream = R"({"messa{ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa{ge":{one:two}})");

    //  } within a string should be ok
    inputStream = R"({"messa}ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa}ge":{one:two}})");

    //  }{ within a string should be ok
    inputStream = R"({"messa}{ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa}{ge":{one:two}})");

    //  {} within a string should be ok
    inputStream = R"({"messa{}ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa{}ge":{one:two}})");

    //string may contain \"
    inputStream = R"({"mes\"sa{ge":{one:two}}{"id":35})";
    //inputStream:{"mes\"sa{ge":{one:two}}{"id":35}
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"mes\"sa{ge":{one:two}})");


    inputStream = R"({"mes\\"sa{ge":{one:two}}{"id":35})";
    // inputStream: {"mes\\"sa{ge":{one:two}}{"id":35}
    // / does not escape within JSON String, so the string should not be ended after mes\"
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"mes\\"sa{ge":{one:two}})");
}

TEST_F(UtilTest, convertVectorToVariantVector){

    std::vector<int> intVector;
    std::vector<Variant> variantVector;

    intVector.push_back(2);
    intVector.push_back(5);
    intVector.push_back(-1);

    variantVector.push_back(Variant::make<int>(2));
    variantVector.push_back(Variant::make<int>(5));
    variantVector.push_back(Variant::make<int>(-1));

    std::vector<Variant> convertedVariantVector = util::convertVectorToVariantVector<int>(intVector);
    std::vector<int> convertedIntVector = util::convertVariantVectorToVector<int>(variantVector);

    EXPECT_EQ(convertedVariantVector, variantVector);
    EXPECT_EQ(convertedIntVector, intVector);

    std::vector<Variant> reconvertedVariantVector = util::convertVectorToVariantVector<int>(convertedIntVector);
    std::vector<int> reconvertedIntVector = util::convertVariantVectorToVector<int>(convertedVariantVector);

    EXPECT_EQ(reconvertedVariantVector, variantVector);
    EXPECT_EQ(reconvertedIntVector, intVector);


}

TEST_F(UtilTest, typeIdSingleType) {
    EXPECT_EQ(0, util::getTypeId<void>());
    EXPECT_GT(util::getTypeId<std::string>(), 0);
    EXPECT_NE(util::getTypeId<std::string>(), util::getTypeId<std::int32_t>());
}

TEST_F(UtilTest, typeIdCompositeType){
    int typeId1 = util::getTypeId<std::string, std::int32_t, float>();
    EXPECT_GT(typeId1, 0);

    int typeId2 = util::getTypeId<std::int32_t, std::string, float>();
    EXPECT_NE(typeId1, typeId2);
    int typeIdTEverythingStruct = util::getTypeId<joynr::types::TestTypes::TEverythingStruct>();
    EXPECT_GT(typeIdTEverythingStruct, 0);
    EXPECT_NE(typeId1, typeIdTEverythingStruct);
    EXPECT_NE(typeId2, typeIdTEverythingStruct);
}

TEST_F(UtilTest, typeIdVector){
    int typeIdVectorOfInt = util::getTypeId<std::vector<std::int32_t>>();
    EXPECT_NE(typeIdVectorOfInt, 0);

    int typeIdVectorOfTEverythingStruct = util::getTypeId<std::vector<joynr::types::TestTypes::TEverythingStruct>>();
    EXPECT_NE(typeIdVectorOfTEverythingStruct, 0);
    EXPECT_NE(typeIdVectorOfInt, typeIdVectorOfTEverythingStruct);
}

TEST_F(UtilTest, valueOfFloatVector){
    std::vector<float> expectedFloatVector = {1.1f, 1.2f, 1.3f};

    std::vector<Variant> variantVector;
    for(std::size_t i = 0; i<expectedFloatVector.size(); i++){
        variantVector.push_back(Variant::NULL_VARIANT());
    }
    std::transform(
                expectedFloatVector.cbegin(),
                expectedFloatVector.cend(),
                variantVector.begin(),
                [](const float value) { return Variant::make<double>(value); }
    );
    std::vector<float> floatVector = util::valueOf<std::vector<float>>(Variant::make<std::vector<Variant>>(variantVector));
    for(std::size_t i = 0; i < expectedFloatVector.size(); i++) {
        EXPECT_EQ(expectedFloatVector[i], floatVector[i]);
    }
}

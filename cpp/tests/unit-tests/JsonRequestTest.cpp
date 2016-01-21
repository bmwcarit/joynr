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
#include <vector>


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "joynr/Request.h"

using ::testing::A;
using ::testing::_;
using ::testing::A;
using ::testing::Eq;
using ::testing::AllOf;
using ::testing::Property;
using namespace joynr;

class MockArgument  {
public:
    MockArgument() : str1(""), list1(){}
    virtual ~MockArgument() = default;

    MockArgument(const MockArgument& mockArgument) :
        str1(mockArgument.str1),
        list1(mockArgument.list1)
    {}

    MockArgument(std::string atr1, std::vector<std::string>& array1):
        str1(atr1),
        list1(array1)
    {}

    bool operator== (const MockArgument& other) const {
        if (other.str1 == this->str1 && other.list1 == this->list1) {
            return true;
        } else {
            return false;
        }
    }
    bool operator!=(const MockArgument &other) const {
      return !(*this == other);
    }

    std::string str1;
    std::vector<std::string> list1;
};

bool isMockArgumentRegistered = Variant::registerType<MockArgument>("MockArgument");

class JsonRequestTest : public ::testing::Test {
public:
    JsonRequestTest() :
        list(),
        operationName(),
        arg1(),
        arg2(),
        arg3(),
        valueOfArg1(),
        valueOfArg2(),
        valueOfArg3()
    {};

    void SetUp(){
        list.push_back("1");
        list.push_back("2");
        list.push_back("3");
        list.push_back("4");

        operationName = "operation";
        arg1 = "arg1";
        arg2 = "arg2";
        arg3 = "arg3";
        valueOfArg1 = "valueOfArg1";
        valueOfArg2.str1 = std::string("mockargumentStringValue");
        valueOfArg2.list1 = list;
        valueOfArg3 = "valueOfArg3";
    }

    void TearDown(){

    }

    void checkJsonRequest(Request jsonRequest) {
        ASSERT_EQ(operationName, jsonRequest.getMethodName());
        std::vector<Variant> params = jsonRequest.getParams();
        ASSERT_EQ(3, params.size());

        ASSERT_EQ(valueOfArg1, params.at(0).get<std::string>());
        // arg2 is a custom type, so need to extract QVariant value
        ASSERT_EQ(valueOfArg2, params.at(1).get<MockArgument>());
        ASSERT_EQ(valueOfArg3, params.at(2).get<std::string>());
    }

protected:
    std::vector<std::string> list;
    std::string operationName;
    std::string arg1;
    std::string arg2;
    std::string arg3;

    std::string valueOfArg1;
    MockArgument valueOfArg2;
    std::string valueOfArg3;
};


typedef JsonRequestTest JsonRequestDeathTest;



TEST_F(JsonRequestTest, buildJsonRequest)
{
    // Build the argument list
    std::vector<Variant> args;
    args.push_back(Variant::make<std::string>(valueOfArg1));
    args.push_back(Variant::make<MockArgument>(valueOfArg2));
    args.push_back(Variant::make<std::string>(valueOfArg3));

    // Build the request
    Request jsonRequest;
    jsonRequest.setMethodName(operationName);
    jsonRequest.setParams(args);

    checkJsonRequest(jsonRequest);
}

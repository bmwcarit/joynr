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
#include "joynr/Variant.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/JsonTokenizer.h"
#include "ExampleTypes.h"
#include "joynr/Request.h"
#include "jsonserializer/RequestSerializer.h"
#include "joynr/joynrlogging.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <vector>
#include <utility>
#include <iostream>
#include <string>
#include <cassert>
#include <initializer_list>

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/types/TestTypes/TEverythingStruct.h"
#include "joynr/JsonSerializer.h"

#include <QString>

using namespace ::testing;
using namespace joynr;

//---- Example usage -----------------------------------------------------------
class JoynrJsonSerializerTest : public ::testing::Test {

public:
    JoynrJsonSerializerTest(){}
    void SetUp() {
    }
protected:
    static joynr::joynr_logging::Logger* logger;
};

joynr::joynr_logging::Logger* JoynrJsonSerializerTest::logger(
        joynr::joynr_logging::Logging::getInstance()->getLogger("TST", "JoynrJsonSerializerTest")
);

TEST_F(JoynrJsonSerializerTest, exampleVariant)
{
    // Create a datastructure
    std::vector<Variant> variants;

    variants.push_back(Variant::make<SomeType>());
    variants.push_back(Variant::make<SomeOtherType>(2));
    variants.push_back(Variant::make<int>(3));
    variants.push_back(Variant::make<std::string>("Hello World"));

    variants.push_back(Variant::make<std::vector<SomeOtherType>>
                       (std::initializer_list<SomeOtherType>{
                            SomeOtherType(1),
                            SomeOtherType(2),
                            SomeOtherType(3),
                            SomeOtherType(4),
                            SomeOtherType(5)}));

    // Use the datastructure
    for (auto& v : variants) {
        if (v.is<SomeType>()) {
            std::cout << "Variant is SomeType" << std::endl;
        } else if (v.is<SomeOtherType>()) {
            std::cout << "Variant is SomeOtherType value "
                      << v.get<SomeOtherType>().getA() << std::endl;
        } else if (v.is<int>()) {
            std::cout << "Variant is int value "
                      << v.get<int>() << std::endl;
        } else if (v.is<std::string>()) {
            std::cout << "Variant is std::string value "
                      << v.get<std::string>() << std::endl;
        } else if (v.is<std::vector<SomeOtherType>>()) {
            std::cout << "Variant is a data structure ";
            auto& vec = v.get<std::vector<SomeOtherType>>();
            for (auto& i : vec) {
                std::cout << i.getA() << " ";
            }
            std::cout << std::endl;
        }
    }

}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerKnownType)
{
    std::string json(R"({"a": 5, "b": 6})"); // raw string literal
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        SomeOtherType t;
        ClassDeserializer<SomeOtherType>::deserialize(t, tokenizer.nextObject());
        LOG_TRACE(logger, QString("Deserialized value is %1").arg(t.getA()));
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerUnknownType)
{
    std::string json(R"({"_typeName": "joynr.SomeOtherType","a": 123})");
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        auto variant = deserialize(tokenizer.nextObject());
        assert(variant.is<SomeOtherType>());
        LOG_TRACE(logger, QString("Unknown type deserialized value is %1").arg(variant.get<SomeOtherType>().getA()));
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerValue)
{
    std::string json(R"([{"_typeName": "joynr.SomeOtherType","a": 123},{"_typeName": "joynr.SomeOtherType","a": 456}])"); // raw string literal
    LOG_TRACE(logger, QString("json: %1").arg(QString::fromStdString(json)));
    std::vector<SomeOtherType*> vector = JsonSerializer::deserializeVector<SomeOtherType>(json);
    EXPECT_EQ(vector.size(), 2);
    EXPECT_EQ(vector.at(0)->getA(), 123);
    EXPECT_EQ(vector.at(1)->getA(), 456);

    std::vector<Variant> variantVector;
    variantVector.push_back(Variant::make<SomeOtherType>(*vector.at(0)));
    variantVector.push_back(Variant::make<SomeOtherType>(*vector.at(1)));
    std::string variantVectorStr = JsonSerializer::serializeVector(variantVector);
    LOG_TRACE(logger, QString("variantVector: %1").arg(QString::fromStdString(variantVectorStr)));
    EXPECT_EQ(json, variantVectorStr);

    // Clean up
    for (SomeOtherType* p : vector) {
        delete p;
    }
}

std::string convertPermission(ExamplePermission::Enum e)
{
    switch (e) {
    case ExamplePermission::YES:
        return "YES";
    case ExamplePermission::ASK:
        return "ASK";
    case ExamplePermission::NO:
        return "NO";
    default:
        return "UNKNOWN";
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrType)
{
    std::string json(R"({"_typeName":"joynr.infrastructure.ExampleMasterAccessControlEntry","defaultConsumerPermission":"YES","operation":"*","possibleConsumerPermissions":["YES","NO"]})");
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        auto variant = deserialize(tokenizer.nextObject());
        assert(variant.is<ExampleMasterAccessControlEntry>());
        ExampleMasterAccessControlEntry& entry = variant.get<ExampleMasterAccessControlEntry>();
        LOG_TRACE(logger, QString("ExampleMasterAccessControlEntry JSON: %1").arg(QString::fromStdString(json)));
        LOG_TRACE(logger, QString("ExampleMasterAccessControlEntry operation: %1").arg(QString::fromStdString(entry.getOperation())));
        LOG_TRACE(logger, QString("ExampleMasterAccessControlEntry defaultConsumerPermission: %1").arg(QString::fromStdString(convertPermission(entry.getDefaultConsumerPermission()))));
        std::stringstream strStream;
        for (auto& i : entry.getPossibleConsumerPermissions()) {
            strStream << convertPermission(i) << " ";
        }
        LOG_TRACE(logger, QString("ExampleMasterAccessControlEntry possibleConsumerPermissions: %1").arg(QString::fromStdString(strStream.str())));
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrRequest)
{
    // Create a Request
    Request expectedRequest;
    expectedRequest.setMethodName("realMethod");
    expectedRequest.setRequestReplyId("000-10000-01011");

    // Create a vector of variants to use as parameters
    std::string someString{"Hello World"};
    Variant param1 = Variant::make<std::string>(someString);
    expectedRequest.addParam(param1, "String");
    const int32_t expectedInt = 101;
    Variant param2 = Variant::make<int>(expectedInt);
    expectedRequest.addParam(param2, "Integer");
    SomeOtherType expectedSomeOtherType(2);
    Variant param3 = Variant::make<SomeOtherType>(expectedSomeOtherType);
    expectedRequest.addParam(param3, "SomeOtherType");
    const float expectedFloat = 9.99f;
    Variant param4 = Variant::make<float>(expectedFloat);
    expectedRequest.addParam(param4, "Float");
    bool expectedBool = true;
    Variant param5 = Variant::make<bool>(expectedBool);
    expectedRequest.addParam(param5, "Bool");

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<Request>();
    serializer.serialize(expectedRequest, stream);
    std::string json{ stream.str() };
    LOG_TRACE(logger, QString("Request JSON: %1").arg(QString::fromStdString(json)));

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        Request request;
        ClassDeserializer<Request>::deserialize(request, tokenizer.nextObject());
        std::vector<Variant> params = request.getParams();
        EXPECT_EQ(expectedRequest.getParams().size(), params.size());
        Variant intParam = params[1];
        EXPECT_TRUE(intParam.is<uint64_t>());
        EXPECT_FALSE(intParam.is<int32_t>());
        EXPECT_EQ(expectedInt, static_cast<int32_t>(intParam.get<uint64_t>()));
        Variant someOtherTypeParam = params[2];
        EXPECT_TRUE(someOtherTypeParam.is<SomeOtherType>());
        EXPECT_EQ(expectedSomeOtherType.getA(), someOtherTypeParam.get<SomeOtherType>().getA());
        std::cout << "Deserialized value is " << someOtherTypeParam.get<SomeOtherType>().getA() << std::endl;
        Variant floatParam = params[3];
        EXPECT_TRUE(floatParam.is<double>());
        EXPECT_FALSE(floatParam.is<float>());
        EXPECT_EQ(expectedFloat, static_cast<float>(floatParam.get<double>()));
        Variant boolParam = params[4];
        EXPECT_TRUE(boolParam.is<bool>());
        EXPECT_EQ(expectedBool, boolParam.get<bool>());
    }
}

// test with real MasterAccessControlEntry
TEST_F(JoynrJsonSerializerTest, serializeDeserializeMasterAccessControlEntry)
{
    using namespace joynr::infrastructure::DacTypes;

    std::vector<TrustLevel::Enum> possibleTrustLevels(
                std::initializer_list<TrustLevel::Enum>{
                    TrustLevel::LOW,
                    TrustLevel::MID,
                    TrustLevel::HIGH
                });

    std::vector<Permission::Enum> possiblePermissions(
                std::initializer_list<Permission::Enum>{
                    Permission::NO,
                    Permission::ASK,
                    Permission::YES
                });

    MasterAccessControlEntry expectedMac(R"(*)",
                                                         R"(unittest)",
                                                         R"(vehicle/radio)",
                                                         TrustLevel::LOW,
                                                         possibleTrustLevels,
                                                         TrustLevel::HIGH,
                                                         possibleTrustLevels,
                                                         R"(*)",
                                                         Permission::YES,
                                                         possiblePermissions);

    // Serialize
    std::stringstream stream;
    auto serializer = ClassSerializer<MasterAccessControlEntry>{};
    serializer.serialize(expectedMac, stream);
    std::string json{ stream.str() };

    LOG_TRACE(logger, QString("MAC JSON: %1").arg(QString::fromStdString(json)));

    // Deserialize
    MasterAccessControlEntry mac;
    JsonTokenizer tokenizer(json);
    if (tokenizer.hasNextObject()) {
        ClassDeserializer<MasterAccessControlEntry>::deserialize(mac, tokenizer.nextObject());
    }

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMac.getDefaultConsumerPermission(), mac.getDefaultConsumerPermission());
    EXPECT_EQ(expectedMac.getDefaultRequiredControlEntryChangeTrustLevel(), mac.getDefaultRequiredControlEntryChangeTrustLevel());
    EXPECT_EQ(expectedMac.getDefaultRequiredTrustLevel(), mac.getDefaultRequiredTrustLevel());
    EXPECT_EQ(expectedMac.getDomain(), mac.getDomain());
    EXPECT_EQ(expectedMac.getInterfaceName(), mac.getInterfaceName());
    EXPECT_EQ(expectedMac.getOperation(), mac.getOperation());
    EXPECT_EQ(expectedMac.getPossibleConsumerPermissions(), mac.getPossibleConsumerPermissions());
    EXPECT_EQ(expectedMac.getPossibleRequiredControlEntryChangeTrustLevels(), mac.getPossibleRequiredControlEntryChangeTrustLevels());
    EXPECT_EQ(expectedMac.getPossibleRequiredTrustLevels(), mac.getPossibleRequiredTrustLevels());
    EXPECT_EQ(expectedMac.getUid(), mac.getUid());
    EXPECT_EQ(expectedMac, mac);
}

// test with TEverythingStruct
TEST_F(JoynrJsonSerializerTest, serializeDeserializeTEverythingStruct)
{
    using namespace joynr::types::TestTypes;

    std::vector<uint8_t> byteBuffer(
                std::initializer_list<uint8_t>{
                    1,2,3
                });
    std::vector<uint8_t> uInt8Array(
                std::initializer_list<uint8_t>{
                    3,2,1
                });
    std::vector<TEnum::Enum> enumArray(
                std::initializer_list<TEnum::Enum>{
                    TEnum::TLITERALB
                });
    std::vector<std::string> stringArray(
                std::initializer_list<std::string>{
                    "one", "four"
                });
    std::vector<Vowel::Enum> vowelinies(
                std::initializer_list<Vowel::Enum>{
                    Vowel::A,
                    Vowel::E
                });
    Word wordWithVowelinies{vowelinies};
    Word wordEmpty{};
    std::vector<Word> words(
                std::initializer_list<Word>{
                    wordWithVowelinies,
                    wordEmpty
                });

    TEverythingStruct expectedEverythingStruct{
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9.012,
        10.101f,
        "const std::string &tString",
        true,
        byteBuffer,
        uInt8Array,
        TEnum::TLITERALA,
        enumArray,
        stringArray,
        wordWithVowelinies,
        words
    };

    // Serialize
    std::stringstream stream;
    auto serializer = ClassSerializer<TEverythingStruct>{};
    serializer.serialize(expectedEverythingStruct, stream);
    std::string json{ stream.str() };

    // TODO: replace with logging
    LOG_TRACE(logger, QString("TEverythingStruct JSON: %1").arg(QString::fromStdString(json)));

    // Deserialize
    TEverythingStruct* everythingStruct = JsonSerializer::deserialize<TEverythingStruct>(json);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedEverythingStruct, *everythingStruct);
    delete everythingStruct;
}

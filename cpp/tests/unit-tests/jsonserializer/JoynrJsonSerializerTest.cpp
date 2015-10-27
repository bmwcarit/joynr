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
#include "jsonserializer/JsonTokenizer.h"
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
    Request request;
    request.setMethodName("realMethod");
    request.setRequestReplyId("000-10000-01011");

    // Create a vector of variants to use as parameters
    std::vector<Variant> params;
    std::string someString{"Hello World"};
    Variant param1 = Variant::make<std::string>(someString);
    //params.emplace_back(param);
    request.addParam(param1, "String");
    const int32_t expectedInt = 101;
    Variant param2 = Variant::make<int>(expectedInt);
    request.addParam(param2, "Integer");
    const int aValue = 2;
    Variant param3 = Variant::make<SomeOtherType>(aValue);
    request.addParam(param3, "SomeOtherType");
    const float expectedFloat = 9.99f;
    Variant param4 = Variant::make<float>(expectedFloat);
    request.addParam(param4, "Float");
    bool expectedBool = true;
    Variant param5 = Variant::make<bool>(expectedBool);
    request.addParam(param5, "Bool");

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<Request>{};
    serializer.serialize(request, stream);
    std::string json{ stream.str() };
    LOG_TRACE(logger, QString("Request JSON: %1").arg(QString::fromStdString(json)));

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        Request t;
        ClassDeserializer<Request>::deserialize(t, tokenizer.nextObject());
        const auto& params = t.getParams();
        assert(params.size() == 5);
        const auto& second = params[1];
        assert(second.is<std::string>());
        assert(second.get<std::string>() == std::to_string(expectedInt));
        const auto& third = params[2];
        assert(third.is<SomeOtherType>());
        assert(third.get<SomeOtherType>().getA() == aValue);
        std::cout << "Deserialized value is " << third.get<SomeOtherType>().getA() << std::endl;
        const auto& fourth = params[3];
        assert(fourth.is<std::string>());
        float receivedFloat = strtof(fourth.get<std::string>().c_str(), nullptr);
        assert(receivedFloat == expectedFloat);
        const auto& fifth = params[4];
        assert(fifth.is<std::string>());
        std::string boolStr{fifth.get<std::string>()};
        bool receivedBool = boolStr == "1" ? true : false;
        assert(receivedBool == expectedBool);
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
    TEverythingStruct everythingStruct;
    JsonTokenizer tokenizer(json);
    if (tokenizer.hasNextObject()) {
        ClassDeserializer<TEverythingStruct>::deserialize(everythingStruct, tokenizer.nextObject());
    }

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedEverythingStruct, everythingStruct);
}

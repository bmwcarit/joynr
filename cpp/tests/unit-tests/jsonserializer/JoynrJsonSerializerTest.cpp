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
#include "joynr/Reply.h"
#include "jsonserializer/RequestSerializer.h"
#include "jsonserializer/ReplySerializer.h"
#include "joynr/Logger.h"
#include "joynr/SubscriptionPublication.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <vector>
#include <utility>
#include <iostream>
#include <string>
#include <cassert>
#include <initializer_list>
#include <functional>

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/types/TestTypes/TEverythingStruct.h"
#include "joynr/types/TestTypes/TStruct.h"
#include "joynr/types/TestTypes/TEnum.h"
#include "joynr/types/TestTypes/TEverythingMap.h"
#include "joynr/types/TestTypes/TStringKeyMap.h"
#include "joynr/types/TestTypes/TIntegerKeyMap.h"
#include "joynr/types/TestTypes/TDoubleKeyMap.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/MessagingQos.h"
#include "joynr/JsonSerializer.h"
#include "joynr/tests/test/MethodWithErrorEnumExtendedErrorEnum.h"
#include "joynr/tests/test/MethodWithErrorEnumExtendedErrorEnumSerializer.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"

using namespace ::testing;
using namespace joynr;

//---- Example usage -----------------------------------------------------------
class JoynrJsonSerializerTest : public ::testing::Test {

public:
    JoynrJsonSerializerTest() = default;
    void SetUp() {
    }
protected:
    ADD_LOGGER(JoynrJsonSerializerTest);
    void testSerializationOfTStruct(joynr::types::TestTypes::TStruct expectedStruct);
private:
};

INIT_LOGGER(JoynrJsonSerializerTest);

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
        JOYNR_LOG_TRACE(logger, "Deserialized value is {}",t.getA());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerUnknownType)
{
    std::string json(R"({"_typeName":"joynr.SomeOtherType","a": 123})");
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        auto variant = deserialize(tokenizer.nextObject());
        assert(variant.is<SomeOtherType>());
        JOYNR_LOG_TRACE(logger, "Unknown type deserialized value is {}",variant.get<SomeOtherType>().getA());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerValue)
{
    std::string json(R"([{"_typeName":"joynr.SomeOtherType","a": 123},{"_typeName":"joynr.SomeOtherType","a": 456}])"); // raw string literal
    JOYNR_LOG_TRACE(logger, "json: {}",json);
    std::vector<SomeOtherType*> vector = JsonSerializer::deserializeVector<SomeOtherType>(json);
    EXPECT_EQ(vector.size(), 2);
    EXPECT_EQ(vector.at(0)->getA(), 123);
    EXPECT_EQ(vector.at(1)->getA(), 456);

    std::vector<Variant> variantVector;
    variantVector.push_back(Variant::make<SomeOtherType>(*vector.at(0)));
    variantVector.push_back(Variant::make<SomeOtherType>(*vector.at(1)));
    std::string variantVectorStr = JsonSerializer::serializeVector(variantVector);
    JOYNR_LOG_TRACE(logger, "variantVector: {}",variantVectorStr);
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
        JOYNR_LOG_TRACE(logger, "ExampleMasterAccessControlEntry JSON: {}",json);
        JOYNR_LOG_TRACE(logger, "ExampleMasterAccessControlEntry operation: {}",entry.getOperation());
        JOYNR_LOG_TRACE(logger, "ExampleMasterAccessControlEntry defaultConsumerPermission: {}",convertPermission(entry.getDefaultConsumerPermission()));
        std::stringstream strStream;
        for (auto& i : entry.getPossibleConsumerPermissions()) {
            strStream << convertPermission(i) << " ";
        }
        JOYNR_LOG_TRACE(logger, "ExampleMasterAccessControlEntry possibleConsumerPermissions: {}",strStream.str());
    }
}

void initializeRequestWithComplexValues(Request& request) {
    using namespace types::TestTypes;
    request.setMethodName("methodWithComplexParameters");
    request.setRequestReplyId("000-10000-01100");

    Variant param1 = Variant::make<TEverythingStruct>(TEverythingStruct());
    Variant param2 = Variant::make<TEverythingExtendedStruct>(TEverythingExtendedStruct());
    Variant param3 = Variant::make<TDoubleKeyMap>(TDoubleKeyMap());
    Variant param4 = Variant::make<TEnum::Enum>(TEnum::TLITERALA);
    request.addParam(param1, "joynr.types.TestTypes.TEverythingStruct");
    request.addParam(param2, "joynr.types.TestTypes.TEverythingExtendedStruct");
    request.addParam(param3, "joynr.types.TestTypes.TDoubleKeyMap");
    request.addParam(param4, "joynr.types.TestTypes.TLITERALA");
}

void compareRequestWithComplexValues(const Request& expectedRequest, const Request& actualRequest) {
    using namespace types::TestTypes;
    std::vector<Variant> params = actualRequest.getParams();
    EXPECT_EQ(expectedRequest.getParams().size(), params.size());
    Variant param1 = params[0];
    EXPECT_TRUE(param1.is<TEverythingStruct>());
    EXPECT_EQ(expectedRequest.getParams().at(0).get<TEverythingStruct>(), param1.get<TEverythingStruct>());
    Variant param2 = params[1];
    EXPECT_TRUE(param2.is<TEverythingExtendedStruct>());
    EXPECT_EQ(expectedRequest.getParams().at(1).get<TEverythingExtendedStruct>(), param2.get<TEverythingExtendedStruct>());
    Variant param3 = params[2];
    EXPECT_TRUE(param3.is<TDoubleKeyMap>());
    EXPECT_EQ(expectedRequest.getParams().at(2).get<TDoubleKeyMap>(), param3.get<TDoubleKeyMap>());
    Variant param4 = params[3];
    EXPECT_TRUE(param4.is<std::string>());
    EXPECT_EQ(expectedRequest.getParams().at(3).get<TEnum::Enum>(), TEnum::getEnum(param4.get<std::string>()));
}

void initializeRequestWithPrimitiveValues(Request& request) {
    request.setMethodName("realMethod");
    request.setRequestReplyId("000-10000-01011");

    // Create a vector of variants to use as parameters
    std::string someString{"Hello World"};
    Variant param1 = Variant::make<std::string>(someString);
    request.addParam(param1, "String");
    const std::int32_t expectedInt = 101;
    Variant param2 = Variant::make<int>(expectedInt);
    request.addParam(param2, "Integer");
    SomeOtherType expectedSomeOtherType(2);
    Variant param3 = Variant::make<SomeOtherType>(expectedSomeOtherType);
    request.addParam(param3, "SomeOtherType");
    const float expectedFloat = 9.99f;
    Variant param4 = Variant::make<float>(expectedFloat);
    request.addParam(param4, "Float");
    bool expectedBool = true;
    Variant param5 = Variant::make<bool>(expectedBool);
    request.addParam(param5, "Bool");
}

void compareRequestWithPrimitiveValues(const Request& expectedRequest, const Request& actualRequest) {
    std::vector<Variant> params = actualRequest.getParams();
    EXPECT_EQ(expectedRequest.getParams().size(), params.size());
    Variant intParam = params[1];
    EXPECT_TRUE(intParam.is<std::uint64_t>());
    EXPECT_FALSE(intParam.is<std::int32_t>());
    EXPECT_EQ(expectedRequest.getParams().at(1).get<int>(), static_cast<std::int32_t>(intParam.get<std::uint64_t>()));
    Variant someOtherTypeParam = params[2];
    EXPECT_TRUE(someOtherTypeParam.is<SomeOtherType>());
    EXPECT_EQ(expectedRequest.getParams().at(2).get<SomeOtherType>().getA(), someOtherTypeParam.get<SomeOtherType>().getA());
    std::cout << "Deserialized value is " << someOtherTypeParam.get<SomeOtherType>().getA() << std::endl;
    Variant floatParam = params[3];
    EXPECT_TRUE(floatParam.is<double>());
    EXPECT_FALSE(floatParam.is<float>());
    EXPECT_FLOAT_EQ(expectedRequest.getParams().at(3).get<float>(),
                    static_cast<float>(floatParam.get<double>()));
    Variant boolParam = params[4];
    EXPECT_TRUE(boolParam.is<bool>());
    EXPECT_EQ(expectedRequest.getParams().at(4).get<bool>(), boolParam.get<bool>());
}

void checkRequest(const Request& expectedRequest, std::function<void(const Request&, const Request&)> compareFct, Logger& logger){
    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<Request>();
    serializer.serialize(expectedRequest, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "Request JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        Request request;
        ClassDeserializer<Request>::deserialize(request, tokenizer.nextObject());
        compareFct(expectedRequest, request);
    }

}

TEST_F(JoynrJsonSerializerTest, exampleSerializerTestWithJoynrRequestOfPrimitiveParameters)
{
    // Create, initialize & check request with primitive parameters
    Request expectedRequest;
    initializeRequestWithPrimitiveValues(expectedRequest);
    checkRequest(expectedRequest, compareRequestWithPrimitiveValues, logger);
}

TEST_F(JoynrJsonSerializerTest, exampleSerializerTestWithJoynrRequestOfComplexParameters)
{
    // Create, initialize & check request with complex parameters
    Request expectedRequest;
    initializeRequestWithComplexValues(expectedRequest);
    checkRequest(expectedRequest, compareRequestWithComplexValues, logger);
}

TEST_F(JoynrJsonSerializerTest, serializeJoynrMessage)
{
    // Create a Request
    Request expectedRequest;

    initializeRequestWithPrimitiveValues(expectedRequest);
    expectedRequest.addParam(Variant::make<std::string>(R"("quotedString")"), "string");
    JoynrMessage expectedMessage = JoynrMessageFactory().createRequest("sender",
                                                                      "receiver",
                                                                      MessagingQos(),
                                                                      expectedRequest);

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<JoynrMessage>();
    serializer.serialize(expectedMessage, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "JoynrMessage JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        JoynrMessage message;
        ClassDeserializer<JoynrMessage>::deserialize(message, tokenizer.nextObject());
        JOYNR_LOG_TRACE(logger, "JoynrMessage payload JSON: {}",message.getPayload());
        Request* request = JsonSerializer::deserialize<Request>(message.getPayload());
        compareRequestWithPrimitiveValues(expectedRequest, *request);
        delete request;
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerAplicationException)
{
    using namespace joynr::tests;
    std::string literal = test::MethodWithErrorEnumExtendedErrorEnum::getLiteral(
                test::MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION);
    // Create a ApplicationException
    exceptions::ApplicationException exception(
                literal,
                Variant::make<test::MethodWithErrorEnumExtendedErrorEnum::Enum>(test::MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION),
                literal,
                test::MethodWithErrorEnumExtendedErrorEnum::getTypeName());

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<exceptions::ApplicationException>{};
    serializer.serialize(exception, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "exceptions::ApplicationException JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        exceptions::ApplicationException t;
        ClassDeserializer<exceptions::ApplicationException>::deserialize(t, tokenizer.nextObject());
        ASSERT_EQ(t.getError<test::MethodWithErrorEnumExtendedErrorEnum::Enum>(), exception.getError<test::MethodWithErrorEnumExtendedErrorEnum::Enum>());
        ASSERT_EQ(t.getMessage(), exception.getMessage());
        ASSERT_EQ(t.getErrorTypeName(), exception.getErrorTypeName());
        ASSERT_EQ(t.getName(), exception.getName());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerProviderRuntimeException)
{
    // Create a ProviderRuntimeException
    exceptions::ProviderRuntimeException exception;
    std::string detailMessage{"Message of ProviderRuntimeException"};
    exception.setMessage(detailMessage);

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<exceptions::ProviderRuntimeException>{};
    serializer.serialize(exception, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "exceptions::ProviderRuntimeException JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        exceptions::ProviderRuntimeException t;
        ClassDeserializer<exceptions::ProviderRuntimeException>::deserialize(t, tokenizer.nextObject());
        ASSERT_EQ(t.getMessage(), detailMessage);
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerDiscoveryException)
{
    // Create a DiscoveryException
    exceptions::DiscoveryException exception;
    std::string detailMessage{"Message of DiscoveryException"};
    exception.setMessage(detailMessage);

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<exceptions::DiscoveryException>{};
    serializer.serialize(exception, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "exceptions::DiscoveryException JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        exceptions::DiscoveryException t;
        ClassDeserializer<exceptions::DiscoveryException>::deserialize(t, tokenizer.nextObject());
        ASSERT_EQ(t.getMessage(), detailMessage);
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerMethodInvocationException)
{
    // Create a MethodInvocationException
    exceptions::MethodInvocationException exception;
    std::string detailMessage{"Message of MethodInvocationException"};
    exception.setMessage(detailMessage);

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<exceptions::MethodInvocationException>{};
    serializer.serialize(exception, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "exceptions::MethodInvocationException JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        exceptions::MethodInvocationException t;
        ClassDeserializer<exceptions::MethodInvocationException>::deserialize(t, tokenizer.nextObject());
        ASSERT_EQ(t.getMessage(), detailMessage);
    }
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializerEmptyStruct)
{
    // Create a PublicationMissedException
    using namespace joynr::system::RoutingTypes;

    Address expectedAddress;

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<Address>{};
    serializer.serialize(expectedAddress, stream);
    std::string json = stream.str();
    JOYNR_LOG_TRACE(logger, "Address JSON: {}", json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        Address actualAddress;
        ClassDeserializer<Address>::deserialize(actualAddress, tokenizer.nextObject());
        EXPECT_EQ(expectedAddress, actualAddress);
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerPublicationMissedException)
{
    // Create a PublicationMissedException
    exceptions::PublicationMissedException exception;
    std::string subscriptionId{"SubscriptionId of PublicationMissedException"};
    exception.setSubscriptionId(subscriptionId);

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<exceptions::PublicationMissedException>{};
    serializer.serialize(exception, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "exceptions::PublicationMissedException JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        exceptions::PublicationMissedException t;
        ClassDeserializer<exceptions::PublicationMissedException>::deserialize(t, tokenizer.nextObject());
        ASSERT_EQ(t.getSubscriptionId(), subscriptionId);
        ASSERT_EQ(t.getMessage(), subscriptionId);
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrTimeOutException)
{
    // Create a JoynrTimeOutException
    exceptions::JoynrTimeOutException exception;
    std::string detailMessage{"Message of JoynrTimeOutException"};
    exception.setMessage(detailMessage);

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<exceptions::JoynrTimeOutException>{};
    serializer.serialize(exception, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "exceptions::JoynrTimeOutException JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        exceptions::JoynrTimeOutException t;
        ClassDeserializer<exceptions::JoynrTimeOutException>::deserialize(t, tokenizer.nextObject());
        ASSERT_EQ(t.getMessage(), detailMessage);
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrReplyWithProviderRuntimeException)
{
    // Create a Reply
    Reply reply;
    exceptions::ProviderRuntimeException error("Message of ProviderRuntimeException");
    reply.setError(joynr::exceptions::JoynrExceptionUtil::createVariant(error));

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<Reply>{};
    serializer.serialize(reply, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "Reply JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        Reply t;
        ClassDeserializer<Reply>::deserialize(t, tokenizer.nextObject());
        assert(!t.getError().isEmpty());
        const joynr::exceptions::ProviderRuntimeException& deserializedError(t.getError().get<joynr::exceptions::ProviderRuntimeException>());
        ASSERT_EQ(deserializedError.getMessage(), error.getMessage());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrReplyWithApplicationException)
{
    // Create a Reply
    Reply reply;
    using namespace joynr::tests;
    std::string literal = test::MethodWithErrorEnumExtendedErrorEnum::getLiteral(
                MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION);
    // Create a ApplicationException
    exceptions::ApplicationException error(
                literal,
                Variant::make<test::MethodWithErrorEnumExtendedErrorEnum::Enum>(MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION),
                literal,
                test::MethodWithErrorEnumExtendedErrorEnum::getTypeName());
    reply.setError(joynr::exceptions::JoynrExceptionUtil::createVariant(error));

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<Reply>{};
    serializer.serialize(reply, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "Reply JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        Reply t;
        ClassDeserializer<Reply>::deserialize(t, tokenizer.nextObject());
        assert(!t.getError().isEmpty());
        const joynr::exceptions::ApplicationException& deserializedError(t.getError().get<joynr::exceptions::ApplicationException>());
        ASSERT_EQ(deserializedError.getMessage(), error.getMessage());
        ASSERT_EQ(deserializedError.getError<test::MethodWithErrorEnumExtendedErrorEnum::Enum>(), error.getError<test::MethodWithErrorEnumExtendedErrorEnum::Enum>());
        ASSERT_EQ(deserializedError.getErrorTypeName(), error.getErrorTypeName());
        ASSERT_EQ(deserializedError.getName(), error.getName());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrReply)
{
    // Create a Reply
    Reply expectedReply;
    std::string someString{"Hello World"};
    std::vector<Variant> expectedResponse;
    expectedResponse.push_back(Variant::make<std::string>(someString));
    const std::int32_t expectedInt = 101;
    expectedResponse.push_back(Variant::make<int>(expectedInt));
    SomeOtherType expectedSomeOtherType(2);
    expectedResponse.push_back(Variant::make<SomeOtherType>(expectedSomeOtherType));
    const float expectedFloat = 9.99f;
    expectedResponse.push_back(Variant::make<float>(expectedFloat));
    bool expectedBool = true;
    expectedResponse.push_back(Variant::make<bool>(expectedBool));

    expectedReply.setResponse(expectedResponse);
    expectedReply.setRequestReplyId("000-10000-01100");

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<Reply>{};
    serializer.serialize(expectedReply, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "Reply JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        Reply reply;
        ClassDeserializer<Reply>::deserialize(reply, tokenizer.nextObject());
        std::vector<Variant> response = reply.getResponse();
        EXPECT_EQ(expectedReply.getResponse().size(), response.size());
        Variant first = response[0];
        EXPECT_TRUE(first.is<std::string>());
        EXPECT_EQ(someString, first.get<std::string>());
        Variant intParam = response[1];
        EXPECT_TRUE(intParam.is<std::uint64_t>());
        EXPECT_FALSE(intParam.is<std::int32_t>());
        EXPECT_EQ(expectedInt, static_cast<std::int32_t>(intParam.get<std::uint64_t>()));
        Variant someOtherTypeParam = response[2];
        EXPECT_TRUE(someOtherTypeParam.is<SomeOtherType>());
        EXPECT_EQ(expectedSomeOtherType.getA(), someOtherTypeParam.get<SomeOtherType>().getA());
        std::cout << "Deserialized value is " << someOtherTypeParam.get<SomeOtherType>().getA() << std::endl;
        Variant floatParam = response[3];
        EXPECT_TRUE(floatParam.is<double>());
        EXPECT_FALSE(floatParam.is<float>());
        EXPECT_EQ(expectedFloat, static_cast<float>(floatParam.get<double>()));
        Variant boolParam = response[4];
        EXPECT_TRUE(boolParam.is<bool>());
        EXPECT_EQ(expectedBool, boolParam.get<bool>());
        EXPECT_EQ(expectedReply.getRequestReplyId(), reply.getRequestReplyId());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrSubscriptionPublicationWithProviderRuntimeException)
{
    // Create a Publication
    SubscriptionPublication publication;
    exceptions::ProviderRuntimeException error("Message of ProviderRuntimeException");
    publication.setError(joynr::exceptions::JoynrExceptionUtil::createVariant(error));

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<SubscriptionPublication>{};
    serializer.serialize(publication, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "SubscriptionPublication JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        SubscriptionPublication t;
        ClassDeserializer<SubscriptionPublication>::deserialize(t, tokenizer.nextObject());
        assert(!t.getError().isEmpty());
        const joynr::exceptions::ProviderRuntimeException& deserializedError(t.getError().get<joynr::exceptions::ProviderRuntimeException>());
        ASSERT_EQ(deserializedError.getMessage(), error.getMessage());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrSubscriptionPublicationWithApplicationException)
{
    // Create a Publication
    SubscriptionPublication publication;
    using namespace joynr::tests;
    std::string literal = test::MethodWithErrorEnumExtendedErrorEnum::getLiteral(
                MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION);
    // Create a ApplicationException
    exceptions::ApplicationException error(
                literal,
                Variant::make<test::MethodWithErrorEnumExtendedErrorEnum::Enum>(MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION),
                literal,
                test::MethodWithErrorEnumExtendedErrorEnum::getTypeName());
    publication.setError(joynr::exceptions::JoynrExceptionUtil::createVariant(error));

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<SubscriptionPublication>{};
    serializer.serialize(publication, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "SubscriptionPublication JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        SubscriptionPublication t;
        ClassDeserializer<SubscriptionPublication>::deserialize(t, tokenizer.nextObject());
        assert(!t.getError().isEmpty());
        const joynr::exceptions::ApplicationException& deserializedError(t.getError().get<joynr::exceptions::ApplicationException>());
        ASSERT_EQ(deserializedError.getMessage(), error.getMessage());
        ASSERT_EQ(deserializedError.getError<test::MethodWithErrorEnumExtendedErrorEnum::Enum>(), error.getError<test::MethodWithErrorEnumExtendedErrorEnum::Enum>());
        ASSERT_EQ(deserializedError.getErrorTypeName(), error.getErrorTypeName());
        ASSERT_EQ(deserializedError.getName(), error.getName());
    }
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerSubscriptionPublication)
{
    // Create a publication
    SubscriptionPublication expectedPublication;
    std::string someString{"Hello World"};
    std::vector<Variant> expectedResponse;
    expectedResponse.push_back(Variant::make<std::string>(someString));
    const std::int32_t expectedInt = 101;
    expectedResponse.push_back(Variant::make<int>(expectedInt));
    SomeOtherType expectedSomeOtherType(2);
    expectedResponse.push_back(Variant::make<SomeOtherType>(expectedSomeOtherType));
    const float expectedFloat = 9.99f;
    expectedResponse.push_back(Variant::make<float>(expectedFloat));
    bool expectedBool = true;
    expectedResponse.push_back(Variant::make<bool>(expectedBool));

    expectedPublication.setResponse(expectedResponse);
    expectedPublication.setSubscriptionId("000-10000-01101");

    // Serialize into JSON
    std::stringstream stream;
    auto serializer = ClassSerializer<SubscriptionPublication>();
    serializer.serialize(expectedPublication, stream);
    std::string json{ stream.str() };
    JOYNR_LOG_TRACE(logger, "SubscriptionPublication JSON: {}",json);

    // Deserialize from JSON
    JsonTokenizer tokenizer(json);

    if (tokenizer.hasNextObject()) {
        SubscriptionPublication publication;
        ClassDeserializer<SubscriptionPublication>::deserialize(publication, tokenizer.nextObject());
        std::vector<Variant> response = publication.getResponse();
        EXPECT_EQ(expectedPublication.getResponse().size(), response.size());
        Variant first = response[0];
        EXPECT_TRUE(first.is<std::string>());
        EXPECT_EQ(someString, first.get<std::string>());
        Variant intParam = response[1];
        EXPECT_TRUE(intParam.is<std::uint64_t>());
        EXPECT_FALSE(intParam.is<std::int32_t>());
        EXPECT_EQ(expectedInt, static_cast<std::int32_t>(intParam.get<std::uint64_t>()));
        Variant someOtherTypeParam = response[2];
        EXPECT_TRUE(someOtherTypeParam.is<SomeOtherType>());
        EXPECT_EQ(expectedSomeOtherType.getA(), someOtherTypeParam.get<SomeOtherType>().getA());
        Variant floatParam = response[3];
        EXPECT_TRUE(floatParam.is<double>());
        EXPECT_FALSE(floatParam.is<float>());
        EXPECT_EQ(expectedFloat, static_cast<float>(floatParam.get<double>()));
        Variant boolParam = response[4];
        EXPECT_TRUE(boolParam.is<bool>());
        EXPECT_EQ(expectedBool, boolParam.get<bool>());
        EXPECT_EQ(expectedPublication.getSubscriptionId(), publication.getSubscriptionId());
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

    JOYNR_LOG_TRACE(logger, "MAC JSON: {}",json);

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

template <typename T>
void serializeDeserializeMap(T expectedMap, Logger& logger) {
    // Serialize
    std::stringstream stream;
    auto serializer = ClassSerializer<T>{};
    serializer.serialize(expectedMap, stream);
    std::string json{ stream.str() };

    // TODO: replace with logging
    JOYNR_LOG_DEBUG(logger, "ExpectedMap Json: {}",json);
    JsonTokenizer tokenizer(json);

    T actualMap;
    // Deserialize
    ClassDeserializer<T>::deserialize(actualMap, tokenizer.nextObject());

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMap, actualMap);
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTDoubleKeyMap)
{
    using namespace joynr::types::TestTypes;
    TDoubleKeyMap expectedMap;
    expectedMap.insert({1.1, "StringValue1"});
    expectedMap.insert({2.2, "StringValue2"});

    serializeDeserializeMap<TDoubleKeyMap>(expectedMap, logger);
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTIntegerKeyMap)
{
    using namespace joynr::types::TestTypes;
    TIntegerKeyMap expectedMap;
    expectedMap.insert({1, "StringValue1"});
    expectedMap.insert({2, "StringValue2"});

    serializeDeserializeMap<TIntegerKeyMap>(expectedMap, logger);
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTEverythingMap)
{
    using namespace joynr::types::TestTypes;

    TEverythingMap expectedMap;
    expectedMap.insert({TEnum::TLITERALA, TEverythingExtendedStruct()});
    expectedMap.insert({TEnum::TLITERALB, TEverythingExtendedStruct()});

    serializeDeserializeMap<TEverythingMap>(expectedMap, logger);
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTStringKeyMap)
{
    using namespace joynr::types::TestTypes;

    TStringKeyMap expectedMap;
    expectedMap.insert({"StringKey1", "StringValue1"});
    expectedMap.insert({"StringKey2", "StringValue2"});

    serializeDeserializeMap<TStringKeyMap>(expectedMap, logger);
}

// test with TEverythingStruct
TEST_F(JoynrJsonSerializerTest, serializeDeserializeTEverythingStruct)
{
    using namespace joynr::types::TestTypes;

    std::vector<std::uint8_t> byteBuffer(
                std::initializer_list<std::uint8_t>{
                    1,2,3
                });
    std::vector<std::uint8_t> uInt8Array(
                std::initializer_list<std::uint8_t>{
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
    TStringKeyMap stringMap;
    stringMap.insert({"StringKey", "StringValue"});
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
        words,
        stringMap
    };

    // Serialize
    std::stringstream stream;
    auto serializer = ClassSerializer<TEverythingStruct>{};
    serializer.serialize(expectedEverythingStruct, stream);
    std::string json{ stream.str() };

    // TODO: replace with logging
    JOYNR_LOG_TRACE(logger, "TEverythingStruct JSON: {}",json);

    // Deserialize
    TEverythingStruct* everythingStruct = JsonSerializer::deserialize<TEverythingStruct>(json);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedEverythingStruct, *everythingStruct);
    delete everythingStruct;
}

void JoynrJsonSerializerTest::testSerializationOfTStruct(joynr::types::TestTypes::TStruct expectedStruct) {
    using namespace joynr::types::TestTypes;

    std::stringstream stream;
    auto serializer = ClassSerializer<TStruct>{};
    serializer.serialize(expectedStruct, stream);
    std::string json{ stream.str() };

    // TODO: replace with logging
    JOYNR_LOG_TRACE(logger, "TStruct JSON: {}",json);

    // Deserialize
    TStruct* actualStruct = JsonSerializer::deserialize<TStruct>(json);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedStruct, *actualStruct);
    delete actualStruct;

}

// test with TEverythingStruct
TEST_F(JoynrJsonSerializerTest, serializeDeserializeStructContainingStringMember)
{
    using namespace joynr::types::TestTypes;
    TStruct expectedStruct{
        1.1,
        2,
        "normalString"
    };
    // Serialize
    testSerializationOfTStruct(expectedStruct);
    expectedStruct.setTString(R"(\"String containing quotas\")");
    testSerializationOfTStruct(expectedStruct);
}

// test with TEverythingStruct
TEST_F(JoynrJsonSerializerTest, correctEscapingOfStrings)
{
    EXPECT_EQ(addEscapeForSpecialCharacters("normalString"), "normalString");
    EXPECT_EQ(addEscapeForSpecialCharacters(R"(\stringWithBackSlash)"), R"(\\stringWithBackSlash)");
    EXPECT_EQ(addEscapeForSpecialCharacters(R"("stringWithQuotas")"), R"(\"stringWithQuotas\")");
    EXPECT_EQ(addEscapeForSpecialCharacters(R"(\"stringWithBackSlashAndQuotas\")"), R"(\\\"stringWithBackSlashAndQuotas\\\")");
}

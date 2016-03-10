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
#include <PrettyPrint.h>
#include <limits>
#include "joynr/Util.h"
#include "joynr/types/TestTypes/TEnum.h"
#include "joynr/types/TestTypes/TStruct.h"
#include "joynr/types/TestTypes/TStructExtended.h"
#include "joynr/types/TestTypes/TStructComposition.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/types/ChannelUrlInformation.h"
#include "joynr/types/CapabilityInformation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JsonSerializer.h"
#include "joynr/Logger.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/tests/testTypes/TestEnum.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/TypeUtil.h"

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include <chrono>

#include <sstream>

using namespace joynr;

class JsonSerializerTest : public testing::Test {

protected:
    template<class T>
    void serializeDeserializeReply(T value) {

        // setup reply object
        Reply reply;
        reply.setRequestReplyId("TEST-requestReplyId");
        std::vector<Variant> response;
        response.push_back(Variant::make<T>(value));
        reply.setResponse(std::move(response));

        std::stringstream expectedReplyStringStream;
        expectedReplyStringStream << R"({"_typeName":"joynr.Reply","requestReplyId": )";
        expectedReplyStringStream << R"(")" << reply.getRequestReplyId() << R"(",)";
        expectedReplyStringStream << R"("response": [)";
        ClassSerializer<T> serializer;
        serializer.serialize(value, expectedReplyStringStream);
        expectedReplyStringStream << R"(]})";

        std::string expectedReplyString = expectedReplyStringStream.str();

        std::string jsonReply = JsonSerializer::serialize(reply);

        EXPECT_EQ(expectedReplyString, jsonReply);

        Reply receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);
        Variant responseVariant = receivedReply.getResponse().at(0);

        T responseValue;
        if (responseVariant.is<std::int64_t>()) {
            responseValue = static_cast<T>(responseVariant.get<std::int64_t>());
        } else if(responseVariant.is<std::uint64_t>()) {
            responseValue = static_cast<T>(responseVariant.get<std::uint64_t>());
        } else {
            responseValue = responseVariant.get<T>();
        }
        EXPECT_EQ(value, responseValue);
    }

    ADD_LOGGER(JsonSerializerTest);
};

INIT_LOGGER(JsonSerializerTest);

TEST_F(JsonSerializerTest, serialize_deserialize_SubscriptionRequest) {
    SubscriptionRequest request;
    Variant subscriptionQos = Variant::make<SubscriptionQos>(SubscriptionQos(5000));
    request.setQos(subscriptionQos);
    std::string result = JsonSerializer::serialize<SubscriptionRequest>(request);
    JOYNR_LOG_DEBUG(logger, "result: {}", result);
    SubscriptionRequest desRequest = JsonSerializer::deserialize<SubscriptionRequest>(result);
    EXPECT_TRUE(request == desRequest);
}

TEST_F(JsonSerializerTest, serialize_deserialize_BroadcastSubscriptionRequest) {
    BroadcastSubscriptionRequest request;
    OnChangeSubscriptionQos qos{5000, 2000};
    request.setQos(qos);
    BroadcastFilterParameters filterParams;
    filterParams.setFilterParameter("MyFilter", "MyFilterValue");
    request.setFilterParameters(filterParams);
    request.setSubscribeToName("myAttribute");
    std::string requestJson = JsonSerializer::serialize<BroadcastSubscriptionRequest>(request);
    JOYNR_LOG_DEBUG(logger, requestJson);
    BroadcastSubscriptionRequest desRequest = JsonSerializer::deserialize<BroadcastSubscriptionRequest>(requestJson);
    EXPECT_TRUE(request == desRequest);
}

TEST_F(JsonSerializerTest, serialize_deserialize_JoynrMessage) {

    Request expectedRequest;
    expectedRequest.setMethodName("serialize_JoynrMessage");
    expectedRequest.setRequestReplyId("xyz");
    JoynrMessage expectedJoynrMessage;
    JoynrTimePoint testExpiryDate = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()) + std::chrono::milliseconds(10000000);
    expectedJoynrMessage.setHeaderExpiryDate(testExpiryDate);
    expectedJoynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST);
    expectedJoynrMessage.setPayload(JsonSerializer::serialize(expectedRequest));
    std::string serializedContent(JsonSerializer::serialize(expectedJoynrMessage));
    JOYNR_LOG_DEBUG(logger, "serialize_JoynrMessage: actual : {}",serializedContent);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.JoynrMessage","header": )";
    expectedStringStream << R"({"expiryDate": ")" << testExpiryDate.time_since_epoch().count() << R"(",)";
    expectedStringStream << R"("msgId": ")" << expectedJoynrMessage.getHeaderMessageId() << R"("},)";
    expectedStringStream << R"("payload": "{\"_typeName\":\"joynr.Request\",\"methodName\": \")" << expectedRequest.getMethodName() << R"(\",)";
    expectedStringStream << R"(\"paramDatatypes\": [],\"params\": [],\"requestReplyId\": \")" << expectedRequest.getRequestReplyId() << R"(\"}",)";
    expectedStringStream << R"("type": "request"})";

    std::string expectedString = expectedStringStream.str();
    JOYNR_LOG_DEBUG(logger, "serialize_JoynrMessage: expected: {}", expectedString);
    EXPECT_EQ(expectedString, serializedContent);

    JoynrMessage joynrMessage = JsonSerializer::deserialize<JoynrMessage>(serializedContent);
    JOYNR_LOG_DEBUG(logger, "joynrMessage->getPayload(): {}", joynrMessage.getPayload());
    Request request = JsonSerializer::deserialize<Request>(joynrMessage.getPayload());
    JOYNR_LOG_DEBUG(logger, "joynrMessage->getType(): {}", joynrMessage.getType());
    EXPECT_EQ(joynrMessage.getType(), expectedJoynrMessage.getType());
    JOYNR_LOG_DEBUG(logger, "request->getMethodName(): {}", request.getMethodName());
    EXPECT_EQ(request.getMethodName(), expectedRequest.getMethodName());
}

TEST_F(JsonSerializerTest, serialize_deserialize_byte_array) {

    // Build a list to test with
    std::vector<std::uint8_t> expectedUint8Vector;//{0x01, 0x02, 0x03, 0xff, 0xfe, 0xfd};
    expectedUint8Vector.push_back(1);
    expectedUint8Vector.push_back(2);
    expectedUint8Vector.push_back(3);
    expectedUint8Vector.push_back(255);
    expectedUint8Vector.push_back(254);
    expectedUint8Vector.push_back(253);

    // Set the request method name
    Request request;
    request.setMethodName("serialize_deserialize_byte_array");

    // Set the request parameters
    std::vector<Variant> expectedVariantVectorParam = TypeUtil::toVectorOfVariants(expectedUint8Vector);
    request.addParam(Variant::make<std::vector<Variant>>(expectedVariantVectorParam), "List");

    // Serialize the request
    std::string serializedRequestJson = JsonSerializer::serialize<Request>(request);

    std::stringstream jsonStringStream;
    jsonStringStream << R"({"_typeName":"joynr.Request",)" <<
                R"("methodName": "serialize_deserialize_byte_array",)" <<
                R"("paramDatatypes": ["List"],)" <<
                R"("params": [[1,2,3,255,254,253]],)" <<
                R"("requestReplyId": ")" << request.getRequestReplyId() << R"("})";
    std::string expectedRequestJson = jsonStringStream.str();

    JOYNR_LOG_DEBUG(logger, "expected: {}", expectedRequestJson);
    JOYNR_LOG_DEBUG(logger, "serialized: {}", serializedRequestJson);
    EXPECT_EQ(expectedRequestJson, serializedRequestJson);

    // Deserialize the request
    Request deserializedRequest = JsonSerializer::deserialize<Request>(serializedRequestJson);
    std::vector<Variant> deserializedParams = deserializedRequest.getParams();
    std::vector<Variant> deserializedVariantVectorParam = deserializedParams.at(0).get<std::vector<Variant>>();

    EXPECT_EQ(expectedVariantVectorParam, deserializedVariantVectorParam);

    std::vector<Variant>::const_iterator expectedIt(expectedVariantVectorParam.begin());
    std::vector<Variant>::const_iterator deserializedIt(deserializedVariantVectorParam.begin());
    while (expectedIt != expectedVariantVectorParam.end() && deserializedIt != deserializedVariantVectorParam.end()) {
        EXPECT_EQ("UInt64", deserializedIt->getTypeName());
        EXPECT_EQ(expectedIt->get<std::uint8_t>(), static_cast<std::uint8_t>(deserializedIt->get<std::uint64_t>()));
        expectedIt++;
        deserializedIt++;
    }

    EXPECT_EQ(expectedVariantVectorParam.end(), expectedIt);
    EXPECT_EQ(deserializedVariantVectorParam.end(), deserializedIt);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt8) {
    // std::int8_t alias (signed) char
    std::int8_t int8MinValue = std::numeric_limits<std::int8_t>::min();
    std::int8_t int8MaxValue = std::numeric_limits<std::int8_t>::max();

    serializeDeserializeReply<std::int8_t>(int8MinValue);
    serializeDeserializeReply<std::int8_t>(int8MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt8) {
    // std::uint8_t alias unsigned char
    std::uint8_t unsignedInt8MinValue = std::numeric_limits<std::uint8_t>::min();
    std::uint8_t unsignedInt8MaxValue = std::numeric_limits<std::uint8_t>::max();

    serializeDeserializeReply<std::uint8_t>(unsignedInt8MinValue);
    serializeDeserializeReply<std::uint8_t>(unsignedInt8MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt16) {
    // std::int16_t alias (signed) short
    std::int16_t int16MinValue = std::numeric_limits<std::int16_t>::min();
    std::int16_t int16MaxValue = std::numeric_limits<std::int16_t>::max();

    serializeDeserializeReply<std::int16_t>(int16MinValue);
    serializeDeserializeReply<std::int16_t>(int16MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt16) {
    // std::uint16_t alias unsigned short
    std::uint16_t unsignedInt16MinValue = std::numeric_limits<std::uint16_t>::min();
    std::uint16_t unsignedInt16MaxValue = std::numeric_limits<std::uint16_t>::max();

    serializeDeserializeReply<std::uint16_t>(unsignedInt16MinValue);
    serializeDeserializeReply<std::uint16_t>(unsignedInt16MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt32) {
    // std::int32_t alias (signed) int
    std::int32_t int32MinValue = std::numeric_limits<std::int32_t>::min();
    std::int32_t int32MaxValue = std::numeric_limits<std::int32_t>::max();

    serializeDeserializeReply<std::int32_t>(int32MinValue);
    serializeDeserializeReply<std::int32_t>(int32MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt32) {
    // std::uint32_t alias unsigned int
    std::uint32_t unsignedInt32MinValue = std::numeric_limits<std::uint32_t>::min();
    std::uint32_t unsignedInt32MaxValue = std::numeric_limits<std::uint32_t>::max();

    serializeDeserializeReply<std::uint32_t>(unsignedInt32MinValue);
    serializeDeserializeReply<std::uint32_t>(unsignedInt32MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt64) {
    // std::uint8_t alias (signed) long long
    std::int64_t int64MinValue = std::numeric_limits<std::uint8_t>::min();
    std::uint8_t int64MaxValue = std::numeric_limits<std::uint8_t>::max();

    serializeDeserializeReply<std::uint8_t>(int64MinValue);
    serializeDeserializeReply<std::uint8_t>(int64MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt64) {
    // std::uint64_t alias unsigned long long
    std::uint64_t unsignedInt64MinValue = std::numeric_limits<std::uint64_t>::min();
    std::uint64_t unsignedInt64MaxValue = std::numeric_limits<std::uint64_t>::max();

    serializeDeserializeReply<std::uint64_t>(unsignedInt64MinValue);
    serializeDeserializeReply<std::uint64_t>(unsignedInt64MaxValue);
}

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params1) {
    // Set the request method name
    Request request;
    request.setMethodName("methodEnumDoubleParameters");

    // Set the request parameters
    request.addParam(Variant::make<tests::testTypes::TestEnum::Enum>(tests::testTypes::TestEnum::ONE), "joynr.tests.testTypes.TestEnum.Enum");
    request.addParam(Variant::make<double>(2.2), "Double");

    // Serialize the request
    std::string serializedContent = JsonSerializer::serialize<Request>(request);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.Request",)";
    expectedStringStream << R"("methodName": "methodEnumDoubleParameters",)";
    expectedStringStream << R"("paramDatatypes": ["joynr.tests.testTypes.TestEnum.Enum","Double"],)";
    expectedStringStream << R"("params": ["ONE",2.2],)";
    expectedStringStream << R"("requestReplyId": ")" << request.getRequestReplyId() << R"(")";
    expectedStringStream << R"(})";

    std::string expected = expectedStringStream.str();

    EXPECT_EQ(expected, serializedContent);
}

TEST_F(JsonSerializerTest, deserialize_operation_with_enum) {

    // Deserialize a request containing a Java style enum parameter
    std::string serializedContent(R"({"_typeName":"joynr.Request",)"
                                  R"("methodName": "methodEnumDoubleParameters",)"
                                  R"("paramDatatypes": ["joynr.tests.testTypes.TestEnum.Enum","Double"],)"
                                  R"("params": ["ONE",2.2]})");

    Request request = JsonSerializer::deserialize<Request>(serializedContent);
    std::vector<Variant> params = request.getParams();

    // Check the deserialized values
    Variant enumParam = params.at(0);
    Variant doubleParam = params.at(1);
    EXPECT_TRUE(enumParam.is<std::string>());
    EXPECT_EQ(std::string("ONE"), enumParam.get<std::string>());
    EXPECT_TRUE(doubleParam.is<double>());
    EXPECT_EQ(2.2, doubleParam.get<double>());

    // Extract the parameters in the same way as a RequestInterpreter
    tests::testTypes::TestEnum::Enum value = util::convertVariantToEnum<tests::testTypes::TestEnum>(enumParam);
    EXPECT_EQ(1, value);
}

TEST_F(JsonSerializerTest, deserializeTypeWithEnumList) {

    using namespace infrastructure::DacTypes;

    // Deserialize a type containing multiple enum lists
    std::string serializedContent(
                R"({"_typeName":"joynr.infrastructure.DacTypes.MasterAccessControlEntry",)"
                R"("defaultConsumerPermission": "NO",)"
                R"("defaultRequiredControlEntryChangeTrustLevel": "LOW",)"
                R"("defaultRequiredTrustLevel": "LOW",)"
                R"("domain": "unittest",)"
                R"("interfaceName": "vehicle/radio",)"
                R"("operation": "*",)"
                R"("possibleConsumerPermissions": ["YES","NO"],)"
                R"("possibleRequiredControlEntryChangeTrustLevels": ["HIGH","MID","LOW"],)"
                R"("possibleRequiredTrustLevels": ["HIGH","MID","LOW"],)"
                R"("uid": "*"})");

    infrastructure::DacTypes::MasterAccessControlEntry mac = JsonSerializer::deserialize<infrastructure::DacTypes::MasterAccessControlEntry>(serializedContent);

    // Check scalar enums
    EXPECT_EQ(Permission::NO, mac.getDefaultConsumerPermission());
    EXPECT_EQ(TrustLevel::LOW, mac.getDefaultRequiredTrustLevel());

    // Check enum lists
    std::vector<Permission::Enum> possibleRequiredPermissions;
    possibleRequiredPermissions.push_back(Permission::YES);
    possibleRequiredPermissions.push_back(Permission::NO);
    EXPECT_EQ(possibleRequiredPermissions, mac.getPossibleConsumerPermissions());

    std::vector<TrustLevel::Enum> possibleRequiredTrustLevels;
    possibleRequiredTrustLevels.push_back(TrustLevel::HIGH);
    possibleRequiredTrustLevels.push_back(TrustLevel::MID);
    possibleRequiredTrustLevels.push_back(TrustLevel::LOW);
    EXPECT_EQ(possibleRequiredTrustLevels, mac.getPossibleRequiredTrustLevels());
}

TEST_F(JsonSerializerTest, serializeDeserializeTypeWithEnumList) {

    using namespace infrastructure::DacTypes;

    std::vector<TrustLevel::Enum> possibleTrustLevels;
    possibleTrustLevels.push_back(TrustLevel::LOW);
    possibleTrustLevels.push_back(TrustLevel::MID);
    possibleTrustLevels.push_back(TrustLevel::HIGH);
    std::vector<Permission::Enum> possiblePermissions;
    possiblePermissions.push_back(Permission::NO);
    possiblePermissions.push_back(Permission::ASK);
    possiblePermissions.push_back(Permission::YES);

    infrastructure::DacTypes::MasterAccessControlEntry expectedMac(R"(*)",
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
    std::string serializedContent = JsonSerializer::serialize<infrastructure::DacTypes::MasterAccessControlEntry>(expectedMac);
    JOYNR_LOG_DEBUG(logger, "Serialized expectedMac: {}", serializedContent);

    // Deserialize the result
    infrastructure::DacTypes::MasterAccessControlEntry mac = JsonSerializer::deserialize<infrastructure::DacTypes::MasterAccessControlEntry>(serializedContent);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMac, mac);
}

using namespace infrastructure::DacTypes;

void deserializePermission(const std::string& serializedPermission, const Permission::Enum& expectation) {
    Variant variant = Variant::make<std::string>(serializedPermission);

    // Deserialize the result and compare
    std::string jsonStr = "{ \"value\" : \"" + serializedPermission + "\" }";
    JsonTokenizer tokenizer(jsonStr);
    Permission::Enum deserializedEnum;
    PrimitiveDeserializer<Permission::Enum>::deserialize(deserializedEnum, tokenizer.nextObject().nextField().value());
    EXPECT_EQ(expectation, deserializedEnum);
    EXPECT_EQ(expectation, joynr::util::valueOf<Permission::Enum>(variant));
}

void serializeAndDeserializePermission(const Permission::Enum& input, const std::string& inputAsString, Logger& logger) {
    // Serialize
    std::string serializedContent(JsonSerializer::serialize<Permission::Enum>(input));
    JOYNR_LOG_DEBUG(logger, "Serialized permission for input: {}, {}", inputAsString, serializedContent);

    deserializePermission(serializedContent.substr(1, serializedContent.size()-2 ), input);
}

TEST_F(JsonSerializerTest, serializeDeserializeTypeEnum) {
    using namespace infrastructure::DacTypes;

    ASSERT_NO_THROW(serializeAndDeserializePermission(Permission::NO, "Permission::NO", logger));

    ASSERT_ANY_THROW(serializeAndDeserializePermission(static_cast<Permission::Enum>(999), "999", logger));
}

TEST_F(JsonSerializerTest, deserializeTypeEnum) {
    using namespace infrastructure::DacTypes;

    ASSERT_NO_THROW(deserializePermission("NO", Permission::NO));

    ASSERT_ANY_THROW(deserializePermission("999", static_cast<Permission::Enum>(999)));
}

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params2) {

    // Set the request method name
    Request request;
    request.setMethodName("methodStringDoubleParameters");

    // Set the request parameters
    request.addParam(Variant::make<std::string>("testStringParam"), "String");
    request.addParam(Variant::make<double>(3.33), "Double");
    request.addParam(Variant::make<float>(1.25e-9), "Float");

    // Serialize the request
    std::string serializedContent = JsonSerializer::serialize<Request>(request);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.Request",)";
    expectedStringStream << R"("methodName": "methodStringDoubleParameters",)";
    expectedStringStream << R"("paramDatatypes": ["String","Double","Float"],)";
    expectedStringStream << R"("params": ["testStringParam",3.33,1.25e-09],)";
    expectedStringStream << R"("requestReplyId": ")" << request.getRequestReplyId() << R"(")";
    expectedStringStream << R"(})";

    std::string expected = expectedStringStream.str();

    JOYNR_LOG_DEBUG(logger, "Serialized method call: {}", serializedContent);
    JOYNR_LOG_DEBUG(logger, "Expected method call: {}", expected);

    // Expected literal is:
    // { "_typeName" : "joynr.Request", "methodName" : "methodStringDoubleParameters", "paramDatatypes" : [ "String", "Double" ], "params" : { "doubleParam" : 3.33, "stringParam" : "testStringParam" } }
    EXPECT_EQ(expected, serializedContent);

}


TEST_F(JsonSerializerTest, serialize_deserialize_TStruct) {

    types::TestTypes::TStruct tStruct;
    tStruct.setTDouble(0.123456789);
    tStruct.setTInt64(64);
    tStruct.setTString("myTestString");

    std::string expectedTStruct(
                R"({)"
                R"("_typeName":"joynr.types.TestTypes.TStruct",)"
                R"("tDouble": 0.123456789,)"
                R"("tInt64": 64,)"
                R"("tString": "myTestString")"
                R"(})"
                );

    std::string serializedContent = JsonSerializer::serialize<types::TestTypes::TStruct>(tStruct);
    JOYNR_LOG_DEBUG(logger, serializedContent);
    EXPECT_EQ(expectedTStruct, serializedContent);

    types::TestTypes::TStruct tStructDeserialized = JsonSerializer::deserialize<types::TestTypes::TStruct>(serializedContent);

    EXPECT_EQ(tStruct, tStructDeserialized);
}

TEST_F(JsonSerializerTest, serialize_deserialize_TStructExtended) {

    types::TestTypes::TStructExtended tStructExt;
    tStructExt.setTDouble(0.123456789);
    tStructExt.setTInt64(64);
    tStructExt.setTString("myTestString");
    tStructExt.setTEnum(types::TestTypes::TEnum::TLITERALA);
    tStructExt.setTInt32(32);

    std::string expectedTStructExt(
                R"({)"
                R"("_typeName":"joynr.types.TestTypes.TStructExtended",)"
                R"("tDouble": 0.123456789,)"
                R"("tInt64": 64,)"
                R"("tString": "myTestString",)"
                R"("tEnum": "TLITERALA",)"
                R"("tInt32": 32)"
                R"(})"
                );

    std::string serializedTStructExt = JsonSerializer::serialize<types::TestTypes::TStructExtended>(tStructExt);
    JOYNR_LOG_DEBUG(logger, serializedTStructExt);

    EXPECT_EQ(expectedTStructExt, serializedTStructExt);
    types::TestTypes::TStructExtended deserializedTStructExt = JsonSerializer::deserialize<types::TestTypes::TStructExtended>(serializedTStructExt);

    EXPECT_EQ(tStructExt, deserializedTStructExt);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocation) {
    types::Localisation::GpsLocation gps1(1.1, 1.2, 1.3, types::Localisation::GpsFixEnum::MODE3D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);

    // Expected literal is:
    Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    std::vector<Variant> response;
    response.push_back(Variant::make<types::Localisation::GpsLocation>(gps1));
    reply.setResponse(std::move(response));

    std::stringstream expectedReplyStringStream;
    expectedReplyStringStream << R"({)";
    expectedReplyStringStream << R"("_typeName":"joynr.Reply",)";
    expectedReplyStringStream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
    expectedReplyStringStream << R"("response": [{)";
    expectedReplyStringStream << R"("_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedReplyStringStream << R"("longitude": 1.1,)";
    expectedReplyStringStream << R"("latitude": 1.2,)";
    expectedReplyStringStream << R"("altitude": 1.3,)";
    expectedReplyStringStream << R"("gpsFix": "MODE3D",)";
    expectedReplyStringStream << R"("heading": 1.4,)";
    expectedReplyStringStream << R"("quality": 1.5,)";
    expectedReplyStringStream << R"("elevation": 1.6,)";
    expectedReplyStringStream << R"("bearing": 1.7,)";
    expectedReplyStringStream << R"("gpsTime": 18,)";
    expectedReplyStringStream << R"("deviceTime": 19,)";
    expectedReplyStringStream << R"("time": 110)";
    expectedReplyStringStream << R"(}])";
    expectedReplyStringStream << R"(})";

    std::string expectedReply = expectedReplyStringStream.str();

    std::string jsonReply = JsonSerializer::serialize<Reply>(reply);

    EXPECT_EQ(expectedReply, jsonReply);

    Reply receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply.getResponse().size() == 1);
    EXPECT_TRUE(receivedReply.getResponse().at(0).is<types::Localisation::GpsLocation>());

    types::Localisation::GpsLocation gps2 = receivedReply.getResponse().at(0).get<types::Localisation::GpsLocation>();

    EXPECT_EQ(gps1, gps2)
            << "Gps locations gps1 " << gps1.toString()
            << " and gps2 " << gps2.toString() << " are not the same";
}

TEST_F(JsonSerializerTest, deserialize_replyWithVoid) {

    // null response with type invalid
    std::vector<Variant> response;
    Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    reply.setResponse(std::move(response));

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.Reply",)";
    expectedStringStream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
    expectedStringStream << R"("response": []})";
    std::string expected = expectedStringStream.str();

    std::string jsonReply = JsonSerializer::serialize<Reply>(reply);

    EXPECT_EQ(expected, jsonReply);

    JOYNR_LOG_DEBUG(logger, "Serialized Reply: {}", jsonReply);

    Reply receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);
    EXPECT_EQ(reply, receivedReply);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocationList) {

    std::vector<Variant> locList;
    locList.push_back(Variant::make<types::Localisation::GpsLocation>(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 17));
    locList.push_back(Variant::make<types::Localisation::GpsLocation>(4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODENOFIX, 0.0, 0.0, 0.0, 0.0, 0, 0, 18));

    EXPECT_EQ(locList.size(), 2);

    Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    std::vector<Variant> response;
    response.push_back(TypeUtil::toVariant(locList));
    reply.setResponse(std::move(response));

    EXPECT_EQ(reply.getResponse().size(), 1);

    // Expected literal
    std::stringstream expectedReplyStringStream;
    expectedReplyStringStream << R"({)";
    expectedReplyStringStream << R"("_typeName":"joynr.Reply",)";
    expectedReplyStringStream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
    expectedReplyStringStream << R"("response": [[{)";
    expectedReplyStringStream << R"("_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedReplyStringStream << R"("longitude": 1.1,)";
    expectedReplyStringStream << R"("latitude": 2.2,)";
    expectedReplyStringStream << R"("altitude": 3.3,)";
    expectedReplyStringStream << R"("gpsFix": "MODE3D",)";
    expectedReplyStringStream << R"("heading": 0.0,)";
    expectedReplyStringStream << R"("quality": 0.0,)";
    expectedReplyStringStream << R"("elevation": 0.0,)";
    expectedReplyStringStream << R"("bearing": 0.0,)";
    expectedReplyStringStream << R"("gpsTime": 0,)";
    expectedReplyStringStream << R"("deviceTime": 0,)";
    expectedReplyStringStream << R"("time": 17)";
    expectedReplyStringStream << R"(},{)";
    expectedReplyStringStream << R"("_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedReplyStringStream << R"("longitude": 4.4,)";
    expectedReplyStringStream << R"("latitude": 5.5,)";
    expectedReplyStringStream << R"("altitude": 6.6,)";
    expectedReplyStringStream << R"("gpsFix": "MODENOFIX",)";
    expectedReplyStringStream << R"("heading": 0.0,)";
    expectedReplyStringStream << R"("quality": 0.0,)";
    expectedReplyStringStream << R"("elevation": 0.0,)";
    expectedReplyStringStream << R"("bearing": 0.0,)";
    expectedReplyStringStream << R"("gpsTime": 0,)";
    expectedReplyStringStream << R"("deviceTime": 0,)";
    expectedReplyStringStream << R"("time": 18)";
    expectedReplyStringStream << R"()}]])";
    expectedReplyStringStream << R"(})";

    std::string expectedReply = expectedReplyStringStream.str();

    std::string jsonReply = JsonSerializer::serialize<Reply>(reply);
    JOYNR_LOG_DEBUG(logger, jsonReply);
    //EXPECT_EQ(expectedReply, jsonReply);

    Reply receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply.getResponse().at(0).is<std::vector<Variant>>());
    std::vector<Variant> receivedReplyResponse = receivedReply.getResponse().at(0).get<std::vector<Variant>>();
    EXPECT_EQ(receivedReplyResponse.size(), 2);
    std::vector<Variant>::const_iterator i_received = receivedReplyResponse.begin();
    std::vector<Variant> originalResponse = reply.getResponse();
    EXPECT_EQ(originalResponse.size(), 1);
    Variant firstItem = originalResponse.at(0);
    EXPECT_TRUE(firstItem.is<std::vector<Variant>>());
    std::vector<Variant> originalResponseList = firstItem.get<std::vector<Variant>>();
    EXPECT_EQ(originalResponseList.size(),2);
    std::vector<Variant>::const_iterator i_origin = originalResponseList.begin();
    while(i_received != receivedReplyResponse.end()) {
        types::Localisation::GpsLocation receivedIterValue = i_received->get<types::Localisation::GpsLocation>();
        types::Localisation::GpsLocation originItervalue = i_origin->get<types::Localisation::GpsLocation>();
        JOYNR_LOG_DEBUG(logger, receivedIterValue.toString());
        EXPECT_EQ(originItervalue, receivedIterValue);
        i_received++;
        i_origin++;
    }
}

TEST_F(JsonSerializerTest, serialize_deserialize_trip) {
    std::vector<types::Localisation::GpsLocation> locations;
    locations.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,17));
    locations.push_back(types::Localisation::GpsLocation(4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,317));
    locations.push_back(types::Localisation::GpsLocation(7.7, 8.8, 9.9, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,3317));

    std::string expected(
                        R"({"_typeName":"joynr.types.Localisation.Trip",)"
                        R"("locations": [{"_typeName":"joynr.types.Localisation.GpsLocation",)"
                        R"("longitude": 1.1,)"
                        R"("latitude": 2.2,)"
                        R"("altitude": 3.3,)"
                        R"("gpsFix": "MODE3D",)"
                        R"("heading": 0.0,)"
                        R"("quality": 0.0,)"
                        R"("elevation": 0.0,)"
                        R"("bearing": 0.0,)"
                        R"("gpsTime": 0,)"
                        R"("deviceTime": 0,)"
                        R"("time": 17},)"
                        R"({"_typeName":"joynr.types.Localisation.GpsLocation",)"
                        R"("longitude": 4.4,)"
                        R"("latitude": 5.5,)"
                        R"("altitude": 6.6,)"
                        R"("gpsFix": "MODE3D",)"
                        R"("heading": 0.0,)"
                        R"("quality": 0.0,)"
                        R"("elevation": 0.0,)"
                        R"("bearing": 0.0,)"
                        R"("gpsTime": 0,)"
                        R"("deviceTime": 0,)"
                        R"("time": 317},)"
                        R"({"_typeName":"joynr.types.Localisation.GpsLocation",)"
                        R"("longitude": 7.7,)"
                        R"("latitude": 8.8,)"
                        R"("altitude": 9.9,)"
                        R"("gpsFix": "MODE3D",)"
                        R"("heading": 0.0,)"
                        R"("quality": 0.0,)"
                        R"("elevation": 0.0,)"
                        R"("bearing": 0.0,)"
                        R"("gpsTime": 0,)"
                        R"("deviceTime": 0,)"
                        R"("time": 3317}],)"
                        R"("tripTitle": "trip1_name"})"
                );

    // Expected literal is:
    types::Localisation::Trip trip1(locations, "trip1_name");

    std::string serializedContent = JsonSerializer::serialize(Variant::make<types::Localisation::Trip>(trip1));
    EXPECT_EQ(expected, serializedContent);

    types::Localisation::Trip trip2 = JsonSerializer::deserialize<types::Localisation::Trip>(serializedContent);
    EXPECT_EQ(trip1, trip2) << "trips \n trip1: " << trip1.toString().c_str()
                            << " and \n trip2: " << trip2.toString().c_str()
                            << "\n are not the same";
}


TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequest) {

    std::vector<types::Localisation::GpsLocation> locations;
    locations.push_back(types::Localisation::GpsLocation(1.1, 1.2, 1.3, types::Localisation::GpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110));
    locations.push_back(types::Localisation::GpsLocation(2.1, 2.2, 2.3, types::Localisation::GpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210));
    locations.push_back(types::Localisation::GpsLocation(3.1, 3.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 3.4, 3.5, 3.6, 3.7, 38, 39, 310));

    types::Localisation::Trip trip1(locations, "trip1_name");

    Request request1;
    request1.setMethodName("serialize_deserialize_JsonRequestTest_method");

    std::vector<Variant> params;
    std::string contentParam1("contentParam1");
    params.push_back(Variant::make<std::string>(contentParam1));
    params.push_back(Variant::make<types::Localisation::Trip>(trip1));
    //To serialize a std::vector<...> it has to be stored as a std::vector<Variant>
    std::vector<Variant> vector;
    vector.push_back(Variant::make<int>(2));
    params.push_back(TypeUtil::toVariant(vector));

    request1.setParams(params);

    std::string serializedContent = JsonSerializer::serialize(request1);
    JOYNR_LOG_DEBUG(logger, serializedContent);

    Request request2 = JsonSerializer::deserialize<Request>(serializedContent);

    std::vector<Variant> paramsReceived = request2.getParams();

    EXPECT_EQ(paramsReceived.at(0).get<std::string>(), contentParam1);
    EXPECT_EQ(paramsReceived.at(1).get<types::Localisation::Trip>(), trip1);

    EXPECT_EQ(request1, request2);
}

TEST_F(JsonSerializerTest, serialize_deserialize_Reply_with_Array_as_Response) {
    std::vector<types::CapabilityInformation> capabilityInformations;
    types::CapabilityInformation cap1(types::CapabilityInformation("domain1", "interface1", types::ProviderQos(), "channel1", "participant1"));
    capabilityInformations.push_back(cap1);
    capabilityInformations.push_back(types::CapabilityInformation("domain2", "interface2", types::ProviderQos(), "channel2", "participant2"));

    Reply reply;

    std::vector<Variant> response;
    reply.setRequestReplyId("serialize_deserialize_Reply_with_Array_as_Response");
    response.push_back(joynr::TypeUtil::toVariant(capabilityInformations));
    reply.setResponse(std::move(response));
    std::string serializedContent = JsonSerializer::serialize<Reply>(reply);
    JOYNR_LOG_DEBUG(logger, serializedContent);

    Reply deserializedReply = JsonSerializer::deserialize<Reply>(serializedContent);

    response = deserializedReply.getResponse();

    std::vector<Variant> receivedCaps = response.at(0).get<std::vector<Variant>>();
    types::CapabilityInformation receivedCap1 = receivedCaps.at(0).get<types::CapabilityInformation>();
    EXPECT_EQ(receivedCap1, cap1);
    EXPECT_EQ(deserializedReply.getRequestReplyId(), "serialize_deserialize_Reply_with_Array_as_Response");
}

TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequestWithLists) {

    //creating Request
    std::vector<types::Localisation::GpsLocation> inputLocationList;
    inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,17));
    inputLocationList.push_back(types::Localisation::GpsLocation(4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,317));
    inputLocationList.push_back(types::Localisation::GpsLocation(7.7, 8.8, 9.9, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,3317));

    Request request1;
    request1.setMethodName("serialize_deserialize_JsonRequestTest_method");
    std::vector<Variant> inputvl = TypeUtil::toVectorOfVariants(inputLocationList);
    request1.addParam(Variant::make<std::vector<Variant>>(inputvl), "List");

    std::stringstream expectedStringStream;
                expectedStringStream << R"({"_typeName":"joynr.Request",)";
                expectedStringStream << R"("methodName": "serialize_deserialize_JsonRequestTest_method",)";
                expectedStringStream << R"("paramDatatypes": ["List"],)";
                expectedStringStream << R"("params": [[{"_typeName":"joynr.types.Localisation.GpsLocation",)";
                expectedStringStream << R"("longitude": 1.1,)";
                expectedStringStream << R"("latitude": 2.2,)";
                expectedStringStream << R"("altitude": 3.3,)";
                expectedStringStream << R"("gpsFix": "MODE3D",)";
                expectedStringStream << R"("heading": 0.0,)";
                expectedStringStream << R"("quality": 0.0,)";
                expectedStringStream << R"("elevation": 0.0,)";
                expectedStringStream << R"("bearing": 0.0,)";
                expectedStringStream << R"("gpsTime": 0,)";
                expectedStringStream << R"("deviceTime": 0,)";
                expectedStringStream << R"("time": 17},)";
                expectedStringStream << R"({"_typeName":"joynr.types.Localisation.GpsLocation",)";
                expectedStringStream << R"("longitude": 4.4,)";
                expectedStringStream << R"("latitude": 5.5,)";
                expectedStringStream << R"("altitude": 6.6,)";
                expectedStringStream << R"("gpsFix": "MODE3D",)";
                expectedStringStream << R"("heading": 0.0,)";
                expectedStringStream << R"("quality": 0.0,)";
                expectedStringStream << R"("elevation": 0.0,)";
                expectedStringStream << R"("bearing": 0.0,)";
                expectedStringStream << R"("gpsTime": 0,)";
                expectedStringStream << R"("deviceTime": 0,)";
                expectedStringStream << R"("time": 317},)";
                expectedStringStream << R"({"_typeName":"joynr.types.Localisation.GpsLocation",)";
                expectedStringStream << R"("longitude": 7.7,)";
                expectedStringStream << R"("latitude": 8.8,)";
                expectedStringStream << R"("altitude": 9.9,)";
                expectedStringStream << R"("gpsFix": "MODE3D",)";
                expectedStringStream << R"("heading": 0.0,)";
                expectedStringStream << R"("quality": 0.0,)";
                expectedStringStream << R"("elevation": 0.0,)";
                expectedStringStream << R"("bearing": 0.0,)";
                expectedStringStream << R"("gpsTime": 0,)";
                expectedStringStream << R"("deviceTime": 0,)";
                expectedStringStream << R"("time": 3317}]],)";
                expectedStringStream << R"("requestReplyId": ")" << request1.getRequestReplyId() << R"("})";

    std::string expected = expectedStringStream.str();

    //serializing Request
    std::string serializedContent = JsonSerializer::serialize<Request>(request1);
    EXPECT_EQ(expected, serializedContent);

    //deserializing Request
    Request request2 = JsonSerializer::deserialize<Request>(serializedContent);
    std::vector<Variant> paramsReceived = request2.getParams();

    JOYNR_LOG_DEBUG(logger, "x1 {}", paramsReceived.at(0).getTypeName());
    ASSERT_TRUE(paramsReceived.at(0).is<std::vector<Variant>>()) << "Cannot convert the field of the Param Map to a std::vector<Variant>";
    std::vector<Variant> returnvl = paramsReceived.at(0).get<std::vector<Variant>>();
    ASSERT_TRUE(returnvl.size() == 3) << "list size size != 3";
    JOYNR_LOG_DEBUG(logger, returnvl.at(0).getTypeName());

    ASSERT_TRUE(returnvl.at(0).is<types::Localisation::GpsLocation>()) << "Cannot convert the first entry of the return List to GpsLocation";

    std::vector<types::Localisation::GpsLocation> resultLocationList = util::convertVariantVectorToVector<types::Localisation::GpsLocation>(returnvl);
    EXPECT_EQ(resultLocationList.at(1), types::Localisation::GpsLocation(4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0, 317));
}

//// Test to investigate whether long lists cause a problem with during deserialization
TEST_F(JsonSerializerTest, serialize_deserialize_ListComplexity) {

    //creating Request
    std::vector<types::Localisation::GpsLocation> inputLocationList;

    // Create a JSON list
    int firstListSize = 10000;
    for (int i = 0; i < firstListSize; i++) {
        inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0, 17));
    }

    // Create a request that uses this list
    Request request1;
    request1.setMethodName("serialize_deserialize_JsonRequestTest_method");
    std::vector<Variant> params1;
    std::vector<Variant> inputvl = TypeUtil::toVectorOfVariants(inputLocationList);
    params1.push_back(Variant::make<std::vector<Variant>>(inputvl));
    request1.setParams(params1);

    //Time request serialization
    JoynrTimePoint start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    std::string newSerializedContent = JsonSerializer::serialize<Request>(request1);
    JoynrTimePoint end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    int serializationElapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Time request deserialization
    start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    Request deserialized1 = JsonSerializer::deserialize<Request>(newSerializedContent);
    std::ignore = deserialized1;
    end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    int deserializationElapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Double the size of the JSON list
    for (int i = 0; i < firstListSize; i++) {
        inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0,0,0,0,0,17));
    }

    // Create a request that uses this bigger list
    Request request2;
    request2.setMethodName("serialize_deserialize_JsonRequestTest_method");
    std::vector<Variant> params2;
    start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    std::vector<Variant> inputv2 = TypeUtil::toVectorOfVariants(inputLocationList);
    end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    int convertedElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    JOYNR_LOG_DEBUG(logger, "converted to vector<Variant> {} in {} milliseconds", firstListSize, convertedElapsed);
    // to silence unused-variable compiler warnings
    std::ignore = convertedElapsed;
    params2.push_back(Variant::make<std::vector<Variant>>(inputv2));
    request2.setParams(params2);

    //Time request serialization with new serializer
    start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    newSerializedContent = JsonSerializer::serialize<Request>(request2);
    end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    int serializationElapsed2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Time request deserialization with new serializer
    start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    Request deserialized2 = JsonSerializer::deserialize<Request>(newSerializedContent);
    std::ignore = deserialized2;
    end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    int deserializationElapsed2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    JOYNR_LOG_DEBUG(logger, "{} Objects serialized in {} milliseconds", firstListSize, serializationElapsed1);
    JOYNR_LOG_DEBUG(logger, "{} Objects serialized in {} milliseconds", firstListSize*2, serializationElapsed2);
    JOYNR_LOG_DEBUG(logger, "{} Objects serialized in {} milliseconds", firstListSize, deserializationElapsed1);
    JOYNR_LOG_DEBUG(logger, "{} Objects serialized in {} milliseconds", firstListSize*2, deserializationElapsed2);

    // Assert O(N) complexity
    ASSERT_TRUE(serializationElapsed2 < (serializationElapsed1 * serializationElapsed1));
    ASSERT_TRUE(deserializationElapsed2 < (deserializationElapsed1 * deserializationElapsed1));
}

TEST_F(JsonSerializerTest, serialize_deserialize_EndpointAddress) {
    joynr::system::RoutingTypes::ChannelAddress joynr("TEST_channelId");
    joynr::system::RoutingTypes::CommonApiDbusAddress dbus("domain", "interfacename", "id");
    joynr::system::RoutingTypes::WebSocketAddress wsServer(
                joynr::system::RoutingTypes::WebSocketProtocol::WS,
                "localhost",
                42,
                "some/path"
                );
    joynr::system::RoutingTypes::WebSocketClientAddress wsClient("TEST_clientId");

    // serialize
    std::string joynrSerialized = JsonSerializer::serialize<joynr::system::RoutingTypes::ChannelAddress>(joynr);
    std::string dbusSerialized = JsonSerializer::serialize<joynr::system::RoutingTypes::CommonApiDbusAddress>(dbus);
    std::string wsServerSerialized = JsonSerializer::serialize<joynr::system::RoutingTypes::WebSocketAddress>(wsServer);
    std::string wsClientSerialized = JsonSerializer::serialize<joynr::system::RoutingTypes::WebSocketClientAddress>(wsClient);

    JOYNR_LOG_DEBUG(logger, "serialized Joynr address: {}", joynrSerialized);
    JOYNR_LOG_DEBUG(logger, "serialized Dbus address: {}", dbusSerialized);
    JOYNR_LOG_DEBUG(logger, "serialized WS server address: {}", wsServerSerialized);
    JOYNR_LOG_DEBUG(logger, "serialized WS client address: {}", wsClientSerialized);

    // deserialize
    joynr::system::RoutingTypes::ChannelAddress joynrDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::ChannelAddress>(joynrSerialized);
    joynr::system::RoutingTypes::CommonApiDbusAddress dbusDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::CommonApiDbusAddress>(dbusSerialized);
    joynr::system::RoutingTypes::WebSocketAddress wsServerDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::WebSocketAddress>(wsServerSerialized);
    joynr::system::RoutingTypes::WebSocketClientAddress wsClientDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::WebSocketClientAddress>(wsClientSerialized);

    EXPECT_EQ(joynr, joynrDeserialized);
    EXPECT_EQ(dbus, dbusDeserialized);
    EXPECT_EQ(wsServer, wsServerDeserialized);
    EXPECT_EQ(wsClient, wsClientDeserialized);
}

TEST_F(JsonSerializerTest, serialize_deserialize_CapabilityInformation) {

    std::string expected(
                R"({"_typeName":"joynr.types.CapabilityInformation",)"
                R"("domain": "domain",)"
                R"("interfaceName": "",)"
                R"("providerQos": {)"
                R"("_typeName":"joynr.types.ProviderQos",)"
                R"("customParameters": [],)"
                R"("priority": 2,)"
                R"("scope": "GLOBAL",)"
                R"("supportsOnChangeSubscriptions": false},)"
                R"("channelId": "channeldId",)"
                R"("participantId": ""})"
                );

    // Expected literal is:
    // { "_typeName" : "joynr.types.CapabilityInformation", "channelId" : "channeldId", "domain" : "domain", "interfaceName" : "", "participantId" : "", "providerQos" : { "_typeName" : "joynr.types.ProviderQos", "isLocalOnly" : false, "onChangeSubscriptions" : false, "priority" : 2, "qos" : [  ], "Version" : 4 }

    types::ProviderQos qos;
    qos.setPriority(2);
    types::CapabilityInformation capabilityInformation;
    capabilityInformation.setChannelId("channeldId");
    capabilityInformation.setDomain("domain");
    capabilityInformation.setProviderQos(qos);
    JOYNR_LOG_DEBUG(logger, "capabilityInformation {}", capabilityInformation.toString());

    std::string serialized = JsonSerializer::serialize<types::CapabilityInformation>(capabilityInformation);
    EXPECT_EQ(expected, serialized);
    // deserialize
    JOYNR_LOG_DEBUG(logger, "serialized capabilityInformation {}",serialized);

    types::CapabilityInformation deserializedCI = JsonSerializer::deserialize<types::CapabilityInformation>(serialized);

    EXPECT_EQ(capabilityInformation, deserializedCI);
    JOYNR_LOG_DEBUG(logger, "deserialized capabilityInformation {}", deserializedCI.toString());
}

//// Test of ChannelURLInformation which is of type std::Vector<std::string>.
TEST_F(JsonSerializerTest, serialize_deserialize_ChannelURLInformation) {
    std::vector<std::string> urls;
    urls.push_back("http://example1.com/");
    urls.push_back("http://example2.com/");
    types::ChannelUrlInformation urlInformation(urls);

    // Serialize the URL Information
    std::string serialized = JsonSerializer::serialize(Variant::make<types::ChannelUrlInformation>(urlInformation));
    JOYNR_LOG_DEBUG(logger, "serialized ChannelUrlInformation {}", serialized);

    // Expected JSON : { "_typeName" : "joynr.types.ChannelUrlInformation", "urls" : [ "http://example1.com/", "http://example2.com/" ] }
    std::string expected("{\"_typeName\":\"joynr.types.ChannelUrlInformation\",\"urls\": [\"http://example1.com/\",\"http://example2.com/\"]}");

    EXPECT_EQ(expected, serialized);

    // Deserialize
    types::ChannelUrlInformation deserializedInfo = JsonSerializer::deserialize<types::ChannelUrlInformation>(serialized);

    // Check the structure
    std::vector<std::string> deserializedUrls = deserializedInfo.getUrls();
    EXPECT_EQ(urls, deserializedUrls);
}

TEST_F(JsonSerializerTest, deserialize_ProviderQos) {
    joynr::types::ProviderQos qos;

    std::string jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[],\"priority\":5,\"scope\":\"LOCAL\",\"supportsOnChangeSubscriptions\":false}");

    joynr::types::ProviderQos providerQos = JsonSerializer::deserialize<joynr::types::ProviderQos>(jsonProviderQos);

    EXPECT_EQ(providerQos.getScope(), joynr::types::ProviderScope::LOCAL);
    EXPECT_EQ(providerQos.getPriority(), 5);
}

TEST_F(JsonSerializerTest, serialize_ProviderQos) {
    joynr::types::ProviderQos qos;
    qos.setScope(joynr::types::ProviderScope::LOCAL);
    qos.setPriority(5);

    std::string jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\": [],\"priority\": 5,\"scope\": \"LOCAL\",\"supportsOnChangeSubscriptions\": false}");

    std::string result = JsonSerializer::serialize<joynr::types::ProviderQos>(qos);

    EXPECT_EQ(jsonProviderQos, result);
}


TEST_F(JsonSerializerTest, deserialize_GPSLocation) {

    std::string jsonGPS(
                R"({"_typeName":"joynr.types.Localisation.GpsLocation",)"
                R"("longitude": 1.1,)"
                R"("latitude": 2.2,)"
                R"("altitude": 3.3,)"
                R"("gpsFix": "MODE3D",)"
                R"("heading": 0.0,)"
                R"("quality": 0.0,)"
                R"("elevation": 0.0,)"
                R"("bearing": 0.0,)"
                R"("gpsTime": 0,)"
                R"("deviceTime": 0,)"
                R"("time": 17})"
                );

    joynr::types::Localisation::GpsLocation receivedGps = JsonSerializer::deserialize<joynr::types::Localisation::GpsLocation>(jsonGPS);
    EXPECT_EQ(3.3, receivedGps.getAltitude());
}

TEST_F(JsonSerializerTest, serialize_OnchangeWithKeepAliveSubscription) {

    OnChangeWithKeepAliveSubscriptionQos qos(750, 100, 900, 1050);

    std::string jsonQos = JsonSerializer::serialize<OnChangeWithKeepAliveSubscriptionQos>(qos);
    JOYNR_LOG_DEBUG(logger, "serialized OnChangeWithKeepAliveSubscriptionQos {}", jsonQos);

    OnChangeWithKeepAliveSubscriptionQos desQos = JsonSerializer::deserialize<OnChangeWithKeepAliveSubscriptionQos>(jsonQos);

    jsonQos = JsonSerializer::serialize<OnChangeWithKeepAliveSubscriptionQos>(desQos);
    JOYNR_LOG_DEBUG(logger, "serialized OnChangeWithKeepAliveSubscriptionQos {}", jsonQos);

    EXPECT_EQ(qos, desQos);
}

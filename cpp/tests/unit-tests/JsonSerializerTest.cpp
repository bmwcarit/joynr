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
#include <limits>
#include <memory>
#include <chrono>
#include <sstream>

#include <gtest/gtest.h>
#include <boost/algorithm/string/predicate.hpp>

#include "joynr/Util.h"
#include "joynr/types/TestTypes/TEnum.h"
#include "joynr/types/TestTypes/TStruct.h"
#include "joynr/types/TestTypes/TStructExtended.h"
#include "joynr/types/TestTypes/TStructComposition.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/types/GlobalDiscoveryEntry.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/Version.h"
#include "joynr/Logger.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/tests/testTypes/TestEnum.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/Directory.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"

#include "tests/PrettyPrint.h"
#include "tests/JoynrTest.h"

using namespace joynr;

class JsonSerializerTest : public testing::Test
{
protected:
    ADD_LOGGER(JsonSerializerTest)
};

TEST_F(JsonSerializerTest, serialize_deserialize_SubscriptionRequest)
{
    SubscriptionRequest request;
    auto subscriptionQos = std::make_shared<SubscriptionQos>(5000);
    request.setQos(subscriptionQos);
    std::string result = joynr::serializer::serializeToJson(request);
    JOYNR_LOG_DEBUG(logger(), "result: {}", result);
    SubscriptionRequest desRequest;
    joynr::serializer::deserializeFromJson(desRequest, result);
    EXPECT_TRUE(request == desRequest);
}

TEST_F(JsonSerializerTest, serialize_deserialize_BroadcastSubscriptionRequest)
{
    BroadcastSubscriptionRequest request;
    auto qos = std::make_shared<OnChangeSubscriptionQos>(5000, 1000, 2000);
    request.setQos(qos);
    BroadcastFilterParameters filterParams;
    filterParams.setFilterParameter("MyFilter", "MyFilterValue");
    request.setFilterParameters(filterParams);
    request.setSubscribeToName("myAttribute");
    std::string requestJson = joynr::serializer::serializeToJson(request);
    JOYNR_LOG_DEBUG(logger(), requestJson);
    BroadcastSubscriptionRequest desRequest;
    joynr::serializer::deserializeFromJson(desRequest, requestJson);
    EXPECT_TRUE(request == desRequest);
}

TEST_F(JsonSerializerTest, serialize_deserialize_byte_array)
{

    // Build a list to test with
    using Uint8Vec = std::vector<std::uint8_t>;
    Uint8Vec expectedUint8Vector; //{0x01, 0x02, 0x03, 0xff, 0xfe, 0xfd};
    expectedUint8Vector.push_back(1);
    expectedUint8Vector.push_back(2);
    expectedUint8Vector.push_back(3);
    expectedUint8Vector.push_back(255);
    expectedUint8Vector.push_back(254);
    expectedUint8Vector.push_back(253);

    // Set the request method name
    joynr::Request request;
    request.setMethodName("serialize_deserialize_byte_array");

    // Set the request parameters
    request.setParamDatatypes({"List"});
    request.setParams(expectedUint8Vector);

    // Serialize the request
    std::string serializedRequestJson = joynr::serializer::serializeToJson(request);

    std::stringstream jsonStringStream;
    jsonStringStream << R"({"_typeName":"joynr.Request",)"
                     <<
            R"("methodName":"serialize_deserialize_byte_array",)"
                     <<
            R"("paramDatatypes":["List"],)"
                     <<
            R"("params":[[1,2,3,255,254,253]],)"
                     <<
            R"("requestReplyId":")" << request.getRequestReplyId() << R"("})";
    std::string expectedRequestJson = jsonStringStream.str();

    JOYNR_LOG_DEBUG(logger(), "expected: {}", expectedRequestJson);
    JOYNR_LOG_DEBUG(logger(), "serialized: {}", serializedRequestJson);
    EXPECT_EQ(expectedRequestJson, serializedRequestJson);

    // Deserialize the request
    joynr::Request deserializedRequest;
    joynr::serializer::deserializeFromJson(deserializedRequest, serializedRequestJson);
    Uint8Vec deserializedVector;
    deserializedRequest.getParams(deserializedVector);
    EXPECT_EQ(expectedUint8Vector, deserializedVector);
}

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params1)
{
    // Set the request method name
    joynr::Request request;
    request.setMethodName("methodEnumDoubleParameters");

    // Set the request parameters
    request.setParamDatatypes({"joynr.tests.testTypes.TestEnum.Enum", "Double"});
    request.setParams(tests::testTypes::TestEnum::ONE, 2.2);

    // Serialize the request
    std::string serializedContent = joynr::serializer::serializeToJson(request);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.Request",)";
    expectedStringStream << R"("methodName":"methodEnumDoubleParameters",)";
    expectedStringStream << R"("paramDatatypes":["joynr.tests.testTypes.TestEnum.Enum","Double"],)";
    expectedStringStream << R"("params":["ONE",2.2],)";
    expectedStringStream << R"("requestReplyId":")" << request.getRequestReplyId() << R"(")";
    expectedStringStream << R"(})";

    std::string expected = expectedStringStream.str();

    EXPECT_EQ(expected, serializedContent);
}

TEST_F(JsonSerializerTest, deserialize_operation_with_enum)
{

    tests::testTypes::TestEnum::Enum expectedEnumParam = tests::testTypes::TestEnum::ONE;
    double expectedDoubleParam = 2.2;

    // Deserialize a request containing a Java style enum parameter
    std::string serializedContent(
            R"({"_typeName":"joynr.Request",)"
            R"("methodName":"methodEnumDoubleParameters",)"
            R"("paramDatatypes":["joynr.tests.testTypes.TestEnum.Enum","Double"],)"
            R"("params":["ONE",2.2],)"
            R"("requestReplyId":"789eaj21312390"})");

    joynr::Request request;
    joynr::serializer::deserializeFromJson(request, serializedContent);

    tests::testTypes::TestEnum::Enum deserializedEnumParam;
    double deserializedDoubleParam;
    request.getParams(deserializedEnumParam, deserializedDoubleParam);

    EXPECT_EQ(expectedEnumParam, deserializedEnumParam);
    EXPECT_EQ(expectedDoubleParam, deserializedDoubleParam);
}

TEST_F(JsonSerializerTest, deserializeTypeWithEnumList)
{

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

    infrastructure::DacTypes::MasterAccessControlEntry mac;
    joynr::serializer::deserializeFromJson(mac, serializedContent);

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

TEST_F(JsonSerializerTest, serializeDeserializeTypeWithEnumList)
{

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
    std::string serializedContent = joynr::serializer::serializeToJson(expectedMac);
    JOYNR_LOG_DEBUG(logger(), "Serialized expectedMac: {}", serializedContent);

    // Deserialize the result
    infrastructure::DacTypes::MasterAccessControlEntry mac;
    joynr::serializer::deserializeFromJson(mac, serializedContent);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMac, mac);
}

using namespace infrastructure::DacTypes;

void deserializePermission(const std::string& serializedPermission,
                           const Permission::Enum& expectation)
{
    // Deserialize the result and compare
    Permission::Enum deserializedEnum;
    joynr::serializer::deserializeFromJson(deserializedEnum, serializedPermission);
    EXPECT_EQ(expectation, deserializedEnum);
}

void serializeAndDeserializePermission(const Permission::Enum& input,
                                       const std::string& inputAsString,
                                       Logger& logger)
{
    // Serialize
    std::string serializedContent = joynr::serializer::serializeToJson(input);
    JOYNR_LOG_DEBUG(
            logger, "Serialized permission for input: {}, {}", inputAsString, serializedContent);

    deserializePermission(serializedContent, input);
}

TEST_F(JsonSerializerTest, serializeDeserializeTypeEnum)
{
    using namespace infrastructure::DacTypes;

    JOYNR_ASSERT_NO_THROW(serializeAndDeserializePermission(Permission::NO, R"("NO")", logger()));

    ASSERT_ANY_THROW(
            serializeAndDeserializePermission(static_cast<Permission::Enum>(999), "999", logger()));
}

TEST_F(JsonSerializerTest, deserializeTypeEnum)
{
    using namespace infrastructure::DacTypes;

    JOYNR_ASSERT_NO_THROW(deserializePermission(R"("NO")", Permission::NO));

    ASSERT_ANY_THROW(deserializePermission("999", static_cast<Permission::Enum>(999)));
}

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params2)
{

    // Set the request method name
    Request request;
    request.setMethodName("methodStringDoubleParameters");

    // Set the request parameters
    request.setParamDatatypes({"String", "Double", "Float"});
    request.setParams("testStringParam", 3.33, 1.25e-9f);

    // Serialize the request
    std::string serializedContent = joynr::serializer::serializeToJson(request);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.Request",)";
    expectedStringStream << R"("methodName":"methodStringDoubleParameters",)";
    expectedStringStream << R"("paramDatatypes":["String","Double","Float"],)";
    expectedStringStream << R"("params":["testStringParam",3.33,1.2499999924031613e-9],)";
    expectedStringStream << R"("requestReplyId":")" << request.getRequestReplyId() << R"(")";
    expectedStringStream << R"(})";

    std::string expected = expectedStringStream.str();

    JOYNR_LOG_DEBUG(logger(), "Serialized method call: {}", serializedContent);
    JOYNR_LOG_DEBUG(logger(), "Expected method call: {}", expected);

    EXPECT_EQ(expected, serializedContent);
}

TEST_F(JsonSerializerTest, serialize_deserialize_TStruct)
{

    types::TestTypes::TStruct tStruct;
    tStruct.setTDouble(0.123456789);
    tStruct.setTInt64(64);
    tStruct.setTString("myTestString");

    std::string expectedTStruct(
            R"({)"
            R"("_typeName":"joynr.types.TestTypes.TStruct",)"
            R"("tDouble":0.123456789,)"
            R"("tInt64":64,)"
            R"("tString":"myTestString")"
            R"(})");

    std::string serializedContent = joynr::serializer::serializeToJson(tStruct);
    JOYNR_LOG_DEBUG(logger(), serializedContent);
    EXPECT_EQ(expectedTStruct, serializedContent);

    types::TestTypes::TStruct tStructDeserialized;
    joynr::serializer::deserializeFromJson(tStructDeserialized, serializedContent);

    EXPECT_EQ(tStruct, tStructDeserialized);
}

TEST_F(JsonSerializerTest, serialize_deserialize_TStructExtended)
{

    types::TestTypes::TStructExtended tStructExt;
    tStructExt.setTDouble(0.123456789);
    tStructExt.setTInt64(64);
    tStructExt.setTString("myTestString");
    tStructExt.setTEnum(types::TestTypes::TEnum::TLITERALA);
    tStructExt.setTInt32(32);

    std::string expectedTStructExt(
            R"({)"
            R"("_typeName":"joynr.types.TestTypes.TStructExtended",)"
            R"("tDouble":0.123456789,)"
            R"("tInt64":64,)"
            R"("tString":"myTestString",)"
            R"("tEnum":"TLITERALA",)"
            R"("tInt32":32)"
            R"(})");

    std::string serializedTStructExt = joynr::serializer::serializeToJson(tStructExt);
    JOYNR_LOG_DEBUG(logger(), serializedTStructExt);

    EXPECT_EQ(expectedTStructExt, serializedTStructExt);
    types::TestTypes::TStructExtended deserializedTStructExt;
    joynr::serializer::deserializeFromJson(deserializedTStructExt, serializedTStructExt);

    EXPECT_EQ(tStructExt, deserializedTStructExt);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocation)
{
    types::Localisation::GpsLocation gps1(1.1,
                                          1.2,
                                          1.3,
                                          types::Localisation::GpsFixEnum::MODE3D,
                                          1.4,
                                          1.5,
                                          1.6,
                                          1.7,
                                          18,
                                          19,
                                          110);

    // Expected literal is:
    Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    reply.setResponse(gps1);

    std::stringstream expectedReplyStringStream;
    expectedReplyStringStream << R"({)";
    expectedReplyStringStream << R"("_typeName":"joynr.Reply",)";
    expectedReplyStringStream << R"("response":[{)";
    expectedReplyStringStream << R"("_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedReplyStringStream << R"("longitude":1.1,)";
    expectedReplyStringStream << R"("latitude":1.2,)";
    expectedReplyStringStream << R"("altitude":1.3,)";
    expectedReplyStringStream << R"("gpsFix":"MODE3D",)";
    expectedReplyStringStream << R"("heading":1.4,)";
    expectedReplyStringStream << R"("quality":1.5,)";
    expectedReplyStringStream << R"("elevation":1.6,)";
    expectedReplyStringStream << R"("bearing":1.7,)";
    expectedReplyStringStream << R"("gpsTime":18,)";
    expectedReplyStringStream << R"("deviceTime":19,)";
    expectedReplyStringStream << R"("time":110)";
    expectedReplyStringStream << R"(}],)";
    expectedReplyStringStream << R"("requestReplyId":")" << reply.getRequestReplyId() << R"(")";
    expectedReplyStringStream << R"(})";

    std::string expectedReply = expectedReplyStringStream.str();

    std::string jsonReply = joynr::serializer::serializeToJson(reply);

    EXPECT_EQ(expectedReply, jsonReply);

    joynr::Reply receivedReply;
    joynr::serializer::deserializeFromJson(receivedReply, jsonReply);

    EXPECT_TRUE(receivedReply.hasResponse());

    types::Localisation::GpsLocation receivedGps;
    receivedReply.getResponse(receivedGps);

    EXPECT_EQ(gps1, receivedGps);
}

TEST_F(JsonSerializerTest, deserialize_replyWithVoid)
{

    joynr::Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    // void response
    reply.setResponse();

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.Reply",)";
    expectedStringStream << R"("response":[],)";
    expectedStringStream << R"("requestReplyId":")" << reply.getRequestReplyId() << R"("})";
    std::string expected = expectedStringStream.str();

    std::string jsonReply = joynr::serializer::serializeToJson(reply);

    EXPECT_EQ(expected, jsonReply);

    JOYNR_LOG_DEBUG(logger(), "Serialized Reply: {}", jsonReply);

    joynr::Reply receivedReply;
    joynr::serializer::deserializeFromJson(receivedReply, jsonReply);
    EXPECT_EQ(reply, receivedReply);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocationList)
{

    using GpsLocationList = std::vector<types::Localisation::GpsLocation>;
    GpsLocationList locList;
    locList.emplace_back(
            1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 17);
    locList.emplace_back(4.4,
                         5.5,
                         6.6,
                         types::Localisation::GpsFixEnum::MODENOFIX,
                         0.0,
                         0.0,
                         0.0,
                         0.0,
                         0,
                         0,
                         18);

    joynr::Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    reply.setResponse(locList);

    // Expected literal
    std::stringstream expectedReplyStringStream;
    expectedReplyStringStream << R"({)";
    expectedReplyStringStream << R"("_typeName":"joynr.Reply",)";
    expectedReplyStringStream << R"("response":[[{)";
    expectedReplyStringStream << R"("_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedReplyStringStream << R"("longitude":1.1,)";
    expectedReplyStringStream << R"("latitude":2.2,)";
    expectedReplyStringStream << R"("altitude":3.3,)";
    expectedReplyStringStream << R"("gpsFix":"MODE3D",)";
    expectedReplyStringStream << R"("heading":0.0,)";
    expectedReplyStringStream << R"("quality":0.0,)";
    expectedReplyStringStream << R"("elevation":0.0,)";
    expectedReplyStringStream << R"("bearing":0.0,)";
    expectedReplyStringStream << R"("gpsTime":0,)";
    expectedReplyStringStream << R"("deviceTime":0,)";
    expectedReplyStringStream << R"("time":17)";
    expectedReplyStringStream << R"(},{)";
    expectedReplyStringStream << R"("_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedReplyStringStream << R"("longitude":4.4,)";
    expectedReplyStringStream << R"("latitude":5.5,)";
    expectedReplyStringStream << R"("altitude":6.6,)";
    expectedReplyStringStream << R"("gpsFix":"MODENOFIX",)";
    expectedReplyStringStream << R"("heading":0.0,)";
    expectedReplyStringStream << R"("quality":0.0,)";
    expectedReplyStringStream << R"("elevation":0.0,)";
    expectedReplyStringStream << R"("bearing":0.0,)";
    expectedReplyStringStream << R"("gpsTime":0,)";
    expectedReplyStringStream << R"("deviceTime":0,)";
    expectedReplyStringStream << R"("time":18)";
    expectedReplyStringStream << R"(}]],)";
    expectedReplyStringStream << R"("requestReplyId":")" << reply.getRequestReplyId() << R"(")";
    expectedReplyStringStream << R"(})";

    std::string expectedReply = expectedReplyStringStream.str();

    std::string jsonReply = joynr::serializer::serializeToJson(reply);
    JOYNR_LOG_DEBUG(logger(), jsonReply);
    EXPECT_EQ(expectedReply, jsonReply);

    joynr::Reply receivedReply;
    joynr::serializer::deserializeFromJson(receivedReply, jsonReply);

    GpsLocationList receivedLocList;
    receivedReply.getResponse(receivedLocList);

    EXPECT_EQ(locList, receivedLocList);
}

TEST_F(JsonSerializerTest, serialize_deserialize_trip)
{
    std::vector<types::Localisation::GpsLocation> locations;
    locations.push_back(types::Localisation::GpsLocation(
            1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 17));
    locations.push_back(types::Localisation::GpsLocation(
            4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 317));
    locations.push_back(types::Localisation::GpsLocation(7.7,
                                                         8.8,
                                                         9.9,
                                                         types::Localisation::GpsFixEnum::MODE3D,
                                                         0.0,
                                                         0.0,
                                                         0.0,
                                                         0.0,
                                                         0,
                                                         0,
                                                         3317));

    std::string expected(
            R"({"_typeName":"joynr.types.Localisation.Trip",)"
            R"("locations":[{"_typeName":"joynr.types.Localisation.GpsLocation",)"
            R"("longitude":1.1,)"
            R"("latitude":2.2,)"
            R"("altitude":3.3,)"
            R"("gpsFix":"MODE3D",)"
            R"("heading":0.0,)"
            R"("quality":0.0,)"
            R"("elevation":0.0,)"
            R"("bearing":0.0,)"
            R"("gpsTime":0,)"
            R"("deviceTime":0,)"
            R"("time":17},)"
            R"({"_typeName":"joynr.types.Localisation.GpsLocation",)"
            R"("longitude":4.4,)"
            R"("latitude":5.5,)"
            R"("altitude":6.6,)"
            R"("gpsFix":"MODE3D",)"
            R"("heading":0.0,)"
            R"("quality":0.0,)"
            R"("elevation":0.0,)"
            R"("bearing":0.0,)"
            R"("gpsTime":0,)"
            R"("deviceTime":0,)"
            R"("time":317},)"
            R"({"_typeName":"joynr.types.Localisation.GpsLocation",)"
            R"("longitude":7.7,)"
            R"("latitude":8.8,)"
            R"("altitude":9.9,)"
            R"("gpsFix":"MODE3D",)"
            R"("heading":0.0,)"
            R"("quality":0.0,)"
            R"("elevation":0.0,)"
            R"("bearing":0.0,)"
            R"("gpsTime":0,)"
            R"("deviceTime":0,)"
            R"("time":3317}],)"
            R"("tripTitle":"trip1_name"})");

    // Expected literal is:
    types::Localisation::Trip trip1(locations, "trip1_name");

    std::string serializedContent = joynr::serializer::serializeToJson(trip1);
    EXPECT_EQ(expected, serializedContent);

    types::Localisation::Trip trip2;
    joynr::serializer::deserializeFromJson(trip2, serializedContent);
    EXPECT_EQ(trip1, trip2) << "trips \n trip1: " << trip1.toString().c_str()
                            << " and \n trip2: " << trip2.toString().c_str()
                            << "\n are not the same";
}

TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequestWithLists)
{

    // creating Request
    using GpsLocationList = std::vector<types::Localisation::GpsLocation>;
    GpsLocationList inputLocationList;
    inputLocationList.push_back(types::Localisation::GpsLocation(
            1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 17));
    inputLocationList.push_back(types::Localisation::GpsLocation(
            4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 317));
    inputLocationList.push_back(
            types::Localisation::GpsLocation(7.7,
                                             8.8,
                                             9.9,
                                             types::Localisation::GpsFixEnum::MODE3D,
                                             0.0,
                                             0.0,
                                             0.0,
                                             0.0,
                                             0,
                                             0,
                                             3317));

    joynr::Request request1;
    request1.setMethodName("serialize_deserialize_JsonRequestTest_method");
    request1.setParams(inputLocationList);
    request1.setParamDatatypes({"List"});

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName":"joynr.Request",)";
    expectedStringStream << R"("methodName":"serialize_deserialize_JsonRequestTest_method",)";
    expectedStringStream << R"("paramDatatypes":["List"],)";
    expectedStringStream << R"("params":[[{"_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedStringStream << R"("longitude":1.1,)";
    expectedStringStream << R"("latitude":2.2,)";
    expectedStringStream << R"("altitude":3.3,)";
    expectedStringStream << R"("gpsFix":"MODE3D",)";
    expectedStringStream << R"("heading":0.0,)";
    expectedStringStream << R"("quality":0.0,)";
    expectedStringStream << R"("elevation":0.0,)";
    expectedStringStream << R"("bearing":0.0,)";
    expectedStringStream << R"("gpsTime":0,)";
    expectedStringStream << R"("deviceTime":0,)";
    expectedStringStream << R"("time":17},)";
    expectedStringStream << R"({"_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedStringStream << R"("longitude":4.4,)";
    expectedStringStream << R"("latitude":5.5,)";
    expectedStringStream << R"("altitude":6.6,)";
    expectedStringStream << R"("gpsFix":"MODE3D",)";
    expectedStringStream << R"("heading":0.0,)";
    expectedStringStream << R"("quality":0.0,)";
    expectedStringStream << R"("elevation":0.0,)";
    expectedStringStream << R"("bearing":0.0,)";
    expectedStringStream << R"("gpsTime":0,)";
    expectedStringStream << R"("deviceTime":0,)";
    expectedStringStream << R"("time":317},)";
    expectedStringStream << R"({"_typeName":"joynr.types.Localisation.GpsLocation",)";
    expectedStringStream << R"("longitude":7.7,)";
    expectedStringStream << R"("latitude":8.8,)";
    expectedStringStream << R"("altitude":9.9,)";
    expectedStringStream << R"("gpsFix":"MODE3D",)";
    expectedStringStream << R"("heading":0.0,)";
    expectedStringStream << R"("quality":0.0,)";
    expectedStringStream << R"("elevation":0.0,)";
    expectedStringStream << R"("bearing":0.0,)";
    expectedStringStream << R"("gpsTime":0,)";
    expectedStringStream << R"("deviceTime":0,)";
    expectedStringStream << R"("time":3317}]],)";
    expectedStringStream << R"("requestReplyId":")" << request1.getRequestReplyId() << R"("})";

    std::string expected = expectedStringStream.str();

    // serializing Request
    std::string serializedContent = joynr::serializer::serializeToJson(request1);
    EXPECT_EQ(expected, serializedContent);

    // deserializing Request
    joynr::Request request2;
    joynr::serializer::deserializeFromJson(request2, serializedContent);
    GpsLocationList deserializedLocationList;
    request2.getParams(deserializedLocationList);
    EXPECT_EQ(inputLocationList, deserializedLocationList);
}

TEST_F(JsonSerializerTest, serialize_deserialize_EndpointAddress)
{
    joynr::system::RoutingTypes::ChannelAddress joynr(
            "TEST_channelId", "TEST_messagingEndpointUrl");
    joynr::system::RoutingTypes::WebSocketAddress wsServer(
            joynr::system::RoutingTypes::WebSocketProtocol::WS, "localhost", 42, "some/path");
    joynr::system::RoutingTypes::WebSocketClientAddress wsClient("TEST_clientId");

    // serialize
    std::string joynrSerialized = joynr::serializer::serializeToJson(joynr);
    std::string wsServerSerialized = joynr::serializer::serializeToJson(wsServer);
    std::string wsClientSerialized = joynr::serializer::serializeToJson(wsClient);

    JOYNR_LOG_DEBUG(logger(), "serialized Joynr address: {}", joynrSerialized);
    JOYNR_LOG_DEBUG(logger(), "serialized WS server address: {}", wsServerSerialized);
    JOYNR_LOG_DEBUG(logger(), "serialized WS client address: {}", wsClientSerialized);

    // deserialize
    joynr::system::RoutingTypes::ChannelAddress joynrDeserialized;
    joynr::serializer::deserializeFromJson(joynrDeserialized, joynrSerialized);
    joynr::system::RoutingTypes::WebSocketAddress wsServerDeserialized;
    joynr::serializer::deserializeFromJson(wsServerDeserialized, wsServerSerialized);
    joynr::system::RoutingTypes::WebSocketClientAddress wsClientDeserialized;
    joynr::serializer::deserializeFromJson(wsClientDeserialized, wsClientSerialized);

    EXPECT_EQ(joynr, joynrDeserialized);
    EXPECT_EQ(wsServer, wsServerDeserialized);
    EXPECT_EQ(wsClient, wsClientDeserialized);
}

TEST_F(JsonSerializerTest, serialize_deserialize_GlobalDiscoveryEntry)
{

    std::string expected(
            R"({"_typeName":"joynr.types.GlobalDiscoveryEntry",)"
            R"("providerVersion":{"_typeName":"joynr.types.Version","majorVersion":-1,")"
            R"(minorVersion":-1},)"
            R"("domain":"domain",)"
            R"("interfaceName":"testInterface",)"
            R"("participantId":"someParticipant",)"
            R"("qos":{)"
            R"("_typeName":"joynr.types.ProviderQos",)"
            R"("customParameters":[],)"
            R"("priority":2,)"
            R"("scope":"GLOBAL",)"
            R"("supportsOnChangeSubscriptions":false},)"
            R"("lastSeenDateMs":123,)"
            R"("expiryDateMs":1234,)"
            R"("publicKeyId":"publicKeyId",)"
            R"("address":"serialized_address"})");

    types::ProviderQos qos;
    qos.setPriority(2);
    types::GlobalDiscoveryEntry globalDiscoveryEntry;
    globalDiscoveryEntry.setDomain("domain");
    globalDiscoveryEntry.setQos(qos);
    globalDiscoveryEntry.setLastSeenDateMs(123);
    globalDiscoveryEntry.setExpiryDateMs(1234);
    globalDiscoveryEntry.setParticipantId("someParticipant");
    globalDiscoveryEntry.setAddress("serialized_address");
    globalDiscoveryEntry.setInterfaceName("testInterface");
    globalDiscoveryEntry.setPublicKeyId("publicKeyId");
    JOYNR_LOG_DEBUG(logger(), "GlobalDiscoveryEntry {}", globalDiscoveryEntry.toString());

    std::string serialized = joynr::serializer::serializeToJson(globalDiscoveryEntry);
    JOYNR_LOG_DEBUG(logger(), "serialized GlobalDiscoveryEntry {} ", serialized);
    EXPECT_EQ(expected, serialized);

    types::GlobalDiscoveryEntry deserializedGDE;
    joynr::serializer::deserializeFromJson(deserializedGDE, serialized);

    EXPECT_EQ(globalDiscoveryEntry, deserializedGDE);
    JOYNR_LOG_DEBUG(logger(), "deserialized GlobalDiscoveryEntry {}", deserializedGDE.toString());
}

TEST_F(JsonSerializerTest, deserialize_ProviderQos)
{
    joynr::types::ProviderQos qos;

    std::string jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[]"
                                ",\"priority\":5,\"scope\":\"LOCAL\","
                                "\"supportsOnChangeSubscriptions\":false}");

    joynr::types::ProviderQos providerQos;
    joynr::serializer::deserializeFromJson(providerQos, jsonProviderQos);

    EXPECT_EQ(providerQos.getScope(), joynr::types::ProviderScope::LOCAL);
    EXPECT_EQ(providerQos.getPriority(), 5);
}

TEST_F(JsonSerializerTest, serialize_ProviderQos)
{
    joynr::types::ProviderQos qos;
    qos.setScope(joynr::types::ProviderScope::LOCAL);
    qos.setPriority(5);

    std::string jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[]"
                                ",\"priority\":5,\"scope\":\"LOCAL\","
                                "\"supportsOnChangeSubscriptions\":false}");

    std::string result = joynr::serializer::serializeToJson(qos);

    EXPECT_EQ(jsonProviderQos, result);
}

TEST_F(JsonSerializerTest, deserialize_GPSLocation)
{

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
            R"("time": 17})");

    joynr::types::Localisation::GpsLocation receivedGps;
    joynr::serializer::deserializeFromJson(receivedGps, jsonGPS);
    EXPECT_EQ(3.3, receivedGps.getAltitude());
}

TEST_F(JsonSerializerTest, serialize_OnchangeWithKeepAliveSubscription)
{

    OnChangeWithKeepAliveSubscriptionQos qos(750, 1000, 100, 900, 1050);

    std::string jsonQos = joynr::serializer::serializeToJson(qos);
    JOYNR_LOG_DEBUG(logger(), "serialized OnChangeWithKeepAliveSubscriptionQos {}", jsonQos);

    OnChangeWithKeepAliveSubscriptionQos desQos;
    joynr::serializer::deserializeFromJson(desQos, jsonQos);

    jsonQos = joynr::serializer::serializeToJson(desQos);
    JOYNR_LOG_DEBUG(logger(), "serialized OnChangeWithKeepAliveSubscriptionQos {}", jsonQos);

    EXPECT_EQ(qos, desQos);
}

struct RoutingEntry
{
    RoutingEntry() : address(nullptr), isGloballyVisible(true)
    {
    }
    explicit RoutingEntry(std::shared_ptr<const joynr::system::RoutingTypes::Address> addressLocal,
                          bool isGloballyVisibleLocal)
            : address(std::move(addressLocal)), isGloballyVisible(isGloballyVisibleLocal)
    {
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(address), MUESLI_NVP(isGloballyVisible));
    }

    std::shared_ptr<const joynr::system::RoutingTypes::Address> address;
    bool isGloballyVisible;
};

TEST_F(JsonSerializerTest, RoutingTypeAddressesSerializerTest)
{
    using RoutingTable = Directory<std::string, RoutingEntry>;
    auto singleThreadedIoService = std::make_shared<SingleThreadedIOService>();
    RoutingTable routingTable("routingTable", singleThreadedIoService->getIOService());

    bool isGloballyVisible = true;
    auto ptrToRoutingWebSocketEntry = std::make_shared<RoutingEntry>(
            std::make_shared<joynr::system::RoutingTypes::WebSocketAddress>(), !isGloballyVisible);
    auto ptrToRoutingChannelAddressEntry = std::make_shared<RoutingEntry>(
            std::make_shared<joynr::system::RoutingTypes::ChannelAddress>(), isGloballyVisible);
    auto ptrToRoutingMqttAddressEntry = std::make_shared<RoutingEntry>(
            std::make_shared<joynr::system::RoutingTypes::MqttAddress>(), isGloballyVisible);
    auto ptrToRoutingBrowserAddressEntry = std::make_shared<RoutingEntry>(
            std::make_shared<joynr::system::RoutingTypes::BrowserAddress>(), !isGloballyVisible);
    auto ptrToRoutingWebSocketClientAddressEntry = std::make_shared<RoutingEntry>(
            std::make_shared<joynr::system::RoutingTypes::WebSocketClientAddress>(),
            !isGloballyVisible);

    routingTable.add("WebSocketAddress", ptrToRoutingWebSocketEntry);
    routingTable.add("ChannelAddress", ptrToRoutingChannelAddressEntry);
    routingTable.add("MqttAddress", ptrToRoutingMqttAddressEntry);
    routingTable.add("BrowserAddress", ptrToRoutingBrowserAddressEntry);
    routingTable.add("WebSocketClientAddress", ptrToRoutingWebSocketClientAddressEntry);

    const std::string serializedRoutingTable = joynr::serializer::serializeToJson(routingTable);
    JOYNR_LOG_TRACE(logger(), serializedRoutingTable);

    RoutingTable deserializedRoutingTable(
            "deserializedRoutingTable", singleThreadedIoService->getIOService());
    joynr::serializer::deserializeFromJson(deserializedRoutingTable, serializedRoutingTable);

    EXPECT_TRUE(boost::starts_with(
            deserializedRoutingTable.lookup("WebSocketAddress")->address->toString(),
            "WebSocketAddress"));
    EXPECT_TRUE(boost::starts_with(
            deserializedRoutingTable.lookup("ChannelAddress")->address->toString(),
            "ChannelAddress"));
    EXPECT_TRUE(boost::starts_with(
            deserializedRoutingTable.lookup("MqttAddress")->address->toString(), "MqttAddress"));
    EXPECT_TRUE(boost::starts_with(
            deserializedRoutingTable.lookup("BrowserAddress")->address->toString(),
            "BrowserAddress"));
    EXPECT_TRUE(boost::starts_with(
            deserializedRoutingTable.lookup("WebSocketClientAddress")->address->toString(),
            "WebSocketClientAddress"));

    EXPECT_FALSE(deserializedRoutingTable.lookup("WebSocketAddress")->isGloballyVisible);
    EXPECT_TRUE(deserializedRoutingTable.lookup("ChannelAddress")->isGloballyVisible);
    EXPECT_TRUE(deserializedRoutingTable.lookup("MqttAddress")->isGloballyVisible);
    EXPECT_FALSE(deserializedRoutingTable.lookup("BrowserAddress")->isGloballyVisible);
    EXPECT_FALSE(deserializedRoutingTable.lookup("WebSocketClientAddress")->isGloballyVisible);
}

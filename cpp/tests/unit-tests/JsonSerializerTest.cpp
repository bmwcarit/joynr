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
#include <QVariant>
#include <limits>
#include "joynr/Util.h"
#include "joynr/types/TestTypes_QtTEnum.h"
#include "joynr/types/TestTypes_QtTStruct.h"
#include "joynr/types/TestTypes_QtTStructExtended.h"
#include "joynr/types/TestTypes_QtTStructComposition.h"
#include "joynr/types/Localisation_QtTrip.h"
#include "joynr/types/QtChannelUrlInformation.h"
#include "joynr/types/QtCapabilityInformation.h"
#include "joynr/types/QtProviderQos.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/JsonSerializer.h"
#include "joynr/joynrlogging.h"
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/system/RoutingTypes_QtChannelAddress.h"
#include "joynr/system/RoutingTypes_QtCommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes_QtWebSocketAddress.h"
#include "joynr/system/RoutingTypes_QtWebSocketClientAddress.h"
#include "joynr/tests/testTypes_QtTestEnum.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/QtOnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/QtPeriodicSubscriptionQos.h"

#include "joynr/infrastructure/DacTypes_QtMasterAccessControlEntry.h"
#include "QTime"
#include <chrono>
#include "qjson/serializer.h"

#include <sstream>

using namespace joynr;
using namespace joynr_logging;
using namespace std::chrono;

// TODO:
// 1. If the decision is made to use c++11, g++ >= version 5.5 then JSON literals can be
//    encoded using raw string literals.

class JsonSerializerTest : public testing::Test {
public:
    JsonSerializerTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "JsonSerializerTest"))
    {

    }

    void test(types::TestTypes::QtTStruct tStruct){
        LOG_DEBUG(logger, tStruct.toString());
    }

protected:
    template<class T>
    void serializeDeserializeReply(T value) {

        // setup reply object
        Reply reply;
        reply.setRequestReplyId("TEST-requestReplyId");
        std::vector<Variant> response;
        response.push_back(Variant::make<T>(value));
        reply.setResponse(response);

        std::stringstream expectedReplyStringStream;
        expectedReplyStringStream << R"({"_typeName": "joynr.Reply","requestReplyId": )";
        expectedReplyStringStream << R"(")" << reply.getRequestReplyId() << R"(",)";
        expectedReplyStringStream << R"("response": [)";
        ClassSerializer<T> serializer;
        serializer.serialize(value, expectedReplyStringStream);
        expectedReplyStringStream << R"(]})";

        std::string expectedReplyString = expectedReplyStringStream.str();

        std::string jsonReply = JsonSerializer::serialize(reply);

        EXPECT_EQ(expectedReplyString, jsonReply);

        Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);
        LOG_DEBUG(logger, QString("receivedReply->getResponse().at(0).get<std::string>()  : %1").arg(QString::number(receivedReply->getResponse().at(0).get<T>())));

        EXPECT_EQ(value, receivedReply->getResponse().at(0).get<T>());

        // clean up
        delete receivedReply;
    }

    joynr_logging::Logger* logger;
};

TEST_F(JsonSerializerTest, serialize_deserialize_SubscriptionRequest) {
    SubscriptionRequest request;
    Variant subscriptionQos = Variant::make<SubscriptionQos>(SubscriptionQos(5000));
    request.setQos(subscriptionQos);
    std::string result = JsonSerializer::serialize<SubscriptionRequest>(request);
    LOG_DEBUG(logger, QString("result: %1").arg(QString::fromStdString(result)));
    SubscriptionRequest* desRequest = JsonSerializer::deserialize<SubscriptionRequest>(result);
    EXPECT_TRUE(request == *desRequest);

    // Clean up
    delete desRequest;
}

TEST_F(JsonSerializerTest, serialize_deserialize_BroadcastSubscriptionRequest) {
    BroadcastSubscriptionRequest request;
    OnChangeSubscriptionQos qos{5000, 2000};
    request.setQos(qos);
    BroadcastFilterParameters filterParams;
    filterParams.setFilterParameter("MyFilter", "MyFilterValue");
    request.setFilterParameters(Variant::make<BroadcastFilterParameters>(filterParams));
    std::string requestJson = JsonSerializer::serialize<BroadcastSubscriptionRequest>(request);
    LOG_DEBUG(logger, QString::fromStdString(requestJson));
    BroadcastSubscriptionRequest* desRequest = JsonSerializer::deserialize<BroadcastSubscriptionRequest>(requestJson);
    EXPECT_TRUE(request == *desRequest);
}

TEST_F(JsonSerializerTest, serialize_deserialize_JoynrMessage) {

    Request expectedRequest;
    expectedRequest.setMethodName("serialize_JoynrMessage");
    expectedRequest.setRequestReplyId("xyz");
    JoynrMessage expectedJoynrMessage;
    JoynrTimePoint testExpiryDate = time_point_cast<milliseconds>(system_clock::now()) + milliseconds(10000000);
    expectedJoynrMessage.setHeaderExpiryDate(testExpiryDate);
    expectedJoynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST);
    expectedJoynrMessage.setPayload(JsonSerializer::serialize(expectedRequest));
    std::string serializedContent(JsonSerializer::serialize(expectedJoynrMessage));
    LOG_DEBUG(logger, QString("serialize_JoynrMessage: actual  : %1").arg(QString::fromStdString(serializedContent)));

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName": "joynr.JoynrMessage","headerMap": )";
    expectedStringStream << R"({"expiryDate": ")" << testExpiryDate.time_since_epoch().count() << R"(",)";
    expectedStringStream << R"("msgId": ")" << expectedJoynrMessage.getHeaderMessageId() << R"("},)";
    expectedStringStream << R"("payload": "{\"_typeName\": \"joynr.Request\",\"methodName\": \")" << expectedRequest.getMethodName() << R"(\",)";
    expectedStringStream << R"(\"paramDatatypes\": [],\"params\": [],\"requestReplyId\": \")" << expectedRequest.getRequestReplyId() << R"(\"}",)";
    expectedStringStream << R"("type": "request"})";

    std::string expectedString = expectedStringStream.str();
    LOG_DEBUG(logger, QString("serialize_JoynrMessage: expected: %1").arg(QString::fromStdString(expectedString)));
    EXPECT_EQ(expectedString, serializedContent);

    JoynrMessage* joynrMessage = JsonSerializer::deserialize<JoynrMessage>(serializedContent);
    LOG_DEBUG(logger, QString("joynrMessage->getPayload(): %1").arg(QString::fromStdString(joynrMessage->getPayload())));
    Request* request = JsonSerializer::deserialize<Request>(joynrMessage->getPayload());
    LOG_DEBUG(logger, QString("joynrMessage->getType(): %1").arg(QString::fromStdString(joynrMessage->getType())));
    EXPECT_EQ(joynrMessage->getType(), expectedJoynrMessage.getType());
    LOG_DEBUG(logger, QString("request->getMethodName(): %1").arg(QString::fromStdString(request->getMethodName())));
    EXPECT_EQ(request->getMethodName(), expectedRequest.getMethodName());
    // Clean up
    delete request;
    delete joynrMessage;
}

TEST_F(JsonSerializerTest, serialize_deserialize_byte_array) {

    // Build a list to test with
    std::vector<int8_t> list;//{0x01, 0x02, 0x03, 0xff, 0xfe, 0xfd};
    list.push_back(0x01);
    list.push_back(0x02);
    list.push_back(0x03);
    list.push_back(0xff);
    list.push_back(0xfe);
    list.push_back(0xfd);

    // Set the request method name
    Request request;
    request.setMethodName("serialize_deserialize_byte_array");

    // Set the request parameters
    std::vector<Variant> variantVector = TypeUtil::toVectorOfVariants(list);
    request.addParam(Variant::make<std::vector<Variant>>(variantVector), "List");

    // Serialize the request
    std::string serializedContent = JsonSerializer::serialize<Request>(request);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName": "joynr.Request",)" <<
                R"("methodName": "serialize_deserialize_byte_array",)" <<
                R"("paramDatatypes": ["List"],)" <<
                R"("params": [[1,2,3,-1,-2,-3]],)" <<
                R"("requestReplyId": ")" << request.getRequestReplyId() << R"("})";
    std::string expected = expectedStringStream.str();

    LOG_DEBUG(logger, QString("expected: %1").arg(QString::fromStdString(expected)));
    EXPECT_EQ(expected, serializedContent);

    // Deserialize the request
    Request* deserializedRequest = JsonSerializer::deserialize<Request>(serializedContent);
    std::vector<Variant> paramsReceived = deserializedRequest->getParams();
    std::vector<Variant> deserializedVariantList = paramsReceived.at(0).get<std::vector<Variant>>();

    EXPECT_EQ(variantVector, deserializedVariantList);

    std::vector<Variant>::const_iterator i(variantVector.begin());
    int j = 0;
    while (i != variantVector.end()) {
        LOG_DEBUG(logger, QString("expected variant list [%1]=%2").arg(QString::number(j)).arg(QString::number(i->get<uint8_t>())));
        i++;
        j++;
    }

    i = deserializedVariantList.begin();
    j = 0;
    while (i != deserializedVariantList.end()) {
        LOG_DEBUG(logger, QString("deserialized variant list [%1]=%2").arg(QString::number(j)).arg(QString::number(i->get<uint8_t>())));
        i++;
        j++;
    }

    delete deserializedRequest;
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt8) {
    // int8_t alias (signed) char
    int8_t int8MinValue = std::numeric_limits<int8_t>::min();
    int8_t int8MaxValue = std::numeric_limits<int8_t>::max();

    serializeDeserializeReply<int8_t>(int8MinValue);
    serializeDeserializeReply<int8_t>(int8MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt8) {
    // uint8_t alias unsigned char
    uint8_t unsignedInt8MinValue = std::numeric_limits<uint8_t>::min();
    uint8_t unsignedInt8MaxValue = std::numeric_limits<uint8_t>::max();

    serializeDeserializeReply<uint8_t>(unsignedInt8MinValue);
    serializeDeserializeReply<uint8_t>(unsignedInt8MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt16) {
    // int16_t alias (signed) short
    int16_t int16MinValue = std::numeric_limits<int16_t>::min();
    int16_t int16MaxValue = std::numeric_limits<int16_t>::max();

    serializeDeserializeReply<int16_t>(int16MinValue);
    serializeDeserializeReply<int16_t>(int16MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt16) {
    // uint16_t alias unsigned short
    uint16_t unsignedInt16MinValue = std::numeric_limits<uint16_t>::min();
    uint16_t unsignedInt16MaxValue = std::numeric_limits<uint16_t>::max();

    serializeDeserializeReply<uint16_t>(unsignedInt16MinValue);
    serializeDeserializeReply<uint16_t>(unsignedInt16MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt32) {
    // int32_t alias (signed) int
    int32_t int32MinValue = std::numeric_limits<int32_t>::min();
    int32_t int32MaxValue = std::numeric_limits<int32_t>::max();

    serializeDeserializeReply<int32_t>(int32MinValue);
    serializeDeserializeReply<int32_t>(int32MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt32) {
    // uint32_t alias unsigned int
    uint32_t unsignedInt32MinValue = std::numeric_limits<uint32_t>::min();
    uint32_t unsignedInt32MaxValue = std::numeric_limits<uint32_t>::max();

    serializeDeserializeReply<uint32_t>(unsignedInt32MinValue);
    serializeDeserializeReply<uint32_t>(unsignedInt32MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt64) {
    // uint8_t alias (signed) long long
    int64_t int64MinValue = std::numeric_limits<uint8_t>::min();
    uint8_t int64MaxValue = std::numeric_limits<uint8_t>::max();

    serializeDeserializeReply<uint8_t>(int64MinValue);
    serializeDeserializeReply<uint8_t>(int64MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt64) {
    // uint64_t alias unsigned long long
    uint64_t unsignedInt64MinValue = std::numeric_limits<uint64_t>::min();
    uint64_t unsignedInt64MaxValue = std::numeric_limits<uint64_t>::max();

    serializeDeserializeReply<uint64_t>(unsignedInt64MinValue);
    serializeDeserializeReply<uint64_t>(unsignedInt64MaxValue);
}

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params1) {

    int typeId = qRegisterMetaType<tests::testTypes::QtTestEnum::Enum>("tests::testTypes::QtTestEnum::Enum");

    // Set the request method name
    Request request;
    request.setMethodName("methodEnumDoubleParameters");

    // Set the request parameters
    request.addParam(Variant::make<tests::testTypes::TestEnum::Enum>(tests::testTypes::TestEnum::ONE), "joynr.tests.testTypes.TestEnum.Enum");
    request.addParam(Variant::make<double>(2.2), "Double");

    // Serialize the request
    std::string serializedContent = JsonSerializer::serialize<Request>(request);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName": "joynr.Request",)";
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
    std::string serializedContent(R"({"_typeName": "joynr.Request",)"
                                  R"("methodName": "methodEnumDoubleParameters",)"
                                  R"("paramDatatypes": ["joynr.tests.testTypes.TestEnum.Enum","Double"],)"
                                  R"("params": ["ONE",2.2]})");

    Request *request = JsonSerializer::deserialize<Request>(serializedContent);
    std::vector<Variant> params = request->getParams();

    // Check the deserialized values
    Variant enumParam = params.at(0);
    Variant doubleParam = params.at(1);
    EXPECT_TRUE(enumParam.is<std::string>());
    EXPECT_EQ(std::string("ONE"), enumParam.get<std::string>());
    EXPECT_TRUE(doubleParam.is<double>());
    EXPECT_EQ(2.2, doubleParam.get<double>());

    // Extract the parameters in the same way as a RequestInterpreter
    tests::testTypes::TestEnum::Enum value = Util::convertVariantToEnum<tests::testTypes::TestEnum>(enumParam);
    EXPECT_EQ(1, value);
    delete(request);
}

TEST_F(JsonSerializerTest, deserializeTypeWithEnumList) {

    using namespace infrastructure::DacTypes;

    // Deserialize a type containing multiple enum lists
    std::string serializedContent(
                R"({"_typeName": "joynr.infrastructure.DacTypes.MasterAccessControlEntry",)"
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

    infrastructure::DacTypes::MasterAccessControlEntry *mac = JsonSerializer::deserialize<infrastructure::DacTypes::MasterAccessControlEntry>(serializedContent);

    // Check scalar enums
    EXPECT_EQ(Permission::NO, mac->getDefaultConsumerPermission());
    EXPECT_EQ(TrustLevel::LOW, mac->getDefaultRequiredTrustLevel());

    // Check enum lists
    std::vector<Permission::Enum> possibleRequiredPermissions;
    possibleRequiredPermissions.push_back(Permission::YES);
    possibleRequiredPermissions.push_back(Permission::NO);
    EXPECT_EQ(possibleRequiredPermissions, mac->getPossibleConsumerPermissions());

    std::vector<TrustLevel::Enum> possibleRequiredTrustLevels;
    possibleRequiredTrustLevels.push_back(TrustLevel::HIGH);
    possibleRequiredTrustLevels.push_back(TrustLevel::MID);
    possibleRequiredTrustLevels.push_back(TrustLevel::LOW);
    EXPECT_EQ(possibleRequiredTrustLevels, mac->getPossibleRequiredTrustLevels());

    delete(mac);
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

    // Deserialize the result
    infrastructure::DacTypes::MasterAccessControlEntry *mac = JsonSerializer::deserialize<infrastructure::DacTypes::MasterAccessControlEntry>(serializedContent);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMac, *mac);

    delete(mac);
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
    expectedStringStream << R"({"_typeName": "joynr.Request",)";
    expectedStringStream << R"("methodName": "methodStringDoubleParameters",)";
    expectedStringStream << R"("paramDatatypes": ["String","Double","Float"],)";
    expectedStringStream << R"("params": ["testStringParam",3.33,1.24999999e-09],)";
    expectedStringStream << R"("requestReplyId": ")" << request.getRequestReplyId() << R"(")";
    expectedStringStream << R"(})";

    std::string expected = expectedStringStream.str();

    LOG_DEBUG(logger, QString::fromStdString("Serialized method call: "+ serializedContent));
    LOG_DEBUG(logger, QString::fromStdString("Expected method call: "+ expected));

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
                R"("_typeName": "joynr.types.TestTypes.TStruct",)"
                R"("tDouble": 0.123456789,)"
                R"("tInt64": 64,)"
                R"("tString": "myTestString")"
                R"(})"
                );

    std::string serializedContent = JsonSerializer::serialize<types::TestTypes::TStruct>(tStruct);
    LOG_DEBUG(logger, QString::fromStdString(serializedContent));
    EXPECT_EQ(expectedTStruct, serializedContent);

    types::TestTypes::TStruct* tStructDeserialized = JsonSerializer::deserialize<types::TestTypes::TStruct>(serializedContent);

    EXPECT_EQ(tStruct, *tStructDeserialized);

    delete tStructDeserialized;
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
                R"("_typeName": "joynr.types.TestTypes.TStructExtended",)"
                R"("tDouble": 0.123456789,)"
                R"("tInt64": 64,)"
                R"("tString": "myTestString",)"
                R"("tEnum": "TLITERALA",)"
                R"("tInt32": 32)"
                R"(})"
                );

    std::string serializedTStructExt = JsonSerializer::serialize<types::TestTypes::TStructExtended>(tStructExt);
    LOG_DEBUG(logger, QString::fromStdString(serializedTStructExt));

    EXPECT_EQ(expectedTStructExt, serializedTStructExt);
    types::TestTypes::TStructExtended* deserializedTStructExt = JsonSerializer::deserialize<types::TestTypes::TStructExtended>(serializedTStructExt);

    EXPECT_EQ(tStructExt, *deserializedTStructExt);

    delete deserializedTStructExt;
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocation) {
    types::Localisation::GpsLocation gps1(1.1, 1.2, 1.3, types::Localisation::GpsFixEnum::MODE3D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);

    // Expected literal is:
    Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    std::vector<Variant> response;
    response.push_back(Variant::make<types::Localisation::GpsLocation>(gps1));
    reply.setResponse(response);

    std::stringstream expectedReplyStringStream;
    expectedReplyStringStream << R"({)";
    expectedReplyStringStream << R"("_typeName": "joynr.Reply",)";
    expectedReplyStringStream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
    expectedReplyStringStream << R"("response": [{)";
    expectedReplyStringStream << R"("_typeName": "joynr.types.Localisation.GpsLocation",)";
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

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply->getResponse().size() == 1);
    EXPECT_TRUE(receivedReply->getResponse().at(0).is<types::Localisation::GpsLocation>());

    types::Localisation::GpsLocation gps2 = receivedReply->getResponse().at(0).get<types::Localisation::GpsLocation>();

    EXPECT_EQ(gps1, gps2)
            << "Gps locations gps1 " << gps1.toString()
            << " and gps2 " << gps2.toString() << " are not the same";

    // Clean up
    delete receivedReply;
}

TEST_F(JsonSerializerTest, deserialize_replyWithVoid) {

    // null response with type invalid
    std::vector<Variant> response;
    Reply reply;
    reply.setRequestReplyId("TEST-requestReplyId");
    reply.setResponse(response);

    std::stringstream expectedStringStream;
    expectedStringStream << R"({"_typeName": "joynr.Reply",)";
    expectedStringStream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
    expectedStringStream << R"("response": []})";
    std::string expected = expectedStringStream.str();

    std::string jsonReply = JsonSerializer::serialize<Reply>(reply);

    EXPECT_EQ(expected, jsonReply);

    LOG_DEBUG(logger, "Serialized Reply: "+ QString::fromStdString(jsonReply));

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    // Clean up
    delete receivedReply;
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
    reply.setResponse(response);

    EXPECT_EQ(reply.getResponse().size(), 1);

    // Expected literal
    std::stringstream expectedReplyStringStream;
    expectedReplyStringStream << R"({)";
    expectedReplyStringStream << R"("_typeName": "joynr.Reply",)";
    expectedReplyStringStream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
    expectedReplyStringStream << R"("response": [[{)";
    expectedReplyStringStream << R"("_typeName": "joynr.types.Localisation.GpsLocation",)";
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
    expectedReplyStringStream << R"("_typeName": "joynr.types.Localisation.GpsLocation",)";
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
    LOG_DEBUG(logger, QString::fromStdString(jsonReply));
    //EXPECT_EQ(expectedReply, jsonReply);

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply->getResponse().at(0).is<std::vector<Variant>>());
    std::vector<Variant> receivedReplyResponse = receivedReply->getResponse().at(0).get<std::vector<Variant>>();
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
        LOG_DEBUG(logger, QString::fromStdString(receivedIterValue.toString()));
        EXPECT_EQ(originItervalue, receivedIterValue);
        i_received++;
        i_origin++;
    }

    delete receivedReply;
}

//TEST_F(JsonSerializerTest, serialize_deserialize_trip) {
//    qRegisterMetaType<joynr::types::Localisation::QtGpsLocation>("joynr::types::Localisation::QtGpsLocation");
//    qRegisterMetaType<joynr__types__Localisation__QtGpsLocation>("joynr__types__Localisation__GpsLocation");

//    qRegisterMetaType<joynr::types::Localisation::QtTrip>("joynr::types::Localisation::QtTrip");

//    QList<types::Localisation::QtGpsLocation> locations;
//    locations.push_back(types::Localisation::QtGpsLocation(1.1, 2.2, 3.3, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,17));
//    locations.push_back(types::Localisation::QtGpsLocation(4.4, 5.5, 6.6, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,317));
//    locations.push_back(types::Localisation::QtGpsLocation(7.7, 8.8, 9.9, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,3317));

//    QByteArray expected("{\"_typeName\":\"joynr.types.Localisation.Trip\","
//                        "\"locations\":[{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
//                        "\"altitude\":3.3,"
//                        "\"bearing\":0.0,"
//                        "\"deviceTime\":0,"
//                        "\"elevation\":0.0,"
//                        "\"gpsFix\":\"MODE3D\","
//                        "\"gpsTime\":0,"
//                        "\"heading\":0.0,"
//                        "\"latitude\":2.2,"
//                        "\"longitude\":1.1,"
//                        "\"quality\":0.0,"
//                        "\"time\":17},"
//                        "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
//                        "\"altitude\":6.6,"
//                        "\"bearing\":0.0,"
//                        "\"deviceTime\":0,"
//                        "\"elevation\":0.0,"
//                        "\"gpsFix\":\"MODE3D\","
//                        "\"gpsTime\":0,"
//                        "\"heading\":0.0,"
//                        "\"latitude\":5.5,"
//                        "\"longitude\":4.4,"
//                        "\"quality\":0.0,"
//                        "\"time\":317},"
//                        "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
//                        "\"altitude\":9.9,"
//                        "\"bearing\":0.0,"
//                        "\"deviceTime\":0,"
//                        "\"elevation\":0.0,"
//                        "\"gpsFix\":\"MODE3D\","
//                        "\"gpsTime\":0,"
//                        "\"heading\":0.0,"
//                        "\"latitude\":8.8,"
//                        "\"longitude\":7.7,"
//                        "\"quality\":0.0,"
//                        "\"time\":3317}],"
//                        "\"tripTitle\":\"trip1_name\"}");

//    // Expected literal is:
//    types::Localisation::QtTrip trip1(locations, QString("trip1_name"));

//    QByteArray serializedContent = JsonSerializer::serializeQObject(QVariant::fromValue(trip1));
//    EXPECT_EQ(expected, serializedContent);

//    types::Localisation::QtTrip* trip2 = JsonSerializer::deserializeQObject<types::Localisation::QtTrip>(serializedContent);
//    EXPECT_EQ(trip1, *trip2) << "trips \n trip1: " << trip1.toString().toLatin1().data()
//                             << " and \n trip2: " << trip2->toString().toLatin1().data()
//                             << "\n are not the same";;

//    delete trip2;
//}


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
    LOG_DEBUG(logger, QString::fromStdString(serializedContent));

    Request* request2 = JsonSerializer::deserialize<Request>(serializedContent);

    std::vector<Variant> paramsReceived = request2->getParams();

    EXPECT_EQ(paramsReceived.at(0).get<std::string>(), contentParam1);
    EXPECT_EQ(paramsReceived.at(1).get<types::Localisation::Trip>(), trip1);

    EXPECT_EQ(request1, *request2);

    delete request2;
}

//TEST_F(JsonSerializerTest, serialize_deserialize_Reply_with_Array_as_Response) {
//    std::vector<types::CapabilityInformation> capabilityInformations;
//    types::CapabilityInformation cap1(types::CapabilityInformation("domain1", "interface1", types::ProviderQos(), "channel1", "participant1"));
//    capabilityInformations.push_back(cap1);
//    capabilityInformations.push_back(types::CapabilityInformation("domain2", "interface2", types::ProviderQos(), "channel2", "participant2"));

//    Reply reply;

//    std::vector<Variant> response;
//    reply.setRequestReplyId("serialize_deserialize_Reply_with_Array_as_Response");
//    //this is the magic code: do not call "append" for lists, because this will append all list elements to the outer list
//    response.push_back(joynr::TypeUtil::toVariant(capabilityInformations));
//    reply.setResponse(response);
//    QByteArray serializedContent = JsonSerializer::serializeQObject(reply);
//    LOG_DEBUG(logger, QString(serializedContent));

//    Reply* deserializedReply = JsonSerializer::deserializeQObject<Reply>(serializedContent);

//    response = deserializedReply->getResponse();

//    std::vector<Variant> receivedCaps = response.at(0).get<std::vector<Variant>>();
//    types::CapabilityInformation receivedCap1 = receivedCaps.at(0).get<types::CapabilityInformation>();
//    EXPECT_EQ(receivedCap1, cap1);
//    EXPECT_EQ(deserializedReply->getRequestReplyId(), "serialize_deserialize_Reply_with_Array_as_Response");

//    delete deserializedReply;
//}

//TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequestWithLists) {

//    //creating Request
//    std::vector<types::Localisation::GpsLocation> inputLocationList;
//    inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,17));
//    inputLocationList.push_back(types::Localisation::GpsLocation(4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,317));
//    inputLocationList.push_back(types::Localisation::GpsLocation(7.7, 8.8, 9.9, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,3317));

//    QString expectedString(
//                "{\"_typeName\":\"joynr.Request\","
//                "\"methodName\":\"serialize_deserialize_JsonRequestTest_method\","
//                "\"paramDatatypes\":[\"List\"],"
//                "\"params\":[[{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
//                "\"altitude\":3.3,"
//                "\"bearing\":0.0,"
//                "\"deviceTime\":0,"
//                "\"elevation\":0.0,"
//                "\"gpsFix\":\"MODE3D\","
//                "\"gpsTime\":0,"
//                "\"heading\":0.0,"
//                "\"latitude\":2.2,"
//                "\"longitude\":1.1,"
//                "\"quality\":0.0,"
//                "\"time\":17},"
//                "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
//                "\"altitude\":6.6,"
//                "\"bearing\":0.0,"
//                "\"deviceTime\":0,"
//                "\"elevation\":0.0,"
//                "\"gpsFix\":\"MODE3D\","
//                "\"gpsTime\":0,"
//                "\"heading\":0.0,"
//                "\"latitude\":5.5,"
//                "\"longitude\":4.4,"
//                "\"quality\":0.0,"
//                "\"time\":317},"
//                "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
//                "\"altitude\":9.9,"
//                "\"bearing\":0.0,"
//                "\"deviceTime\":0,"
//                "\"elevation\":0.0,"
//                "\"gpsFix\":\"MODE3D\","
//                "\"gpsTime\":0,"
//                "\"heading\":0.0,"
//                "\"latitude\":8.8,"
//                "\"longitude\":7.7,"
//                "\"quality\":0.0,"
//                "\"time\":3317}]],"
//                "\"requestReplyId\":\"%1\"}"
//                );

//    Request request1;
//    request1.setMethodName("serialize_deserialize_JsonRequestTest_method");
//    std::vector<Variant> inputvl = TypeUtil::toVectorOfVariants(inputLocationList);
//    request1.addParam(Variant::make<std::vector<Variant>>(inputvl), "List");

//    expectedString = expectedString.arg(TypeUtil::toQt(request1.getRequestReplyId()));
//    QByteArray expected = expectedString.toUtf8();

//    //serializing Request
//    QByteArray serializedContent = JsonSerializer::serializeQObject(request1);
//    EXPECT_EQ(expected, serializedContent);

//    //deserializing Request
//    Request* request2 = JsonSerializer::deserializeQObject<Request>(serializedContent);
//    std::vector<Variant> paramsReceived = request2->getParams();

//    LOG_DEBUG(logger, QString("x1%1").arg(TypeUtil::toQt(paramsReceived.at(0).getTypeName())));
//    ASSERT_TRUE(paramsReceived.at(0).is<std::vector<Variant>>()) << "Cannot convert the field of the Param Map to a std::vector<Variant>";
//    std::vector<Variant> returnvl = paramsReceived.at(0).get<std::vector<Variant>>();
//    ASSERT_TRUE(returnvl.size() == 3) << "list size size != 3";
//    LOG_DEBUG(logger, QString("%1").arg(TypeUtil::toQt(returnvl.at(0).getTypeName())));

//    ASSERT_TRUE(returnvl.at(0).is<types::Localisation::GpsLocation>()) << "Cannot convert the first entry of the return List to QtGpsLocation";

//    std::vector<types::Localisation::GpsLocation> resultLocationList = Util::convertVariantVectorToVector<types::Localisation::GpsLocation>(returnvl);
//    EXPECT_EQ(resultLocationList.at(1), types::Localisation::GpsLocation(4.4, 5.5, 6.6, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0, 317));

//    delete request2;
//}

//// Test to investigate whether long lists cause a problem with during deserialization
//TEST_F(JsonSerializerTest, serialize_deserialize_ListComplexity) {

//    //creating Request
//    std::vector<types::Localisation::GpsLocation> inputLocationList;

//    // Create a JSON list
//    int firstListSize = 10000;
//    for (int i = 0; i < firstListSize; i++) {
//        inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0, 17));
//    }

//    // Create a request that uses this list
//    Request request1;
//    request1.setMethodName("serialize_deserialize_JsonRequestTest_method");
//    std::vector<Variant> params1;
//    std::vector<Variant> inputvl = TypeUtil::toVectorOfVariants(inputLocationList);
//    params1.push_back(Variant::make<std::vector<Variant>>(inputvl));
//    request1.setParams(params1);

//    //Time request serialization
//    QTime serializationTime;
//    serializationTime.start();
//    QByteArray serializedContent = JsonSerializer::serializeQObject(request1);
//    int serializationElapsed1 = serializationTime.elapsed();

//    // Time request deserialization
//    QTime deserializationTime;
//    deserializationTime.start();
//    Request* deserialized1 = JsonSerializer::deserializeQObject<Request>(serializedContent);
//    int deserializationElapsed1 = deserializationTime.elapsed();

//    // Double the size of the JSON list
//    for (int i = 0; i < firstListSize; i++) {
//        inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE3D, 0.0, 0,0,0,0,0,17));
//    }

//    // Create a request that uses this bigger list
//    Request request2;
//    request2.setMethodName("serialize_deserialize_JsonRequestTest_method");
//    std::vector<Variant> params2;
//    std::vector<Variant> inputv2 = TypeUtil::toVectorOfVariants(inputLocationList);
//    params2.push_back(Variant::make<std::vector<Variant>>(inputv2));
//    request2.setParams(params2);

//    // Time request serialization
//    serializationTime.restart();
//    serializedContent = JsonSerializer::serializeQObject(request2);
//    int serializationElapsed2 = serializationTime.elapsed();

//    // Time request deserialization
//    deserializationTime.restart();
//    Request* deserialized2 = JsonSerializer::deserializeQObject<Request>(serializedContent);
//    int deserializationElapsed2 = deserializationTime.elapsed();

//    LOG_DEBUG(logger, QString("%1 Objects serialized in %2 milliseconds").arg(firstListSize).arg(serializationElapsed1));
//    LOG_DEBUG(logger, QString("%1 Objects serialized in %2 milliseconds").arg(firstListSize * 2).arg(serializationElapsed2));
//    LOG_DEBUG(logger, QString("%1 Objects deserialized in %2 milliseconds").arg(firstListSize).arg(deserializationElapsed1));
//    LOG_DEBUG(logger, QString("%1 Objects deserialized in %2 milliseconds").arg(firstListSize * 2).arg(deserializationElapsed2));

//    // Assert O(N) complexity
//    ASSERT_TRUE(serializationElapsed2 < serializationElapsed1 * serializationElapsed1);
//    ASSERT_TRUE(deserializationElapsed2 < deserializationElapsed1 * deserializationElapsed1);

//    delete deserialized1;
//    delete deserialized2;
//}

//TEST_F(JsonSerializerTest, serialize_deserialize_EndpointAddress) {
//    qRegisterMetaType<joynr::system::RoutingTypes::QtChannelAddress>("joynr::system::RoutingTypes::QtChannelAddress");
//    qRegisterMetaType<joynr::system::RoutingTypes::QtCommonApiDbusAddress>("joynr::system::RoutingTypes::QtCommonApiDbusAddress");

//    joynr::system::RoutingTypes::QtChannelAddress joynr("TEST_channelId");
//    joynr::system::RoutingTypes::QtCommonApiDbusAddress dbus("domain", "interfacename", "id");
//    joynr::system::RoutingTypes::QtWebSocketAddress wsServer(
//                joynr::system::RoutingTypes::QtWebSocketProtocol::WS,
//                "localhost",
//                42,
//                "some/path"
//                );
//    joynr::system::RoutingTypes::QtWebSocketClientAddress wsClient("TEST_clientId");

//    // serialize
//    QByteArray joynrSerialized = JsonSerializer::serializeQObject(joynr);
//    QByteArray dbusSerialized = JsonSerializer::serializeQObject(dbus);
//    QByteArray wsServerSerialized = JsonSerializer::serializeQObject(wsServer);
//    QByteArray wsClientSerialized = JsonSerializer::serializeQObject(wsClient);

//    LOG_DEBUG(logger, "serialized Joynr address: "+ QString(joynrSerialized));
//    LOG_DEBUG(logger, "serialized Dbus address: "+ QString(dbusSerialized));
//    LOG_DEBUG(logger, QString("serialized WS server address: %0").arg(QString(wsServerSerialized)));
//    LOG_DEBUG(logger, QString("serialized WS client address: %0").arg(QString(wsClientSerialized)));

//    // deserialize
//    joynr::system::RoutingTypes::QtChannelAddress* joynrDeserialized = JsonSerializer::deserializeQObject<joynr::system::RoutingTypes::QtChannelAddress>(joynrSerialized);
//    joynr::system::RoutingTypes::QtCommonApiDbusAddress* dbusDeserialized = JsonSerializer::deserializeQObject<joynr::system::RoutingTypes::QtCommonApiDbusAddress>(dbusSerialized);
//    joynr::system::RoutingTypes::QtWebSocketAddress* wsServerDeserialized = JsonSerializer::deserializeQObject<joynr::system::RoutingTypes::QtWebSocketAddress>(wsServerSerialized);
//    joynr::system::RoutingTypes::QtWebSocketClientAddress* wsClientDeserialized = JsonSerializer::deserializeQObject<joynr::system::RoutingTypes::QtWebSocketClientAddress>(wsClientSerialized);

//    EXPECT_EQ(joynr, *joynrDeserialized);
//    EXPECT_EQ(dbus, *dbusDeserialized);
//    EXPECT_EQ(wsServer, *wsServerDeserialized);
//    EXPECT_EQ(wsClient, *wsClientDeserialized);

//    delete joynrDeserialized;
//    delete dbusDeserialized;
//    delete wsServerDeserialized;
//    delete wsClientDeserialized;
//}

//TEST_F(JsonSerializerTest, serialize_deserialize_CapabilityInformation) {

//    QByteArray expected("{\"_typeName\":\"joynr.types.CapabilityInformation\","
//                        "\"channelId\":\"channeldId\","
//                        "\"domain\":\"domain\","
//                        "\"interfaceName\":\"\","
//                        "\"participantId\":\"\","
//                        "\"providerQos\":{"
//                        "\"_typeName\":\"joynr.types.ProviderQos\","
//                        "\"customParameters\":[],"
//                        "\"priority\":2,"
//                        "\"providerVersion\":4,"
//                        "\"scope\":\"GLOBAL\","
//                        "\"supportsOnChangeSubscriptions\":false}}");

//    // Expected literal is:
//    // { "_typeName" : "joynr.types.CapabilityInformation", "channelId" : "channeldId", "domain" : "domain", "interfaceName" : "", "participantId" : "", "providerQos" : { "_typeName" : "joynr.types.ProviderQos", "isLocalOnly" : false, "onChangeSubscriptions" : false, "priority" : 2, "qos" : [  ], "Version" : 4 }

//    //TODO: also remove the variables qRegisterMetaTypeQos
//    qRegisterMetaType<joynr::types::QtProviderQos>("joynr::types::QtProviderQos");
//    qRegisterMetaType<joynr__types__QtProviderQos>("joynr__types__ProviderQos");
//    qRegisterMetaType<joynr::types::QtCapabilityInformation>("joynr::types::QtCapabilityInformation");
//    qRegisterMetaType<joynr__types__QtCapabilityInformation>("joynr__types__CapabilityInformation");

//    types::QtProviderQos qos;
//    qos.setPriority(2);
//    qos.setProviderVersion(4);
//    types::QtCapabilityInformation capabilityInformation;
//    capabilityInformation.setChannelId("channeldId");
//    capabilityInformation.setDomain("domain");
//    capabilityInformation.setProviderQos(qos);
//    LOG_DEBUG(logger,"capabilityInformation" + capabilityInformation.toString());

//    QByteArray serialized = JsonSerializer::serializeQObject(QVariant::fromValue(capabilityInformation));
//    EXPECT_EQ(expected, serialized);
//    // deserialize
//    LOG_DEBUG(logger,"serialized capabilityInformation" + QString(serialized));

//    types::QtCapabilityInformation* deserializedCI = JsonSerializer::deserializeQObject<types::QtCapabilityInformation>(serialized);

//    EXPECT_EQ(capabilityInformation, *deserializedCI);
//    LOG_DEBUG(logger,"deserialized capabilityInformation" + deserializedCI->toString());
//}

//// Test of ChannelURLInformation which is of type QList<QString>.
//// QList<QString> is a special case in some places (for an example see Request.h)
//TEST_F(JsonSerializerTest, serialize_deserialize_ChannelURLInformation) {
//    qRegisterMetaType<joynr::types::QtChannelUrlInformation>("joynr::types::QtChannelUrlInformation");
//    qRegisterMetaType<joynr__types__QtChannelUrlInformation>("joynr__types__ChannelUrlInformation");

//    QList<QString> urls;
//    urls.append("http://example1.com/");
//    urls.append("http://example2.com/");
//    types::QtChannelUrlInformation urlInformation(urls);

//    // Serialize the URL Information
//    QByteArray serialized = JsonSerializer::serializeQObject(QVariant::fromValue(urlInformation));
//    LOG_DEBUG(logger,"serialized QtChannelUrlInformation" + QString(serialized));

//    // Expected JSON : { "_typeName" : "joynr.types.ChannelUrlInformation", "urls" : [ "http://example1.com/", "http://example2.com/" ] }
//    QByteArray expected("{\"_typeName\":\"joynr.types.ChannelUrlInformation\",\"urls\":[\"http://example1.com/\",\"http://example2.com/\"]}");

//    EXPECT_EQ(expected, serialized);

//    // Deserialize
//    types::QtChannelUrlInformation* deserializedInfo = JsonSerializer::deserializeQObject<types::QtChannelUrlInformation>(serialized);

//    // Check the structure
//    QList<QString> deserializedUrls = deserializedInfo->getUrls();
//    EXPECT_EQ(urls, deserializedUrls);
//}

//TEST_F(JsonSerializerTest, deserialize_ProviderQos) {
//    joynr::types::QtProviderQos qos;

//    QByteArray jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[],\"priority\":5,\"providerVersion\":3,\"scope\":\"LOCAL\",\"supportsOnChangeSubscriptions\":false}");

//    joynr::types::QtProviderQos* providerQos = JsonSerializer::deserializeQObject<joynr::types::QtProviderQos>(jsonProviderQos);

//    EXPECT_EQ(providerQos->getScope(), joynr::types::QtProviderScope::LOCAL);
//    EXPECT_EQ(providerQos->getProviderVersion(), 3);
//    EXPECT_EQ(providerQos->getPriority(), 5);

//    delete providerQos;
//}

//TEST_F(JsonSerializerTest, serialize_ProviderQos) {
//    joynr::types::QtProviderQos qos;
//    qos.setScope(joynr::types::QtProviderScope::LOCAL);
//    qos.setPriority(5);
//    qos.setProviderVersion(-1);

//    QByteArray jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[],\"priority\":5,\"providerVersion\":-1,\"scope\":\"LOCAL\",\"supportsOnChangeSubscriptions\":false}");

//    QByteArray result = JsonSerializer::serializeQObject(qos);

//    EXPECT_EQ(jsonProviderQos, result);
//}


//TEST_F(JsonSerializerTest, deserialize_GPSLocation) {
//    qRegisterMetaType<joynr::types::Localisation::QtGpsLocation>("joynr::types::Localisation::QtGpsLocation");
//    qRegisterMetaType<types::Localisation::QtGpsFixEnum>();
//    qRegisterMetaType<types::Localisation::QtGpsFixEnum::Enum>("types::Localisation::QtGpsFixEnum");

//    QByteArray jsonGPS(
//                "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
//                "\"accuracy\":0.0,"
//                "\"altitude\":3.3,"
//                "\"bearing\":0.0,"
//                "\"deviceTime\":0,"
//                "\"elevation\":0.0,"
//                "\"gpsFix\":1,"
//                "\"gpsTime\":0,"
//                "\"latitude\":2.2,"
//                "\"longitude\":1.1,"
//                "\"time\":17}"
//                );
//    joynr::types::Localisation::QtGpsLocation* receivedReply = JsonSerializer::deserializeQObject<joynr::types::Localisation::QtGpsLocation>(jsonGPS);
//    // Clean up
//    delete receivedReply;
//}

//TEST_F(JsonSerializerTest, serialize_OnchangeWithKeepAliveSubscription) {

//    joynr::QtOnChangeWithKeepAliveSubscriptionQos qos(750, 100, 900, 1050);

//    QByteArray jsonQos = JsonSerializer::serializeQObject(qos);
//    LOG_DEBUG(logger,"serialized QtOnChangeWithKeepAliveSubscriptionQos" + QString(jsonQos));

//    joynr::QtOnChangeWithKeepAliveSubscriptionQos* desQos = JsonSerializer::deserializeQObject<joynr::QtOnChangeWithKeepAliveSubscriptionQos>(jsonQos);

//    jsonQos = JsonSerializer::serializeQObject(*desQos);
//    LOG_DEBUG(logger,"serialized QtOnChangeWithKeepAliveSubscriptionQos" + QString(jsonQos));


//    EXPECT_EQ(qos, *desQos);

//    // Clean up
//    delete desQos;
//}

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
        qRegisterMetaType<joynr::Reply>("joynr::Reply");

        // setup reply object
        Reply reply;
        reply.setRequestReplyId(QString("TEST-requestReplyId"));
        QList<QVariant> response;
        response.append(QVariant::fromValue(value));
        reply.setResponse(response);

        QString expectedReplyString(
                    "{"
                        "\"_typeName\":\"joynr.Reply\","
                        "\"requestReplyId\":\"%1\","
                        "\"response\":[%2]"
                    "}"
        );
        expectedReplyString = expectedReplyString
                .arg(reply.getRequestReplyId())
                .arg(value);
        QByteArray expectedReply = expectedReplyString.toUtf8();

        QByteArray jsonReply = JsonSerializer::serialize(reply);

        EXPECT_EQ_QBYTEARRAY(expectedReply, jsonReply);

        Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

        EXPECT_EQ(value, receivedReply->getResponse().at(0).value<T>());

        // clean up
        delete receivedReply;
    }

    joynr_logging::Logger* logger;
};

TEST_F(JsonSerializerTest, serialize_deserialize_SubscriptionRequest) {
    qRegisterMetaType<joynr::SubscriptionRequest>();
    SubscriptionRequest request;
    std::shared_ptr<QtSubscriptionQos> subscriptionQos(new QtSubscriptionQos(5000));
    request.setQos(subscriptionQos);
    QByteArray result = JsonSerializer::serialize(request);
    LOG_DEBUG(logger, QString(result));
    SubscriptionRequest* desRequest = JsonSerializer::deserialize<SubscriptionRequest>(result);
    EXPECT_TRUE(request == *desRequest);
}

TEST_F(JsonSerializerTest, serialize_deserialize_BroadcastSubscriptionRequest) {
    qRegisterMetaType<joynr::BroadcastSubscriptionRequest>();
    BroadcastSubscriptionRequest request;
    std::shared_ptr<QtOnChangeSubscriptionQos> subscriptionQos(new QtOnChangeSubscriptionQos(5000, 2000));
    request.setQos(subscriptionQos);
    QtBroadcastFilterParameters filterParams;
    filterParams.setFilterParameter("MyFilter", "MyFilterValue");
    request.setFilterParameters(filterParams);
    QByteArray requestJson = JsonSerializer::serialize(request);
    LOG_DEBUG(logger, QString(requestJson));
    BroadcastSubscriptionRequest* desRequest = JsonSerializer::deserialize<BroadcastSubscriptionRequest>(requestJson);
    EXPECT_TRUE(request == *desRequest);
}

TEST_F(JsonSerializerTest, serialize_JoynrMessage) {
    qRegisterMetaType<joynr::Request>();
    qRegisterMetaType<joynr::JoynrMessage>();
    Request request;
    request.setMethodName("serialize_JoynrMessage");
    request.setRequestReplyId("xyz");
    JoynrMessage joynrMessage;
    JoynrTimePoint testExpiryDate = time_point_cast<milliseconds>(system_clock::now()) + milliseconds(10000000);
    joynrMessage.setHeaderExpiryDate(testExpiryDate);
    joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST);
    joynrMessage.setPayload(JsonSerializer::serialize(request));
    QByteArray serializedContent(JsonSerializer::serialize(joynrMessage));
    LOG_DEBUG(logger, QString("serialize_JoynrMessage: actual  : %1").arg(QString(serializedContent)));

    QString expected(
                "{\"_typeName\":\"joynr.JoynrMessage\","
                "\"header\":{\"expiryDate\":\%1,\"msgId\":\"%2\"},"
                "\"payload\":\"{\\\"_typeName\\\":\\\"joynr.Request\\\","
                "\\\"methodName\\\":\\\"%3\\\","
                "\\\"paramDatatypes\\\":[],"
                "\\\"params\\\":[],"
                "\\\"requestReplyId\\\":\\\"%4\\\"}\","
                "\"type\":\"request\"}"
    );
    expected = expected.arg(QString::number(testExpiryDate.time_since_epoch().count())).arg(joynrMessage.getHeaderMessageId()).arg(request.getMethodName()).
            arg(TypeUtil::toQt(request.getRequestReplyId()));

    LOG_DEBUG(logger, QString("serialize_JoynrMessage: expected: %1").arg(expected));
    EXPECT_EQ(expected, QString(serializedContent));
}

TEST_F(JsonSerializerTest, serialize_deserialize_byte_array) {

    // Build a list to test with
    QList<qint8> list;
    list << 0x01 << 0x02 << 0x03 << 0xff << 0xfe << 0xfd;

    qRegisterMetaType<joynr::Request>("joynr::Request");

    // Set the request method name
    Request request;
    request.setMethodName(QString("serialize_deserialize_byte_array"));

    // Set the request parameters
    QVariantList variantList = Util::convertListToVariantList(list);
    request.addParam(variantList, "List");

    // Serialize the request
    QByteArray serializedContent = JsonSerializer::serialize(request);

    QString expected(
                "{\"_typeName\":\"joynr.Request\","
                "\"methodName\":\"serialize_deserialize_byte_array\","
                "\"paramDatatypes\":[\"List\"],"
                "\"params\":[[1,2,3,-1,-2,-3]],"
                "\"requestReplyId\":\"%1\"}"
    );
    expected = expected.arg(TypeUtil::toQt(request.getRequestReplyId()));

    LOG_DEBUG(logger, QString("expected: %1").arg(expected));
    EXPECT_EQ(expected, QString(serializedContent));

    // Deserialize the request
    Request* deserializedRequest = JsonSerializer::deserialize<Request>(serializedContent);
    QList<QVariant> paramsReceived = deserializedRequest->getParams();
    QVariantList deserializedVariantList = paramsReceived.at(0).value<QVariantList>();

    EXPECT_EQ(variantList, deserializedVariantList);

    QListIterator<QVariant> i(variantList);
    int j = 0;
    while (i.hasNext()) {
        LOG_DEBUG(logger, QString("expected variant list [%1]=%2").arg(QString::number(j)).arg(QString::number(i.next().value<qint8>())));
        j++;
    }

    i = QListIterator<QVariant>(deserializedVariantList);
    j = 0;
    while (i.hasNext()) {
        LOG_DEBUG(logger, QString("deserialized variant list [%1]=%2").arg(QString::number(j)).arg(QString::number(i.next().value<qint8>())));
        j++;
    }

    delete deserializedRequest;
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt8) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // qint8 alias (signed) char
    qint8 int8MinValue = std::numeric_limits<qint8>::min();
    qint8 int8MaxValue = std::numeric_limits<qint8>::max();

    serializeDeserializeReply<qint8>(int8MinValue);
    serializeDeserializeReply<qint8>(int8MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt8) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // quint8 alias unsigned char
    quint8 unsignedInt8MinValue = std::numeric_limits<quint8>::min();
    quint8 unsignedInt8MaxValue = std::numeric_limits<quint8>::max();

    serializeDeserializeReply<quint8>(unsignedInt8MinValue);
    serializeDeserializeReply<quint8>(unsignedInt8MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt16) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // qint16 alias (signed) short
    qint16 int16MinValue = std::numeric_limits<qint16>::min();
    qint16 int16MaxValue = std::numeric_limits<qint16>::max();

    serializeDeserializeReply<qint16>(int16MinValue);
    serializeDeserializeReply<qint16>(int16MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt16) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // quint16 alias unsigned short
    quint16 unsignedInt16MinValue = std::numeric_limits<quint16>::min();
    quint16 unsignedInt16MaxValue = std::numeric_limits<quint16>::max();

    serializeDeserializeReply<quint16>(unsignedInt16MinValue);
    serializeDeserializeReply<quint16>(unsignedInt16MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt32) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // qint32 alias (signed) int
    qint32 int32MinValue = std::numeric_limits<qint32>::min();
    qint32 int32MaxValue = std::numeric_limits<qint32>::max();

    serializeDeserializeReply<qint32>(int32MinValue);
    serializeDeserializeReply<qint32>(int32MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt32) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // quint32 alias unsigned int
    quint32 unsignedInt32MinValue = std::numeric_limits<quint32>::min();
    quint32 unsignedInt32MaxValue = std::numeric_limits<quint32>::max();

    serializeDeserializeReply<quint32>(unsignedInt32MinValue);
    serializeDeserializeReply<quint32>(unsignedInt32MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithInt64) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // qint64 alias (signed) long long
    qint64 int64MinValue = std::numeric_limits<qint64>::min();
    qint64 int64MaxValue = std::numeric_limits<qint64>::max();

    serializeDeserializeReply<qint64>(int64MinValue);
    serializeDeserializeReply<qint64>(int64MaxValue);
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithUnsignedInt64) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // quint64 alias unsigned long long
    quint64 unsignedInt64MinValue = std::numeric_limits<quint64>::min();
    quint64 unsignedInt64MaxValue = std::numeric_limits<quint64>::max();

    serializeDeserializeReply<quint64>(unsignedInt64MinValue);
    serializeDeserializeReply<quint64>(unsignedInt64MaxValue);
}

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params1) {

    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<tests::testTypes::QtTestEnum>();
    int typeId = qRegisterMetaType<tests::testTypes::QtTestEnum::Enum>("tests::testTypes::QtTestEnum::Enum");

    // Set the request method name
    Request request;
    request.setMethodName(QString("methodEnumDoubleParameters"));

    // Set the request parameters
    request.addParam(QVariant::fromValue(tests::testTypes::QtTestEnum::ONE), "joynr.tests.TestEnum");
    request.addParam(2.2, "Double");

    // Serialize the request
    QJson::Serializer::registerEnum(typeId, tests::testTypes::QtTestEnum::staticMetaObject.enumerator(0));
    QString serializedContent = JsonSerializer::serialize(request);

    QString expected(
                "{\"_typeName\":\"joynr.Request\","
                "\"methodName\":\"methodEnumDoubleParameters\","
                "\"paramDatatypes\":[\"joynr.tests.TestEnum\",\"Double\"],"
                "\"params\":[\"ONE\",2.2],"
                "\"requestReplyId\":\"%1\"}"
    );
    expected = expected.arg(TypeUtil::toQt(request.getRequestReplyId()));

    EXPECT_EQ_QSTRING(expected, serializedContent);
}

TEST_F(JsonSerializerTest, deserialize_operation_with_enum) {

    qRegisterMetaType<joynr::Request>();
    qRegisterMetaType<tests::testTypes::QtTestEnum>();
    qRegisterMetaType<tests::testTypes::QtTestEnum::Enum>("tests::testTypes::QtTestEnum::Enum");

    // Deserialize a request containing a Java style enum parameter
    QByteArray serializedContent("{\"_typeName\":\"joynr.Request\","
                                 "\"methodName\":\"methodEnumDoubleParameters\","
                                 "\"paramDatatypes\":[\"joynr.tests.TestEnum\",\"Double\"],"
                                 "\"params\":[\"ONE\",2.2]}");

    Request *request = JsonSerializer::deserialize<Request>(serializedContent);
    QList<QVariant> params = request->getParams();

    // Check the deserialized values
    QVariant enumParam = params.at(0);
    QVariant doubleParam = params.at(1);
    EXPECT_TRUE(enumParam.canConvert<QString>());
    EXPECT_EQ(QString("ONE"), enumParam.value<QString>());
    EXPECT_TRUE(doubleParam.canConvert<double>());
    EXPECT_EQ(2.2, doubleParam.value<double>());

    // Extract the parameters in the same way as a RequestInterpreter
    tests::testTypes::QtTestEnum::Enum value = Util::convertVariantToEnum<tests::testTypes::QtTestEnum>(enumParam);
    EXPECT_EQ(1, value);
    delete(request);
}

TEST_F(JsonSerializerTest, deserializeTypeWithEnumList) {

    using namespace infrastructure::DacTypes;

    qRegisterMetaType<QtPermission>();
    qRegisterMetaType<QtPermission::Enum>("joynr::infrastructure::DacTypes::QtPermission::Enum");
    qRegisterMetaType<QtTrustLevel>();
    qRegisterMetaType<QtTrustLevel::Enum>("joynr::infrastructure::DacTypes::QtTrustLevel::Enum");
    qRegisterMetaType<QtMasterAccessControlEntry>("joynr::infrastructure::DacTypes::QtMasterAccessControlEntry");

    // Deserialize a type containing multiple enum lists
    QByteArray serializedContent("{\"_typeName\":\"joynr.infrastructure.DacTypes.MasterAccessControlEntry\","
                                 "\"defaultConsumerPermission\":\"NO\","
                                 "\"defaultRequiredControlEntryChangeTrustLevel\":\"LOW\","
                                 "\"defaultRequiredTrustLevel\":\"LOW\","
                                 "\"domain\":\"unittest\","
                                 "\"interfaceName\":\"vehicle/radio\","
                                 "\"operation\":\"*\","
                                 "\"possibleConsumerPermissions\":[\"YES\",\"NO\"],"
                                 "\"possibleRequiredControlEntryChangeTrustLevels\":[\"HIGH\",\"MID\",\"LOW\"],"
                                 "\"possibleRequiredTrustLevels\":[\"HIGH\",\"MID\",\"LOW\"],"
                                 "\"uid\":\"*\"}");

    infrastructure::DacTypes::QtMasterAccessControlEntry *mac = JsonSerializer::deserialize<infrastructure::DacTypes::QtMasterAccessControlEntry>(serializedContent);

    // Check scalar enums
    EXPECT_EQ(QtPermission::NO, mac->getDefaultConsumerPermission());
    EXPECT_EQ(QtTrustLevel::LOW, mac->getDefaultRequiredTrustLevel());

    // Check enum lists
    QList<QtPermission::Enum> possibleRequiredPermissions;
    possibleRequiredPermissions << QtPermission::YES << QtPermission::NO;
    EXPECT_EQ(possibleRequiredPermissions, mac->getPossibleConsumerPermissions());

    QList<QtTrustLevel::Enum> possibleRequiredTrustLevels;
    possibleRequiredTrustLevels << QtTrustLevel::HIGH << QtTrustLevel::MID << QtTrustLevel::LOW;
    EXPECT_EQ(possibleRequiredTrustLevels, mac->getPossibleRequiredTrustLevels());

    delete(mac);
}

TEST_F(JsonSerializerTest, serializeDeserializeTypeWithEnumList) {

    using namespace infrastructure::DacTypes;

    qRegisterMetaType<QtPermission>();
    qRegisterMetaType<QtPermission::Enum>("joynr::infrastructure::DacTypes::QtPermission::Enum");
    qRegisterMetaType<QtTrustLevel>();
    qRegisterMetaType<QtTrustLevel::Enum>("joynr::infrastructure::DacTypes::QtTrustLevel::Enum");
    qRegisterMetaType<QtMasterAccessControlEntry>("joynr::infrastructure::DacTypes::QtMasterAccessControlEntry");

    QList<QtTrustLevel::Enum> possibleTrustLevels;
    possibleTrustLevels << QtTrustLevel::LOW << QtTrustLevel::MID << QtTrustLevel::HIGH;
    QList<QtPermission::Enum> possiblePermissions;
    possiblePermissions << QtPermission::NO << QtPermission::ASK << QtPermission::YES;

    infrastructure::DacTypes::QtMasterAccessControlEntry expectedMac(QStringLiteral("*"),
                                                         QStringLiteral("unittest"),
                                                         QStringLiteral("vehicle/radio"),
                                                         QtTrustLevel::LOW,
                                                         possibleTrustLevels,
                                                         QtTrustLevel::HIGH,
                                                         possibleTrustLevels,
                                                         QStringLiteral("*"),
                                                         QtPermission::YES,
                                                         possiblePermissions);

    // Serialize
    QByteArray serializedContent = JsonSerializer::serialize(expectedMac);

    // Deserialize the result
    infrastructure::DacTypes::QtMasterAccessControlEntry *mac = JsonSerializer::deserialize<infrastructure::DacTypes::QtMasterAccessControlEntry>(serializedContent);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMac, *mac);

    delete(mac);
}

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params2) {

    qRegisterMetaType<joynr::Request>("joynr::Request");

    // Set the request method name
    Request request;
    request.setMethodName(QString("methodStringDoubleParameters"));

    // Set the request parameters
    request.addParam(QString("testStringParam"), "String");
    request.addParam(3.33, "Double");

    // Serialize the request
    QString serializedContent = JsonSerializer::serialize(request);

    QString expected(
                "{\"_typeName\":\"joynr.Request\","
                "\"methodName\":\"methodStringDoubleParameters\","
                "\"paramDatatypes\":[\"String\",\"Double\"],"
                "\"params\":[\"testStringParam\",3.33],"
                "\"requestReplyId\":\"%1\"}"
    );
    expected = expected.arg(TypeUtil::toQt(request.getRequestReplyId()));

    LOG_DEBUG(logger, "Serialized method call: "+ serializedContent);
    LOG_DEBUG(logger, "Expected method call: "+ expected);

    // Expected literal is:
    // { "_typeName" : "joynr.Request", "methodName" : "methodStringDoubleParameters", "paramDatatypes" : [ "String", "Double" ], "params" : { "doubleParam" : 3.33, "stringParam" : "testStringParam" } }
    EXPECT_EQ(expected, serializedContent);

}


TEST_F(JsonSerializerTest, serialize_deserialize_TStruct) {
    qRegisterMetaType<joynr::types::TestTypes::QtTStruct>("joynr::types::TestTypes::QtTStruct");
    qRegisterMetaType<joynr__types__TestTypes__QtTStruct>("joynr__types__TestTypes__TStruct");

    types::TestTypes::QtTStruct tStruct;
    tStruct.setTDouble(0.123456789);
    tStruct.setTInt64(64);
    tStruct.setTString("myTestString");

    QByteArray expectedTStruct(
                "{"
                    "\"_typeName\":\"joynr.types.TestTypes.TStruct\","
                    "\"tDouble\":0.123456789,"
                    "\"tInt64\":64,"
                    "\"tString\":\"myTestString\""
                "}"
    );

    QByteArray serializedContent = JsonSerializer::serialize(QVariant::fromValue(tStruct));
    LOG_DEBUG(logger, QString::fromUtf8(serializedContent));
    EXPECT_EQ(expectedTStruct, serializedContent);

    types::TestTypes::QtTStruct* tStructDeserialized = JsonSerializer::deserialize<types::TestTypes::QtTStruct>(serializedContent);

    EXPECT_EQ(tStruct, *tStructDeserialized);

    delete tStructDeserialized;
}

TEST_F(JsonSerializerTest, serialize_deserialize_TStructExtended) {
    qRegisterMetaType<joynr::types::TestTypes::QtTStructExtended>("joynr::types::TestTypes::QtTStructExtended");
    qRegisterMetaType<joynr__types__TestTypes__QtTStructExtended>("joynr__types__TestTypes__TStructExtended");

    types::TestTypes::QtTStructExtended tStructExt;
    tStructExt.setTDouble(0.123456789);
    tStructExt.setTInt64(64);
    tStructExt.setTString("myTestString");
    tStructExt.setTEnum(types::TestTypes::QtTEnum::TLITERALA);
    tStructExt.setTInt32(32);

    QByteArray expectedTStructExt(
                "{"
                    "\"_typeName\":\"joynr.types.TestTypes.TStructExtended\","
                    "\"tDouble\":0.123456789,"
                    "\"tEnum\":\"TLITERALA\","
                    "\"tInt32\":32,"
                    "\"tInt64\":64,"
                    "\"tString\":\"myTestString\""
                "}"
    );

    QByteArray serializedTStructExt = JsonSerializer::serialize(QVariant::fromValue(tStructExt));
    LOG_DEBUG(logger, QString::fromUtf8(serializedTStructExt));

    EXPECT_EQ(expectedTStructExt, serializedTStructExt);
    types::TestTypes::QtTStructExtended* deserializedTStructExt = JsonSerializer::deserialize<types::TestTypes::QtTStructExtended>(serializedTStructExt);

    EXPECT_EQ(tStructExt, *deserializedTStructExt);

    delete deserializedTStructExt;
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocation) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");
    types::Localisation::QtGpsLocation gps1(1.1, 1.2, 1.3, types::Localisation::QtGpsFixEnum::MODE3D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
//    types::Localisation::QtGpsLocation gps1(1.1, 2.2, 3.3,types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 17);

    // Expected literal is:
    Reply reply;
    reply.setRequestReplyId(QString("TEST-requestReplyId"));
    QList<QVariant> response;
    response.append(QVariant::fromValue(gps1));
    reply.setResponse(response);

    QString expectedReplyString(
                "{"
                    "\"_typeName\":\"joynr.Reply\","
                    "\"requestReplyId\":\"%1\","
                    "\"response\":[{"
                        "\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                        "\"altitude\":1.3,"
                        "\"bearing\":1.7,"
                        "\"deviceTime\":19,"
                        "\"elevation\":1.6,"
                        "\"gpsFix\":\"MODE3D\","
                        "\"gpsTime\":18,"
                        "\"heading\":1.4,"
                        "\"latitude\":1.2,"
                        "\"longitude\":1.1,"
                        "\"quality\":1.5,"
                        "\"time\":110"
                    "}]"
                "}"
    );

    expectedReplyString = expectedReplyString.arg(reply.getRequestReplyId());
    QByteArray expectedReply = expectedReplyString.toUtf8();

    QByteArray jsonReply = JsonSerializer::serialize(reply);

    EXPECT_EQ(expectedReply, jsonReply);

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply->getResponse().size() == 1);
    EXPECT_TRUE(receivedReply->getResponse().at(0).canConvert<types::Localisation::QtGpsLocation>());

//    QtGpsLocation gps2;

//    QJson::QObjectHelper::qvariant2qobject(receivedReply->getResponse().value<QVariantMap>(), &gps2);

    types::Localisation::QtGpsLocation gps2 = receivedReply->getResponse().at(0).value<types::Localisation::QtGpsLocation>();

    EXPECT_EQ(gps1, gps2)
            << "Gps locations gps1 " << gps1.toString().toLatin1().data()
            << " and gps2 " << gps2.toString().toLatin1().data() << " are not the same";

    // Clean up
    delete receivedReply;
}

TEST_F(JsonSerializerTest, deserialize_replyWithVoid) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // null response with type invalid
    QList<QVariant> response;
    Reply reply;
    reply.setRequestReplyId(QString("TEST-requestReplyId"));
    reply.setResponse(response);

    QString expected(
                "{\"_typeName\":\"joynr.Reply\","
                "\"requestReplyId\":\"%1\","
                "\"response\":[]}"
    );
    expected = expected.arg(reply.getRequestReplyId());


    QByteArray jsonReply = JsonSerializer::serialize(reply);

    EXPECT_EQ(expected, QString(jsonReply));

    LOG_DEBUG(logger, "Serialized Reply: "+ QString::fromUtf8(jsonReply));

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    // Clean up
    delete receivedReply;
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocationList) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    QList<QVariant> locList;
    locList.append(QVariant::fromValue(types::Localisation::QtGpsLocation(1.1, 2.2, 3.3, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0, 0.0, 0.0, 0, 0, 17)));
    locList.append(QVariant::fromValue(types::Localisation::QtGpsLocation(4.4, 5.5, 6.6, types::Localisation::QtGpsFixEnum::MODENOFIX, 0.0, 0.0, 0.0, 0.0, 0, 0, 18)));

    QString expectedReplyString(
                "{"
                    "\"_typeName\":\"joynr.Reply\","
                    "\"requestReplyId\":\"%1\","
                    "\"response\":[[{"
                        "\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                        "\"altitude\":3.3,"
                        "\"bearing\":0.0,"
                        "\"deviceTime\":0,"
                        "\"elevation\":0.0,"
                        "\"gpsFix\":\"MODE3D\","
                        "\"gpsTime\":0,"
                        "\"heading\":0.0,"
                        "\"latitude\":2.2,"
                        "\"longitude\":1.1,"
                        "\"quality\":0.0,"
                        "\"time\":17"
                    "},{"
                        "\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                        "\"altitude\":6.6,"
                        "\"bearing\":0.0,"
                        "\"deviceTime\":0,"
                        "\"elevation\":0.0,"
                        "\"gpsFix\":\"MODENOFIX\","
                        "\"gpsTime\":0,"
                        "\"heading\":0.0,"
                        "\"latitude\":5.5,"
                        "\"longitude\":4.4,"
                        "\"quality\":0.0,"
                        "\"time\":18"
                    "}]]"
                "}"
    );

    // Expected literal is:
    Reply reply;
    reply.setRequestReplyId(QString("TEST-requestReplyId"));
    QList<QVariant> response;
    response.append(QVariant::fromValue(locList));
    reply.setResponse(response);
    expectedReplyString = expectedReplyString.arg(reply.getRequestReplyId());
    QByteArray expectedReply = expectedReplyString.toUtf8();

    QByteArray jsonReply = JsonSerializer::serialize(reply);

    EXPECT_EQ(expectedReply, jsonReply);

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply->getResponse().at(0).canConvert<QList<QVariant> >());
    QListIterator<QVariant> i_received(receivedReply->getResponse().at(0).value<QList<QVariant> >());
    QListIterator<QVariant> i_origin(reply.getResponse().at(0).value<QList<QVariant> >());
    while(i_received.hasNext()) {
        types::Localisation::QtGpsLocation receivedIterValue = i_received.next().value<types::Localisation::QtGpsLocation>();
        types::Localisation::QtGpsLocation originItervalue = i_origin.next().value<types::Localisation::QtGpsLocation>();
        LOG_DEBUG(logger, receivedIterValue.toString());
        EXPECT_EQ(originItervalue, receivedIterValue);
    }

    delete receivedReply;
}

TEST_F(JsonSerializerTest, serialize_deserialize_trip) {
    qRegisterMetaType<joynr::types::Localisation::QtGpsLocation>("joynr::types::Localisation::QtGpsLocation");
    qRegisterMetaType<joynr__types__Localisation__QtGpsLocation>("joynr__types__Localisation__GpsLocation");

    qRegisterMetaType<joynr::types::Localisation::QtTrip>("joynr::types::Localisation::QtTrip");

    QList<types::Localisation::QtGpsLocation> locations;
    locations.push_back(types::Localisation::QtGpsLocation(1.1, 2.2, 3.3, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,17));
    locations.push_back(types::Localisation::QtGpsLocation(4.4, 5.5, 6.6, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,317));
    locations.push_back(types::Localisation::QtGpsLocation(7.7, 8.8, 9.9, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,3317));

    QByteArray expected("{\"_typeName\":\"joynr.types.Localisation.Trip\","
                        "\"locations\":[{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
						"\"altitude\":3.3,"
						"\"bearing\":0.0,"
						"\"deviceTime\":0,"
						"\"elevation\":0.0,"
						"\"gpsFix\":\"MODE3D\","
						"\"gpsTime\":0,"
                        "\"heading\":0.0,"
                        "\"latitude\":2.2,"
                        "\"longitude\":1.1,"
                        "\"quality\":0.0,"
                        "\"time\":17},"
                        "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                        "\"altitude\":6.6,"
						"\"bearing\":0.0,"
						"\"deviceTime\":0,"
						"\"elevation\":0.0,"
						"\"gpsFix\":\"MODE3D\","
						"\"gpsTime\":0,"
                        "\"heading\":0.0,"
                        "\"latitude\":5.5,"
                        "\"longitude\":4.4,"
                        "\"quality\":0.0,"
                        "\"time\":317},"
                        "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                        "\"altitude\":9.9,"
						"\"bearing\":0.0,"
						"\"deviceTime\":0,"
						"\"elevation\":0.0,"
						"\"gpsFix\":\"MODE3D\","
						"\"gpsTime\":0,"
                        "\"heading\":0.0,"
                        "\"latitude\":8.8,"
                        "\"longitude\":7.7,"
                        "\"quality\":0.0,"
                        "\"time\":3317}],"
                        "\"tripTitle\":\"trip1_name\"}");

    // Expected literal is:
    types::Localisation::QtTrip trip1(locations, QString("trip1_name"));

    QByteArray serializedContent = JsonSerializer::serialize(QVariant::fromValue(trip1));
    EXPECT_EQ(expected, serializedContent);

    types::Localisation::QtTrip* trip2 = JsonSerializer::deserialize<types::Localisation::QtTrip>(serializedContent);
    EXPECT_EQ(trip1, *trip2) << "trips \n trip1: " << trip1.toString().toLatin1().data()
                             << " and \n trip2: " << trip2->toString().toLatin1().data()
                             << "\n are not the same";;

    delete trip2;
}


TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequest) {
    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<joynr::types::Localisation::QtTrip>("joynr::types::Localisation::QtTrip");

    QList<types::Localisation::QtGpsLocation> locations;
    locations.push_back(types::Localisation::QtGpsLocation(1.1, 1.2, 1.3, types::Localisation::QtGpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110));
    locations.push_back(types::Localisation::QtGpsLocation(2.1, 2.2, 2.3, types::Localisation::QtGpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210));
    locations.push_back(types::Localisation::QtGpsLocation(3.1, 3.2, 3.3, types::Localisation::QtGpsFixEnum::MODE2D, 3.4, 3.5, 3.6, 3.7, 38, 39, 310));

    types::Localisation::QtTrip trip1(locations, QString("trip1_name"));

    Request request1;
    request1.setMethodName(QString("serialize_deserialize_JsonRequestTest_method"));

    QVariantList params;
    QString contentParam1("contentParam1");
    params.append(QVariant::fromValue(contentParam1));
    params.append(QVariant::fromValue(trip1));
    qRegisterMetaType<QList<int> >("QlistInt");
    //To serialize a QList<...> it has to be stored as a QList<QVariant>
    QList<QVariant> list;
    list.append(QVariant::fromValue(2));
    params.append(QVariant::fromValue(list));

    request1.setParams(params);

    QByteArray serializedContent = JsonSerializer::serialize(request1);
    LOG_DEBUG(logger, QString::fromUtf8(serializedContent));

    Request* request2 = JsonSerializer::deserialize<Request>(serializedContent);

    QVariantList paramsReceived = request2->getParams();

    EXPECT_EQ(paramsReceived.at(0).value<QString>(), contentParam1);
    EXPECT_EQ(paramsReceived.at(1).value<types::Localisation::QtTrip>(), trip1);

    //EXPECT_EQ on the Request doesn't work, because the == method of QVariantMap says:
    // "Warning: This function doesn't support custom types registered with qRegisterMetaType()."
    //EXPECT_EQ(request1, *request2);

    delete request2;
}

TEST_F(JsonSerializerTest, serialize_deserialize_Reply_with_Array_as_Response) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");
    qRegisterMetaType<joynr::types::QtCapabilityInformation>("joynr::types::QtCapabilityInformation");

    QList<types::QtCapabilityInformation> capabilityInformations;
    types::QtCapabilityInformation cap1(types::QtCapabilityInformation("domain1", "interface1", types::QtProviderQos(), "channel1", "participant1"));
    capabilityInformations.append(cap1);
    capabilityInformations.append(types::QtCapabilityInformation("domain2", "interface2", types::QtProviderQos(), "channel2", "participant2"));

    Reply reply;

    QVariantList response;
    reply.setRequestReplyId("serialize_deserialize_Reply_with_Array_as_Response");
    //this is the magic code: do not call "append" for lists, because this will append all list elements to the outer list
    response.insert(0, joynr::Util::convertListToVariantList<joynr::types::QtCapabilityInformation>(capabilityInformations));
    reply.setResponse(response);
 QByteArray serializedContent = JsonSerializer::serialize(reply);
    LOG_DEBUG(logger, QString::fromUtf8(serializedContent));

    Reply* deserializedReply = JsonSerializer::deserialize<Reply>(serializedContent);

    response = deserializedReply->getResponse();

    QVariantList receivedCaps = response.at(0).value<QVariantList>();
    types::QtCapabilityInformation receivedCap1 = receivedCaps.at(0).value<types::QtCapabilityInformation>();
    EXPECT_EQ(receivedCap1, cap1);
    EXPECT_EQ(deserializedReply->getRequestReplyId(), "serialize_deserialize_Reply_with_Array_as_Response");

    delete deserializedReply;
}

TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequestWithLists) {
    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<joynr::types::Localisation::QtGpsLocation>("joynr::types::Localisation::QtGpsLocation");
    qRegisterMetaType<joynr__types__Localisation__QtGpsLocation>("joynr__types__Localisation__GpsLocation");

    QString contentParam1("ListParameter");

    //creating Request
    QList<types::Localisation::QtGpsLocation> inputLocationList;
    inputLocationList.push_back(types::Localisation::QtGpsLocation(1.1, 2.2, 3.3, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,17));
    inputLocationList.push_back(types::Localisation::QtGpsLocation(4.4, 5.5, 6.6, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,317));
    inputLocationList.push_back(types::Localisation::QtGpsLocation(7.7, 8.8, 9.9, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0,3317));

    QString expectedString(
                "{\"_typeName\":\"joynr.Request\","
                "\"methodName\":\"serialize_deserialize_JsonRequestTest_method\","
                "\"paramDatatypes\":[\"List\"],"
                "\"params\":[[{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                "\"altitude\":3.3,"
				"\"bearing\":0.0,"
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
				"\"gpsFix\":\"MODE3D\","
				"\"gpsTime\":0,"
                "\"heading\":0.0,"
                "\"latitude\":2.2,"
                "\"longitude\":1.1,"
                "\"quality\":0.0,"
                "\"time\":17},"
                "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                "\"altitude\":6.6,"
				"\"bearing\":0.0,"
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
				"\"gpsFix\":\"MODE3D\","
				"\"gpsTime\":0,"
                "\"heading\":0.0,"
                "\"latitude\":5.5,"
                "\"longitude\":4.4,"
                "\"quality\":0.0,"
                "\"time\":317},"
                "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
                "\"altitude\":9.9,"
				"\"bearing\":0.0,"
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
				"\"gpsFix\":\"MODE3D\","
				"\"gpsTime\":0,"
                "\"heading\":0.0,"
                "\"latitude\":8.8,"
                "\"longitude\":7.7,"
                "\"quality\":0.0,"
                "\"time\":3317}]],"
                "\"requestReplyId\":\"%1\"}"
    );

    Request request1;
    request1.setMethodName(QString("serialize_deserialize_JsonRequestTest_method"));
    QList<QVariant> inputQvl = Util::convertListToVariantList(inputLocationList);
    request1.addParam(inputQvl, "List");

    expectedString = expectedString.arg(TypeUtil::toQt(request1.getRequestReplyId()));
    QByteArray expected = expectedString.toUtf8();

    //serializing Request
    QByteArray serializedContent = JsonSerializer::serialize(request1);
    EXPECT_EQ(expected, serializedContent);

    //deserializing Request
    Request* request2 = JsonSerializer::deserialize<Request>(serializedContent);
    QVariantList paramsReceived = request2->getParams();

    LOG_DEBUG(logger, QString("x1") + QString(paramsReceived.at(0).typeName()));
    ASSERT_TRUE(paramsReceived.at(0).canConvert<QList<QVariant> >()) << "Cannot convert the field of the Param Map to a QList<QVariant>";
    QList<QVariant> returnQvl = paramsReceived.at(0).value<QList<QVariant> >();
    ASSERT_TRUE(returnQvl.size() == 3) << "list size size != 3";
    LOG_DEBUG(logger, QString(returnQvl.at(0).typeName()));

    ASSERT_TRUE(returnQvl.at(0).canConvert<types::Localisation::QtGpsLocation>()) << "Cannot convert the first entry of the return List to QtGpsLocation";

    QList<types::Localisation::QtGpsLocation> resultLocationList = Util::convertVariantListToList<types::Localisation::QtGpsLocation>(returnQvl);
    EXPECT_EQ(resultLocationList.at(1), types::Localisation::QtGpsLocation(4.4, 5.5, 6.6, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0, 317));

    delete request2;
}

// Test to investigate whether long lists cause a problem with during deserialization
TEST_F(JsonSerializerTest, serialize_deserialize_ListComplexity) {
    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<joynr::types::Localisation::QtGpsLocation>("joynr::types::Localisation::QtGpsLocation");
    qRegisterMetaType<joynr__types__Localisation__QtGpsLocation>("joynr__types__Localisation__GpsLocation");

    QString contentParam1("ListParameter");

    //creating Request
    QList<types::Localisation::QtGpsLocation> inputLocationList;

    // Create a JSON list
    int firstListSize = 10000;
    for (int i = 0; i < firstListSize; i++) {
        inputLocationList.push_back(types::Localisation::QtGpsLocation(1.1, 2.2, 3.3, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0.0,0.0,0.0,0,0, 17));
    }

    // Create a request that uses this list
    Request request1;
    request1.setMethodName(QString("serialize_deserialize_JsonRequestTest_method"));
    QVariantList params1;
    QList<QVariant> inputQvl = Util::convertListToVariantList(inputLocationList);
    params1.append(QVariant::fromValue(inputQvl));
    request1.setParams(params1);

    //Time request serialization
    QTime serializationTime;
    serializationTime.start();
    QByteArray serializedContent = JsonSerializer::serialize(request1);
    int serializationElapsed1 = serializationTime.elapsed();

    // Time request deserialization
    QTime deserializationTime;
    deserializationTime.start();
    Request* deserialized1 = JsonSerializer::deserialize<Request>(serializedContent);
    int deserializationElapsed1 = deserializationTime.elapsed();

    // Double the size of the JSON list
    for (int i = 0; i < firstListSize; i++) {
        inputLocationList.push_back(types::Localisation::QtGpsLocation(1.1, 2.2, 3.3, types::Localisation::QtGpsFixEnum::MODE3D, 0.0, 0,0,0,0,0,17));
    }

    // Create a request that uses this bigger list
    Request request2;
    request2.setMethodName(QString("serialize_deserialize_JsonRequestTest_method"));
    QVariantList params2;
    QList<QVariant> inputQv2 = Util::convertListToVariantList(inputLocationList);
    params2.append(QVariant::fromValue(inputQv2));
    request2.setParams(params2);

    // Time request serialization
    serializationTime.restart();
    serializedContent = JsonSerializer::serialize(request2);
    int serializationElapsed2 = serializationTime.elapsed();

    // Time request deserialization
    deserializationTime.restart();
    Request* deserialized2 = JsonSerializer::deserialize<Request>(serializedContent);
    int deserializationElapsed2 = deserializationTime.elapsed();

    LOG_DEBUG(logger, QString("%1 Objects serialized in %2 milliseconds").arg(firstListSize).arg(serializationElapsed1));
    LOG_DEBUG(logger, QString("%1 Objects serialized in %2 milliseconds").arg(firstListSize * 2).arg(serializationElapsed2));
    LOG_DEBUG(logger, QString("%1 Objects deserialized in %2 milliseconds").arg(firstListSize).arg(deserializationElapsed1));
    LOG_DEBUG(logger, QString("%1 Objects deserialized in %2 milliseconds").arg(firstListSize * 2).arg(deserializationElapsed2));

    // Assert O(N) complexity
    ASSERT_TRUE(serializationElapsed2 < serializationElapsed1 * serializationElapsed1);
    ASSERT_TRUE(deserializationElapsed2 < deserializationElapsed1 * deserializationElapsed1);

    delete deserialized1;
    delete deserialized2;
}

TEST_F(JsonSerializerTest, serialize_deserialize_EndpointAddress) {
    qRegisterMetaType<joynr::system::RoutingTypes::QtChannelAddress>("joynr::system::RoutingTypes::QtChannelAddress");
    qRegisterMetaType<joynr::system::RoutingTypes::QtCommonApiDbusAddress>("joynr::system::RoutingTypes::QtCommonApiDbusAddress");

    joynr::system::RoutingTypes::QtChannelAddress joynr("TEST_channelId");
    joynr::system::RoutingTypes::QtCommonApiDbusAddress dbus("domain", "interfacename", "id");
    joynr::system::RoutingTypes::QtWebSocketAddress wsServer(
                joynr::system::RoutingTypes::QtWebSocketProtocol::WS,
                "localhost",
                42,
                "some/path"
    );
    joynr::system::RoutingTypes::QtWebSocketClientAddress wsClient("TEST_clientId");

    // serialize
    QByteArray joynrSerialized = JsonSerializer::serialize(joynr);
    QByteArray dbusSerialized = JsonSerializer::serialize(dbus);
    QByteArray wsServerSerialized = JsonSerializer::serialize(wsServer);
    QByteArray wsClientSerialized = JsonSerializer::serialize(wsClient);
    
    LOG_DEBUG(logger, "serialized Joynr address: "+ QString::fromUtf8(joynrSerialized));
    LOG_DEBUG(logger, "serialized Dbus address: "+ QString::fromUtf8(dbusSerialized));
    LOG_DEBUG(logger, QString("serialized WS server address: %0").arg(QString::fromUtf8(wsServerSerialized)));
    LOG_DEBUG(logger, QString("serialized WS client address: %0").arg(QString::fromUtf8(wsClientSerialized)));

    // deserialize
    joynr::system::RoutingTypes::QtChannelAddress* joynrDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::QtChannelAddress>(joynrSerialized);
    joynr::system::RoutingTypes::QtCommonApiDbusAddress* dbusDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::QtCommonApiDbusAddress>(dbusSerialized);
    joynr::system::RoutingTypes::QtWebSocketAddress* wsServerDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::QtWebSocketAddress>(wsServerSerialized);
    joynr::system::RoutingTypes::QtWebSocketClientAddress* wsClientDeserialized = JsonSerializer::deserialize<joynr::system::RoutingTypes::QtWebSocketClientAddress>(wsClientSerialized);

    EXPECT_EQ(joynr, *joynrDeserialized);
    EXPECT_EQ(dbus, *dbusDeserialized);
    EXPECT_EQ(wsServer, *wsServerDeserialized);
    EXPECT_EQ(wsClient, *wsClientDeserialized);

    delete joynrDeserialized;
    delete dbusDeserialized;
    delete wsServerDeserialized;
    delete wsClientDeserialized;
}

TEST_F(JsonSerializerTest, serialize_deserialize_CapabilityInformation) {

    QByteArray expected("{\"_typeName\":\"joynr.types.CapabilityInformation\","
                        "\"channelId\":\"channeldId\","
                        "\"domain\":\"domain\","
                        "\"interfaceName\":\"\","
                        "\"participantId\":\"\","
                        "\"providerQos\":{"
                        "\"_typeName\":\"joynr.types.ProviderQos\","
                        "\"customParameters\":[],"
                        "\"priority\":2,"
                        "\"providerVersion\":4,"
                        "\"scope\":\"GLOBAL\","
                        "\"supportsOnChangeSubscriptions\":false}}");

    // Expected literal is:
    // { "_typeName" : "joynr.types.CapabilityInformation", "channelId" : "channeldId", "domain" : "domain", "interfaceName" : "", "participantId" : "", "providerQos" : { "_typeName" : "joynr.types.ProviderQos", "isLocalOnly" : false, "onChangeSubscriptions" : false, "priority" : 2, "qos" : [  ], "Version" : 4 }

    //TODO: also remove the variables qRegisterMetaTypeQos
    qRegisterMetaType<joynr::types::QtProviderQos>("joynr::types::QtProviderQos");
    qRegisterMetaType<joynr__types__QtProviderQos>("joynr__types__ProviderQos");
    qRegisterMetaType<joynr::types::QtCapabilityInformation>("joynr::types::QtCapabilityInformation");
    qRegisterMetaType<joynr__types__QtCapabilityInformation>("joynr__types__CapabilityInformation");

    types::QtProviderQos qos;
    qos.setPriority(2);
    qos.setProviderVersion(4);
    types::QtCapabilityInformation capabilityInformation;
    capabilityInformation.setChannelId("channeldId");
    capabilityInformation.setDomain("domain");
    capabilityInformation.setProviderQos(qos);
    LOG_DEBUG(logger,"capabilityInformation" + capabilityInformation.toString());

    QByteArray serialized = JsonSerializer::serialize(QVariant::fromValue(capabilityInformation));
    EXPECT_EQ(expected, serialized);
    // deserialize
    LOG_DEBUG(logger,"serialized capabilityInformation" + QString::fromUtf8(serialized));

    types::QtCapabilityInformation* deserializedCI = JsonSerializer::deserialize<types::QtCapabilityInformation>(serialized);

    EXPECT_EQ(capabilityInformation, *deserializedCI);
    LOG_DEBUG(logger,"deserialized capabilityInformation" + deserializedCI->toString());
}

// Test of ChannelURLInformation which is of type QList<QString>.
// QList<QString> is a special case in some places (for an example see Request.h)
TEST_F(JsonSerializerTest, serialize_deserialize_ChannelURLInformation) {
    qRegisterMetaType<joynr::types::QtChannelUrlInformation>("joynr::types::QtChannelUrlInformation");
    qRegisterMetaType<joynr__types__QtChannelUrlInformation>("joynr__types__ChannelUrlInformation");

    QList<QString> urls;
    urls.append("http://example1.com/");
    urls.append("http://example2.com/");
    types::QtChannelUrlInformation urlInformation(urls);

    // Serialize the URL Information
    QByteArray serialized = JsonSerializer::serialize(QVariant::fromValue(urlInformation));
    LOG_DEBUG(logger,"serialized QtChannelUrlInformation" + QString::fromUtf8(serialized));

    // Expected JSON : { "_typeName" : "joynr.types.ChannelUrlInformation", "urls" : [ "http://example1.com/", "http://example2.com/" ] }
    QByteArray expected("{\"_typeName\":\"joynr.types.ChannelUrlInformation\",\"urls\":[\"http://example1.com/\",\"http://example2.com/\"]}");

    EXPECT_EQ(expected, serialized);

    // Deserialize
    types::QtChannelUrlInformation* deserializedInfo = JsonSerializer::deserialize<types::QtChannelUrlInformation>(serialized);

    // Check the structure
    QList<QString> deserializedUrls = deserializedInfo->getUrls();
    EXPECT_EQ(urls, deserializedUrls);
}

TEST_F(JsonSerializerTest, deserialize_ProviderQos) {
    joynr::types::QtProviderQos qos;

    QByteArray jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[],\"priority\":5,\"providerVersion\":3,\"scope\":\"LOCAL\",\"supportsOnChangeSubscriptions\":false}");

    joynr::types::QtProviderQos* providerQos = JsonSerializer::deserialize<joynr::types::QtProviderQos>(jsonProviderQos);

    EXPECT_EQ(providerQos->getScope(), joynr::types::QtProviderScope::LOCAL);
    EXPECT_EQ(providerQos->getProviderVersion(), 3);
    EXPECT_EQ(providerQos->getPriority(), 5);

    delete providerQos;
}

TEST_F(JsonSerializerTest, serialize_ProviderQos) {
    joynr::types::QtProviderQos qos;
    qos.setScope(joynr::types::QtProviderScope::LOCAL);
    qos.setPriority(5);
    qos.setProviderVersion(-1);

    QByteArray jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[],\"priority\":5,\"providerVersion\":-1,\"scope\":\"LOCAL\",\"supportsOnChangeSubscriptions\":false}");

    QByteArray result = JsonSerializer::serialize(qos);

    EXPECT_EQ(jsonProviderQos, result);
}


TEST_F(JsonSerializerTest, deserialize_GPSLocation) {
    qRegisterMetaType<joynr::types::Localisation::QtGpsLocation>("joynr::types::Localisation::QtGpsLocation");
    qRegisterMetaType<types::Localisation::QtGpsFixEnum>();
    qRegisterMetaType<types::Localisation::QtGpsFixEnum::Enum>("types::Localisation::QtGpsFixEnum");

    QByteArray jsonGPS(
                "{\"_typeName\":\"joynr.types.Localisation.GpsLocation\","
				"\"accuracy\":0.0,"
                "\"altitude\":3.3,"
				"\"bearing\":0.0,"
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
                "\"gpsFix\":1,"
				"\"gpsTime\":0,"
                "\"latitude\":2.2,"
                "\"longitude\":1.1,"
                "\"time\":17}"
    );
    joynr::types::Localisation::QtGpsLocation* receivedReply = JsonSerializer::deserialize<joynr::types::Localisation::QtGpsLocation>(jsonGPS);
    // Clean up
    delete receivedReply;
}

TEST_F(JsonSerializerTest, serialize_OnchangeWithKeepAliveSubscription) {
    qRegisterMetaType<joynr::QtSubscriptionQos>("joynr::QtSubscriptionQos");
    qRegisterMetaType<std::shared_ptr<QtSubscriptionQos>>();

    qRegisterMetaType<joynr::QtOnChangeSubscriptionQos>("joynr::QtOnChangeSubscriptionQos");
    qRegisterMetaType<std::shared_ptr<QtOnChangeSubscriptionQos>>();

    qRegisterMetaType<joynr::QtOnChangeWithKeepAliveSubscriptionQos>("joynr::QtOnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<std::shared_ptr<joynr::QtOnChangeWithKeepAliveSubscriptionQos>>();


    joynr::QtOnChangeWithKeepAliveSubscriptionQos qos(750, 100, 900, 1050);

    QByteArray jsonQos = JsonSerializer::serialize(qos);
    LOG_DEBUG(logger,"serialized QtOnChangeWithKeepAliveSubscriptionQos" + QString::fromUtf8(jsonQos));

    joynr::QtOnChangeWithKeepAliveSubscriptionQos* desQos = JsonSerializer::deserialize<joynr::QtOnChangeWithKeepAliveSubscriptionQos>(jsonQos);

    jsonQos = JsonSerializer::serialize(*desQos);
    LOG_DEBUG(logger,"serialized QtOnChangeWithKeepAliveSubscriptionQos" + QString::fromUtf8(jsonQos));


    EXPECT_EQ(qos, *desQos);

    // Clean up
    delete desQos;
}

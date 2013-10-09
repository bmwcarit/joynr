/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include <QVariant>
#include <QSharedPointer>
#include "joynr/Util.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/types/Location.h"
#include "joynr/types/GpsFixEnum.h"
#include "joynr/types/Trip.h"
#include "joynr/types/ChannelUrlInformation.h"
#include "joynr/social/PickupRequest.h"
#include "joynr/types/CapabilityInformation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/JsonSerializer.h"
#include "joynr/joynrlogging.h"
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "common/some-ip/SomeIpEndpointAddress.h"
#include "joynr/tests/TestEnum.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"

using namespace joynr;
using namespace joynr_logging;

// TODO:
// 1. If the decision is made to use c++11, g++ >= version 5.5 then JSON literals can be
//    encoded using raw string literals.

class JsonSerializerTest : public testing::Test {
public:
    JsonSerializerTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "JsonSerializerTest"))
    {

    }

    void test(types::GpsLocation location){
        LOG_DEBUG(logger, location.toString());
    }

protected:
    joynr_logging::Logger* logger;
};

TEST_F(JsonSerializerTest, serialize_deserialize_SubscriptionRequest) {
    qRegisterMetaType<joynr::SubscriptionRequest>();
    SubscriptionRequest request;
    QSharedPointer<SubscriptionQos> x(new SubscriptionQos(5000));
    request.setQos(x);
    QByteArray result = JsonSerializer::serialize(request);
    LOG_DEBUG(logger, QString(result));
    SubscriptionRequest* desRequest = JsonSerializer::deserialize<SubscriptionRequest>(result);
    EXPECT_TRUE(request == *desRequest);
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
    expected = expected.arg(request.getRequestReplyId());

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

TEST_F(JsonSerializerTest, serialize_operation_with_multiple_params1) {

    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<tests::TestEnum>();
    int typeId = qRegisterMetaType<tests::TestEnum::Enum>("tests::TestEnum");

    // Set the request method name
    Request request;
    request.setMethodName(QString("methodEnumDoubleParameters"));

    // Set the request parameters
    request.addParam(QVariant::fromValue(tests::TestEnum::ONE), "joynr.tests.TestEnum");
    request.addParam(2.2, "Double");

    // Serialize the request
    QJson::Serializer::registerEnum(typeId, tests::TestEnum::staticMetaObject.enumerator(0));
    QString serializedContent = JsonSerializer::serialize(request);

    QString expected(
                "{\"_typeName\":\"joynr.Request\","
                "\"methodName\":\"methodEnumDoubleParameters\","
                "\"paramDatatypes\":[\"joynr.tests.TestEnum\",\"Double\"],"
                "\"params\":[\"ONE\",2.2],"
                "\"requestReplyId\":\"%1\"}"
    );
    expected = expected.arg(request.getRequestReplyId());

    EXPECT_EQ(expected, QString(serializedContent));
    LOG_DEBUG(logger, "Serialized method call: "+ serializedContent);
    LOG_DEBUG(logger, "Expected method call: "+ expected);

    // Expected literal is:
    // { "_typeName" : "joynr.Request", "methodName" : "methodEnumDoubleParameters", "paramDatatypes" : [ "Integer", "Double" ], "params" : { "enumParam" : "ONE", "doubleParam" : 2.22 } }
    EXPECT_EQ(expected, serializedContent);

}

TEST_F(JsonSerializerTest, deserialize_operation_with_enum) {

    qRegisterMetaType<joynr::Request>();
    qRegisterMetaType<tests::TestEnum>();
    qRegisterMetaType<tests::TestEnum::Enum>("tests::TestEnum");

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
    tests::TestEnum::Enum value = Util::convertVariantToEnum<tests::TestEnum>(enumParam);
    EXPECT_EQ(1, value);
    delete(request);
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
    expected = expected.arg(request.getRequestReplyId());

    LOG_DEBUG(logger, "Serialized method call: "+ serializedContent);
    LOG_DEBUG(logger, "Expected method call: "+ expected);

    // Expected literal is:
    // { "_typeName" : "joynr.Request", "methodName" : "methodStringDoubleParameters", "paramDatatypes" : [ "String", "Double" ], "params" : { "doubleParam" : 3.33, "stringParam" : "testStringParam" } }
    EXPECT_EQ(expected, serializedContent);

}


TEST_F(JsonSerializerTest, serialize_deserialize_gpslocation) {
    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<joynr__types__GpsLocation>("joynr__types__GpsLocation");

    types::GpsLocation gps1(types::GpsFixEnum::Mode3D, 1.123456789, 2.2, 3.3,0.0,0.0,0.0,0,0,17);

    QString expected("{\"_typeName\":\"joynr.types.GpsLocation\","
						"\"accuracy\":0.0,"
						"\"altitude\":3.3,"
						"\"bearing\":0.0,"
						"\"deviceTime\":0,"
						"\"elevation\":0.0,"
                        "\"gpsFix\":\"Mode3D\","
						"\"gpsTime\":0,"
                        "\"latitude\":2.2,"
                        "\"longitude\":1.123456789,"
                        "\"time\":17}");

    // Expected literal is:
    // { "_typeName" : "joynr.types.GpsLocation", "altitude" : 3.3, "gpsFix" : 3, "latitude" : 2.2, "longitude" : 1.123456789, "time" : 17 }

    QByteArray serializedContent = JsonSerializer::serialize(QVariant::fromValue(gps1));
    LOG_DEBUG(logger, QString(serializedContent));
    EXPECT_EQ(expected, QString(serializedContent));
    types::GpsLocation* gps2 = JsonSerializer::deserialize<types::GpsLocation>(serializedContent);

    EXPECT_EQ(gps1, *gps2);

    delete gps2;
}

TEST_F(JsonSerializerTest, serialize_deserialize_location) {
    qRegisterMetaType<joynr::types::Location>("joynr::types::Location");
    qRegisterMetaType<joynr__types__Location>("joynr__types__Location");

    types::Location location;
    location.setGpsFix(types::GpsFixEnum::Mode3D);
    location.setLongitude(1.1);
    location.setLatitude(2.2);
    location.setAccuracy(0.0);
    location.setAltitude(3.3);
    location.setBearing(0.0);
    location.setDeviceTime(0);
    location.setElevation(0.0);
    location.setGpsTime(0);
    location.setTime(17);
    location.setDescription("location.description");
    location.setCountry("location.country");
    location.setCity("location.city");
    location.setStreet("location.street");
    location.setStreetNumber("location.streetNumber");
    location.setPostalCode("location.postalCode");
    location.setTitle("location.title");

    QByteArray expectedLocation(
            "{"
                "\"_typeName\":\"joynr.types.Location\","
				"\"accuracy\":0.0,"
				"\"altitude\":3.3,"
				"\"bearing\":0.0,"
                "\"city\":\"location.city\","
                "\"country\":\"location.country\","
                "\"description\":\"location.description\","
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
                "\"gpsFix\":\"Mode3D\","
				"\"gpsTime\":0,"
                "\"latitude\":2.2,"
                "\"longitude\":1.1,"
                "\"postalCode\":\"location.postalCode\","
                "\"street\":\"location.street\","
                "\"streetNumber\":\"location.streetNumber\","
                "\"time\":17,"
                "\"title\":\"location.title\""
            "}"
    );

    QByteArray serializedLocation = JsonSerializer::serialize(QVariant::fromValue(location));
    LOG_DEBUG(logger, QString::fromUtf8(serializedLocation));

    EXPECT_EQ(expectedLocation, serializedLocation);
    types::Location* deserializedLocation = JsonSerializer::deserialize<types::Location>(serializedLocation);

    EXPECT_EQ(location, *deserializedLocation);

    delete deserializedLocation;
}

TEST_F(JsonSerializerTest, serialize_deserialize_replyWithGpsLocation) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");
    types::GpsLocation gps1(types::GpsFixEnum::Mode3D, 1.1, 2.2, 3.3, 0.0,0.0,0.0,0,0, 17);

    // Expected literal is:
    Reply reply;
    reply.setRequestReplyId(QString("TEST-requestReplyId"));
    reply.setResponse(QVariant::fromValue(gps1));

    QString expected(
                "{\"_typeName\":\"joynr.Reply\","
                "\"requestReplyId\":\"%1\","
                "\"response\":{"
                "\"_typeName\":\"joynr.types.GpsLocation\","
                "\"accuracy\":0.0,"
                "\"altitude\":3.3,"
                "\"bearing\":0.0,"
                "\"deviceTime\":0,"
                "\"elevation\":0.0,"
                "\"gpsFix\":\"Mode3D\","
                "\"gpsTime\":0,"
                "\"latitude\":2.2,"
                "\"longitude\":1.1,"
                "\"time\":17}}"
    );
    expected = expected.arg(reply.getRequestReplyId());

    QByteArray jsonReply = JsonSerializer::serialize(reply);

    EXPECT_EQ(expected, QString(jsonReply));
    LOG_DEBUG(logger, QString::fromUtf8(jsonReply));

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply->getResponse().canConvert<types::GpsLocation>());

//    GpsLocation gps2;

//    QJson::QObjectHelper::qvariant2qobject(receivedReply->getResponse().value<QVariantMap>(), &gps2);

    types::GpsLocation gps2 = receivedReply->getResponse().value<types::GpsLocation>();

    EXPECT_EQ(gps1, gps2)
            << "Gps locations gps1 " << gps1.toString().toLatin1().data()
            << " and gps2 " << gps2.toString().toLatin1().data() << " are not the same";

    // Clean up
    delete receivedReply;
}

TEST_F(JsonSerializerTest, deserialize_replyWithVoid) {
    qRegisterMetaType<joynr::Reply>("joynr::Reply");

    // null response with type invalid
    QVariant response;
    Reply reply;
    reply.setRequestReplyId(QString("TEST-requestReplyId"));
    reply.setResponse(response);

    QString expected(
                "{\"_typeName\":\"joynr.Reply\","
                "\"requestReplyId\":\"%1\","
                "\"response\":null}"
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
    locList.append(QVariant::fromValue(types::GpsLocation(types::GpsFixEnum::Mode3D, 1.1, 2.2, 3.3, 0.0,0.0,0.0,0,0,17)));
    locList.append(QVariant::fromValue(types::GpsLocation(types::GpsFixEnum::ModeNoFix, 4.4, 5.5, 6.6, 0.0,0.0,0.0,0,0,18)));

    QString expected(
                "{\"_typeName\":\"joynr.Reply\","
                "\"requestReplyId\":\"%1\","
                "\"response\":"
                "[{"
                "\"_typeName\":\"joynr.types.GpsLocation\","
                "\"accuracy\":0.0,"
                "\"altitude\":3.3,"
                "\"bearing\":0.0,"
                "\"deviceTime\":0,"
                "\"elevation\":0.0,"
                "\"gpsFix\":\"Mode3D\","
                "\"gpsTime\":0,"
                "\"latitude\":2.2,"
                "\"longitude\":1.1,"
                "\"time\":17},"
                "{"
                "\"_typeName\":\"joynr.types.GpsLocation\","
                "\"accuracy\":0.0,"
                "\"altitude\":6.6,"
                "\"bearing\":0.0,"
                "\"deviceTime\":0,"
                "\"elevation\":0.0,"
                "\"gpsFix\":\"ModeNoFix\","
                "\"gpsTime\":0,"
                "\"latitude\":5.5,"
                "\"longitude\":4.4,"
                "\"time\":18}]}"
    );

    // Expected literal is:
    Reply reply;
    reply.setRequestReplyId(QString("TEST-requestReplyId"));
    reply.setResponse(QVariant::fromValue(locList));
    expected = expected.arg(reply.getRequestReplyId());

    QByteArray jsonReply = JsonSerializer::serialize(reply);

    EXPECT_EQ(expected, QString(jsonReply));
    LOG_DEBUG(logger, QString::fromUtf8(jsonReply));

    Reply* receivedReply = JsonSerializer::deserialize<Reply>(jsonReply);

    EXPECT_TRUE(receivedReply->getResponse().canConvert<QList<QVariant> >());
    QListIterator<QVariant> i_received(receivedReply->getResponse().value<QList<QVariant> >());
    QListIterator<QVariant> i_origin(reply.getResponse().value<QList<QVariant> >());
    while(i_received.hasNext()) {
        types::GpsLocation receivedIterValue = i_received.next().value<types::GpsLocation>();
        types::GpsLocation originItervalue = i_origin.next().value<types::GpsLocation>();
        LOG_DEBUG(logger, receivedIterValue.toString());
        EXPECT_EQ(originItervalue, receivedIterValue);
    }

    delete receivedReply;
}

TEST_F(JsonSerializerTest, serialize_deserialize_trip) {
    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<joynr__types__GpsLocation>("joynr__types__GpsLocation");

    qRegisterMetaType<joynr::types::Trip>("joynr::types::Trip");

    QList<types::GpsLocation> locations;
    locations.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 1.1, 2.2, 3.3, 0.0,0.0,0.0,0,0,17));
    locations.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 4.4, 5.5, 6.6, 0.0,0.0,0.0,0,0,317));
    locations.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 7.7, 8.8, 9.9, 0.0,0.0,0.0,0,0,3317));

    QByteArray expected("{\"_typeName\":\"joynr.types.Trip\","
                        "\"locations\":[{\"_typeName\":\"joynr.types.GpsLocation\","
						"\"accuracy\":0.0,"
						"\"altitude\":3.3,"
						"\"bearing\":0.0,"
						"\"deviceTime\":0,"
						"\"elevation\":0.0,"
						"\"gpsFix\":\"Mode3D\","
						"\"gpsTime\":0,"
                        "\"latitude\":2.2,"
                        "\"longitude\":1.1,"
                        "\"time\":17},"
                        "{\"_typeName\":\"joynr.types.GpsLocation\","
						"\"accuracy\":0.0,"
                        "\"altitude\":6.6,"
						"\"bearing\":0.0,"
						"\"deviceTime\":0,"
						"\"elevation\":0.0,"
						"\"gpsFix\":\"Mode3D\","
						"\"gpsTime\":0,"
                        "\"latitude\":5.5,"
                        "\"longitude\":4.4,"
                        "\"time\":317},"
                        "{\"_typeName\":\"joynr.types.GpsLocation\","
						"\"accuracy\":0.0,"
                        "\"altitude\":9.9,"
						"\"bearing\":0.0,"
						"\"deviceTime\":0,"
						"\"elevation\":0.0,"
						"\"gpsFix\":\"Mode3D\","
						"\"gpsTime\":0,"
                        "\"latitude\":8.8,"
                        "\"longitude\":7.7,"
                        "\"time\":3317}],"
                        "\"tripTitle\":\"trip1_name\"}");

    // Expected literal is:
    types::Trip trip1(locations, QString("trip1_name"));

    QByteArray serializedContent = JsonSerializer::serialize(QVariant::fromValue(trip1));
    EXPECT_EQ(expected, serializedContent);
    LOG_DEBUG(logger, QString::fromUtf8(serializedContent));

    types::Trip* trip2 = JsonSerializer::deserialize<types::Trip>(serializedContent);
    EXPECT_EQ(trip1, *trip2) << "trips \n trip1: " << trip1.toString().toLatin1().data()
                             << " and \n trip2: " << trip2->toString().toLatin1().data()
                             << "\n are not the same";;

    delete trip2;
}


TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequest) {
    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<joynr::types::Trip>("joynr::types::Trip");

    QList<types::GpsLocation> locations;
    locations.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 1.1, 2.2, 3.3, 0.0, 0.0, 0.0, 0, 0, 17));
    locations.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 4.4, 5.5, 6.6, 0.0, 0.0, 0.0, 0, 0, 317));
    locations.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 7.7, 8.8, 9.9, 0.0, 0.0, 0.0, 0, 0, 3317));
    //Location does not anymore inherit from GpsLocation, so this has been removed:
    locations.push_back(types::Location(types::GpsFixEnum::Mode3D, 7.7, 8.8, 9.9, 0.0, 0.0, 0.0, 0, 0, 3317, "", "", "", "", "", "", ""));

    //test(types::Location(types::GpsFixEnum::Mode3D, 7.7, 8.8, 9.9, 0.0, 0.0, 0.0, 0, 0, 3317, "", "", "", "", "", "", ""));
    types::Trip trip1(locations, QString("trip1_name"));

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
    EXPECT_EQ(paramsReceived.at(1).value<types::Trip>(), trip1);

    //EXPECT_EQ on the Request doesn't work, because the == method of QVariantMap says:
    // "Warning: This function doesn't support custom types registered with qRegisterMetaType()."
    //EXPECT_EQ(request1, *request2);

    delete request2;
}


TEST_F(JsonSerializerTest, serialize_deserialize_JsonRequestWithLists) {
    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<joynr__types__GpsLocation>("joynr__types__GpsLocation");

    QString contentParam1("ListParameter");

    //creating Request
    QList<types::GpsLocation> inputLocationList;
    inputLocationList.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 1.1, 2.2, 3.3, 0.0,0.0,0.0,0,0,17));
    inputLocationList.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 4.4, 5.5, 6.6, 0.0,0.0,0.0,0,0,317));
    inputLocationList.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 7.7, 8.8, 9.9, 0.0,0.0,0.0,0,0,3317));

    QString expected(
                "{\"_typeName\":\"joynr.Request\","
                "\"methodName\":\"serialize_deserialize_JsonRequestTest_method\","
                "\"paramDatatypes\":[\"List\"],"
                "\"params\":[[{\"_typeName\":\"joynr.types.GpsLocation\","
				"\"accuracy\":0.0,"
                "\"altitude\":3.3,"
				"\"bearing\":0.0,"
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
				"\"gpsFix\":\"Mode3D\","
				"\"gpsTime\":0,"
                "\"latitude\":2.2,"
                "\"longitude\":1.1,"
                "\"time\":17},"
                "{\"_typeName\":\"joynr.types.GpsLocation\","
				"\"accuracy\":0.0,"
                "\"altitude\":6.6,"
				"\"bearing\":0.0,"
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
				"\"gpsFix\":\"Mode3D\","
				"\"gpsTime\":0,"
                "\"latitude\":5.5,"
                "\"longitude\":4.4,"
                "\"time\":317},"
                "{\"_typeName\":\"joynr.types.GpsLocation\","
				"\"accuracy\":0.0,"
                "\"altitude\":9.9,"
				"\"bearing\":0.0,"
				"\"deviceTime\":0,"
				"\"elevation\":0.0,"
				"\"gpsFix\":\"Mode3D\","
				"\"gpsTime\":0,"
                "\"latitude\":8.8,"
                "\"longitude\":7.7,"
                "\"time\":3317}]],"
                "\"requestReplyId\":\"%1\"}"
    );

    Request request1;
    request1.setMethodName(QString("serialize_deserialize_JsonRequestTest_method"));
    QList<QVariant> inputQvl = Util::convertListToVariantList(inputLocationList);
    request1.addParam(inputQvl, "List");

    expected = expected.arg(request1.getRequestReplyId());

    //serializing Request
    QByteArray serializedContent = JsonSerializer::serialize(request1);
    EXPECT_EQ(expected, QString(serializedContent));
    LOG_DEBUG(logger, QString::fromUtf8(serializedContent));

    //deserializing Request
    Request* request2 = JsonSerializer::deserialize<Request>(serializedContent);
    QVariantList paramsReceived = request2->getParams();

    LOG_DEBUG(logger, QString("x1") + QString(paramsReceived.at(0).typeName()));
    ASSERT_TRUE(paramsReceived.at(0).canConvert<QList<QVariant> >()) << "Cannot convert the field of the Param Map to a QList<QVariant>";
    QList<QVariant> returnQvl = paramsReceived.at(0).value<QList<QVariant> >();
    ASSERT_TRUE(returnQvl.size() == 3) << "list size size != 3";
    LOG_DEBUG(logger, QString(returnQvl.at(0).typeName()));

    ASSERT_TRUE(returnQvl.at(0).canConvert<types::GpsLocation>()) << "Cannot convert the first entry of the return List to GpsLocation";

    QList<types::GpsLocation> resultLocationList = Util::convertVariantListToList<types::GpsLocation>(returnQvl);
    EXPECT_EQ(resultLocationList.at(1), types::GpsLocation(types::GpsFixEnum::Mode3D, 4.4, 5.5, 6.6, 0.0,0.0,0.0,0,0, 317));

    delete request2;
}

// Test to investigate whether long lists cause a problem with during deserialization
TEST_F(JsonSerializerTest, serialize_deserialize_ListComplexity) {
    qRegisterMetaType<joynr::Request>("joynr::Request");
    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<joynr__types__GpsLocation>("joynr__types__GpsLocation");

    QString contentParam1("ListParameter");

    //creating Request
    QList<types::GpsLocation> inputLocationList;

    // Create a JSON list
    int firstListSize = 10000;
    for (int i = 0; i < firstListSize; i++) {
        inputLocationList.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 1.1, 2.2, 3.3, 0.0,0.0,0.0,0,0, 17));
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
        inputLocationList.push_back(types::GpsLocation(types::GpsFixEnum::Mode3D, 1.1, 2.2, 3.3, 0,0,0,0,0,17));
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
    qRegisterMetaType<joynr::JoynrMessagingEndpointAddress>("joynr::JoynrMessagingEndpointAddress");
    qRegisterMetaType<joynr::SomeIpEndpointAddress>("joynr::SomeIpEndpointAddress");

    JoynrMessagingEndpointAddress joynr("TEST_channelId");
    SomeIpEndpointAddress someip("TEST_ipAddress", 42);

    // serialize
    QByteArray joynrSerialized = JsonSerializer::serialize(joynr);
    QByteArray someipSerialized = JsonSerializer::serialize(someip);
    
    LOG_DEBUG(logger, "serialized Joynr endpoint address: "+ QString::fromUtf8(joynrSerialized));
    LOG_DEBUG(logger, "serialized SomeIP endpoint address: "+ QString::fromUtf8(someipSerialized));

    // deserialize
    JoynrMessagingEndpointAddress* joynrDeserialized = JsonSerializer::deserialize<JoynrMessagingEndpointAddress>(joynrSerialized);
    SomeIpEndpointAddress* someipDeserialized = JsonSerializer::deserialize<SomeIpEndpointAddress>(someipSerialized);

    EXPECT_EQ(joynr, *joynrDeserialized);
    EXPECT_EQ(someip, *someipDeserialized);

    delete joynrDeserialized;
    delete someipDeserialized;
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
    qRegisterMetaType<joynr::types::ProviderQos>("joynr::types::ProviderQos");
    qRegisterMetaType<joynr__types__ProviderQos>("joynr__types__ProviderQos");
    qRegisterMetaType<joynr::types::CapabilityInformation>("joynr::types::CapabilityInformation");
    qRegisterMetaType<joynr__types__CapabilityInformation>("joynr__types__CapabilityInformation");

    types::ProviderQos qos;
    qos.setPriority(2);
    qos.setProviderVersion(4);
    types::CapabilityInformation capabilityInformation;
    capabilityInformation.setChannelId("channeldId");
    capabilityInformation.setDomain("domain");
    capabilityInformation.setProviderQos(qos);
    LOG_DEBUG(logger,"capabilityInformation" + capabilityInformation.toString());

    QByteArray serialized = JsonSerializer::serialize(QVariant::fromValue(capabilityInformation));
    EXPECT_EQ(expected, serialized);
    // deserialize
    LOG_DEBUG(logger,"serialized capabilityInformation" + QString::fromUtf8(serialized));

    types::CapabilityInformation* deserializedCI = JsonSerializer::deserialize<types::CapabilityInformation>(serialized);

    EXPECT_EQ(capabilityInformation, *deserializedCI);
    LOG_DEBUG(logger,"deserialized capabilityInformation" + deserializedCI->toString());
}


TEST_F(JsonSerializerTest, serialize_deserialize_PickupRequest) {

    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<joynr__types__GpsLocation>("joynr__types__GpsLocation");
    qRegisterMetaType<joynr::social::PickupRequest>("joynr::social::PickupRequest");

    types::GpsLocation loc;
    loc.setAltitude(2);

    social::PickupRequest request;
    request.setLocation(loc);
    request.setMessage("message");
    LOG_DEBUG(logger,"request Metaname: " + QString(request.metaObject()->className()));

    QByteArray serialized = JsonSerializer::serialize(QVariant::fromValue(request));
    // deserialize
    LOG_DEBUG(logger,"serialized request: " + request.toString());

    social::PickupRequest* deserializedRequest = JsonSerializer::deserialize<social::PickupRequest>(serialized);

    LOG_DEBUG(logger,"deserialized request Metaname" + QString(deserializedRequest->metaObject()->className()));

    EXPECT_EQ(request, *deserializedRequest);
    LOG_DEBUG(logger,"deserialized request: " + deserializedRequest->toString());
}

// Test of ChannelURLInformation which is of type QList<QString>.
// QList<QString> is a special case in some places (for an example see Request.h)
TEST_F(JsonSerializerTest, serialize_deserialize_ChannelURLInformation) {
    qRegisterMetaType<joynr::types::ChannelUrlInformation>("joynr::types::ChannelUrlInformation");
    qRegisterMetaType<joynr__types__ChannelUrlInformation>("joynr__types__ChannelUrlInformation");

    QList<QString> urls;
    urls.append("http://example1.com/");
    urls.append("http://example2.com/");
    types::ChannelUrlInformation urlInformation(urls);

    // Serialize the URL Information
    QByteArray serialized = JsonSerializer::serialize(QVariant::fromValue(urlInformation));
    LOG_DEBUG(logger,"serialized ChannelUrlInformation" + QString::fromUtf8(serialized));

    // Expected JSON : { "_typeName" : "joynr.types.ChannelUrlInformation", "urls" : [ "http://example1.com/", "http://example2.com/" ] }
    QByteArray expected("{\"_typeName\":\"joynr.types.ChannelUrlInformation\",\"urls\":[\"http://example1.com/\",\"http://example2.com/\"]}");

    EXPECT_EQ(expected, serialized);

    // Deserialize
    types::ChannelUrlInformation* deserializedInfo = JsonSerializer::deserialize<types::ChannelUrlInformation>(serialized);

    // Check the structure
    QList<QString> deserializedUrls = deserializedInfo->getUrls();
    EXPECT_EQ(urls, deserializedUrls);
}

TEST_F(JsonSerializerTest, deserialize_ProviderQos) {
    joynr::types::ProviderQos qos;

    QByteArray jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[],\"priority\":5,\"providerVersion\":3,\"scope\":\"LOCAL\",\"supportsOnChangeSubscriptions\":false}");

    joynr::types::ProviderQos* providerQos = JsonSerializer::deserialize<joynr::types::ProviderQos>(jsonProviderQos);

    EXPECT_EQ(providerQos->getScope(), joynr::types::ProviderScope::LOCAL);
    EXPECT_EQ(providerQos->getProviderVersion(), 3);
    EXPECT_EQ(providerQos->getPriority(), 5);

    delete providerQos;
}

TEST_F(JsonSerializerTest, serialize_ProviderQos) {
    joynr::types::ProviderQos qos;
    qos.setScope(joynr::types::ProviderScope::LOCAL);
    qos.setPriority(5);
    qos.setProviderVersion(-1);

    QByteArray jsonProviderQos("{\"_typeName\":\"joynr.types.ProviderQos\",\"customParameters\":[],\"priority\":5,\"providerVersion\":-1,\"scope\":\"LOCAL\",\"supportsOnChangeSubscriptions\":false}");

    QByteArray result = JsonSerializer::serialize(qos);

    EXPECT_EQ(jsonProviderQos, result);
}


TEST_F(JsonSerializerTest, deserialize_GPSLocation) {
    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<types::GpsFixEnum>();
    qRegisterMetaType<types::GpsFixEnum::Enum>("types::GpsFixEnum");

    QByteArray jsonGPS(
                "{\"_typeName\":\"joynr.types.GpsLocation\","
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
    joynr::types::GpsLocation* receivedReply = JsonSerializer::deserialize<joynr::types::GpsLocation>(jsonGPS);
    // Clean up
    delete receivedReply;
}

TEST_F(JsonSerializerTest, serialize_OnchangeWithKeepAliveSubscription) {
    qRegisterMetaType<joynr::SubscriptionQos>("joynr::SubscriptionQos");
    qRegisterMetaType<QSharedPointer<SubscriptionQos>>();

    qRegisterMetaType<joynr::OnChangeSubscriptionQos>("joynr::OnChangeSubscriptionQos");
    qRegisterMetaType<QSharedPointer<OnChangeSubscriptionQos>>();

    qRegisterMetaType<joynr::OnChangeWithKeepAliveSubscriptionQos>("joynr::OnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<QSharedPointer<joynr::OnChangeWithKeepAliveSubscriptionQos>>();


    joynr::OnChangeWithKeepAliveSubscriptionQos qos(750, 100, 900, 1050);

    QByteArray jsonQos = JsonSerializer::serialize(qos);
    LOG_DEBUG(logger,"serialized OnChangeWithKeepAliveSubscriptionQos" + QString::fromUtf8(jsonQos));

    joynr::OnChangeWithKeepAliveSubscriptionQos* desQos = JsonSerializer::deserialize<joynr::OnChangeWithKeepAliveSubscriptionQos>(jsonQos);

    jsonQos = JsonSerializer::serialize(*desQos);
    LOG_DEBUG(logger,"serialized OnChangeWithKeepAliveSubscriptionQos" + QString::fromUtf8(jsonQos));


    EXPECT_EQ(qos, *desQos);

    // Clean up
    delete desQos;
}

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

#include <sstream>
#include <utility>
#include <typeinfo>
#include <memory>
#include <tuple>
#include <string>

#include "tests/utils/Gtest.h"

#include "joynr/MutableMessageFactory.h"
#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Logger.h"

#include "joynr/types/TestTypes/TEverythingStruct.h"
#include "joynr/types/TestTypes/TEverythingExtendedStruct.h"
#include "joynr/types/TestTypes/TStruct.h"
#include "joynr/types/TestTypes/TEnum.h"
#include "joynr/tests/test/MethodWithErrorEnumExtendedErrorEnum.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/types/Version.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

#include "joynr/serializer/Serializer.h"

template <typename Serializer>
class RequestReplySerializerTest : public ::testing::Test
{
public:
    RequestReplySerializerTest()
            : Test(),
              complexParametersDatatypes({"joynr.types.TestTypes.TEverythingStruct",
                                          "joynr.types.TestTypes.TEverythingExtendedStruct",
                                          "joynr.types.TestTypes.TEnum"}),
              primitiveParametersDatatypes({"String", "Integer", "Float", "Bool"}),
              complexParametersValues(),
              primitiveParametersValues(std::string("Hello World"), 101, 9.99f, true),
              responseValues(std::string("Hello World"), 101, 9.99f, true, {1, 2, 3, 4})
    {
    }

protected:
    using OutputStream = muesli::StringOStream;
    using InputStream = muesli::StringIStream;

    using OutputArchive = typename Serializer::template OutputArchive<OutputStream>;
    using InputArchive = typename Serializer::template InputArchive<InputStream>;

    template <typename T>
    std::string serialize(const T& value)
    {
        OutputStream stream;
        OutputArchive oarchive(stream);
        oarchive(value);
        JOYNR_LOG_TRACE(logger(), "serialized to " + stream.getString());
        return stream.getString();
    }

    template <typename T>
    void deserialize(std::string str, T& value)
    {
        JOYNR_LOG_TRACE(logger(), "trying to deserialize from JSON: {}", str);
        InputStream stream(std::move(str));
        auto iarchive = std::make_shared<InputArchive>(stream);
        (*iarchive)(value);
    }

    // we need to serialize, then deserialize the request/reply to get it in a state to get the data
    // out again
    template <typename T>
    T initOutgoingIncoming(const T& outgoingType)
    {
        T incomingType;
        deserialize(serialize(outgoingType), incomingType);
        return incomingType;
    }

    template <typename Tuple, std::size_t... Indices>
    void setRequestParamsFromTuple(joynr::Request& request,
                                   Tuple tuple,
                                   std::index_sequence<Indices...>)
    {
        request.setParams(std::move(std::get<Indices>(tuple))...);
    }

    template <typename Tuple, std::size_t... Indices>
    void setReplyResponseFromTuple(joynr::Reply& reply,
                                   Tuple tuple,
                                   std::index_sequence<Indices...>)
    {
        reply.setResponse(std::move(std::get<Indices>(tuple))...);
    }

    template <typename... Ts>
    joynr::Request initRequest(std::string methodName,
                               std::string requestReplyId,
                               std::vector<std::string> paramDataTypes,
                               std::tuple<Ts...> paramValues)
    {
        joynr::Request outgoingRequest;
        outgoingRequest.setMethodName(methodName);
        outgoingRequest.setRequestReplyId(requestReplyId);
        outgoingRequest.setParamDatatypes(std::move(paramDataTypes));
        setRequestParamsFromTuple(outgoingRequest, paramValues, std::index_sequence_for<Ts...>{});

        return outgoingRequest;
    }

    template <typename T>
    joynr::Reply initReply(std::string requestReplyId, std::shared_ptr<T> error)
    {
        joynr::Reply outgoingReply;
        outgoingReply.setRequestReplyId(requestReplyId);
        outgoingReply.setError(error);
        return initOutgoingIncoming(outgoingReply);
    }

    template <typename... Ts>
    joynr::Reply initReply(std::string requestReplyId, std::tuple<Ts...> responseTuple)
    {
        joynr::Reply outgoingReply;
        outgoingReply.setRequestReplyId(requestReplyId);
        setReplyResponseFromTuple(outgoingReply, responseTuple, std::index_sequence_for<Ts...>{});
        return initOutgoingIncoming(outgoingReply);
    }

    joynr::Request initializeRequestWithComplexValues()
    {
        return initRequest("methodWithComplexParameters",
                           "000-10000-01100",
                           complexParametersDatatypes,
                           complexParametersValues);
    }

    joynr::Request initializeRequestWithPrimitiveValues()
    {
        return initRequest("realMethod",
                           "000-10000-01011",
                           primitiveParametersDatatypes,
                           primitiveParametersValues);
    }

    template <typename Tuple, std::size_t... Indices>
    void compareRequestData(joynr::Request& request,
                            Tuple expectedData,
                            std::index_sequence<Indices...>)
    {
        Tuple extractedData;
        request.getParams(std::get<Indices>(extractedData)...);
        EXPECT_EQ(expectedData, extractedData);
    }

    template <typename Tuple, std::size_t... Indices>
    void compareReplyData(joynr::Reply& reply, Tuple expectedData, std::index_sequence<Indices...>)
    {
        Tuple extractedData;
        reply.getResponse(std::get<Indices>(extractedData)...);
        EXPECT_EQ(expectedData, extractedData);
    }

    template <typename Tuple>
    using IndicesForTuple = std::make_index_sequence<std::tuple_size<Tuple>::value>;

    template <typename Tuple>
    auto getIndicesForTuple(Tuple&)
    {
        return IndicesForTuple<Tuple>{};
    }

    void compareRequestWithComplexValues(joynr::Request& request)
    {
        EXPECT_EQ(complexParametersDatatypes, request.getParamDatatypes());
        compareRequestData(
                request, complexParametersValues, getIndicesForTuple(complexParametersValues));
    }

    void compareRequestWithPrimitiveValues(joynr::Request& request)
    {
        EXPECT_EQ(primitiveParametersDatatypes, request.getParamDatatypes());
        compareRequestData(
                request, primitiveParametersValues, getIndicesForTuple(primitiveParametersValues));
    }

    void compareReplyWithExpectedResponse(joynr::Reply& reply)
    {
        ASSERT_TRUE(reply.hasResponse());
        compareReplyData(reply, responseValues, this->getIndicesForTuple(responseValues));
    }

    std::vector<std::string> complexParametersDatatypes;

    std::vector<std::string> primitiveParametersDatatypes;

    std::tuple<joynr::types::TestTypes::TEverythingStruct,
               joynr::types::TestTypes::TEverythingExtendedStruct,
               joynr::types::TestTypes::TEnum::Enum> complexParametersValues;

    std::tuple<std::string, int, float, bool> primitiveParametersValues;

    std::tuple<std::string, int, float, bool, std::vector<int32_t>> responseValues;
    ADD_LOGGER(RequestReplySerializerTest)
};

struct JsonSerializer
{
    template <typename Stream>
    using OutputArchive = muesli::JsonOutputArchive<Stream>;

    template <typename Stream>
    using InputArchive = muesli::JsonInputArchive<Stream>;
};

// typelist of serializers which shall be tested in the following tests
using Serializers = ::testing::Types<JsonSerializer>;

TYPED_TEST_SUITE(RequestReplySerializerTest, Serializers,);

TYPED_TEST(RequestReplySerializerTest, exampleDeserializerJoynrReplyWithProviderRuntimeException)
{
    auto error = std::make_shared<joynr::exceptions::ProviderRuntimeException>(
            "Message of ProviderRuntimeException");

    joynr::Reply reply = this->initReply("does-not-matter", error);
    auto deserializedError = reply.getError();

    const joynr::exceptions::ProviderRuntimeException* const errorPtr = error.get();
    const joynr::exceptions::JoynrException* const deserializedErrorPtr = deserializedError.get();
    ASSERT_EQ(typeid(*errorPtr), typeid(*deserializedErrorPtr));
    ASSERT_EQ(deserializedError->getMessage(), error->getMessage());
}

TYPED_TEST(RequestReplySerializerTest, exampleDeserializerJoynrReplyWithApplicationException)
{
    using namespace joynr::tests;
    std::string literal = test::MethodWithErrorEnumExtendedErrorEnum::getLiteral(
            test::MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION);
    // Create a ApplicationException
    auto error = std::make_shared<joynr::exceptions::ApplicationException>(
            literal, std::make_shared<test::MethodWithErrorEnumExtendedErrorEnum>());

    joynr::Reply reply = this->initReply("does-not-matter", error);
    auto deserializedError = reply.getError();

    const joynr::exceptions::ApplicationException* const errorPtr = error.get();
    const joynr::exceptions::JoynrException* const deserializedErrorPtr = deserializedError.get();
    ASSERT_EQ(typeid(*errorPtr), typeid(*deserializedErrorPtr));

    auto deserializedApplicationException =
            std::dynamic_pointer_cast<joynr::exceptions::ApplicationException>(deserializedError);
    ASSERT_TRUE(deserializedApplicationException != nullptr);
    ASSERT_EQ(*deserializedApplicationException, *error);
}

TYPED_TEST(RequestReplySerializerTest, exampleDeserializerJoynrReply)
{
    joynr::Reply reply = this->initReply("000-10000-01100", this->responseValues);
    this->compareReplyWithExpectedResponse(reply);
}

TYPED_TEST(RequestReplySerializerTest, exampleSerializerTestWithJoynrRequestOfPrimitiveParameters)
{
    // Create, initialize & check request with primitive parameters
    joynr::Request request = this->initializeRequestWithPrimitiveValues();
    this->compareRequestWithPrimitiveValues(request);
}

TYPED_TEST(RequestReplySerializerTest, exampleSerializerTestWithJoynrRequestOfComplexParameters)
{
    // Create, initialize & check request with complex parameters
    joynr::Request request = this->initializeRequestWithComplexValues();
    this->compareRequestWithComplexValues(request);
}

TYPED_TEST(RequestReplySerializerTest, serializeMessage)
{
    // Create a Request
    const bool isLocalMessage = true;
    joynr::Request outgoingRequest = this->initializeRequestWithPrimitiveValues();
    joynr::MutableMessage outgoingMessage = joynr::MutableMessageFactory().createRequest(
            "sender", "receiver", joynr::MessagingQos(), outgoingRequest, isLocalMessage);
    std::unique_ptr<joynr::ImmutableMessage> incomingMessage =
            outgoingMessage.getImmutableMessage();

    joynr::Request incomingRequest;
    smrf::ByteArrayView bodyView = incomingMessage->getUnencryptedBody();
    std::string serializedPayloadStr(bodyView.data(), bodyView.data() + bodyView.size());
    this->deserialize(serializedPayloadStr, incomingRequest);
    this->compareRequestWithPrimitiveValues(incomingRequest);
}

TYPED_TEST(RequestReplySerializerTest, serialize_deserialize_RequestWithGpsLocationList)
{
    using joynr::types::Localisation::GpsLocation;
    using joynr::types::Localisation::GpsFixEnum;
    using joynr::types::Localisation::Trip;

    using GpsLocationList = std::vector<GpsLocation>;
    GpsLocationList locations;
    locations.emplace_back(1.1, 1.2, 1.3, GpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    locations.emplace_back(2.1, 2.2, 2.3, GpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    locations.emplace_back(3.1, 3.2, 3.3, GpsFixEnum::MODE2D, 3.4, 3.5, 3.6, 3.7, 38, 39, 310);

    Trip trip1(locations, "trip1_name");
    std::string stringParam("contentParam1");
    int intParam = 2;

    auto requestParamTuple = std::make_tuple(stringParam, trip1, intParam);

    joynr::Request request = this->initRequest("", "", {}, requestParamTuple);
    this->compareRequestData(
            request, requestParamTuple, this->getIndicesForTuple(requestParamTuple));
}

TYPED_TEST(RequestReplySerializerTest, serialize_deserialize_ReplyWithArrayAsResponse)
{
    using joynr::types::GlobalDiscoveryEntry;
    using joynr::types::ProviderQos;
    joynr::types::Version providerVersion(47, 11);
    std::int64_t lastSeenMs = 3;
    std::int64_t expiryDateMs = 7;

    std::string publicKeyId("publicKeyId");
    joynr::system::RoutingTypes::MqttAddress mqttAddress1("localhost", "topic1");
    std::string serializedAddress1 = joynr::serializer::serializeToJson(mqttAddress1);
    joynr::system::RoutingTypes::MqttAddress mqttAddress2("localhost", "topic2");
    std::string serializedAddress2 = joynr::serializer::serializeToJson(mqttAddress2);

    std::vector<GlobalDiscoveryEntry> globalDiscoveryEntries;
    globalDiscoveryEntries.emplace_back(providerVersion,
                                        "domain1",
                                        "interface1",
                                        "participant1",
                                        ProviderQos(),
                                        lastSeenMs,
                                        expiryDateMs,
                                        publicKeyId,
                                        serializedAddress1);
    globalDiscoveryEntries.emplace_back(providerVersion,
                                        "domain2",
                                        "interface2",
                                        "participant2",
                                        ProviderQos(),
                                        lastSeenMs,
                                        expiryDateMs,
                                        publicKeyId,
                                        serializedAddress2);

    const std::string requestReplyId("serialize_deserialize_Reply_with_Array_as_Response");

    using ResponseTuple = std::tuple<std::vector<GlobalDiscoveryEntry>>;
    ResponseTuple responseTuple{globalDiscoveryEntries};
    joynr::Reply reply = this->initReply(requestReplyId, responseTuple);
    this->compareReplyData(reply, responseTuple, this->getIndicesForTuple(responseTuple));
}

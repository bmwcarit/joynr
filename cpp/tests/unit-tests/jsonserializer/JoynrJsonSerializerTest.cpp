/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#include <utility>
#include <iostream>
#include <string>
#include <cassert>
#include <functional>
#include <memory>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/ByteBuffer.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/Logger.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/types/TestTypes/TEverythingStruct.h"
#include "joynr/types/TestTypes/TStruct.h"
#include "joynr/types/TestTypes/TEnum.h"
#include "joynr/types/TestTypes/TEverythingMap.h"
#include "joynr/types/TestTypes/TStringKeyMap.h"
#include "joynr/types/TestTypes/TIntegerKeyMap.h"
#include "joynr/types/TestTypes/TStringToByteBufferMap.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/tests/test/MethodWithErrorEnumExtendedErrorEnum.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/types/Version.h"
#include "tests/PrettyPrint.h"

using namespace ::testing;
using namespace joynr;

//---- Example usage -----------------------------------------------------------
class JoynrJsonSerializerTest : public ::testing::Test
{

public:
    JoynrJsonSerializerTest() = default;
    void SetUp()
    {
    }

protected:
    ADD_LOGGER(JoynrJsonSerializerTest)
    void testSerializationOfTStruct(joynr::types::TestTypes::TStruct expectedStruct);

private:
};

TEST_F(JoynrJsonSerializerTest, exampleDeserializerApplicationException)
{
    using namespace joynr::tests;
    std::string literal = test::MethodWithErrorEnumExtendedErrorEnum::getLiteral(
            test::MethodWithErrorEnumExtendedErrorEnum::BASE_ERROR_TYPECOLLECTION);
    // Create a ApplicationException
    exceptions::ApplicationException exception(
            literal, std::make_shared<test::MethodWithErrorEnumExtendedErrorEnum>(literal));

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(exception);
    JOYNR_LOG_TRACE(logger(), "exceptions::ApplicationException JSON: {}", json);

    // Deserialize from JSON
    exceptions::ApplicationException t;
    joynr::serializer::deserializeFromJson(t, json);
    ASSERT_EQ(t.getMessage(), exception.getMessage());
    ASSERT_EQ(t.getName(), exception.getName());
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerProviderRuntimeException)
{
    // Create a ProviderRuntimeException
    exceptions::ProviderRuntimeException exception;
    std::string detailMessage{"Message of ProviderRuntimeException"};
    exception.setMessage(detailMessage);

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(exception);
    JOYNR_LOG_TRACE(logger(), "exceptions::ProviderRuntimeException JSON: {}", json);

    // Deserialize from JSON

    exceptions::ProviderRuntimeException t;
    joynr::serializer::deserializeFromJson(t, json);
    ASSERT_EQ(t.getMessage(), detailMessage);
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerDiscoveryException)
{
    // Create a DiscoveryException
    exceptions::DiscoveryException exception;
    std::string detailMessage{"Message of DiscoveryException"};
    exception.setMessage(detailMessage);

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(exception);
    JOYNR_LOG_TRACE(logger(), "exceptions::DiscoveryException JSON: {}", json);

    // Deserialize from JSON

    exceptions::DiscoveryException t;
    joynr::serializer::deserializeFromJson(t, json);
    ASSERT_EQ(t.getMessage(), detailMessage);
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerMethodInvocationException)
{
    // Create a MethodInvocationException
    exceptions::MethodInvocationException exception;
    std::string expectedDetailMessage{"Message of MethodInvocationException"};
    types::Version expectedProviderVersion(47, 11);
    exception.setMessage(expectedDetailMessage);
    exception.setProviderVersion(expectedProviderVersion);

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(exception);
    JOYNR_LOG_TRACE(logger(), "exceptions::MethodInvocationException JSON: {}", json);

    // Deserialize from JSON
    exceptions::MethodInvocationException deserializedException;
    joynr::serializer::deserializeFromJson(deserializedException, json);
    ASSERT_EQ(expectedDetailMessage, deserializedException.getMessage());
    ASSERT_EQ(expectedProviderVersion, deserializedException.getProviderVersion());
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializerEmptyStruct)
{
    // Create a PublicationMissedException
    using namespace joynr::system::RoutingTypes;

    // fully qualified type required here to avoid ambiguity with
    // gtest/gmock Address
    joynr::system::RoutingTypes::Address expectedAddress;

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(expectedAddress);
    JOYNR_LOG_TRACE(logger(), "Address JSON: {}", json);

    // Deserialize from JSON

    // fully qualified type required here to avoid ambiguity with
    // gtest/gmock Address
    joynr::system::RoutingTypes::Address actualAddress;
    joynr::serializer::deserializeFromJson(actualAddress, json);
    EXPECT_EQ(expectedAddress, actualAddress);
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerPublicationMissedException)
{
    // Create a PublicationMissedException
    exceptions::PublicationMissedException exception;
    std::string subscriptionId{"SubscriptionId of PublicationMissedException"};
    exception.setSubscriptionId(subscriptionId);

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(exception);
    JOYNR_LOG_TRACE(logger(), "exceptions::PublicationMissedException JSON: {}", json);

    // Deserialize from JSON
    exceptions::PublicationMissedException t;
    joynr::serializer::deserializeFromJson(t, json);
    ASSERT_EQ(t.getSubscriptionId(), subscriptionId);
    ASSERT_EQ(t.getMessage(), subscriptionId);
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerJoynrTimeOutException)
{
    // Create a JoynrTimeOutException
    exceptions::JoynrTimeOutException exception;
    std::string detailMessage{"Message of JoynrTimeOutException"};
    exception.setMessage(detailMessage);

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(exception);
    JOYNR_LOG_TRACE(logger(), "exceptions::JoynrTimeOutException JSON: {}", json);

    // Deserialize from JSON
    exceptions::JoynrTimeOutException t;
    joynr::serializer::deserializeFromJson(t, json);
    ASSERT_EQ(t.getMessage(), detailMessage);
}

TEST_F(JoynrJsonSerializerTest,
       exampleDeserializerJoynrSubscriptionPublicationWithProviderRuntimeException)
{
    // Create a Publication
    SubscriptionPublication publication;
    publication.setSubscriptionId("testSubscriptionId");
    publication.setError(std::make_shared<exceptions::ProviderRuntimeException>(
            "Message of ProviderRuntimeException"));

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(publication);
    JOYNR_LOG_TRACE(logger(), "SubscriptionPublication JSON: {}", json);

    SubscriptionPublication deserializedSubscriptionPublication;
    joynr::serializer::deserializeFromJson(deserializedSubscriptionPublication, json);

    EXPECT_EQ(publication, deserializedSubscriptionPublication);
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerSubscriptionPublication)
{
    // Create a publication
    SubscriptionPublication expectedPublication;
    std::string someString{"Hello World"};
    const std::int32_t expectedInt = 101;
    const float expectedFloat = 9.99f;
    bool expectedBool = true;

    expectedPublication.setResponse(someString, expectedInt, expectedFloat, expectedBool);
    expectedPublication.setSubscriptionId("000-10000-01101");

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(expectedPublication);
    JOYNR_LOG_TRACE(logger(), "SubscriptionPublication JSON: {}", json);

    SubscriptionPublication deserializedSubscriptionPublication;
    joynr::serializer::deserializeFromJson(deserializedSubscriptionPublication, json);

    EXPECT_EQ(expectedPublication, deserializedSubscriptionPublication);
}

TEST_F(JoynrJsonSerializerTest, exampleDeserializerMulticastSubscriptionRequest)
{
    // Create a multicast subscription request
    MulticastSubscriptionRequest expectedRequest;
    expectedRequest.setMulticastId("multicastId");
    expectedRequest.setSubscriptionId("000-10000-01101");
    expectedRequest.setSubscribeToName("subscribeToName");
    auto qos = std::make_shared<MulticastSubscriptionQos>();
    expectedRequest.setQos(qos);

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(expectedRequest);
    JOYNR_LOG_TRACE(logger(), "MulticastSubscriptionRequest JSON: {}", json);

    MulticastSubscriptionRequest deserializedMulticastSubscriptionRequest;
    joynr::serializer::deserializeFromJson(deserializedMulticastSubscriptionRequest, json);

    EXPECT_EQ(expectedRequest, deserializedMulticastSubscriptionRequest);
}

TEST_F(JoynrJsonSerializerTest, deserializerMulticastPublicationWithProviderRuntimeException)
{
    // Create a Publication
    MulticastPublication publication;
    publication.setMulticastId("testSubscriptionId");
    publication.setError(std::make_shared<exceptions::ProviderRuntimeException>(
            "Message of ProviderRuntimeException"));

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(publication);
    JOYNR_LOG_TRACE(logger(), "MulticastPublication JSON: {}", json);

    MulticastPublication deserializedMulticastPublication;
    joynr::serializer::deserializeFromJson(deserializedMulticastPublication, json);

    EXPECT_EQ(publication, deserializedMulticastPublication);
}

TEST_F(JoynrJsonSerializerTest, deserializerMulticastPublication)
{
    // Create a publication
    MulticastPublication expectedPublication;
    std::string someString{"Hello World"};
    const std::int32_t expectedInt = 101;
    const float expectedFloat = 9.99f;
    bool expectedBool = true;

    expectedPublication.setResponse(someString, expectedInt, expectedFloat, expectedBool);
    expectedPublication.setMulticastId("000-10000-01101");

    // Serialize into JSON
    std::string json = joynr::serializer::serializeToJson(expectedPublication);
    JOYNR_LOG_TRACE(logger(), "MulticastPublication JSON: {}", json);

    MulticastPublication deserializedMulticastPublication;
    joynr::serializer::deserializeFromJson(deserializedMulticastPublication, json);

    EXPECT_EQ(expectedPublication, deserializedMulticastPublication);
}

// test with real MasterAccessControlEntry
TEST_F(JoynrJsonSerializerTest, serializeDeserializeMasterAccessControlEntry)
{
    using namespace joynr::infrastructure::DacTypes;

    std::vector<TrustLevel::Enum> possibleTrustLevels(
            {TrustLevel::LOW, TrustLevel::MID, TrustLevel::HIGH});

    std::vector<Permission::Enum> possiblePermissions(
            {Permission::NO, Permission::ASK, Permission::YES});

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
    std::string json = joynr::serializer::serializeToJson(expectedMac);

    JOYNR_LOG_TRACE(logger(), "MAC JSON: {}", json);

    // Deserialize
    MasterAccessControlEntry mac;
    joynr::serializer::deserializeFromJson(mac, json);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMac.getDefaultConsumerPermission(), mac.getDefaultConsumerPermission());
    EXPECT_EQ(expectedMac.getDefaultRequiredControlEntryChangeTrustLevel(),
              mac.getDefaultRequiredControlEntryChangeTrustLevel());
    EXPECT_EQ(expectedMac.getDefaultRequiredTrustLevel(), mac.getDefaultRequiredTrustLevel());
    EXPECT_EQ(expectedMac.getDomain(), mac.getDomain());
    EXPECT_EQ(expectedMac.getInterfaceName(), mac.getInterfaceName());
    EXPECT_EQ(expectedMac.getOperation(), mac.getOperation());
    EXPECT_EQ(expectedMac.getPossibleConsumerPermissions(), mac.getPossibleConsumerPermissions());
    EXPECT_EQ(expectedMac.getPossibleRequiredControlEntryChangeTrustLevels(),
              mac.getPossibleRequiredControlEntryChangeTrustLevels());
    EXPECT_EQ(expectedMac.getPossibleRequiredTrustLevels(), mac.getPossibleRequiredTrustLevels());
    EXPECT_EQ(expectedMac.getUid(), mac.getUid());
    EXPECT_EQ(expectedMac, mac);
}

template <typename T>
void serializeDeserializeMap(const T& expectedMap, Logger& logger)
{
    // Serialize
    std::string json = joynr::serializer::serializeToJson(expectedMap);

    JOYNR_LOG_DEBUG(logger, "ExpectedMap Json: {}", json);

    // Deserialize
    T actualMap;
    joynr::serializer::deserializeFromJson(actualMap, json);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedMap, actualMap);
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTIntegerKeyMap)
{
    using namespace joynr::types::TestTypes;
    TIntegerKeyMap expectedMap;
    expectedMap.insert({1, "StringValue1"});
    expectedMap.insert({2, "StringValue2"});

    serializeDeserializeMap<TIntegerKeyMap>(expectedMap, logger());
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTEverythingMap)
{
    using namespace joynr::types::TestTypes;

    TEverythingMap expectedMap;
    expectedMap.insert({TEnum::TLITERALA, TEverythingExtendedStruct()});
    expectedMap.insert({TEnum::TLITERALB, TEverythingExtendedStruct()});

    serializeDeserializeMap<TEverythingMap>(expectedMap, logger());
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTStringKeyMap)
{
    using namespace joynr::types::TestTypes;

    TStringKeyMap expectedMap;
    expectedMap.insert({"StringKey1", "StringValue1"});
    expectedMap.insert({"StringKey2", "StringValue2"});

    serializeDeserializeMap<TStringKeyMap>(expectedMap, logger());
}

TEST_F(JoynrJsonSerializerTest, serializeDeserializeTStringToByteBufferMap)
{
    using namespace joynr::types::TestTypes;

    TStringToByteBufferMap expectedMap;
    expectedMap.insert({"StringKey1", {0, 1, 2, 3, 4, 5}});
    expectedMap.insert({"StringKey2", {10, 9, 8, 7, 6}});

    serializeDeserializeMap<TStringToByteBufferMap>(expectedMap, logger());
}

// test with TEverythingStruct
TEST_F(JoynrJsonSerializerTest, serializeDeserializeTEverythingStruct)
{
    using namespace joynr::types::TestTypes;

    ByteBuffer byteBuffer({1, 2, 3});
    std::vector<std::uint8_t> uInt8Array({3, 2, 1});
    std::vector<TEnum::Enum> enumArray({TEnum::TLITERALB});
    std::vector<std::string> stringArray({"one", "four"});
    std::vector<Vowel::Enum> vowelinies({Vowel::A, Vowel::E});
    Word wordWithVowelinies{vowelinies};
    Word wordEmpty{};
    std::vector<Word> words({wordWithVowelinies, wordEmpty});
    TStringKeyMap stringMap;
    stringMap.insert({"StringKey", "StringValue"});

    TypeDefForTStruct typeDefForTStruct{};

    TEverythingStruct expectedEverythingStruct{1,
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
                                               stringMap,
                                               typeDefForTStruct};

    // Serialize
    std::string json = joynr::serializer::serializeToJson(expectedEverythingStruct);

    JOYNR_LOG_TRACE(logger(), "TEverythingStruct JSON: {}", json);

    // Deserialize
    TEverythingStruct everythingStruct;
    joynr::serializer::deserializeFromJson(everythingStruct, json);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedEverythingStruct, everythingStruct);
}

void JoynrJsonSerializerTest::testSerializationOfTStruct(
        joynr::types::TestTypes::TStruct expectedStruct)
{
    using namespace joynr::types::TestTypes;

    std::string json = joynr::serializer::serializeToJson(expectedStruct);

    JOYNR_LOG_TRACE(logger(), "TStruct JSON: {}", json);

    // Deserialize
    TStruct actualStruct;
    joynr::serializer::deserializeFromJson(expectedStruct, json);

    // Check that the object serialized/deserialized correctly
    EXPECT_EQ(expectedStruct, actualStruct);
}

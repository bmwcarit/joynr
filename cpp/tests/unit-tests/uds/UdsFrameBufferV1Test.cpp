/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include <cstring>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "libjoynr/uds/UdsFrameBufferV1.h"

#include "joynr/system/RoutingTypes/UdsClientAddress.h"

#include "tests/PrettyPrint.h"

using namespace joynr;
using namespace testing;

constexpr size_t headerSize =
        sizeof(UdsFrameBufferV1::Cookie) + sizeof(UdsFrameBufferV1::BodyLength);
constexpr UdsFrameBufferV1::BodyLength testMessageSize = 0x00000400;

template <typename ASIOBUF>
smrf::ByteVector convertAsioBuffer(const ASIOBUF& buf)
{
    const auto* dataBegin = static_cast<const smrf::Byte*>(buf.data());
    return smrf::ByteVector(dataBegin, dataBegin + buf.size());
}

smrf::ByteVector getTestData()
{
    smrf::ByteVector testData;
    for (UdsFrameBufferV1::BodyLength i = 0; i < testMessageSize; i++) {
        testData.push_back(i % std::numeric_limits<smrf::Byte>::max());
    }

    return testData;
}

TEST(UdsFrameBufferV1Test, defaultCtor)
{
    UdsFrameBufferV1 test;
    ASSERT_EQ(test.raw().size(), headerSize) << "Frame buffer not initialized with header";
    ASSERT_THAT(convertAsioBuffer(test.raw()), Each(0))
            << "Frame header (size element) not initialized with 0";
    ASSERT_EQ(test.header().size(), headerSize) << "Frame buffer header has not expected length";
    try {
        test.body();
        FAIL() << "Empty frame should not be interpreted as message frame";
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        EXPECT_THAT(e.what(), HasSubstr("'MJ'"));
    }
    try {
        test.readMessage();
        FAIL() << "Empty frame should not be interpreted as message frame";
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        EXPECT_THAT(e.what(), HasSubstr("'MJM1'"));
    }
    try {
        test.readInit();
        FAIL() << "Empty frame should not be interpreted as init-frame";
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        EXPECT_THAT(e.what(), HasSubstr("'MJI1'"));
    }
}

TEST(UdsFrameBufferV1Test, messageCtor)
{
    smrf::ByteVector testData = getTestData();

    const auto testView = smrf::ByteArrayView(testData);
    UdsFrameBufferV1 test(testView);
    ASSERT_EQ(test.header().size(), headerSize);
    ASSERT_EQ(test.body().size(), testMessageSize) << "Message body has not been resized.";
    ASSERT_EQ(test.raw().size(), testMessageSize + headerSize);

    const auto raw = convertAsioBuffer(test.raw());
    ASSERT_THAT(smrf::ByteVector(raw.begin(), raw.begin() + sizeof(UdsFrameBufferV1::Cookie)),
                ElementsAre('M', 'J', 'M', '1'));
    ASSERT_THAT(smrf::ByteVector(
                        raw.begin() + sizeof(UdsFrameBufferV1::Cookie), raw.begin() + headerSize),
                ElementsAre(0, 0, 4, 0));
}

TEST(UdsFrameBufferV1Test, clientAddressCtor)
{
    const joynr::system::RoutingTypes::UdsClientAddress testAddress("Hello World");
    const auto testSerialized = joynr::serializer::serializeToJson(testAddress);
    const smrf::ByteVector testData(testSerialized.begin(), testSerialized.end());

    // Check serialize
    UdsFrameBufferV1 test(testAddress);
    ASSERT_EQ(test.header().size(), headerSize);
    ASSERT_EQ(test.body().size(), testData.size()) << "Message body has not been resized.";
    ASSERT_EQ(test.raw().size(), testData.size() + headerSize);
    const auto raw = convertAsioBuffer(test.raw());
    ASSERT_THAT(smrf::ByteVector(raw.begin(), raw.begin() + sizeof(UdsFrameBufferV1::Cookie)),
                ElementsAre('M', 'J', 'I', '1'));
    ASSERT_THAT(convertAsioBuffer(test.body()), ElementsAreArray(testSerialized));
}

TEST(UdsFrameBufferV1Test, bodyExceptions)
{
    UdsFrameBufferV1 test;
    constexpr UdsFrameBufferV1::BodyLength testSize =
            std::numeric_limits<UdsFrameBufferV1::BodyLength>::max();
    std::memcpy(test.header().data(),
                UdsFrameBufferV1::_msgMagicCookie.data(),
                sizeof(UdsFrameBufferV1::Cookie));
    auto* data = static_cast<smrf::Byte*>(test.header().data());
    std::memcpy(data + sizeof(UdsFrameBufferV1::Cookie),
                &testSize,
                sizeof(UdsFrameBufferV1::BodyLength));
    try {
        const auto allocatedSize = test.body().size();
        ASSERT_EQ(allocatedSize, testSize); // No allocation limit set for test environment.
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        EXPECT_THAT(std::string(e.what()), HasSubstr(std::to_string(testSize)));
    }
}

TEST(UdsFrameBufferV1Test, readMessage)
{
    const smrf::ByteVector testData = getTestData();
    const auto testView = smrf::ByteArrayView(testData);
    UdsFrameBufferV1 testDataBuffer(testView);

    UdsFrameBufferV1 test;
    std::memcpy(test.header().data(), testDataBuffer.header().data(), test.header().size());
    std::memcpy(test.body().data(), testDataBuffer.body().data(), test.body().size());
    ASSERT_THAT(convertAsioBuffer(test.readMessage()), ElementsAreArray(testData));

    // Check initialization after read
    ASSERT_EQ(test.raw().size(), headerSize);
    ASSERT_THAT(convertAsioBuffer(test.raw()), Each(0));
}

TEST(UdsFrameBufferV1Test, readInit)
{
    const joynr::system::RoutingTypes::UdsClientAddress testAddress("Hello World");
    UdsFrameBufferV1 testDataBuffer(testAddress);

    UdsFrameBufferV1 test;
    std::memcpy(test.header().data(), testDataBuffer.header().data(), test.header().size());
    std::memcpy(test.body().data(), testDataBuffer.body().data(), test.body().size());
    ASSERT_EQ(test.readInit(), testAddress);

    // Check initialization after read
    ASSERT_EQ(test.raw().size(), headerSize);
    ASSERT_THAT(convertAsioBuffer(test.raw()), Each(0));
}

TEST(UdsFrameBufferV1Test, readInitExceptions)
{
    const joynr::system::RoutingTypes::UdsClientAddress testAddress("Hello World");
    UdsFrameBufferV1 test(testAddress);
    const smrf::Byte wrongByte = 0xFF;
    std::memcpy(test.body().data(), &wrongByte, 1);

    try {
        test.readInit();
        FAIL() << "Should not be able to deserialize wrong byte.";
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        EXPECT_THAT(std::string(e.what()), HasSubstr("decode"));
    }
}

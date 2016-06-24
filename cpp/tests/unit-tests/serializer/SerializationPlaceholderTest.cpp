/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <muesli/streams/StringIStream.h>
#include <muesli/streams/StringOStream.h>

#include <muesli/BaseArchive.h>
#include <muesli/ArchiveRegistry.h>

#include "joynr/serializer/SerializerTraits.h"

using namespace testing;

namespace tag { struct mock; }

struct MockBase
{
    MOCK_METHOD1(serializeNullPtr, void(const std::nullptr_t&));
    MOCK_METHOD1(serializeString, void(const std::string&));
};

template <typename Stream>
struct MockInputArchive : MockBase, muesli::BaseArchive<muesli::tags::InputArchive, MockInputArchive<Stream>>
{
    using Parent = muesli::BaseArchive<muesli::tags::InputArchive, MockInputArchive<Stream>>;
    MockInputArchive () : Parent(this) {}
};
MUESLI_REGISTER_INPUT_ARCHIVE(MockInputArchive, tag::mock)

template <typename Stream>
struct MockOutputArchive : MockBase, muesli::BaseArchive<muesli::tags::OutputArchive, MockOutputArchive<Stream>>
{
    using Parent = muesli::BaseArchive<muesli::tags::OutputArchive, MockOutputArchive<Stream>>;
    MockOutputArchive () : Parent(this) {}
};
MUESLI_REGISTER_OUTPUT_ARCHIVE(MockOutputArchive, tag::mock)

template <typename Archive>
void serialize(Archive& archive, std::string& value)
{
    // forward call to archive since we cannot mock template methods
    archive.serializeString(value);
}

template <typename Archive>
void serialize(Archive& archive, std::nullptr_t& value)
{
    // forward call to archive since we cannot mock template methods
    archive.serializeNullPtr(value);
}

template <typename Archive>
struct MockDeserializable
{
    MockDeserializable(Archive&){}

    template <typename T>
    void get(T&) {}
};

namespace joynr
{
namespace serializer
{

template <>
struct SerializerTraits<tag::mock>
{
    template <typename Archive>
    using Deserializable = MockDeserializable<Archive>;
};

} // namespacer serializer
} // namespace joynr

#include "joynr/serializer/SerializationPlaceholder.h"


using InputArchive = MockInputArchive<muesli::StringIStream>;
using OutputArchive = MockOutputArchive<muesli::StringOStream>;

TEST(SerializationPlaceholderTest, initiallyEmpty)
{
    joynr::serializer::SerializationPlaceholder placeholder;
    ASSERT_FALSE(placeholder.containsOutboundData());
    ASSERT_FALSE(placeholder.containsInboundData());
}

TEST(SerializationPlaceholderTest, outbound)
{
    joynr::serializer::SerializationPlaceholder placeholder;

    std::string payload = "hello world";
    placeholder.setData(payload);
    ASSERT_TRUE(placeholder.containsOutboundData());
    ASSERT_FALSE(placeholder.containsInboundData());

    OutputArchive oarchive;
    EXPECT_CALL(oarchive, serializeString(Eq(payload)));
    oarchive(placeholder);
}

TEST(SerializationPlaceholderTest, emptyPlaceholderSerializesAsNullptr)
{
    joynr::serializer::SerializationPlaceholder placeholder;
    OutputArchive oarchive;
    EXPECT_CALL(oarchive, serializeNullPtr(_));
    oarchive(placeholder);
}

// FIXME: test crashes since the wrong type is stored in the variant which leads to calling the wrong Deserializable's destructor
TEST(SerializationPlaceholderTest, DISABLED_inbound)
{
    joynr::serializer::SerializationPlaceholder placeholder;

    InputArchive iarchive;
    iarchive(placeholder);
    ASSERT_FALSE(placeholder.containsOutboundData());
    ASSERT_TRUE(placeholder.containsInboundData());
}

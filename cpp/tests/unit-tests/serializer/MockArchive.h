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

#ifndef MOCKARCHIVE_H_
#define MOCKARCHIVE_H_

#include "tests/utils/Gmock.h"
#include <memory>
#include <string>

#include <muesli/ArchiveRegistry.h>
#include <muesli/BaseArchive.h>

#include "joynr/serializer/SerializerTraits.h"

using namespace testing;

namespace tag
{
struct mock;
}

struct MockBase {
    MOCK_METHOD1(serializeNullPtr, void(const std::nullptr_t&));
    MOCK_METHOD1(serializeString, void(const std::string&));
};

template <typename Stream>
struct MockInputArchive
        : MockBase,
          muesli::BaseArchive<muesli::tags::InputArchive, MockInputArchive<Stream>> {
    using Parent = muesli::BaseArchive<muesli::tags::InputArchive, MockInputArchive<Stream>>;
    MockInputArchive() : Parent(this)
    {
    }
    MockInputArchive(Stream&) : Parent(this)
    {
    }
};
MUESLI_REGISTER_INPUT_ARCHIVE(MockInputArchive, tag::mock)

template <typename Stream>
struct MockOutputArchive
        : MockBase,
          muesli::BaseArchive<muesli::tags::OutputArchive, MockOutputArchive<Stream>> {
    using Parent = muesli::BaseArchive<muesli::tags::OutputArchive, MockOutputArchive<Stream>>;
    MockOutputArchive(Stream&) : Parent(this)
    {
    }
    MockOutputArchive() : Parent(this)
    {
    }
};
MUESLI_REGISTER_OUTPUT_ARCHIVE(MockOutputArchive, tag::mock)

// only tuples of size one are supported in this test
template <typename Archive, typename T>
void serialize(Archive& archive, std::tuple<T>& value)
{
    // forward the first (and only) element of this tuple
    archive(std::get<0>(value));
}

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

template <typename Archive, typename T>
void serialize(Archive&, T&&)
{
    // no-op as fallback
}

template <typename Archive>
struct MockDeserializable {
    MockDeserializable(Archive&)
    {
    }

    template <typename T>
    void get(T&&)
    {
    }
};

namespace joynr
{
namespace serializer
{

template <>
struct SerializerTraits<tag::mock> {
    template <typename Archive>
    using Deserializable = MockDeserializable<Archive>;

    static constexpr const char* id()
    {
        return "mock";
    }
};

} // namespace serializer
} // namespace joynr

#endif // MOCKARCHIVE_H_

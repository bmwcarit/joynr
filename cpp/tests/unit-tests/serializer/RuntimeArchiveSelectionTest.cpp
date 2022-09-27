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
#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

// clang-format off
#include "tests/unit-tests/serializer/MockArchive.h" // must be included prior to Serializer.h
#include "joynr/serializer/Serializer.h"
// clang-format on

template <typename ExpectedArchive>
struct DemoType {
    MOCK_METHOD0(expectedCalled, void());
    MOCK_METHOD0(unexpectedCalled, void());

    template <typename UnexpectedArchive>
    void serialize(UnexpectedArchive&)
    {
        unexpectedCalled();
    }

    void serialize(ExpectedArchive&)
    {
        expectedCalled();
    }
};

template <typename T>
class RuntimeArchiveSelectionTestMustSucceed : public ::testing::Test
{
protected:
    template <typename Id, template <typename> class ArchiveTraits, typename Stream, typename Fun>
    void run(Stream& streamParam, Fun&& getter)
    {
        using Tag = typename Id::Tag;
        using ExpectedArchiveImpl = typename ArchiveTraits<Tag>::template type<Stream>;
        EXPECT_NO_THROW(auto archive = getter(Id::id(), streamParam);
                        DemoType<ExpectedArchiveImpl> demo;
                        EXPECT_CALL(demo, expectedCalled()).Times(1);
                        EXPECT_CALL(demo, unexpectedCalled()).Times(0);
                        archive(demo););
    }
};

template <typename T>
class RuntimeArchiveSelectionTestMustFail : public ::testing::Test
{
};

template <typename TagType>
struct GetId {
    using Tag = TagType;
    static constexpr const char* id()
    {
        return joynr::serializer::SerializerTraits<Tag>::id();
    }
};

template <typename TagType>
struct GetInputData;

template <>
struct GetInputData<tag::mock> {
    static constexpr const char* data()
    {
        return "";
    }
};

template <>
struct GetInputData<muesli::tags::json> {
    static constexpr const char* data()
    {
        return "{}";
    }
};

struct NonExistingTag;

template <>
struct GetId<NonExistingTag> {
    static constexpr const char* id()
    {
        return "non-existing-tag";
    }
};

using SuceedingTags = ::testing::Types<GetId<muesli::tags::json>, GetId<tag::mock>>;
TYPED_TEST_SUITE(RuntimeArchiveSelectionTestMustSucceed, SuceedingTags, );

using FailingTags = ::testing::Types<GetId<NonExistingTag>>;
TYPED_TEST_SUITE(RuntimeArchiveSelectionTestMustFail, FailingTags, );

TYPED_TEST(RuntimeArchiveSelectionTestMustSucceed, getOutputArchive)
{
    auto getter = [](auto&& id, auto&& stream) {
        return joynr::serializer::getOutputArchive(id, stream);
    };
    muesli::StringOStream ostream;
    this->template run<TypeParam, muesli::OutputArchiveTraits>(ostream, getter);
}

TYPED_TEST(RuntimeArchiveSelectionTestMustSucceed, getInputArchive)
{
    using namespace joynr::serializer;
    std::string input = GetInputData<typename TypeParam::Tag>::data();
    muesli::StringIStream istream(input);
    auto getter = [](auto&& id, auto&& stream) {
        return joynr::serializer::getInputArchive(id, stream);
    };
    this->template run<TypeParam, muesli::InputArchiveTraits>(istream, getter);
}

TYPED_TEST(RuntimeArchiveSelectionTestMustFail, getNonExistingOutputArchive)
{
    using namespace joynr::serializer;
    muesli::StringOStream ostream;
    ASSERT_THROW(getOutputArchive(TypeParam::id(), ostream), std::invalid_argument);
}

TYPED_TEST(RuntimeArchiveSelectionTestMustFail, getNonExistingInputArchive)
{
    using namespace joynr::serializer;
    std::string emptyInput;
    muesli::StringIStream istream(emptyInput);
    ASSERT_THROW(getInputArchive(TypeParam::id(), istream), std::invalid_argument);
}

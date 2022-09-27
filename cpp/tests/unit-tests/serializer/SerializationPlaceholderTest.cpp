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

#include <string>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include <muesli/streams/StringIStream.h>
#include <muesli/streams/StringOStream.h>

#include "tests/unit-tests/serializer/MockArchive.h" // must be included prior to Serializer.h

#include "joynr/serializer/SerializationPlaceholder.h"
#include "joynr/serializer/Serializer.h"

using namespace testing;

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

    const std::string payload = "hello world";
    std::string handOverToTest = payload;
    placeholder.setData(handOverToTest);
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

// FIXME: test crashes since the wrong type is stored in the variant which leads to calling the
// wrong Deserializable's destructor
TEST(SerializationPlaceholderTest, DISABLED_inbound)
{
    joynr::serializer::SerializationPlaceholder placeholder;

    InputArchive iarchive;
    iarchive(placeholder);
    ASSERT_FALSE(placeholder.containsOutboundData());
    ASSERT_TRUE(placeholder.containsInboundData());
}

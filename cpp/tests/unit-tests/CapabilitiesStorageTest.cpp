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

#include <cstdint>
#include <string>
#include <thread>
#include <chrono>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "joynr/CapabilitiesStorage.h"
#include "joynr/types/Version.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/DiscoveryEntry.h"

using namespace ::testing;
using namespace joynr;

class CapabilitiesStorageTestBase : public ::testing::Test
{
public:
    CapabilitiesStorageTestBase()
            : domain("domain"),
              interface("interface"),
              participantId("participantId"),
              version(47, 11),
              entry(version,
                    domain,
                    interface,
                    participantId,
                    types::ProviderQos(),
                    1000,
                    10000,
                    "publicKeyId")
    {
    }

protected:
    const std::string domain;
    const std::string interface;
    const std::string participantId;
    const joynr::types::Version version;
    const joynr::types::DiscoveryEntry entry;
};

class LocalCapabilitiesStorageTest : public CapabilitiesStorageTestBase
{
};

TEST_F(LocalCapabilitiesStorageTest, insertWithGbids)
{
    const std::vector<std::string> gbids = {"testGbid1", "testGbid2"};
    capabilities::Storage storage;
    storage.insert(this->entry, gbids);
    ASSERT_EQ(1, storage.size());

    auto optionalEntry = storage.lookupByParticipantId(this->participantId);
    ASSERT_TRUE(optionalEntry.is_initialized());
    EXPECT_EQ(this->entry, *optionalEntry);

    auto it = storage.cbegin();
    ASSERT_TRUE(it != storage.cend());
    EXPECT_EQ(joynr::capabilities::LocalDiscoveryEntry(entry, gbids), *it);
    EXPECT_EQ(gbids, (*it).gbids);
    EXPECT_TRUE(++it == storage.cend());
}

template <typename Storage>
class CapabilitiesStorageTest : public CapabilitiesStorageTestBase
{
};

using StorageTypes = ::testing::Types<capabilities::Storage, capabilities::CachingStorage>;

TYPED_TEST_SUITE(CapabilitiesStorageTest, StorageTypes,);

TYPED_TEST(CapabilitiesStorageTest, initiallyEmpty)
{
    TypeParam storage;
    EXPECT_EQ(0, storage.size());
}

TYPED_TEST(CapabilitiesStorageTest, insertThenLookup)
{
    TypeParam storage;
    storage.insert(this->entry);
    ASSERT_EQ(1, storage.size());

    auto optionalEntry = storage.lookupByParticipantId(this->participantId);
    ASSERT_TRUE(optionalEntry.is_initialized());
    EXPECT_EQ(this->entry, *optionalEntry);
}

TYPED_TEST(CapabilitiesStorageTest, lookupNonExistingParticipantId)
{
    const std::string nonExistingParticipantId = "non-existing";
    TypeParam storage;
    {
        auto optionalEntry = storage.lookupByParticipantId(nonExistingParticipantId);
        ASSERT_FALSE(optionalEntry);
    }

    storage.insert(this->entry);

    {
        auto optionalEntry = storage.lookupByParticipantId(nonExistingParticipantId);
        ASSERT_FALSE(optionalEntry);
    }
}

TYPED_TEST(CapabilitiesStorageTest, insertWithSameParticipantIdOverwritesExistingEntry)
{
    TypeParam storage;
    types::DiscoveryEntry entry2(this->entry);
    entry2.setDomain("new-domain");

    storage.insert(this->entry);
    ASSERT_EQ(1, storage.size());
    auto optionalEntry1 = storage.lookupByParticipantId(this->participantId);
    ASSERT_TRUE(optionalEntry1.is_initialized());
    EXPECT_EQ(this->entry, *optionalEntry1);

    storage.insert(entry2);
    ASSERT_EQ(1, storage.size());
    auto optionalEntry2 = storage.lookupByParticipantId(this->participantId);
    ASSERT_TRUE(optionalEntry2.is_initialized());
    EXPECT_EQ(entry2, *optionalEntry2);

    ASSERT_NE(this->entry, entry2);
}

TYPED_TEST(CapabilitiesStorageTest, removeExpiredEntries)
{
    TypeParam storage;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count();

    joynr::types::DiscoveryEntry entry1(this->version,
                                        this->domain,
                                        "interface1",
                                        "participantId1",
                                        types::ProviderQos(),
                                        0,
                                        now + 100,
                                        "publicKeyId");
    joynr::types::DiscoveryEntry entry2(this->version,
                                        this->domain,
                                        "interface2",
                                        "participantId2",
                                        types::ProviderQos(),
                                        0,
                                        now + 10000,
                                        "publicKeyId");

    storage.insert(entry1);
    storage.insert(entry2);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto removedEntries = storage.removeExpired();

    EXPECT_THAT(removedEntries, Contains(entry1));
    EXPECT_THAT(removedEntries, Not(Contains(entry2)));
}

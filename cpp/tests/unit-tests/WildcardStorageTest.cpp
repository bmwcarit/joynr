/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
#include <tuple>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "libjoynrclustercontroller/access-control/AccessControlUtils.h"
#include "libjoynrclustercontroller/access-control/WildcardStorage.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::access_control;

template <typename T>
using OptionalSet = joynr::access_control::WildcardStorage::OptionalSet<T>;

TEST(WildcardStorageTest, storeEntriesWithSameKey)
{
    const std::string key = "key*";
    joynr::access_control::WildcardStorage storage;
    const int numberOfEntriesToInsert = 10;

    for (int i = 0; i < numberOfEntriesToInsert; ++i) {
        dac::MasterAccessControlEntry entry;
        entry.setDomain(std::string("domain_") + std::to_string(i));
        storage.insert<access_control::wildcards::Domain>(key, entry);
    }

    const OptionalSet<dac::MasterAccessControlEntry> result =
            storage.getLongestMatch<dac::MasterAccessControlEntry>(key);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->size() == numberOfEntriesToInsert);
}

class WildcardStorageTestP : public TestWithParam<std::tuple<std::string, std::string, int, int>>
{
protected:
    WildcardStorageTestP()
            : key_0(std::get<0>(GetParam())),
              key_1(std::get<1>(GetParam())),
              expectedEntryInSet_firstLookup(std::get<2>(GetParam())),
              expectedEntryInSet_secondLookup(std::get<3>(GetParam())),
              mace_0(),
              mace_1()
    {
        mace_0.setDomain(std::get<0>(GetParam()));
        mace_1.setDomain(std::get<1>(GetParam()));
        storage.insert<access_control::wildcards::Domain>(key_0, mace_0);
        storage.insert<access_control::wildcards::Domain>(key_1, mace_1);
    }

    joynr::access_control::WildcardStorage storage;

    // test data
    const std::string key_0;
    const std::string key_1;
    const int expectedEntryInSet_firstLookup;
    const int expectedEntryInSet_secondLookup;
    dac::MasterAccessControlEntry mace_0;
    dac::MasterAccessControlEntry mace_1;

public:
    static bool isACEinSet(const OptionalSet<dac::MasterAccessControlEntry>& set,
                           const dac::MasterAccessControlEntry& mace)
    {
        if (!set) {
            return false;
        }

        for (auto entry : *set) {
            if (entry == mace) {
                return true;
            }
        }

        return false;
    }
};

TEST_P(WildcardStorageTestP, lookupInStorageWith2Entries)
{
    // lookup first key
    OptionalSet<dac::MasterAccessControlEntry> resultSet =
            storage.getLongestMatch<dac::MasterAccessControlEntry>(key_0);
    ASSERT_TRUE(resultSet);
    EXPECT_EQ(resultSet->size(), expectedEntryInSet_firstLookup);
    EXPECT_TRUE(isACEinSet(resultSet, mace_0));

    resultSet->clear();

    // lookup second key
    resultSet = storage.getLongestMatch<dac::MasterAccessControlEntry>(key_1);
    ASSERT_TRUE(resultSet);
    EXPECT_EQ(resultSet->size(), expectedEntryInSet_secondLookup);
    EXPECT_TRUE(isACEinSet(resultSet, mace_1));
}

const std::vector<std::tuple<std::string, std::string, int, int>> testData_TwoChildrenOfParent = {
        // Params: 1st and 2nd key to be inserted in the storage followed by expected entries in
        // first and second lookup

        // two children of root (with domain like separator)
        std::make_tuple("joynr.test.*", "test.joynr.*", 1, 1),
        std::make_tuple("test.joynr.*", "joynr.test.*", 1, 1),

        // two children of root
        std::make_tuple("a*", "b*", 1, 1),
        std::make_tuple("b*", "a*", 1, 1)};

const std::vector<std::tuple<std::string, std::string, int, int>> testData_ChildOfChild = {
        // Params: 1st and 2nd key to be inserted in the storage followed by expected entries in
        // first and second lookup

        // child of child (with empty string)
        std::make_tuple("*", "k*", 1, 2),
        std::make_tuple("k*", "*", 2, 1),

        // child of child
        std::make_tuple("key*", "key0*", 1, 2),
        std::make_tuple("key0*", "key*", 2, 1),

        // child of child (with domain like separator and empty string)
        std::make_tuple("*", "joynr.*", 1, 2),
        std::make_tuple("joynr.*", "*", 2, 1)};

INSTANTIATE_TEST_SUITE_P(lookupChildOfChild,
                        WildcardStorageTestP,
                        ::testing::ValuesIn(testData_ChildOfChild));

INSTANTIATE_TEST_SUITE_P(lookupTwoChildrenOfParent,
                        WildcardStorageTestP,
                        ::testing::ValuesIn(testData_TwoChildrenOfParent));

template <typename T>
class WildcardStorageTestTypedTest : public ::testing::Test
{
};

using AccessControlTestTypes = ::testing::Types<dac::MasterAccessControlEntry,
                                                dac::MediatorAccessControlEntry,
                                                dac::OwnerAccessControlEntry,
                                                dac::MasterRegistrationControlEntry,
                                                dac::MediatorRegistrationControlEntry,
                                                dac::OwnerRegistrationControlEntry>;
TYPED_TEST_SUITE(WildcardStorageTestTypedTest, AccessControlTestTypes,);

TYPED_TEST(WildcardStorageTestTypedTest, insertAndLookupOptionalSet)
{
    joynr::access_control::WildcardStorage storage;

    const std::string key = "key*";
    const TypeParam entry;
    storage.insert<access_control::wildcards::Domain>(key, entry);

    const OptionalSet<TypeParam> resultSet = storage.getLongestMatch<TypeParam>(key);

    ASSERT_TRUE(resultSet);
    EXPECT_EQ(resultSet->size(), 1);
    EXPECT_EQ(*resultSet->begin(), entry);
}

TYPED_TEST(WildcardStorageTestTypedTest, lookupInEmptySet)
{
    joynr::access_control::WildcardStorage storage;
    const std::string key = "key*";
    const OptionalSet<TypeParam> resultSet = storage.getLongestMatch<TypeParam>(key);
    ASSERT_TRUE(!resultSet);
}

TEST(WildcardStorageTest, insertTestWithMultipleEntriesPerKey)
{
    joynr::access_control::WildcardStorage storage;

    const std::string rootKey = "*";
    dac::MasterAccessControlEntry rootEntry;
    rootEntry.setInterfaceName("rootEntry");

    const std::string subkey1 = "wildcard.key.with.common.parts.123*";
    dac::MasterAccessControlEntry entry1;
    entry1.setInterfaceName("entry1");

    dac::MasterAccessControlEntry entry2;
    entry2.setInterfaceName("entry2");

    const std::string subkey2 = "wildcard.key.with.common.parts.456*";
    dac::MasterAccessControlEntry entry3;
    entry3.setInterfaceName("entry3");

    dac::MasterAccessControlEntry entry4;
    entry4.setInterfaceName("entry4");

    dac::MasterAccessControlEntry entry5;
    entry5.setInterfaceName("entry5");

    storage.insert<std::string>(rootKey, rootEntry);
    storage.insert<std::string>(subkey1, entry1);
    storage.insert<std::string>(subkey1, entry2);
    storage.insert<std::string>(subkey2, entry3);
    storage.insert<std::string>(subkey2, entry4);
    storage.insert<std::string>(subkey2, entry5);

    OptionalSet<dac::MasterAccessControlEntry> resultSet =
            storage.getLongestMatch<dac::MasterAccessControlEntry>(subkey1);
    ASSERT_TRUE(resultSet);
    EXPECT_EQ(resultSet->size(), 3); // 2 specific entries + root entry
    EXPECT_TRUE(WildcardStorageTestP::isACEinSet(resultSet, rootEntry));
    EXPECT_TRUE(WildcardStorageTestP::isACEinSet(resultSet, entry1));
    EXPECT_TRUE(WildcardStorageTestP::isACEinSet(resultSet, entry2));

    resultSet = storage.getLongestMatch<dac::MasterAccessControlEntry>(subkey2);
    ASSERT_TRUE(resultSet);
    EXPECT_EQ(resultSet->size(), 4); // 3 specific entries + root entry
    EXPECT_TRUE(WildcardStorageTestP::isACEinSet(resultSet, rootEntry));
    EXPECT_TRUE(WildcardStorageTestP::isACEinSet(resultSet, entry3));
    EXPECT_TRUE(WildcardStorageTestP::isACEinSet(resultSet, entry4));
    EXPECT_TRUE(WildcardStorageTestP::isACEinSet(resultSet, entry5));
}

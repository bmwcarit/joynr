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

#ifndef WILDCARDSTORAGE_H
#define WILDCARDSTORAGE_H

#include <string>
#include <tuple>
#include <unordered_set>

#include <boost/optional.hpp>

#include "libjoynrclustercontroller/access-control/AccessControlUtils.h"
#include "libjoynrclustercontroller/access-control/RadixTree.h"

namespace joynr
{
namespace access_control
{

/*
 * Generic WildCard storage for registration and access
 * control entries.
 */
class WildcardStorage
{
public:
    /*
     * Store all ControlEntry with the same "key" (domain or interface) in a set.
     * Two or multiple entries with the same "key" can be different from one other.
     */
    template <typename T>
    using Set = std::unordered_set<T>;

    template <typename T>
    using OptionalSet = boost::optional<Set<T>>;

    template <typename domainOrInterface, typename ACEntry>
    void insert(const std::string& inputKey, const ACEntry& entry)
    {
        assert(!inputKey.empty());

        std::string key = inputKey;
        // remove wildcard symbol at the end
        key.pop_back();

        RadixTreeNode* longestMatch = storage.longestMatch(key);
        if (longestMatch == nullptr) {
            // if not found then create StorageEntry and insert in radix_tree
            OptionalSet<ACEntry> newSet = Set<ACEntry>();
            newSet->insert(entry);
            StorageEntry newStorageEntry;
            setStorageEntry<ACEntry>(newStorageEntry, newSet);
            storage.insert(std::move(key), std::move(newStorageEntry));
        } else {
            StorageEntry& foundStorageEntry = longestMatch->getValue();
            OptionalSet<ACEntry>& setOfACEntries = getStorageEntry<ACEntry>(foundStorageEntry);
            // does the set exist in the storageEntry?
            if (setOfACEntries) {
                setOfACEntries->insert(entry);
            } else {
                // update entry into the set and then set into the tree
                OptionalSet<ACEntry> newSet = Set<ACEntry>();
                newSet->insert(entry);
                setStorageEntry<ACEntry>(foundStorageEntry, std::move(newSet));
            }
        }
    }

    template <typename ACEntry>
    OptionalSet<ACEntry> getLongestMatch(const std::string& query) const
    {
        // get longest match from storage
        RadixTreeNode* longestMatch = storage.longestMatch(query);
        if (longestMatch == nullptr) {
            // entry does not exist in radix tree
            return boost::none;
        }

        // if there is a set in the StorageEntry for this ACEntry -> return it
        StorageEntry& storageEntry = longestMatch->getValue();
        OptionalSet<ACEntry> result = getStorageEntry<ACEntry>(storageEntry);
        if (result) {
            return result;
        }

        // otherwise try to get an entry from one of the parents in the tree
        auto parents = longestMatch->parents();
        for (auto parentIt = parents.begin(); parentIt != parents.end(); ++parentIt) {
            // if there is a set in the StorageEntry of the parent -> return it
            result = getStorageEntry<ACEntry>((*parentIt)->getValue());
            if (result) {
                return result;
            }
        }

        // otherwise return none
        // longest match exists but no ACE/RCE entry was set for it
        // is this a configuration error?
        return boost::none;
    }

private:
    using StorageEntry = std::tuple<OptionalSet<dac::MasterAccessControlEntry>,
                                    OptionalSet<dac::MediatorAccessControlEntry>,
                                    OptionalSet<dac::OwnerAccessControlEntry>,
                                    OptionalSet<dac::MasterRegistrationControlEntry>,
                                    OptionalSet<dac::MediatorRegistrationControlEntry>,
                                    OptionalSet<dac::OwnerRegistrationControlEntry>>;

    // helper function to retrive a specific ControlEntry from storage
    // look for a DAC type in RadixTree storage
    // return a boost::optional<DAC>
    template <typename ACEntry>
    OptionalSet<ACEntry>& getStorageEntry(StorageEntry& storageEntry) const
    {
        // temporarily return a copy of the entry
        return std::get<OptionalSet<ACEntry>>(storageEntry);
    }

    // helper function to set a specific StorageEntry in the storage
    // recives an OptionalSet<ACEntry> and set it in the tree
    // always overwrite if exists
    template <typename ACEntry>
    void setStorageEntry(StorageEntry& storageEntry, OptionalSet<ACEntry> value)
    {
        std::get<OptionalSet<ACEntry>>(storageEntry).swap(value);
    }

    template <std::size_t index = 0>
    bool isEmpty(const StorageEntry& storageEntry) const
    {
        if (index == std::tuple_size<StorageEntry>::value) {
            return true;
        }

        return std::get<index>(storageEntry) && isEmpty<index + 1>(storageEntry);
    }

    template <typename ACEntry, typename tag>
    struct MatchesKey
    {
        static bool apply(const ACEntry& entry, const std::string& key);
    };

    template <typename ACEntry>
    struct MatchesKey<ACEntry, access_control::wildcards::Domain>
    {
        static bool apply(const ACEntry& entry, const std::string& key)
        {
            return entry.getDomain() == key;
        }
    };

    template <typename ACEntry>
    struct MatchesKey<ACEntry, access_control::wildcards::Interface>
    {
        static bool apply(const ACEntry& entry, const std::string& key)
        {
            return entry.getInterfaceName() == key;
        }
    };

    RadixTree<std::string, StorageEntry> storage;
    using RadixTreeNode = RadixTree<std::string, StorageEntry>::Node;
};

} // namespace access_control
} // namespace joynr

#endif // WILDCARDSTORAGE_H

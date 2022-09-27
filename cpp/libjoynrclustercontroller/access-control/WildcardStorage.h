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

#include <numeric>
#include <string>
#include <tuple>
#include <unordered_set>

#include <boost/optional.hpp>

#include "joynr/serializer/Serializer.h"

#include "AccessControlUtils.h"
#include "RadixTree.h"

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
        assert(inputKey.back() == *joynr::access_control::WILDCARD);

        std::string key = inputKey;
        // remove wildcard symbol at the end
        key.pop_back();

        RadixTreeNode* longestMatch = _storage.longestMatch(key);
        if (longestMatch != nullptr) {
            // node with exact key already in tree
            if (longestMatch->getFullKey() == key) {
                StorageEntry& foundStorageEntry = longestMatch->getValue();
                OptionalSet<ACEntry>& setOfACEntries = getStorageEntry<ACEntry>(foundStorageEntry);
                // does the set exist in the storageEntry?
                if (setOfACEntries) {
                    setOfACEntries->insert(entry);
                    return;
                } else {
                    // update entry into the set and then set into the tree
                    OptionalSet<ACEntry> newSet = Set<ACEntry>();
                    newSet->insert(entry);
                    setStorageEntry<ACEntry>(foundStorageEntry, std::move(newSet));
                    return;
                }
            }
        }

        // if not found then create StorageEntry and insert in radix_tree
        OptionalSet<ACEntry> newSet = Set<ACEntry>();
        newSet->insert(entry);
        StorageEntry newStorageEntry;
        setStorageEntry<ACEntry>(newStorageEntry, newSet);
        _storage.insert(std::move(key), std::move(newStorageEntry));
    }

    template <typename ACEntry>
    OptionalSet<ACEntry> getLongestMatch(const std::string& query) const
    {
        // get longest match from storage
        RadixTreeNode* longestMatch = _storage.longestMatch(query);
        if (longestMatch == nullptr) {
            // entry does not exist in radix tree
            return boost::none;
        }

        // if there is a set in the StorageEntry for this ACEntry -> add it to the result set
        Set<ACEntry> resultSet;

        StorageEntry& storageEntry = longestMatch->getValue();
        auto longestMatchEntry = getStorageEntry<ACEntry>(storageEntry);
        if (longestMatchEntry) {
            resultSet.insert(longestMatchEntry->begin(), longestMatchEntry->end());
        }

        // also add all parents (need to add the entire branch of the radix-tree)
        auto parents = longestMatch->parents();
        for (auto parentIt = parents.begin(); parentIt != parents.end(); ++parentIt) {
            // if there is a set in the StorageEntry of the parent -> return it
            auto parentSet = getStorageEntry<ACEntry>((*parentIt)->getValue());
            if (parentSet) {
                resultSet.insert(parentSet->begin(), parentSet->end());
            }
        }

        if (resultSet.empty()) {
            return boost::none;
        }
        return resultSet;
    }

    std::string toString()
    {
        std::stringstream stream;
        auto visitor = [&stream](const auto& node, const auto& keys) {
            const std::string key =
                    std::accumulate(keys.begin(), keys.end(), std::string(),
                                    [](std::string& s, const auto& i) { return s + i.get(); });
            stream << key << "->" << serializer::serializeToJson(node.getValue()) << std::endl;
        };
        _storage.visit(visitor);
        return stream.str();
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
    struct MatchesKey {
        static bool apply(const ACEntry& entry, const std::string& key);
    };

    template <typename ACEntry>
    struct MatchesKey<ACEntry, access_control::wildcards::Domain> {
        static bool apply(const ACEntry& entry, const std::string& key)
        {
            return entry.getDomain() == key;
        }
    };

    template <typename ACEntry>
    struct MatchesKey<ACEntry, access_control::wildcards::Interface> {
        static bool apply(const ACEntry& entry, const std::string& key)
        {
            return entry.getInterfaceName() == key;
        }
    };

    RadixTree<std::string, StorageEntry> _storage;
    using RadixTreeNode = RadixTree<std::string, StorageEntry>::Node;
};

} // namespace access_control
} // namespace joynr

#endif // WILDCARDSTORAGE_H

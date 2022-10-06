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

#ifndef ACCESSCONTROLUTILS_H
#define ACCESSCONTROLUTILS_H

#include <string>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include "joynr/infrastructure/DacTypes/DomainRoleEntry.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerRegistrationControlEntry.h"

namespace joynr
{
namespace access_control
{

static constexpr const char* WILDCARD = "*";

namespace bmi = boost::multi_index;

namespace dac
{
using namespace joynr::infrastructure::DacTypes;

/*
 * ================================================================
 * Dummy types for Mediator classes
 * ================================================================
 */
struct MediatorAccessControlEntry : public dac::MasterAccessControlEntry {
    using MasterAccessControlEntry::MasterAccessControlEntry;
    MediatorAccessControlEntry() = default;

    MediatorAccessControlEntry(const MasterAccessControlEntry& entry)
            : MasterAccessControlEntry(entry)
    {
    }

    const auto& getOperation() const
    {
        return MasterAccessControlEntry::getOperation();
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<joynr::infrastructure::DacTypes::MasterAccessControlEntry>(this));
    }
};
struct MediatorRegistrationControlEntry : public dac::MasterRegistrationControlEntry {
    using MasterRegistrationControlEntry::MasterRegistrationControlEntry;

    MediatorRegistrationControlEntry() = default;

    MediatorRegistrationControlEntry(const MasterRegistrationControlEntry& entry)
            : MasterRegistrationControlEntry(entry)
    {
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>(
                this));
    }
};
std::size_t hash_value(const MediatorAccessControlEntry& masterAccessControlEntryValue);
std::size_t hash_value(const MediatorRegistrationControlEntry& masterAccessControlEntryValue);
} // namespace dac

namespace wildcards
{
struct Domain;
struct Interface;
} // namespace wildcards

namespace tags
{
struct UidDomainInterfaceOperation;
struct DomainAndInterface;
struct Domain;
} // namespace tags

namespace tableTags
{
struct access;
struct registration;
} // namespace tableTags

template <typename T>
struct MetaTableView;

template <>
struct MetaTableView<dac::MasterAccessControlEntry> {
    using tag = tableTags::access;
};

template <>
struct MetaTableView<dac::MediatorAccessControlEntry> {
    using tag = tableTags::access;
};

template <>
struct MetaTableView<dac::OwnerAccessControlEntry> {
    using tag = tableTags::access;
};

template <>
struct MetaTableView<dac::MasterRegistrationControlEntry> {
    using tag = tableTags::registration;
};

template <>
struct MetaTableView<dac::MediatorRegistrationControlEntry> {
    using tag = tableTags::registration;
};

template <>
struct MetaTableView<dac::OwnerRegistrationControlEntry> {
    using tag = tableTags::registration;
};

struct TableViewTraitsBase {
    // this custom comparator ensures that wildcards come last
    struct WildcardComparator {
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            if (lhs == WILDCARD) {
                return false;
            }
            if (rhs == WILDCARD) {
                return true;
            }
            return (lhs < rhs);
        }
    };

    // this comparator can only be used in the result set
    struct SetUnionComparator {
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            // sort wildcard always at the end
            if (lhs == "*") {
                return false;
            }
            if (rhs == "*") {
                return true;
            }

            // give precedence to not-wildcarded entries
            if (lhs.back() == '*' && rhs.back() != '*') {
                return false;
            }
            if (rhs.back() == '*' && lhs.back() != '*') {
                return true;
            }

            // sort alphabetically
            return (lhs > rhs);
        }
    };

    using DefaultComparator = std::less<std::string>;

    using UidKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::ControlEntry, const std::string&, getUid);
    using DomainKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::ControlEntry,
                                                      const std::string&,
                                                      getDomain);
    using InterfaceKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::ControlEntry,
                                                         const std::string&,
                                                         getInterfaceName);
};

// result set
template <typename Entry>
struct TableViewResultSet : TableViewTraitsBase {
    using Type = bmi::multi_index_container<
            Entry,
            bmi::indexed_by<
                    bmi::ordered_unique<bmi::composite_key<Entry, UidKey, DomainKey, InterfaceKey>,
                                        bmi::composite_key_compare<SetUnionComparator,
                                                                   SetUnionComparator,
                                                                   SetUnionComparator>>>>;
};

template <typename AccessTag, typename Entry>
struct TableViewTraits;

template <typename Entry>
struct TableViewTraits<tableTags::access, Entry> : TableViewTraitsBase {
    using OperationKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(Entry, const std::string&, getOperation);

    using Type = bmi::ordered_unique<
            bmi::tag<tags::UidDomainInterfaceOperation>,
            bmi::composite_key<Entry, UidKey, DomainKey, InterfaceKey, OperationKey>,
            bmi::composite_key_compare<WildcardComparator,
                                       DefaultComparator,
                                       DefaultComparator,
                                       WildcardComparator>>;
};

template <typename Entry>
struct TableViewTraits<tableTags::registration, Entry> : TableViewTraitsBase {
    using Type = bmi::ordered_unique<
            bmi::tag<tags::UidDomainInterfaceOperation>,
            bmi::composite_key<Entry, UidKey, DomainKey, InterfaceKey>,
            bmi::composite_key_compare<WildcardComparator, DefaultComparator, DefaultComparator>>;
};

template <typename Entry>
struct TableMaker {
    using Tag = typename MetaTableView<Entry>::tag;
    using TableViewType = typename TableViewTraits<Tag, Entry>::Type;
    using DomainKey = typename TableViewTraits<Tag, Entry>::DomainKey;
    using InterfaceKey = typename TableViewTraits<Tag, Entry>::InterfaceKey;

    using Type = bmi::multi_index_container<
            Entry,
            bmi::indexed_by<
                    TableViewType,
                    bmi::ordered_non_unique<bmi::tag<tags::DomainAndInterface>,
                                            bmi::composite_key<Entry, DomainKey, InterfaceKey>>,
                    bmi::hashed_non_unique<bmi::tag<tags::Domain>, DomainKey>>>;
};

namespace domain_role
{
using UidKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::DomainRoleEntry, const std::string&, getUid);
using RoleKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::DomainRoleEntry,
                                                const dac::Role::Enum&,
                                                getRole);

using Table = bmi::multi_index_container<
        dac::DomainRoleEntry,
        bmi::indexed_by<
                bmi::ordered_unique<bmi::tag<tags::UidDomainInterfaceOperation>,
                                    bmi::composite_key<dac::DomainRoleEntry, UidKey, RoleKey>>>>;
} // namespace domain_role

} // namespace access_control
} // namespace joynr

namespace std
{

/**
 * @brief Function object that implements a hash function for
 *joynr::infrastructure::DacTypes::MasterAccessControlEntry.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template <>
struct hash<joynr::access_control::dac::MediatorAccessControlEntry> {
    std::size_t operator()(const joynr::access_control::dac::MediatorAccessControlEntry&
                                   mediatorAccessControlEntryValue) const
    {
        return mediatorAccessControlEntryValue.hashCode();
    }
};

template <>
struct hash<joynr::access_control::dac::MediatorRegistrationControlEntry> {
    std::size_t operator()(const joynr::access_control::dac::MediatorRegistrationControlEntry&
                                   mediatorRegistarationControlEntryValue) const
    {
        return mediatorRegistarationControlEntryValue.hashCode();
    }
};
} // namespace std

#endif // ACCESSCONTROLUTILS_H

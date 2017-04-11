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

#ifndef LOCALDOMAINACCESSSTORE_H
#define LOCALDOMAINACCESSSTORE_H

#include <string>
#include <tuple>
#include <set>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/optional.hpp>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/infrastructure/DacTypes/DomainRoleEntry.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

namespace access_control
{

static constexpr const char* WILDCARD = "*";

namespace dac = joynr::infrastructure::DacTypes;
namespace bmi = boost::multi_index;

namespace tags
{
struct UidDomainInterfaceOperation;
struct DomainAndInterface;
struct Domain;
} // namespace tags

template <typename Entry>
struct TableMaker
{
    using UidKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::ControlEntry, const std::string&, getUid);
    using DomainKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::ControlEntry,
                                                      const std::string&,
                                                      getDomain);
    using InterfaceKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(dac::ControlEntry,
                                                         const std::string&,
                                                         getInterfaceName);
    using OperationKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(Entry, const std::string&, getOperation);

    // this custom comparator ensures that wildcards come last
    struct WildcardComparator
    {
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

    using DefaultComparator = std::less<std::string>;

    using Type = bmi::multi_index_container<
            Entry,
            bmi::indexed_by<
                    bmi::ordered_unique<bmi::tag<tags::UidDomainInterfaceOperation>,
                                        bmi::composite_key<Entry,
                                                           UidKey,
                                                           DomainKey,
                                                           InterfaceKey,
                                                           OperationKey>,
                                        bmi::composite_key_compare<WildcardComparator,
                                                                   DefaultComparator,
                                                                   DefaultComparator,
                                                                   WildcardComparator>>,
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

class JOYNRCLUSTERCONTROLLER_EXPORT LocalDomainAccessStore
{
public:
    LocalDomainAccessStore();
    explicit LocalDomainAccessStore(std::string fileName);
    ~LocalDomainAccessStore() = default;

    /**
     * Get the domain roles for the given user.
     *
     * @param userId The user to get the domain roles for. No wildcards supported.
     * @return std::vector of Domain Role Entries that apply to the user uid.
     * The std::vector contains max two entries (since we only have two roles Master and Owner).
     * Used to get domain roles when a user logs in.
     */
    std::vector<infrastructure::DacTypes::DomainRoleEntry> getDomainRoles(
            const std::string& userId);

    /**
     * Get the domain role for the given user and role.
     *
     * @param uid The user to get the domain role for. No wildcards supported.
     * @param role The user to get the domain role for. No wildcards supported.
     * @return Domain Role Entry that apply to the user uid and role.
     * Returned domain role entry domains value is empty list, if no domain role entry for uid
     *found.
     */
    boost::optional<infrastructure::DacTypes::DomainRoleEntry> getDomainRole(
            const std::string& uid,
            infrastructure::DacTypes::Role::Enum role);

    /**
     * Updates given domain role entry. If such doesn't already exist in the store, it will be added
     *to the store.
     *
     * @param updatedEntry Entry that has to be updated.
     * @return If operation succeeded return true.
     */
    bool updateDomainRole(const infrastructure::DacTypes::DomainRoleEntry& updatedEntry);

    /**
     * Removes an entry according to the specified primary key.
     *
     * @param userId UserId whose DRE is going to be removed.
     * @param role UserId role that builds with userId primary key that identifies DRE that has to
     *be removed.
     * @return If operation succeeded return true.
     */
    bool removeDomainRole(const std::string& userId, infrastructure::DacTypes::Role::Enum role);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access
     *rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MasterAcl for that
     *user.
     *
     * @param uid The user id that owns the domains.
     * @return std::vector of master ACEs with entries owned by the user.
     * If no entry has been found for specified uid, then returns master ACE with uid "*".
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMasterAccessControlEntries(
            const std::string& uid) const;

    /**
     * Returns a std::vector of master ACEs applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Master ACL editor app.
     *
     * @param userId The user id that owns the domains.
     * @return std::vector of master ACEs with entries owned by the user.
     * In case userId has no domains with role MASTER, this function returns empty std::vector.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry>
    getEditableMasterAccessControlEntries(const std::string& userId);

    /**
     * Returns a list of master ACEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return List of master ACEs associated to given domain and interface.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMasterAccessControlEntries(
            const std::string& domain,
            const std::string& interfaceName) const;

    /**
     * Get the master access control entries for an incoming message with
     * the given uid, domain, interface.
     *
     * @param uid The userid of the incoming message
     * @param domain The domain being called
     * @param interfaceName The interface being called.
     * @return The matching Master ACEs.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMasterAccessControlEntries(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName);

    /**
     * Get the master access control entry for an incoming message with
     * the given uid, domain, interface, operation.
     *
     * @param uid The userid of the incoming message
     * @param domain The domain being called
     * @param interfaceName The interface being called.
     * @param operation The operation being called
     * @return Master ACE associated to given uid, domain, interface and operation.
     * If no master ACE found for given parameters, returned boost::optional is not initialized.
     */
    boost::optional<infrastructure::DacTypes::MasterAccessControlEntry> getMasterAccessControlEntry(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& operation);

    /**
     * Update given master access control entry.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedMasterAce The entry to add
     * @return false if update fails.
     */
    bool updateMasterAccessControlEntry(
            const infrastructure::DacTypes::MasterAccessControlEntry& updatedMasterAce);

    /**
     * Remove master access control entry uniquely identified with userId, domain, interface and
     *operation.
     *
     * @param userId
     * @param domain
     * @param interfaceName
     * @param operation
     * @return false if remove fails or master ACE that match given parameters was not found.
     */
    bool removeMasterAccessControlEntry(const std::string& userId,
                                        const std::string& domain,
                                        const std::string& interfaceName,
                                        const std::string& operation);

    /**
     * Returns a list of master ACEs from Mediator ACL that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MediatorAcl for that
     *user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master ACEs with entries owned by the uid.
     * If no entry has been found for specified uid, then returns master ACE from Mediator ACL with
     *uid "*".
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMediatorAccessControlEntries(
            const std::string& uid);

    /**
     * Returns a list of master ACEs from Mediator ACL applying to domains the user uid has role
     *MASTER,
     * i.e. the entries the user uid is allowed to edit. Used by an Mediator ACL editor app.
     *
     * @param userId The user id that owns the domains.
     * @return List of master ACEs with entries owned by the user.
     * In case userId has no domains with role MASTER, this function returns empty std::vector.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry>
    getEditableMediatorAccessControlEntries(const std::string& userId);

    /**
     * Returns a list of master ACEs from Mediator ACL that apply to the domain and interface
     *combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return std::vector of master ACEs associated to given domain and interface.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMediatorAccessControlEntries(
            const std::string& domain,
            const std::string& interfaceName);

    /**
     * Get the master ACEs from Mediator ACL for an incoming message with
     * the given uid, domain, interface.
     *
     * @param uid The userid of the incoming message
     * @param domain The domain being called
     * @param interfaceName The interface being called.
     * @return The matching mediator ACEs
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMediatorAccessControlEntries(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName);

    /**
     * Get the mediator access control entry for an incoming message with
     * the given uid, domain, interface, operation.
     *
     * @param uid The userid of the incoming message
     * @param domain The domain being called
     * @param interfaceName The interface being called.
     * @param operation The operation being called
     * @return Mediator ACE  associated to given uid, domain, interface and
     *operation.
     * If no mediator ACE found for given parameters, returned boost::optional is not initialized.
     */
    boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>
    getMediatorAccessControlEntry(const std::string& uid,
                                  const std::string& domain,
                                  const std::string& interfaceName,
                                  const std::string& operation);
    /**
     * Update given master ACE in MediatorACL.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedMediatorAce The entry to add
     * @return false if update fails.
     */
    bool updateMediatorAccessControlEntry(
            const infrastructure::DacTypes::MasterAccessControlEntry& updatedMediatorAce);

    /**
     * Remove mediator ACE from MediatorACL identified with userId, domain, interface and operation.
     *
     * @param userId
     * @param domain
     * @param interfaceName
     * @param operation
     * @return false if remove fails or master ACE that match given parameters was not found.
     */
    bool removeMediatorAccessControlEntry(const std::string& userId,
                                          const std::string& domain,
                                          const std::string& interfaceName,
                                          const std::string& operation);

    /**
     * Returns a list of owner ACEs that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that
     *user.
     *
     * @param uid The user id that owns the domains.
     * @return std::vector of owner ACEs with entries owned by the user.
     */
    std::vector<infrastructure::DacTypes::OwnerAccessControlEntry> getOwnerAccessControlEntries(
            const std::string& uid);

    /**
     * Returns a std::vector of owner ACEs from Owner ACL applying to domains the user uid has role
     *OWNER,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that
     *user.
     *
     * @param userId The user id that owns the domains.
     * @return std::vector of owner ACEs with entries owned by the user.
     * In case userId has no domains with role OWNER, this function returns std::vector of all
     *userId
     *owner ACEs.
     */
    std::vector<infrastructure::DacTypes::OwnerAccessControlEntry>
    getEditableOwnerAccessControlEntries(const std::string& userId);

    /**
     * Returns a list of owner ACEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return std::vector of owner ACEs associated to given domain and interface.
     */
    std::vector<infrastructure::DacTypes::OwnerAccessControlEntry> getOwnerAccessControlEntries(
            const std::string& domain,
            const std::string& interfaceName);

    /**
     * Get the Owner ACEs for the given user,domain and interface.
     *
     * @param userId The userid of the incoming message
     * @param domain The domain being accessed
     * @param interfaceName The interface being accessed
     * @return The matching OwnerACEs
     */
    std::vector<infrastructure::DacTypes::OwnerAccessControlEntry> getOwnerAccessControlEntries(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName);

    /**
     * Get the Owner ACE for the given user,domain and interface and operation.
     *
     * @param userId The userid of the incoming message
     * @param domain The domain being accessed
     * @param interfaceName The interface being accessed
     * @param operation The operation being called
     * @return Owner ACE associated to given uid, domain, interface and operation.
     * If no owner ACE found for given parameters, returned boost::optional is not initialized.
     */
    boost::optional<infrastructure::DacTypes::OwnerAccessControlEntry> getOwnerAccessControlEntry(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& operation);

    /**
     * Update given owner ACE.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedOwnerAce The entry to add
     * @return false if update fails.
     */
    bool updateOwnerAccessControlEntry(
            const infrastructure::DacTypes::OwnerAccessControlEntry& updatedOwnerAce);

    /**
     * Remove ownerAce ACE identified with userId, domain, interface and operation.
     *
     * @param userId
     * @param domain
     * @param interfaceName
     * @param operation
     * @return false if remove fails or ownerAce ACE that match given parameters was not found.
     */
    bool removeOwnerAccessControlEntry(const std::string& userId,
                                       const std::string& domain,
                                       const std::string& interfaceName,
                                       const std::string& operation);

    /**
     * Check if only wildcard operations exist for the given combination of
     * userId, domain and interface
     *
     * @param userId
     * @param domain
     * @param interfaceName
     */
    bool onlyWildcardOperations(const std::string& userId,
                                const std::string& domain,
                                const std::string& interfaceName);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(masterTable),
                MUESLI_NVP(mediatorTable),
                MUESLI_NVP(ownerTable),
                MUESLI_NVP(domainRoleTable));
    }

    /**
     * Return a list of unique pairs (Domain, Interface) for all control entries present in
     * the access store. The list can include duplicates.
     *
     * @return: a forward_list of unique pairs in the form:
     *  1. Domain
     *  2. InterfaceName
     */
    std::set<std::pair<std::string, std::string>> getUniqueDomainInterfaceCombinations() const;

private:
    ADD_LOGGER(LocalDomainAccessStore);
    void persistToFile() const;
    std::string persistenceFileName;

    using MasterTable =
            access_control::TableMaker<access_control::dac::MasterAccessControlEntry>::Type;
    MasterTable masterTable;

    using MediatorTable = MasterTable;
    MediatorTable mediatorTable;

    using OwnerTable =
            access_control::TableMaker<access_control::dac::OwnerAccessControlEntry>::Type;
    OwnerTable ownerTable;

    using DomainRoleTable = access_control::domain_role::Table;
    DomainRoleTable domainRoleTable;

    template <typename Table, typename Value = typename Table::value_type, typename... Args>
    std::vector<Value> getEqualRange(const Table& table, Args&&... args) const
    {
        auto range = table.equal_range(std::make_tuple(std::forward<Args>(args)...));
        std::vector<Value> result;
        std::copy(range.first, range.second, std::back_inserter(result));
        return result;
    }

    template <typename Table, typename... Args>
    bool removeFromTable(Table& table, Args&&... args)
    {
        bool success = false;
        auto it = lookup(table, std::forward<Args>(args)...);
        if (it != table.end()) {
            success = true;
            table.erase(it);
        }
        persistToFile();
        return success;
    }

    template <typename Table, typename Iterator = typename Table::const_iterator, typename... Args>
    typename Table::const_iterator lookup(const Table& table, Args&&... args) const
    {
        return table.find(std::make_tuple(std::forward<Args>(args)...));
    }

    template <typename Table, typename Value = typename Table::value_type, typename... Args>
    boost::optional<Value> lookupOptional(const Table& table, Args&&... args) const
    {
        auto it = lookup(table, std::forward<Args>(args)...);
        boost::optional<Value> result;
        if (it != table.end()) {
            result = *it;
        }
        return result;
    }

    template <typename Table, typename Entry>
    bool insertOrReplace(Table& table, const Entry& updatedEntry)
    {
        bool success = true;
        std::pair<typename Table::iterator, bool> result = table.insert(updatedEntry);
        if (!result.second) {
            // entry exists, update it
            success = table.replace(result.first, updatedEntry);
        }
        persistToFile();
        return success;
    }

    template <typename Table, typename Value = typename Table::value_type>
    boost::optional<Value> lookupOptionalWithWildcard(const Table& table,
                                                      const std::string& uid,
                                                      const std::string& domain,
                                                      const std::string& interfaceName,
                                                      const std::string& operation) const
    {
        boost::optional<Value> entry;
        entry = lookupOptional(table, uid, domain, interfaceName, operation);
        if (!entry) {
            entry = lookupOptional(table, uid, domain, interfaceName, access_control::WILDCARD);
        }
        if (!entry) {
            entry = lookupOptional(
                    table, access_control::WILDCARD, domain, interfaceName, operation);
        }
        if (!entry) {
            entry = lookupOptional(table,
                                   access_control::WILDCARD,
                                   domain,
                                   interfaceName,
                                   access_control::WILDCARD);
        }
        return entry;
    }

    template <typename Table>
    bool checkOnlyWildcardOperations(const Table& table,
                                     const std::string& userId,
                                     const std::string& domain,
                                     const std::string& interfaceName) const
    {
        auto range = table.equal_range(std::make_tuple(userId, domain, interfaceName));
        std::size_t size = std::distance(range.first, range.second);

        if (size == 0) {
            return true;
        }

        if (size > 1) {
            return false;
        }

        return range.first->getOperation() == access_control::WILDCARD;
    }

    template <typename Table, typename Value = typename Table::value_type>
    std::vector<Value> getEditableAccessControlEntries(const Table& table,
                                                       const std::string& userId,
                                                       access_control::dac::Role::Enum role) const
    {
        std::vector<Value> entries;
        auto it = domainRoleTable.find(std::make_tuple(userId, role));
        if (it != domainRoleTable.end()) {
            for (const std::string& domain : it->getDomains()) {
                auto range = table.template get<access_control::tags::Domain>().equal_range(domain);
                std::copy(range.first, range.second, std::back_inserter(entries));
            }
        }
        return entries;
    }

    template <typename Table, typename Value = typename Table::value_type, typename... Args>
    std::vector<Value> getEqualRangeWithUidWildcard(Table& table,
                                                    const std::string& uid,
                                                    Args&&... args) const
    {
        std::vector<Value> entries = getEqualRange(table, uid, std::forward<Args>(args)...);
        std::vector<Value> wildcardEntries =
                getEqualRange(table, access_control::WILDCARD, std::forward<Args>(args)...);
        entries.insert(entries.end(),
                       std::make_move_iterator(wildcardEntries.begin()),
                       std::make_move_iterator(wildcardEntries.end()));
        return entries;
    }
};
} // namespace joynr
#endif // LOCALDOMAINACCESSSTORE_H

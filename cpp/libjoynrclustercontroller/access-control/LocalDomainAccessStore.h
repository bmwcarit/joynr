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

#include <cassert>
#include <set>
#include <string>
#include <tuple>
#include <typeinfo>
#include <vector>

#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/infrastructure/DacTypes/DomainRoleEntry.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/Role.h"
#include "joynr/serializer/Serializer.h"

#include "AccessControlUtils.h"
#include "WildcardStorage.h"

namespace joynr
{
class JOYNRCLUSTERCONTROLLER_EXPORT LocalDomainAccessStore
{
public:
    LocalDomainAccessStore();
    explicit LocalDomainAccessStore(std::string fileName);
    virtual ~LocalDomainAccessStore();

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
     * found.
     */
    boost::optional<infrastructure::DacTypes::DomainRoleEntry> getDomainRole(
            const std::string& uid,
            infrastructure::DacTypes::Role::Enum role);

    /**
     * Updates given domain role entry. If such doesn't already exist in the store, it will be added
     * to the store.
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
     * be removed.
     * @return If operation succeeded return true.
     */
    bool removeDomainRole(const std::string& userId, infrastructure::DacTypes::Role::Enum role);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access
     * rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MasterAcl for that
     * user.
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
    virtual bool updateMasterAccessControlEntry(
            const infrastructure::DacTypes::MasterAccessControlEntry& updatedMasterAce);

    /**
     * Remove master access control entry uniquely identified with userId, domain, interface and
     * operation.
     *
     * @param userId
     * @param domain
     * @param interfaceName
     * @param operation
     * @return false if remove fails or master ACE that match given parameters was not found.
     */
    virtual bool removeMasterAccessControlEntry(const std::string& userId,
                                                const std::string& domain,
                                                const std::string& interfaceName,
                                                const std::string& operation);

    /**
     * Returns a list of master ACEs from Mediator ACL that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MediatorAcl for that
     * user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master ACEs with entries owned by the uid.
     * If no entry has been found for specified uid, then returns master ACE from Mediator ACL with
     * uid "*".
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMediatorAccessControlEntries(
            const std::string& uid);

    /**
     * Returns a list of master ACEs from Mediator ACL applying to domains the user uid has role
     * MASTER,
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
     * combination.
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
     * operation.
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
    virtual bool updateMediatorAccessControlEntry(
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
    virtual bool removeMediatorAccessControlEntry(const std::string& userId,
                                                  const std::string& domain,
                                                  const std::string& interfaceName,
                                                  const std::string& operation);

    /**
     * Returns a list of owner ACEs that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that
     * user.
     *
     * @param uid The user id that owns the domains.
     * @return std::vector of owner ACEs with entries owned by the user.
     */
    std::vector<infrastructure::DacTypes::OwnerAccessControlEntry> getOwnerAccessControlEntries(
            const std::string& uid);

    /**
     * Returns a std::vector of owner ACEs from Owner ACL applying to domains the user uid has role
     * OWNER,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that
     * user.
     *
     * @param userId The user id that owns the domains.
     * @return std::vector of owner ACEs with entries owned by the user.
     * In case userId has no domains with role OWNER, this function returns std::vector of all
     * userId
     * owner ACEs.
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
    virtual bool updateOwnerAccessControlEntry(
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
    virtual bool removeOwnerAccessControlEntry(const std::string& userId,
                                               const std::string& domain,
                                               const std::string& interfaceName,
                                               const std::string& operation);

    /**
     * Returns a list of master registration control entries that define the registration rights
     * of the provider uid.
     * Calling this function blocks the calling thread until update operation finishes.
     *
     * @param uid The provider userId.
     * @return A list of master RCEs for the specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getMasterRegistrationControlEntries(const std::string& uid) const;

    /**
     * Get the master registration control entry for an incoming message with
     * the given uid, domain, interface
     *
     * @param uid The userid of the incoming message
     * @param domain The domain being called
     * @param interfaceName The interface being called.
     * @return Master RCE associated to given uid, domain, interface and operation.
     * If no master RCE found for given parameters, returned boost::optional is not initialized.
     */
    boost::optional<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getMasterRegistrationControlEntry(const std::string& uid,
                                      const std::string& domain,
                                      const std::string& interfaceName);

    /**
     * Returns a list of editable master registration entries for domains for which
     * the user uid has got the role Master,
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Master.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getEditableMasterRegistrationControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     * existent.
     *
     * @param updatedMasterRce The master RCE to be updated.
     * @return true if update succeeded, false otherwise.
     */
    virtual bool updateMasterRegistrationControlEntry(
            const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMasterRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded, false otherwise.
     */
    virtual bool removeMasterRegistrationControlEntry(const std::string& uid,
                                                      const std::string& domain,
                                                      const std::string& interfaceName);

    /**
     * Returns a list of mediator registration control entries that define the registration rights
     * of the provider uid.
     * Calling this function blocks the calling thread until the update operation finishes.
     *
     * @param uid The provider userId.
     * @return A list of mediator RCEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getMediatorRegistrationControlEntries(const std::string& uid);

    /**
     * Returns a list of editable mediator registration entries for domains for which
     * the user uid has got the role Master,
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Master.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getEditableMediatorRegistrationControlEntries(const std::string& uid);

    /**
     * Get the mediator registration control entry for an incoming message with
     * the given uid, domain, interface.
     *
     * @param uid The userid of the incoming message
     * @param domain The domain being called
     * @param interfaceName The interface being called.
     * @return Mediator RCE  associated to given uid, domain, interface and
     * operation.
     * If no mediator RCE found for given parameters, returned boost::optional is not initialized.
     */
    boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>
    getMediatorAccessControlEntry(const std::string& uid,
                                  const std::string& domain,
                                  const std::string& interfaceName);

    /**
     * Get the mediator registration control entry for an incoming message with
     * the given uid, domain, interface, operation.
     *
     * @param uid The userid of the incoming message
     * @param domain The domain being called
     * @param interfaceName The interface being called.
     * @return Mediator ACE  associated to given uid, domain, interface and
     * operation.
     * If no mediator ACE found for given parameters, returned boost::optional is not initialized.
     */
    boost::optional<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getMediatorRegistrationControlEntry(const std::string& uid,
                                        const std::string& domain,
                                        const std::string& interfaceName);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     * existent.
     *
     * @param updatedMediatorRce The mediator RCE to be updated.
     * @return true if update succeeded, false otherwise.
     */
    virtual bool updateMediatorRegistrationControlEntry(
            const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMediatorRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded, false otherwise.
     */
    virtual bool removeMediatorRegistrationControlEntry(const std::string& uid,
                                                        const std::string& domain,
                                                        const std::string& interfaceName);

    /**
     * Returns a list of owner registration control entries that define the registration rights
     * of the provider uid.
     * Calling this function blocks the calling thread until the update operation finishes.
     *
     * @param uid The provider userId.
     * @return A list of owner RCEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry>
    getOwnerRegistrationControlEntries(const std::string& uid);

    /**
     * Get the Owner RCE for the given user,domain and interface and operation.
     *
     * @param userId The userid of the incoming message
     * @param domain The domain being accessed
     * @param interfaceName The interface being accessed
     * @return Owner RCE associated to given uid, domain, interface.
     * If no owner RCE found for given parameters, returned boost::optional is not initialized.
     */
    boost::optional<infrastructure::DacTypes::OwnerRegistrationControlEntry>
    getOwnerRegistrationControlEntry(const std::string& userId,
                                     const std::string& domain,
                                     const std::string& interfaceName);

    /**
     * Returns a list of editable owner registration entries for domains for which
     * the user uid has got the role Owner.
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Owner.
     */
    std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry>
    getEditableOwnerRegistrationControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     * existent.
     *
     * @param updatedOwnerRce The owner RCE to be updated.
     * @return true if update succeeded, false otherwise.
     */
    virtual bool updateOwnerRegistrationControlEntry(
            const infrastructure::DacTypes::OwnerRegistrationControlEntry& updatedOwnerRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded, false otherwise.
     */
    virtual bool removeOwnerRegistrationControlEntry(const std::string& uid,
                                                     const std::string& domain,
                                                     const std::string& interfaceName);

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
        archive(MUESLI_NVP(masterAccessTable),
                MUESLI_NVP(mediatorAccessTable),
                MUESLI_NVP(ownerAccessTable),
                MUESLI_NVP(masterRegistrationTable),
                MUESLI_NVP(mediatorRegistrationTable),
                MUESLI_NVP(ownerRegistrationTable),
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

    /**
     * Merge all tables from from the passed LocalDomainAccessStore into
     * this LocalDomainAccessStore
     *
     * @param store
     * @return: if the merge was successful
     */
    bool mergeDomainAccessStore(const LocalDomainAccessStore& other);

    // Use the logger to print content of entire access store
    void logContent();

private:
    ADD_LOGGER(LocalDomainAccessStore)
    void persistToFile() const;
    bool endsWithWildcard(const std::string& value) const;

    std::string persistenceFileName;
    mutable ReadWriteLock readWriteLock;
    mutable ReadWriteLock readWriteLockWildcard;

    using MasterAccessControlTable =
            access_control::TableMaker<access_control::dac::MasterAccessControlEntry>::Type;
    MasterAccessControlTable masterAccessTable;

    using MediatorAccessControlTable =
            access_control::TableMaker<access_control::dac::MediatorAccessControlEntry>::Type;
    MediatorAccessControlTable mediatorAccessTable;

    using OwnerAccessControlTable =
            access_control::TableMaker<access_control::dac::OwnerAccessControlEntry>::Type;
    OwnerAccessControlTable ownerAccessTable;

    using MasterRegistrationControlTable =
            access_control::TableMaker<access_control::dac::MasterRegistrationControlEntry>::Type;
    MasterRegistrationControlTable masterRegistrationTable;

    using MediatorRegistrationControlTable =
            access_control::TableMaker<access_control::dac::MediatorRegistrationControlEntry>::Type;
    MediatorRegistrationControlTable mediatorRegistrationTable;

    using OwnerRegistrationControlTable =
            access_control::TableMaker<access_control::dac::OwnerRegistrationControlEntry>::Type;
    OwnerRegistrationControlTable ownerRegistrationTable;

    using DomainRoleTable = access_control::domain_role::Table;
    DomainRoleTable domainRoleTable;

    joynr::access_control::WildcardStorage domainWildcardStorage;
    joynr::access_control::WildcardStorage interfaceWildcardStorage;

    template <typename Table, typename Value = typename Table::value_type, typename... Args>
    std::vector<Value> getEqualRange(const Table& table, Args&&... args) const
    {
        ReadLocker lock(readWriteLock);

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

        WriteLocker lock(readWriteLock);

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
        ReadLocker lock(readWriteLock);
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
    typename std::enable_if_t<
            std::is_same<Entry, joynr::access_control::dac::DomainRoleEntry>::value,
            bool>
    insertOrReplace(Table& table, const Entry& updatedEntry, bool persist = true)
    {
        WriteLocker lock(readWriteLock);

        bool success = true;
        std::pair<typename Table::iterator, bool> result = table.insert(updatedEntry);
        if (!result.second) {
            // entry exists, update it
            success = table.replace(result.first, updatedEntry);
        }

        if (persist) {
            persistToFile();
        }

        return success;
    }

    template <typename Table, typename Entry>
    typename std::enable_if_t<
            !std::is_same<Entry, joynr::access_control::dac::DomainRoleEntry>::value,
            bool>
    insertOrReplace(Table& table, const Entry& updatedEntry, bool persist = true)
    {
        WriteLocker lock(readWriteLock);

        bool success = true;
        std::pair<typename Table::iterator, bool> result = table.insert(updatedEntry);
        if (!result.second) {
            // entry exists, update it
            success = table.replace(result.first, updatedEntry);
        }

        addToWildcardStorage(updatedEntry);

        if (persist) {
            persistToFile();
        }

        return success;
    }

    template <typename Fun, typename TableType>
    void applyForTable(Fun f, TableType& table)
    {
        for (auto& entry : table) {
            f(entry);
        }
    }

    template <typename Fun>
    void applyForAllTables(Fun f)
    {
        applyForTable(f, masterAccessTable);
        applyForTable(f, mediatorAccessTable);
        applyForTable(f, ownerAccessTable);
        applyForTable(f, masterRegistrationTable);
        applyForTable(f, mediatorRegistrationTable);
        applyForTable(f, ownerRegistrationTable);
    }

    template <typename Table>
    bool mergeTable(const Table& source, Table& dest)
    {
        const bool persist = false;

        for (const auto& entry : source) {
            if (!insertOrReplace(dest, entry, persist)) {
                return false;
            }
        }
        return true;
    }

    void logControlEntry(
            const infrastructure::DacTypes::MasterAccessControlEntry& masterAccessControlEntry,
            const std::string title);

    void logControlEntry(
            const infrastructure::DacTypes::OwnerAccessControlEntry& ownerAccessControlEntry,
            const std::string title);

    void logControlEntry(const infrastructure::DacTypes::MasterRegistrationControlEntry&
                                 masterRegistrationControlEntry,
                         const std::string title);

    void logControlEntry(const infrastructure::DacTypes::OwnerRegistrationControlEntry&
                                 ownerRegistrationControlEntry,
                         const std::string title);

    void logRoleEntry(const infrastructure::DacTypes::DomainRoleEntry& domainRoleEntry);

    void logTables();

    template <typename Entry>
    void addToWildcardStorage(const Entry& updatedEntry)
    {
        WriteLocker lock(readWriteLockWildcard);

        // If entry ends with wildcard, then add it to the corresponding WildcardStorage
        if (endsWithWildcard(updatedEntry.getDomain())) {
            domainWildcardStorage.insert<access_control::wildcards::Domain>(
                    updatedEntry.getDomain(), updatedEntry);
        }
        if (endsWithWildcard(updatedEntry.getInterfaceName())) {
            interfaceWildcardStorage.insert<access_control::wildcards::Interface>(
                    updatedEntry.getInterfaceName(), updatedEntry);
        }
    }

    template <typename Value>
    access_control::WildcardStorage::Set<Value> filterOnUid(
            const access_control::WildcardStorage::Set<Value>& inputSet,
            const std::string& uid) const
    {
        // look first for the Uid
        access_control::WildcardStorage::Set<Value> tempResult;
        for (const auto& entry : inputSet) {
            auto entryUid = entry.getUid();
            if (entryUid == uid || entryUid == access_control::WILDCARD) {
                tempResult.insert(entry);
            }
        }
        return tempResult;
    }

    bool matchWildcard(const std::string& value, std::string wildcard) const
    {
        wildcard.pop_back();
        return boost::algorithm::starts_with(value, wildcard);
    }

    template <typename Value>
    access_control::WildcardStorage::Set<Value> filterForDomain(
            const access_control::WildcardStorage::OptionalSet<Value>& interfaceSet,
            const std::string& uid,
            const std::string& domain) const
    {
        assert(interfaceSet);

        // all entries in the set have the same interfaceName (as from RadixTree)
        access_control::WildcardStorage::Set<Value> tempResult = filterOnUid(*interfaceSet, uid);
        access_control::WildcardStorage::Set<Value> resultSet;

        // look then at the domain
        for (const auto& value : tempResult) {
            if (value.getDomain() == domain) {
                // exact match
                resultSet.insert(value);
            } else if (endsWithWildcard(value.getDomain()) &&
                       matchWildcard(domain, value.getDomain())) {
                // wildcard match
                resultSet.insert(value);
            }
        }
        return resultSet;
    }

    template <typename Value>
    access_control::WildcardStorage::Set<Value> filterForInterface(
            const access_control::WildcardStorage::OptionalSet<Value>& domainSet,
            const std::string& uid,
            const std::string& interface) const
    {
        assert(domainSet);

        // all entries in the set have the same domain (as from RadixTree)
        access_control::WildcardStorage::Set<Value> tempResult = filterOnUid(*domainSet, uid);
        access_control::WildcardStorage::Set<Value> resultSet;

        // look then at the domain
        for (const auto& value : tempResult) {
            if (value.getInterfaceName() == interface) {
                // exact match
                resultSet.insert(value);
            } else if (endsWithWildcard(value.getInterfaceName()) &&
                       matchWildcard(interface, value.getInterfaceName())) {
                // wildcard match
                resultSet.insert(value);
            }
        }
        return resultSet;
    }

    template <typename Value>
    boost::optional<Value> pickClosest(
            const access_control::WildcardStorage::Set<Value>& set_1,
            const access_control::WildcardStorage::Set<Value>& set_2 = {}) const
    {
        // ordered set with custom comparator
        typename access_control::TableViewResultSet<Value>::Type resultSet;
        for (auto entry : set_1) {
            resultSet.insert(entry);
        }

        for (auto entry : set_2) {
            resultSet.insert(entry);
        }

        // return top
        if (resultSet.empty()) {
            return boost::none;
        }
        return *resultSet.begin();
    }

    template <typename Value>
    boost::optional<Value> lookupDomainInterfaceWithWildcard(const std::string& uid,
                                                             const std::string& domain,
                                                             const std::string& interfaceName) const
    {
        ReadLocker lock(readWriteLockWildcard);

        using OptionalSet = access_control::WildcardStorage::OptionalSet<Value>;
        OptionalSet ifRes = interfaceWildcardStorage.getLongestMatch<Value>(interfaceName);
        OptionalSet dRes = domainWildcardStorage.getLongestMatch<Value>(domain);

        if (ifRes && dRes) {
            auto ifSetResult = filterForDomain(ifRes, uid, domain);
            auto dRetResult = filterForInterface(dRes, uid, interfaceName);
            // both the domain and interface wildcard results match the input parameters
            // selection can be done without taking into account all input parameters
            return pickClosest(ifSetResult, dRetResult);
        }

        if (ifRes) {
            auto ifSetResult = filterForDomain(ifRes, uid, domain);
            // only got a result from the interface wildcard storage
            return pickClosest(ifSetResult);
        }

        if (dRes) {
            // only got a result from the domain wildcard storage
            auto dRetResult = filterForInterface(dRes, uid, interfaceName);
            return pickClosest(dRetResult);
        }

        // no match found
        return boost::none;
    }

    template <typename Table, typename Value = typename Table::value_type>
    boost::optional<Value> lookupOptionalWithWildcard(const Table& table,
                                                      const std::string& uid,
                                                      const std::string& domain,
                                                      const std::string& interfaceName) const
    {
        // Exact match
        boost::optional<Value> entry = lookupOptional(table, uid, domain, interfaceName);

        if (!entry) {
            // try to match with wildcarded userId
            entry = lookupOptional(table, access_control::WILDCARD, domain, interfaceName);
        }

        if (!entry) {
            // try to match with wildcarded domain and/or interface and uid wildcarded
            entry = lookupDomainInterfaceWithWildcard<Value>(uid, domain, interfaceName);
        }

        return entry;
    }

    template <typename Table>
    bool checkOnlyWildcardOperations(const Table& table,
                                     const std::string& userId,
                                     const std::string& domain,
                                     const std::string& interfaceName) const
    {
        ReadLocker lock(readWriteLock);

        auto range = table.equal_range(std::make_tuple(userId, domain, interfaceName));
        std::size_t size = static_cast<std::size_t>(std::distance(range.first, range.second));

        if (size == 0) {
            return true;
        }

        if (size > 1) {
            return false;
        }

        return range.first->getOperation() == access_control::WILDCARD;
    }

    template <typename Table, typename Value = typename Table::value_type>
    std::vector<Value> getEntries(const Table& table,
                                  const std::string& userId,
                                  access_control::dac::Role::Enum role) const
    {
        ReadLocker lock(readWriteLock);

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

    std::vector<access_control::dac::MasterAccessControlEntry> convertMediator(
            const std::vector<access_control::dac::MediatorAccessControlEntry>& mediatorEntries)
    {
        std::vector<access_control::dac::MasterAccessControlEntry> result;
        for (const auto& e : mediatorEntries) {
            result.push_back(e);
        }
        return result;
    }

    boost::optional<access_control::dac::MasterAccessControlEntry> convertMediator(
            const boost::optional<access_control::dac::MediatorAccessControlEntry>& mediatorEntry)
    {
        boost::optional<access_control::dac::MasterAccessControlEntry> result;
        if (mediatorEntry) {
            result = *mediatorEntry;
        }
        return result;
    }

    std::vector<access_control::dac::MasterRegistrationControlEntry> convertMediator(
            const std::vector<access_control::dac::MediatorRegistrationControlEntry>&
                    mediatorEntries)
    {
        std::vector<access_control::dac::MasterRegistrationControlEntry> result;
        for (const auto& e : mediatorEntries) {
            result.push_back(e);
        }
        return result;
    }

    boost::optional<access_control::dac::MasterRegistrationControlEntry> convertMediator(
            const boost::optional<access_control::dac::MediatorRegistrationControlEntry>&
                    mediatorEntry)
    {
        boost::optional<access_control::dac::MasterRegistrationControlEntry> result;
        if (mediatorEntry) {
            result = *mediatorEntry;
        }
        return result;
    }
};
} // namespace joynr
#endif // LOCALDOMAINACCESSSTORE_H

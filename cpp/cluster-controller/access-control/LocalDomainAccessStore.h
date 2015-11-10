/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/infrastructure/DacTypes_QtDomainRoleEntry.h"
#include "joynr/infrastructure/DacTypes_QtMasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes_QtOwnerAccessControlEntry.h"
#include "joynr/Optional.h"
#include <QString>
#include <QtSql/QSqlDatabase>

namespace joynr
{
namespace joynr_logging
{
class Logger;
}
class JOYNRCLUSTERCONTROLLER_EXPORT LocalDomainAccessStore
{
public:
    explicit LocalDomainAccessStore(bool clearDatabaseOnStartup = false);
    ~LocalDomainAccessStore();

    /**
     * Get the domain roles for the given user.
     *
     * \param userId The user to get the domain roles for. No wildcards supported.
     * \return QList of Domain QtRole Entries that apply to the user uid.
     * The QList contains max two entries (since we only have two roles Master and Owner).
     * Used to get domain roles when a user logs in.
     */
    QList<infrastructure::DacTypes::QtDomainRoleEntry> getDomainRoles(const QString& userId);

    /**
     * Get the domain role for the given user and role.
     *
     * \param uid The user to get the domain role for. No wildcards supported.
     * \param role The user to get the domain role for. No wildcards supported.
     * \return Domain QtRole Entry that apply to the user uid and role.
     * Returned domain role entry domains value is empty list, if no domain role entry for uid
     *found.
     */
    Optional<infrastructure::DacTypes::QtDomainRoleEntry> getDomainRole(
            const QString& uid,
            infrastructure::DacTypes::QtRole::Enum role);

    /**
     * Updates given domain role entry. If such doesn't already exist in the store, it will be added
     *to the store.
     *
     * \param updatedEntry Entry that has to be updated.
     * \return If operation succeeded return true.
     */
    bool updateDomainRole(const infrastructure::DacTypes::QtDomainRoleEntry& updatedEntry);

    /**
     * Removes an entry according to the specified primary key.
     *
     * \param userId UserId whose DRE is going to be removed.
     * \param role UserId role that builds with userId primary key that identifies DRE that has to
     *be removed.
     * \return If operation succeeded return true.
     */
    bool removeDomainRole(const QString& userId, infrastructure::DacTypes::QtRole::Enum role);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access
     *rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MasterAcl for that
     *user.
     *
     * \param uid The user id that owns the domains.
     * \return QList of master ACEs with entries owned by the user.
     * If no entry has been found for specified uid, then returns master ACE with uid "*".
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry> getMasterAccessControlEntries(
            const QString& uid);

    /**
     * Returns a QList of master ACEs applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Master ACL editor app.
     *
     * \param userId The user id that owns the domains.
     * \return QList of master ACEs with entries owned by the user.
     * In case userId has no domains with role MASTER, this function returns empty QList.
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry>
    getEditableMasterAccessControlEntries(const QString& userId);

    /**
     * Returns a list of master ACEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * \param domain The domain you search ACE's for.
     * \param interfaceName The interface you search ACE's for.
     * \return List of master ACEs associated to given domain and interface.
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry> getMasterAccessControlEntries(
            const QString& domain,
            const QString& interfaceName);

    /**
     * Get the master access control entries for an incoming message with
     * the given uid, domain, interface.
     *
     * \param uid The userid of the incoming message
     * \param domain The domain being called
     * \param interfaceName The interface being called.
     * \return The matching Master ACEs.
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry> getMasterAccessControlEntries(
            const QString& uid,
            const QString& domain,
            const QString& interfaceName);

    /**
     * Get the master access control entry for an incoming message with
     * the given uid, domain, interface, operation.
     *
     * \param uid The userid of the incoming message
     * \param domain The domain being called
     * \param interfaceName The interface being called.
     * \param operation The operation being called
     * \return Master ACE Optional associated to given uid, domain, interface and operation.
     * If no master ACE found for given parameters, returned Optional is null.
     */
    Optional<infrastructure::DacTypes::QtMasterAccessControlEntry> getMasterAccessControlEntry(
            const QString& uid,
            const QString& domain,
            const QString& interfaceName,
            const QString& operation);

    /**
     * Update given master access control entry.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * \param updatedMasterAce The entry to add
     * \return false if update fails.
     */
    bool updateMasterAccessControlEntry(
            const infrastructure::DacTypes::QtMasterAccessControlEntry& updatedMasterAce);

    /**
     * Remove master access control entry uniquely identified with userId, domain, interface and
     *operation.
     *
     * \param userId
     * \param domain
     * \param interfaceName
     * \param operation
     * \return false if remove fails or master ACE that match given parameters was not found.
     */
    bool removeMasterAccessControlEntry(const QString& userId,
                                        const QString& domain,
                                        const QString& interfaceName,
                                        const QString& operation);

    /**
     * Returns a list of master ACEs from Mediator ACL that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MediatorAcl for that
     *user.
     *
     * \param uid The user id that owns the domains.
     * \return List of master ACEs with entries owned by the uid.
     * If no entry has been found for specified uid, then returns master ACE from Mediator ACL with
     *uid "*".
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry> getMediatorAccessControlEntries(
            const QString& uid);

    /**
     * Returns a list of master ACEs from Mediator ACL applying to domains the user uid has role
     *MASTER,
     * i.e. the entries the user uid is allowed to edit. Used by an Mediator ACL editor app.
     *
     * \param userId The user id that owns the domains.
     * \return List of master ACEs with entries owned by the user.
     * In case userId has no domains with role MASTER, this function returns empty QList.
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry>
    getEditableMediatorAccessControlEntries(const QString& userId);

    /**
     * Returns a list of master ACEs from Mediator ACL that apply to the domain and interface
     *combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * \param domain The domain you search ACE's for.
     * \param interfaceName The interface you search ACE's for.
     * \return QList of master ACEs associated to given domain and interface.
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry> getMediatorAccessControlEntries(
            const QString& domain,
            const QString& interfaceName);

    /**
     * Get the master ACEs from Mediator ACL for an incoming message with
     * the given uid, domain, interface.
     *
     * \param uid The userid of the incoming message
     * \param domain The domain being called
     * \param interfaceName The interface being called.
     * \return The matching mediator ACEs
     */
    QList<infrastructure::DacTypes::QtMasterAccessControlEntry> getMediatorAccessControlEntries(
            const QString& uid,
            const QString& domain,
            const QString& interfaceName);

    /**
     * Get the mediator access control entry for an incoming message with
     * the given uid, domain, interface, operation.
     *
     * \param uid The userid of the incoming message
     * \param domain The domain being called
     * \param interfaceName The interface being called.
     * \param operation The operation being called
     * \return Mediator ACE Optional associated to given uid, domain, interface and operation.
     * If no mediator ACE found for given parameters, returned Optional is null.
     */
    Optional<infrastructure::DacTypes::QtMasterAccessControlEntry> getMediatorAccessControlEntry(
            const QString& uid,
            const QString& domain,
            const QString& interfaceName,
            const QString& operation);
    /**
     * Update given master ACE in MediatorACL.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * \param updatedMediatorAce The entry to add
     * \return false if update fails.
     */
    bool updateMediatorAccessControlEntry(
            const infrastructure::DacTypes::QtMasterAccessControlEntry& updatedMediatorAce);

    /**
     * Remove mediator ACE from MediatorACL identified with userId, domain, interface and operation.
     *
     * \param userId
     * \param domain
     * \param interfaceName
     * \param operation
     * \return false if remove fails or master ACE that match given parameters was not found.
     */
    bool removeMediatorAccessControlEntry(const QString& userId,
                                          const QString& domain,
                                          const QString& interfaceName,
                                          const QString& operation);

    /**
     * Returns a list of owner ACEs that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that
     *user.
     *
     * \param uid The user id that owns the domains.
     * \return QList of owner ACEs with entries owned by the user.
     */
    QList<infrastructure::DacTypes::QtOwnerAccessControlEntry> getOwnerAccessControlEntries(
            const QString& uid);

    /**
     * Returns a QList of owner ACEs from Owner ACL applying to domains the user uid has role OWNER,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that
     *user.
     *
     * \param userId The user id that owns the domains.
     * \return QList of owner ACEs with entries owned by the user.
     * In case userId has no domains with role OWNER, this function returns QList of all userId
     *owner ACEs.
     */
    QList<infrastructure::DacTypes::QtOwnerAccessControlEntry> getEditableOwnerAccessControlEntries(
            const QString& userId);

    /**
     * Returns a list of owner ACEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * \param domain The domain you search ACE's for.
     * \param interfaceName The interface you search ACE's for.
     * \return QList of owner ACEs associated to given domain and interface.
     */
    QList<infrastructure::DacTypes::QtOwnerAccessControlEntry> getOwnerAccessControlEntries(
            const QString& domain,
            const QString& interfaceName);

    /**
     * Get the Owner ACEs for the given user,domain and interface.
     *
     * \param userId The userid of the incoming message
     * \param domain The domain being accessed
     * \param interfaceName The interface being accessed
     * \return The matching OwnerACEs
     */
    QList<infrastructure::DacTypes::QtOwnerAccessControlEntry> getOwnerAccessControlEntries(
            const QString& userId,
            const QString& domain,
            const QString& interfaceName);

    /**
     * Get the Owner ACE for the given user,domain and interface and operation.
     *
     * \param userId The userid of the incoming message
     * \param domain The domain being accessed
     * \param interfaceName The interface being accessed
     * \param operation The operation being called
     * \return Owner ACE Optional associated to given uid, domain, interface and operation.
     * If no owner ACE found for given parameters, returned Optional is null.
     */
    Optional<infrastructure::DacTypes::QtOwnerAccessControlEntry> getOwnerAccessControlEntry(
            const QString& userId,
            const QString& domain,
            const QString& interfaceName,
            const QString& operation);

    /**
     * Update given owner ACE.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * \param updatedOwnerAce The entry to add
     * \return false if update fails.
     */
    bool updateOwnerAccessControlEntry(
            const infrastructure::DacTypes::QtOwnerAccessControlEntry& updatedOwnerAce);

    /**
     * Remove ownerAce ACE identified with userId, domain, interface and operation.
     *
     * \param userId
     * \param domain
     * \param interfaceName
     * \param operation
     * \return false if remove fails or ownerAce ACE that match given parameters was not found.
     */
    bool removeOwnerAccessControlEntry(const QString& userId,
                                       const QString& domain,
                                       const QString& interfaceName,
                                       const QString& operation);

    static constexpr const char* WILDCARD = "*";

private:
    static joynr_logging::Logger* logger;

    static QSqlDatabase db;

    // Bind values to queries
    static constexpr const char* BIND_UID = ":uid";
    static constexpr const char* BIND_DOMAIN = ":domain";
    static constexpr const char* BIND_ROLE = ":role";
    static constexpr const char* BIND_INTERFACE = ":interface";
    static constexpr const char* BIND_OPERATION = ":operation";
    static constexpr const char* BIND_DEFAULT_TRUSTLEVEL = ":defaultRequiredTrustLevel";
    static constexpr const char* BIND_DEFAULT_CHANGETRUSTLEVEL =
            ":defaultRequiredControlEntryTrustLevel";
    static constexpr const char* BIND_DEFAULT_CONSUMERPERMISSION = ":defaultConsumerPermission";
    static constexpr const char* BIND_POSSIBLE_CONSUMERPERMISSIONS = ":possibleConsumerPermissions";
    static constexpr const char* BIND_POSSIBLE_TRUSTLEVELS = ":possibleTrustLevels";
    static constexpr const char* BIND_POSSIBLE_CHANGETRUSTLEVELS = ":possibleChangeTrustLevels";
    static constexpr const char* BIND_REQUIRED_TRUSTLEVEL = ":requiredTrustLevel";
    static constexpr const char* BIND_REQUIRED_CHANGETRUSTLEVEL = ":requiredAceChangeTrustLevel";
    static constexpr const char* BIND_CONSUMERPERMISSION = ":consumerPermission";

    // Queries
    static const QString SELECT_DRE;
    static const QString UPDATE_DRE;
    static const QString DELETE_DRE;
    static const QString GET_UID_MASTER_ACES;
    static const QString GET_DOMAIN_INTERFACE_MASTER_ACES;
    static const QString GET_UID_DOMAIN_INTERFACE_MASTER_ACES;
    static const QString GET_MASTER_ACE;
    static const QString UPDATE_MASTER_ACE;
    static const QString DELETE_MASTER_ACE;
    static const QString GET_EDITABLE_MASTER_ACES;
    static const QString GET_UID_MEDIATOR_ACES;
    static const QString GET_DOMAIN_INTERFACE_MEDIATOR_ACES;
    static const QString GET_UID_DOMAIN_INTERFACE_MEDIATOR_ACES;
    static const QString GET_MEDIATOR_ACE;
    static const QString UPDATE_MEDIATOR_ACE;
    static const QString DELETE_MEDIATOR_ACE;
    static const QString GET_EDITABLE_MEDIATOR_ACES;
    static const QString GET_UID_OWNER_ACES;
    static const QString GET_DOMAIN_INTERFACE_OWNER_ACES;
    static const QString GET_UID_DOMAIN_INTERFACE_OWNER_ACES;
    static const QString GET_OWNER_ACE;
    static const QString UPDATE_OWNER_ACE;
    static const QString DELETE_OWNER_ACE;
    static const QString GET_EDITABLE_OWNER_ACES;

    // Helper functions
    bool insertDomainRoleEntry(const QString& userId,
                               infrastructure::DacTypes::QtRole::Enum role,
                               const QList<QString>& domains);

    QList<infrastructure::DacTypes::QtMasterAccessControlEntry> extractMasterAces(QSqlQuery& query);

    QList<infrastructure::DacTypes::QtOwnerAccessControlEntry> extractOwnerAces(QSqlQuery& query);

    void setPossibleConsumerPermissions(infrastructure::DacTypes::QtMasterAccessControlEntry& entry,
                                        QSqlQuery& query,
                                        int field);

    void setPossibleRequiredTrustLevels(infrastructure::DacTypes::QtMasterAccessControlEntry& entry,
                                        QSqlQuery& query,
                                        int field);

    void setPossibleRequiredControlEntryChangeTrustLevels(
            infrastructure::DacTypes::QtMasterAccessControlEntry& entry,
            QSqlQuery& query,
            int field);

    template <typename T>
    T getEnumField(QSqlQuery& query, int field);

    template <typename T>
    QList<T> deserializeEnumList(const QByteArray& value);

    template <typename T>
    QByteArray serializeEnumList(const QList<T>& value);

    template <typename T>
    Optional<T> firstEntry(const QList<T>& list);

    QSqlQuery createGetAceQuery(const QString& sqlQuery,
                                const QString& uid,
                                const QString& domain,
                                const QString& interfaceName,
                                const QString& operation);

    QSqlQuery createGetAceQuery(const QString& sqlQuery, const QString& uid);

    QSqlQuery createGetAceQuery(const QString& sqlQuery,
                                const QString& domain,
                                const QString& interfaceName);

    QSqlQuery createGetAceQuery(const QString& sqlQuery,
                                const QString& uid,
                                const QString& domain,
                                const QString& interfaceName);

    QSqlQuery createUpdateMasterAceQuery(
            const QString& sqlQuery,
            const infrastructure::DacTypes::QtMasterAccessControlEntry& updatedMasterAce);
    QSqlQuery createRemoveAceQuery(const QString& sqlQuery,
                                   const QString& uid,
                                   const QString& domain,
                                   const QString& interfaceName,
                                   const QString& operation);
    QSqlQuery createGetEditableAceQuery(const QString& sqlQuery,
                                        const QString& uid,
                                        infrastructure::DacTypes::QtRole::Enum role);

    /**
     * Reset store to initial state.
     * NOTE: After this function store will have no entries!!!
     */
    void reset();
};
}
#endif // LOCALDOMAINACCESSSTORE_H

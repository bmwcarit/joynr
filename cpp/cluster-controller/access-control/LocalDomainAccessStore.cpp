/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include "LocalDomainAccessStore.h"

#include <cassert>

#include <boost/format.hpp>
#include <QVector>
#include <QVariant>
#include <QIODevice>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QDataStream>

#include "joynr/Util.h"
#include "AceValidator.h"
#include "joynr/QtTypeUtil.h"

namespace joynr
{
using namespace infrastructure::DacTypes;

INIT_LOGGER(LocalDomainAccessStore);

QSqlDatabase LocalDomainAccessStore::db = QSqlDatabase::addDatabase("QSQLITE");

//--- SQL placeholders -------------------------------------------------------------

const char* LocalDomainAccessStore::BIND_UID = ":uid";
const char* LocalDomainAccessStore::BIND_DOMAIN = ":domain";
const char* LocalDomainAccessStore::BIND_ROLE = ":role";
const char* LocalDomainAccessStore::BIND_INTERFACE = ":interface";
const char* LocalDomainAccessStore::BIND_OPERATION = ":operation";
const char* LocalDomainAccessStore::BIND_DEFAULT_TRUSTLEVEL = ":defaultRequiredTrustLevel";
const char* LocalDomainAccessStore::BIND_DEFAULT_CHANGETRUSTLEVEL =
        ":defaultRequiredControlEntryTrustLevel";
const char* LocalDomainAccessStore::BIND_DEFAULT_CONSUMERPERMISSION = ":defaultConsumerPermission";
const char* LocalDomainAccessStore::BIND_POSSIBLE_CONSUMERPERMISSIONS =
        ":possibleConsumerPermissions";
const char* LocalDomainAccessStore::BIND_POSSIBLE_TRUSTLEVELS = ":possibleTrustLevels";
const char* LocalDomainAccessStore::BIND_POSSIBLE_CHANGETRUSTLEVELS = ":possibleChangeTrustLevels";
const char* LocalDomainAccessStore::BIND_REQUIRED_TRUSTLEVEL = ":requiredTrustLevel";
const char* LocalDomainAccessStore::BIND_REQUIRED_CHANGETRUSTLEVEL = ":requiredAceChangeTrustLevel";
const char* LocalDomainAccessStore::BIND_CONSUMERPERMISSION = ":consumerPermission";

//--- SQL statements -------------------------------------------------------------

const std::string LocalDomainAccessStore::SELECT_DRE =
        (boost::format("SELECT domain from DomainRole WHERE uid = %1% AND role = %2%") % BIND_UID %
         BIND_ROLE).str();

const std::string LocalDomainAccessStore::UPDATE_DRE =
        (boost::format(
                 "INSERT OR REPLACE INTO DomainRole(uid, role, domain) VALUES(%1%, %2%, %3%)") %
         BIND_UID %
         BIND_ROLE %
         BIND_DOMAIN).str();

const std::string LocalDomainAccessStore::DELETE_DRE =
        (boost::format("DELETE FROM DomainRole WHERE uid = %1% AND role = %2%") % BIND_UID %
         BIND_ROLE).str();

const std::string LocalDomainAccessStore::GET_UID_MASTER_ACES =
        (boost::format("SELECT * FROM MasterACL WHERE uid IN (%1% , '*')") % BIND_UID).str();

const std::string LocalDomainAccessStore::GET_DOMAIN_INTERFACE_MASTER_ACES =
        (boost::format("SELECT * FROM MasterACL "
                       "WHERE domain = %1% AND interfaceName = %2%") %
         BIND_DOMAIN %
         BIND_INTERFACE).str();

const std::string LocalDomainAccessStore::GET_UID_DOMAIN_INTERFACE_MASTER_ACES =
        (boost::format("SELECT * FROM MasterACL "
                       "WHERE uid IN (%1% , '*') AND domain = %2% AND interfaceName = %3%") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE).str();

const std::string LocalDomainAccessStore::GET_MASTER_ACE =
        (boost::format("SELECT * FROM MasterACL "
                       "WHERE uid IN (%1% , '*') AND domain = %2% AND interfaceName = %3%"
                       " AND operation IN (%4% , '*') "
                       "ORDER BY CASE uid"
                       " WHEN '*' THEN 2"
                       " ELSE 1 "
                       "END, "
                       "CASE operation"
                       " WHEN '*' THEN 2"
                       " ELSE 1 "
                       "END") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION).str();

const std::string LocalDomainAccessStore::UPDATE_MASTER_ACE =
        (boost::format("INSERT OR REPLACE INTO MasterACL "
                       "(uid, domain, interfaceName, operation,"
                       " defaultRequiredTrustLevel, defaultRequiredControlEntryTrustLevel,"
                       " defaultConsumerPermission, possibleConsumerPermissions,"
                       " possibleTrustLevels, possibleChangeTrustLevels) "
                       "VALUES(%1%, %2%, %3%, %4%, %5%, %6%, %7%, %8%, %9%, %10%)") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION %
         BIND_DEFAULT_TRUSTLEVEL %
         BIND_DEFAULT_CHANGETRUSTLEVEL %
         BIND_DEFAULT_CONSUMERPERMISSION %
         BIND_POSSIBLE_CONSUMERPERMISSIONS %
         BIND_POSSIBLE_TRUSTLEVELS %
         BIND_POSSIBLE_CHANGETRUSTLEVELS).str();

const std::string LocalDomainAccessStore::DELETE_MASTER_ACE =
        (boost::format("DELETE FROM MasterACL "
                       "WHERE uid = %1% AND domain = %2% AND interfaceName = %3%"
                       " AND operation = %4%") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION).str();

const std::string LocalDomainAccessStore::GET_EDITABLE_MASTER_ACES =
        (boost::format("SELECT acl.* FROM DomainRole as role, MasterACL as acl "
                       "WHERE role.uid = %1% AND role.role = %2%"
                       " AND role.domain = acl.domain") %
         BIND_UID %
         BIND_ROLE).str();

const std::string LocalDomainAccessStore::GET_UID_MEDIATOR_ACES =
        (boost::format("SELECT * FROM MediatorACL "
                       "WHERE uid IN (%1% , '*')") %
         BIND_UID).str();

const std::string LocalDomainAccessStore::GET_DOMAIN_INTERFACE_MEDIATOR_ACES =
        (boost::format("SELECT * FROM MediatorACL "
                       "WHERE domain = %1% AND interfaceName = %2%") %
         BIND_DOMAIN %
         BIND_INTERFACE).str();

const std::string LocalDomainAccessStore::GET_UID_DOMAIN_INTERFACE_MEDIATOR_ACES =
        (boost::format("SELECT * FROM MediatorACL "
                       "WHERE uid IN (%1% , '*') AND domain = %2% AND interfaceName = %3%") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE).str();

const std::string LocalDomainAccessStore::GET_MEDIATOR_ACE =
        (boost::format("SELECT * FROM MediatorACL "
                       "WHERE uid IN (%1% , '*') AND domain = %2% AND interfaceName = %3%"
                       " AND operation IN (%4% , '*')"
                       "ORDER BY CASE uid"
                       " WHEN '*' THEN 2"
                       " ELSE 1 "
                       "END, "
                       "CASE operation"
                       " WHEN '*' THEN 2"
                       " ELSE 1 "
                       "END") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION).str();

const std::string LocalDomainAccessStore::UPDATE_MEDIATOR_ACE =
        (boost::format("INSERT OR REPLACE INTO MediatorACL "
                       "(uid, domain, interfaceName, operation,"
                       " defaultRequiredTrustLevel, defaultRequiredControlEntryTrustLevel,"
                       " defaultConsumerPermission, possibleConsumerPermissions,"
                       " possibleTrustLevels, possibleChangeTrustLevels) "
                       "VALUES(%1%, %2%, %3%, %4%, %5%, %6%, %7%, %8%, %9%, %10%)") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION %
         BIND_DEFAULT_TRUSTLEVEL %
         BIND_DEFAULT_CHANGETRUSTLEVEL %
         BIND_DEFAULT_CONSUMERPERMISSION %
         BIND_POSSIBLE_CONSUMERPERMISSIONS %
         BIND_POSSIBLE_TRUSTLEVELS %
         BIND_POSSIBLE_CHANGETRUSTLEVELS).str();

const std::string LocalDomainAccessStore::DELETE_MEDIATOR_ACE =
        (boost::format("DELETE FROM MediatorACL "
                       "WHERE uid = %1% AND domain = %2% AND interfaceName = %3%"
                       " AND operation = %4%") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION).str();

const std::string LocalDomainAccessStore::GET_EDITABLE_MEDIATOR_ACES =
        (boost::format("SELECT acl.* FROM DomainRole as role, MediatorACL as acl "
                       "WHERE role.uid = %1% AND role.role = %2%"
                       " AND role.domain = acl.domain") %
         BIND_UID %
         BIND_ROLE).str();

const std::string LocalDomainAccessStore::GET_UID_OWNER_ACES =
        (boost::format("SELECT * from OwnerACL "
                       "WHERE uid IN (%1%, '*')") %
         BIND_UID).str();

const std::string LocalDomainAccessStore::GET_DOMAIN_INTERFACE_OWNER_ACES =
        (boost::format("SELECT * from OwnerACL "
                       "WHERE domain = %1% AND interfaceName = %2%") %
         BIND_DOMAIN %
         BIND_INTERFACE).str();

const std::string LocalDomainAccessStore::GET_UID_DOMAIN_INTERFACE_OWNER_ACES =
        (boost::format("SELECT * from OwnerACL "
                       "WHERE uid IN (%1% , '*') AND domain = %2% AND interfaceName = %3%") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE).str();

const std::string LocalDomainAccessStore::GET_OWNER_ACE =
        (boost::format("SELECT * from OwnerACL "
                       "WHERE uid IN (%1% , '*') AND domain = %2% AND interfaceName = %3%"
                       " AND operation IN (%4% , '*')"
                       "ORDER BY CASE uid"
                       " WHEN '*' THEN 2"
                       " ELSE 1 "
                       "END, "
                       "CASE operation"
                       " WHEN '*' THEN 2"
                       " ELSE 1 "
                       "END") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION).str();

const std::string LocalDomainAccessStore::UPDATE_OWNER_ACE =
        (boost::format("INSERT OR REPLACE INTO OwnerACL "
                       "(uid, domain, interfaceName, operation, requiredTrustLevel,"
                       " requiredAceChangeTrustLevel, consumerPermission) "
                       "VALUES(%1%, %2%, %3%, %4%, %5%, %6%, %7%)") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION %
         BIND_REQUIRED_TRUSTLEVEL %
         BIND_REQUIRED_CHANGETRUSTLEVEL %
         BIND_CONSUMERPERMISSION).str();

const std::string LocalDomainAccessStore::DELETE_OWNER_ACE =
        (boost::format("DELETE FROM OwnerACL "
                       "WHERE uid = %1% AND domain = %2% AND interfaceName = %3%"
                       " AND operation = %4%") %
         BIND_UID %
         BIND_DOMAIN %
         BIND_INTERFACE %
         BIND_OPERATION).str();

const std::string LocalDomainAccessStore::GET_EDITABLE_OWNER_ACES =
        (boost::format("SELECT acl.* FROM DomainRole as role, OwnerACL as acl "
                       "WHERE role.uid = %1% AND role.role = %2%"
                       " AND role.domain = acl.domain") %
         BIND_UID %
         BIND_ROLE).str();

//--- Generic serialization functions --------------------------------------------

template <typename T>
std::vector<T> LocalDomainAccessStore::deserializeEnumList(const QByteArray& serializedEnumList)
{
    // Deserialize the blob into a std::vector
    QList<int> temp;
    QDataStream stream(serializedEnumList);
    stream >> temp;

    return util::convertIntListToEnumList<T>(temp.toVector().toStdVector());
}

template <typename T>
QByteArray LocalDomainAccessStore::serializeEnumList(const std::vector<T>& enumList)
{
    // Convert to an int list
    std::vector<int> ints = util::convertEnumListToIntList<T>(enumList);

    // Serialize to a bytearray
    QByteArray serializedEnumList;
    QDataStream stream(&serializedEnumList, QIODevice::WriteOnly);
    stream << QtTypeUtil::toQt(ints);

    // Return the blob for binding
    return serializedEnumList;
}

template <typename T>
T LocalDomainAccessStore::getEnumField(QSqlQuery& query, int field)
{
    return static_cast<T>(query.value(field).toInt());
}

//--------------------------------------------------------------------------------

LocalDomainAccessStore::LocalDomainAccessStore(bool clearDatabaseOnStartup)
{
    // Its a compilation problem if  SQLite is unavailable
    assert(db.isValid());

    JOYNR_LOG_DEBUG(logger, "Called LocalDomainAccessStore");

    db.setDatabaseName(":memory:");
    db.open();

    if (clearDatabaseOnStartup) {
        reset();
    }

    // Build the database schema
    // All SQL is static and should be valid
    {
        QSqlQuery createDomainRole("CREATE TABLE IF NOT EXISTS DomainRole("
                                   "    uid TEXT,"
                                   "    role INTEGER,"
                                   "    domain TEXT)",
                                   db);
        bool success = false;
        success = createDomainRole.exec();
        assert(success);

        QSqlQuery createDomainRoleIndex("CREATE INDEX IF NOT EXISTS "
                                        "DomainRoleIdx ON DomainRole (uid, role)",
                                        db);
        success = createDomainRoleIndex.exec();
        assert(success);

        QSqlQuery createMasterACL("CREATE TABLE IF NOT EXISTS MasterACL("
                                  "    uid TEXT,"
                                  "    domain TEXT,"
                                  "    interfaceName TEXT,"
                                  "    operation TEXT,"
                                  "    defaultRequiredTrustLevel INTEGER,"
                                  "    defaultRequiredControlEntryTrustLevel INTEGER,"
                                  "    defaultConsumerPermission INTEGER,"
                                  "    possibleConsumerPermissions BLOB,"
                                  "    possibleTrustLevels BLOB,"
                                  "    possibleChangeTrustLevels BLOB,"
                                  "    PRIMARY KEY(uid, domain, interfaceName, operation))",
                                  db);
        success = createMasterACL.exec();
        assert(success);

        QSqlQuery createMediatorACL("CREATE TABLE IF NOT EXISTS MediatorACL("
                                    "    uid TEXT,"
                                    "    domain TEXT,"
                                    "    interfaceName TEXT,"
                                    "    operation TEXT,"
                                    "    defaultRequiredTrustLevel INTEGER,"
                                    "    defaultRequiredControlEntryTrustLevel INTEGER,"
                                    "    defaultConsumerPermission INTEGER,"
                                    "    possibleConsumerPermissions BLOB,"
                                    "    possibleTrustLevels BLOB,"
                                    "    possibleChangeTrustLevels BLOB,"
                                    "    PRIMARY KEY(uid, domain, interfaceName, operation))",
                                    db);
        success = createMediatorACL.exec();
        assert(success);

        QSqlQuery createOwnerACL("CREATE TABLE IF NOT EXISTS OwnerACL("
                                 "       uid TEXT,"
                                 "       domain TEXT,"
                                 "       interfaceName TEXT,"
                                 "       operation TEXT,"
                                 "       requiredTrustLevel INTEGER,"
                                 "       requiredAceChangeTrustLevel INTEGER,"
                                 "       consumerPermission INTEGER,"
                                 "       PRIMARY KEY(uid, domain, interfaceName, operation))",
                                 db);
        success = createOwnerACL.exec();
        assert(success);
    }
    JOYNR_LOG_DEBUG(logger, "Connection to SQLite DB opened");
}

LocalDomainAccessStore::~LocalDomainAccessStore()
{
    if (db.isOpen()) {
        db.close();
        JOYNR_LOG_DEBUG(logger, "Connection to SQLite DB closed");
    }
}

std::vector<DomainRoleEntry> LocalDomainAccessStore::getDomainRoles(const std::string& userId)
{
    JOYNR_LOG_DEBUG(logger, "execute: entering getDomainRoleEntries with userId {}", userId);

    std::vector<DomainRoleEntry> domainRoles;
    boost::optional<DomainRoleEntry> masterDre = getDomainRole(userId, Role::MASTER);
    if (masterDre) {
        // add dre to resultset only if it defines role for some domain
        domainRoles.push_back(*masterDre);
    }

    boost::optional<DomainRoleEntry> ownerDre = getDomainRole(userId, Role::OWNER);
    if (ownerDre) {
        // add dre to resultset only if it defines role for some domain
        domainRoles.push_back(*ownerDre);
    }

    return domainRoles;
}

boost::optional<DomainRoleEntry> LocalDomainAccessStore::getDomainRole(const std::string& uid,
                                                                       Role::Enum role)
{
    // Execute a query to get the domain role entry
    QSqlQuery query;
    bool success = false;
    success = query.prepare(QtTypeUtil::toQt(SELECT_DRE));
    assert(success);
    query.bindValue(BIND_UID, QtTypeUtil::toQt(uid));
    query.bindValue(BIND_ROLE, QtTypeUtil::toQt(role));
    success = query.exec();
    assert(success);

    int domainField = query.record().indexOf("domain");

    // Extract the results
    std::vector<std::string> domains;
    while (query.next()) {
        domains.push_back(query.value(domainField).toString().toStdString());
    }

    boost::optional<DomainRoleEntry> entry;
    if (domains.empty()) {
        return entry;
    }

    entry = DomainRoleEntry(uid, domains, role);
    return entry;
}

bool LocalDomainAccessStore::updateDomainRole(const DomainRoleEntry& updatedEntry)
{
    JOYNR_LOG_DEBUG(
            logger, "execute: entering updateDomainRoleEntry with uId {}", updatedEntry.getUid());

    bool updateSuccess = insertDomainRoleEntry(
            updatedEntry.getUid(), updatedEntry.getRole(), updatedEntry.getDomains());
    return updateSuccess;
}

bool LocalDomainAccessStore::removeDomainRole(const std::string& userId, Role::Enum role)
{
    JOYNR_LOG_DEBUG(logger, "execute: entering removeDomainRoleEntry with uId {}", userId);

    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(DELETE_DRE));
    query.bindValue(BIND_UID, QtTypeUtil::toQt(userId));
    query.bindValue(BIND_ROLE, role);

    bool removeSuccess = query.exec();
    return removeSuccess;
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const std::string& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_MASTER_ACES, uid);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getEditableMasterAccessControlEntries(
        const std::string& userId)
{
    JOYNR_LOG_DEBUG(
            logger, "execute: entering getEditableMasterAccessControlEntry with uId {}", userId);

    // Get all the Master ACEs for the domains where the user is master
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_MASTER_ACES, userId, Role::MASTER);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const std::string& domain,
        const std::string& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_MASTER_ACES, domain, interfaceName);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_MASTER_ACES, uid, domain, interfaceName);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractMasterAces(query);
}

boost::optional<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    QSqlQuery query = createGetAceQuery(GET_MASTER_ACE, uid, domain, interfaceName, operation);
    bool success = false;
    success = query.exec();
    assert(success);

    std::vector<MasterAccessControlEntry> masterAceList = extractMasterAces(query);
    return firstEntry(masterAceList);
}

bool LocalDomainAccessStore::updateMasterAccessControlEntry(
        const MasterAccessControlEntry& updatedMasterAce)
{
    JOYNR_LOG_DEBUG(
            logger, "execute: entering updateMasterAce with uId {}", updatedMasterAce.getUid());

    // Add/update a master ACE
    QSqlQuery query = createUpdateMasterAceQuery(UPDATE_MASTER_ACE, updatedMasterAce);
    return query.exec();
}

bool LocalDomainAccessStore::removeMasterAccessControlEntry(const std::string& userId,
                                                            const std::string& domain,
                                                            const std::string& interfaceName,
                                                            const std::string& operation)
{
    QSqlQuery query =
            createRemoveAceQuery(DELETE_MASTER_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_MEDIATOR_ACES, uid);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::
        getEditableMediatorAccessControlEntries(const std::string& userId)
{
    JOYNR_LOG_DEBUG(logger, "execute: entering getEditableMediatorAces with uId {}", userId);

    // Get all the Mediator ACEs for the domains where the user is master
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_MEDIATOR_ACES, userId, Role::MASTER);
    bool success = false;
    success = query.exec();
    assert(success);
    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& domain,
        const std::string& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_MEDIATOR_ACES, domain, interfaceName);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_MEDIATOR_ACES, uid, domain, interfaceName);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractMasterAces(query);
}

boost::optional<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    QSqlQuery query = createGetAceQuery(GET_MEDIATOR_ACE, uid, domain, interfaceName, operation);
    bool success = false;
    success = query.exec();
    assert(success);

    std::vector<MasterAccessControlEntry> mediatorAceList = extractMasterAces(query);
    return firstEntry(mediatorAceList);
}

bool LocalDomainAccessStore::updateMediatorAccessControlEntry(
        const MasterAccessControlEntry& updatedMediatorAce)
{
    JOYNR_LOG_DEBUG(
            logger, "execute: entering updateMediatorAce with uId {}", updatedMediatorAce.getUid());

    bool updateSuccess = false;

    boost::optional<MasterAccessControlEntry> masterAceOptional =
            getMasterAccessControlEntry(updatedMediatorAce.getUid(),
                                        updatedMediatorAce.getDomain(),
                                        updatedMediatorAce.getInterfaceName(),
                                        updatedMediatorAce.getOperation());
    AceValidator aceValidator(masterAceOptional,
                              boost::optional<MasterAccessControlEntry>(updatedMediatorAce),
                              boost::optional<OwnerAccessControlEntry>());

    if (aceValidator.isMediatorValid()) {
        // Add/update a mediator ACE
        QSqlQuery query = createUpdateMasterAceQuery(UPDATE_MEDIATOR_ACE, updatedMediatorAce);
        updateSuccess = query.exec();
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeMediatorAccessControlEntry(const std::string& userId,
                                                              const std::string& domain,
                                                              const std::string& interfaceName,
                                                              const std::string& operation)
{
    JOYNR_LOG_DEBUG(logger,
                    "execute: entering removeMediatorAce with userId: {}, domain: {}, "
                    "interfaceName: {}, operation: {}",
                    userId,
                    domain,
                    interfaceName,
                    operation);

    QSqlQuery query =
            createRemoveAceQuery(DELETE_MEDIATOR_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const std::string& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_OWNER_ACES, uid);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractOwnerAces(query);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getEditableOwnerAccessControlEntries(
        const std::string& userId)
{
    JOYNR_LOG_DEBUG(logger, "execute: entering getEditableOwnerAces with uId {}", userId);

    // Get all the Owner ACEs for the domains owned by the user
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_OWNER_ACES, userId, Role::OWNER);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractOwnerAces(query);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const std::string& domain,
        const std::string& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_OWNER_ACES, domain, interfaceName);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractOwnerAces(query);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_OWNER_ACES, userId, domain, interfaceName);
    bool success = false;
    success = query.exec();
    assert(success);

    return extractOwnerAces(query);
}

boost::optional<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntry(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    QSqlQuery query = createGetAceQuery(GET_OWNER_ACE, userId, domain, interfaceName, operation);
    bool success = false;
    success = query.exec();
    assert(success);

    std::vector<OwnerAccessControlEntry> ownerAceList = extractOwnerAces(query);
    return firstEntry(ownerAceList);
}

bool LocalDomainAccessStore::updateOwnerAccessControlEntry(
        const OwnerAccessControlEntry& updatedOwnerAce)
{
    JOYNR_LOG_DEBUG(
            logger, "execute: entering updateOwnerAce with uId {}", updatedOwnerAce.getUid());

    bool updateSuccess = false;

    boost::optional<MasterAccessControlEntry> masterAceOptional =
            getMasterAccessControlEntry(updatedOwnerAce.getUid(),
                                        updatedOwnerAce.getDomain(),
                                        updatedOwnerAce.getInterfaceName(),
                                        updatedOwnerAce.getOperation());
    boost::optional<MasterAccessControlEntry> mediatorAceOptional =
            getMediatorAccessControlEntry(updatedOwnerAce.getUid(),
                                          updatedOwnerAce.getDomain(),
                                          updatedOwnerAce.getInterfaceName(),
                                          updatedOwnerAce.getOperation());
    AceValidator aceValidator(masterAceOptional,
                              mediatorAceOptional,
                              boost::optional<OwnerAccessControlEntry>(updatedOwnerAce));

    if (aceValidator.isOwnerValid()) {
        QSqlQuery query;
        query.prepare(QtTypeUtil::toQt(UPDATE_OWNER_ACE));

        query.bindValue(BIND_UID, QtTypeUtil::toQt(updatedOwnerAce.getUid()));
        query.bindValue(BIND_DOMAIN, QtTypeUtil::toQt(updatedOwnerAce.getDomain()));
        query.bindValue(BIND_INTERFACE, QtTypeUtil::toQt(updatedOwnerAce.getInterfaceName()));
        query.bindValue(BIND_OPERATION, QtTypeUtil::toQt(updatedOwnerAce.getOperation()));

        query.bindValue(BIND_REQUIRED_TRUSTLEVEL, updatedOwnerAce.getRequiredTrustLevel());
        query.bindValue(
                BIND_REQUIRED_CHANGETRUSTLEVEL, updatedOwnerAce.getRequiredAceChangeTrustLevel());
        query.bindValue(BIND_CONSUMERPERMISSION, updatedOwnerAce.getConsumerPermission());
        updateSuccess = query.exec();
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeOwnerAccessControlEntry(const std::string& userId,
                                                           const std::string& domain,
                                                           const std::string& interfaceName,
                                                           const std::string& operation)
{
    JOYNR_LOG_DEBUG(logger,
                    "execute: entering removeOwnerAce with userId: {}, domain: {}, interface: {}, "
                    "operation: {}",
                    userId,
                    domain,
                    interfaceName,
                    operation);

    QSqlQuery query =
            createRemoveAceQuery(DELETE_OWNER_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

bool LocalDomainAccessStore::onlyWildcardOperations(const std::string& userId,
                                                    const std::string& domain,
                                                    const std::string& interfaceName)
{
    return checkOnlyWildcardOperations(
                   getMasterAccessControlEntries(userId, domain, interfaceName)) &&
           checkOnlyWildcardOperations(
                   getMediatorAccessControlEntries(userId, domain, interfaceName)) &&
           checkOnlyWildcardOperations(getOwnerAccessControlEntries(userId, domain, interfaceName));
}

template <typename T>
bool LocalDomainAccessStore::checkOnlyWildcardOperations(const std::vector<T>& aceEntries)
{
    if (aceEntries.empty()) {
        return true;
    }

    if (aceEntries.size() > 1) {
        return false;
    }

    return aceEntries.begin()->getOperation() == LocalDomainAccessStore::WILDCARD;
}

void LocalDomainAccessStore::reset()
{
    JOYNR_LOG_DEBUG(logger, "execute: entering reset store");

    QSqlQuery dropDomainRole("DROP TABLE IF EXISTS DomainRole", db);
    QSqlQuery dropMasterAcl("DROP TABLE IF EXISTS MasterACL", db);
    QSqlQuery dropMediatorAcl("DROP TABLE IF EXISTS MediatorACL", db);
    QSqlQuery dropOwnerAcl("DROP TABLE IF EXISTS OwnerACL", db);
    QSqlQuery vacuum("VACUUM", db);
    bool success = false;
    success = dropDomainRole.exec();
    assert(success);

    success = dropMasterAcl.exec();
    assert(success);

    success = dropMediatorAcl.exec();
    assert(success);

    success = dropOwnerAcl.exec();
    assert(success);

    success = vacuum.exec();
    assert(success);
}

bool LocalDomainAccessStore::insertDomainRoleEntry(const std::string& userId,
                                                   Role::Enum role,
                                                   const std::vector<std::string>& domains)
{

    // Loop through the domains
    auto it = domains.begin();
    while (it != domains.end()) {

        // Insert a record for each domain
        QSqlQuery query;
        std::string domain = *it;

        query.prepare(QtTypeUtil::toQt(UPDATE_DRE));
        query.bindValue(BIND_UID, QtTypeUtil::toQt(userId));
        query.bindValue(BIND_ROLE, role);
        query.bindValue(BIND_DOMAIN, QtTypeUtil::toQt(domain));

        if (!query.exec()) {
            JOYNR_LOG_ERROR(logger, "Could not add domain entry {} {} {}", userId, role, domain);
            return false;
        }
        ++it;
    }

    // Assume success
    return true;
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::extractMasterAces(QSqlQuery& query)
{
    std::vector<MasterAccessControlEntry> masterAces;

    int uidField = query.record().indexOf("uid");
    int domainField = query.record().indexOf("domain");
    int interfaceNameField = query.record().indexOf("interfaceName");
    int defaultRTLField = query.record().indexOf("defaultRequiredTrustLevel");
    int defaultRCETLField = query.record().indexOf("defaultRequiredControlEntryTrustLevel");
    int operationField = query.record().indexOf("operation");
    int defaultConsumerPermissionField = query.record().indexOf("defaultConsumerPermission");
    int possibleConsumerPermissionsField = query.record().indexOf("possibleConsumerPermissions");
    int possibleTrustLevelsField = query.record().indexOf("possibleTrustLevels");
    int possibleChangeTrustLevelsField = query.record().indexOf("possibleChangeTrustLevels");

    while (query.next()) {

        MasterAccessControlEntry entry;

        // Set the scalar fields
        entry.setUid(query.value(uidField).toString().toStdString());
        entry.setDomain(query.value(domainField).toString().toStdString());
        entry.setInterfaceName(query.value(interfaceNameField).toString().toStdString());
        entry.setDefaultRequiredTrustLevel(getEnumField<TrustLevel::Enum>(query, defaultRTLField));
        entry.setDefaultRequiredControlEntryChangeTrustLevel(
                getEnumField<TrustLevel::Enum>(query, defaultRCETLField));
        entry.setOperation(query.value(operationField).toString().toStdString());
        entry.setDefaultConsumerPermission(
                getEnumField<Permission::Enum>(query, defaultConsumerPermissionField));

        // push_back the list fields
        setPossibleConsumerPermissions(entry, query, possibleConsumerPermissionsField);
        setPossibleRequiredTrustLevels(entry, query, possibleTrustLevelsField);
        setPossibleRequiredControlEntryChangeTrustLevels(
                entry, query, possibleChangeTrustLevelsField);

        // push_back the result
        masterAces.push_back(entry);
    }

    return masterAces;
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::extractOwnerAces(QSqlQuery& query)
{
    std::vector<OwnerAccessControlEntry> ownerAces;

    int uidField = query.record().indexOf("uid");
    int domainField = query.record().indexOf("domain");
    int interfaceNameField = query.record().indexOf("interfaceName");
    int operationField = query.record().indexOf("operation");
    int requiredTrustLevelField = query.record().indexOf("requiredTrustLevel");
    int requiredACEChangeTLField = query.record().indexOf("requiredAceChangeTrustLevel");
    int consumerPermissionField = query.record().indexOf("consumerPermission");

    while (query.next()) {

        OwnerAccessControlEntry entry;

        entry.setUid(query.value(uidField).toString().toStdString());
        entry.setDomain(query.value(domainField).toString().toStdString());
        entry.setInterfaceName(query.value(interfaceNameField).toString().toStdString());
        entry.setOperation(query.value(operationField).toString().toStdString());

        entry.setRequiredTrustLevel(getEnumField<TrustLevel::Enum>(query, requiredTrustLevelField));
        entry.setRequiredAceChangeTrustLevel(
                getEnumField<TrustLevel::Enum>(query, requiredACEChangeTLField));
        entry.setConsumerPermission(getEnumField<Permission::Enum>(query, consumerPermissionField));

        // push_back the result
        ownerAces.push_back(entry);
    }

    return ownerAces;
}

void LocalDomainAccessStore::setPossibleConsumerPermissions(MasterAccessControlEntry& entry,
                                                            QSqlQuery& query,
                                                            int field)
{
    // Create a list of permissions
    QByteArray value = query.value(field).toByteArray();
    std::vector<Permission::Enum> permissions = deserializeEnumList<Permission::Enum>(value);

    // Set the result
    entry.setPossibleConsumerPermissions(permissions);
}

void LocalDomainAccessStore::setPossibleRequiredTrustLevels(MasterAccessControlEntry& entry,
                                                            QSqlQuery& query,
                                                            int field)
{
    QByteArray value = query.value(field).toByteArray();
    std::vector<TrustLevel::Enum> trustLevels = deserializeEnumList<TrustLevel::Enum>(value);
    entry.setPossibleRequiredTrustLevels(trustLevels);
}

void LocalDomainAccessStore::setPossibleRequiredControlEntryChangeTrustLevels(
        MasterAccessControlEntry& entry,
        QSqlQuery& query,
        int field)
{
    QByteArray value = query.value(field).toByteArray();
    std::vector<TrustLevel::Enum> trustLevels = deserializeEnumList<TrustLevel::Enum>(value);
    entry.setPossibleRequiredControlEntryChangeTrustLevels(trustLevels);
}

QSqlQuery LocalDomainAccessStore::createUpdateMasterAceQuery(
        const std::string& sqlQuery,
        const MasterAccessControlEntry& updatedMasterAce)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(sqlQuery));

    // Add scalar fields
    query.bindValue(BIND_UID, QtTypeUtil::toQt(updatedMasterAce.getUid()));
    query.bindValue(BIND_DOMAIN, QtTypeUtil::toQt(updatedMasterAce.getDomain()));
    query.bindValue(BIND_INTERFACE, QtTypeUtil::toQt(updatedMasterAce.getInterfaceName()));
    query.bindValue(BIND_OPERATION, QtTypeUtil::toQt(updatedMasterAce.getOperation()));
    query.bindValue(BIND_DEFAULT_TRUSTLEVEL, updatedMasterAce.getDefaultRequiredTrustLevel());
    query.bindValue(BIND_DEFAULT_CHANGETRUSTLEVEL,
                    updatedMasterAce.getDefaultRequiredControlEntryChangeTrustLevel());
    query.bindValue(
            BIND_DEFAULT_CONSUMERPERMISSION, updatedMasterAce.getDefaultConsumerPermission());

    // Serialize list fields
    QByteArray consumerPermissions =
            serializeEnumList(updatedMasterAce.getPossibleConsumerPermissions());
    QByteArray trustLevels = serializeEnumList(updatedMasterAce.getPossibleRequiredTrustLevels());
    QByteArray changeTrustLevels =
            serializeEnumList(updatedMasterAce.getPossibleRequiredControlEntryChangeTrustLevels());

    query.bindValue(BIND_POSSIBLE_CONSUMERPERMISSIONS, consumerPermissions);
    query.bindValue(BIND_POSSIBLE_TRUSTLEVELS, trustLevels);
    query.bindValue(BIND_POSSIBLE_CHANGETRUSTLEVELS, changeTrustLevels);

    return query;
}

QSqlQuery LocalDomainAccessStore::createRemoveAceQuery(const std::string& sqlQuery,
                                                       const std::string& uid,
                                                       const std::string& domain,
                                                       const std::string& interfaceName,
                                                       const std::string& operation)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(sqlQuery));
    query.bindValue(BIND_UID, QtTypeUtil::toQt(uid));
    query.bindValue(BIND_DOMAIN, QtTypeUtil::toQt(domain));
    query.bindValue(BIND_INTERFACE, QtTypeUtil::toQt(interfaceName));
    query.bindValue(BIND_OPERATION, QtTypeUtil::toQt(operation));

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetEditableAceQuery(const std::string& sqlQuery,
                                                            const std::string& uid,
                                                            Role::Enum role)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(sqlQuery));
    query.bindValue(BIND_UID, QtTypeUtil::toQt(uid));
    query.bindValue(BIND_ROLE, role);

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const std::string& sqlQuery,
                                                    const std::string& uid,
                                                    const std::string& domain,
                                                    const std::string& interfaceName,
                                                    const std::string& operation)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(sqlQuery));
    query.bindValue(BIND_UID, QtTypeUtil::toQt(uid));
    query.bindValue(BIND_DOMAIN, QtTypeUtil::toQt(domain));
    query.bindValue(BIND_INTERFACE, QtTypeUtil::toQt(interfaceName));
    query.bindValue(BIND_OPERATION, QtTypeUtil::toQt(operation));

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const std::string& sqlQuery,
                                                    const std::string& uid)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(sqlQuery));
    query.bindValue(BIND_UID, QtTypeUtil::toQt(uid));

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const std::string& sqlQuery,
                                                    const std::string& domain,
                                                    const std::string& interfaceName)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(sqlQuery));
    query.bindValue(BIND_DOMAIN, QtTypeUtil::toQt(domain));
    query.bindValue(BIND_INTERFACE, QtTypeUtil::toQt(interfaceName));

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const std::string& sqlQuery,
                                                    const std::string& uid,
                                                    const std::string& domain,
                                                    const std::string& interfaceName)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QtTypeUtil::toQt(sqlQuery));
    query.bindValue(BIND_UID, QtTypeUtil::toQt(uid));
    query.bindValue(BIND_DOMAIN, QtTypeUtil::toQt(domain));
    query.bindValue(BIND_INTERFACE, QtTypeUtil::toQt(interfaceName));

    return query;
}

template <typename T>
boost::optional<T> LocalDomainAccessStore::firstEntry(const std::vector<T>& list)
{
    boost::optional<T> entry;
    if (!list.empty()) {
        entry = *list.begin();
    }

    return entry;
}

} // namespace joynr

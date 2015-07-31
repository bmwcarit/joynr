/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/joynrlogging.h"
#include "joynr/Util.h"
#include "AceValidator.h"

#include <QIODevice>
#include <QtSql/QSqlQuery>
#include <QSqlRecord>

#include <cassert>

namespace joynr
{
using namespace infrastructure;
using namespace joynr_logging;

QSqlDatabase LocalDomainAccessStore::db = QSqlDatabase::addDatabase("QSQLITE");

Logger* LocalDomainAccessStore::logger =
        Logging::getInstance()->getLogger("MSG", "LocalDomainAccessStore");

//--- SQL statements -------------------------------------------------------------

const QString LocalDomainAccessStore::SELECT_DRE =
        QString("SELECT domain from DomainRole WHERE uid = %1 AND role = %2").arg(BIND_UID).arg(
                BIND_ROLE);

const QString LocalDomainAccessStore::UPDATE_DRE =
        QString("INSERT OR REPLACE INTO DomainRole(uid, role, domain) VALUES(%1, %2, %3)")
                .arg(BIND_UID)
                .arg(BIND_ROLE)
                .arg(BIND_DOMAIN);

const QString LocalDomainAccessStore::DELETE_DRE =
        QString("DELETE FROM DomainRole WHERE uid = %1 AND role = %2").arg(BIND_UID).arg(BIND_ROLE);

const QString LocalDomainAccessStore::GET_UID_MASTER_ACES =
        QString("SELECT * FROM MasterACL WHERE uid IN (%1 , '*')").arg(BIND_UID);

const QString LocalDomainAccessStore::GET_DOMAIN_INTERFACE_MASTER_ACES =
        QString("SELECT * FROM MasterACL "
                "WHERE domain = %1 AND interfaceName = %2")
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE);

const QString LocalDomainAccessStore::GET_UID_DOMAIN_INTERFACE_MASTER_ACES =
        QString("SELECT * FROM MasterACL "
                "WHERE uid IN (%1 , '*') AND domain = %2 AND interfaceName = %3")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE);

const QString LocalDomainAccessStore::GET_MASTER_ACE =
        QString("SELECT * FROM MasterACL "
                "WHERE uid IN (%1 , '*') AND domain = %2 AND interfaceName = %3"
                " AND operation IN (%4 , '*') "
                "ORDER BY CASE uid"
                " WHEN '*' THEN 2"
                " ELSE 1 "
                "END, "
                "CASE operation"
                " WHEN '*' THEN 2"
                " ELSE 1 "
                "END")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION);

const QString LocalDomainAccessStore::UPDATE_MASTER_ACE =
        QString("INSERT OR REPLACE INTO MasterACL "
                "(uid, domain, interfaceName, operation,"
                " defaultRequiredTrustLevel, defaultRequiredControlEntryTrustLevel,"
                " defaultConsumerPermission, possibleConsumerPermissions,"
                " possibleTrustLevels, possibleChangeTrustLevels) "
                "VALUES(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10)")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION)
                .arg(BIND_DEFAULT_TRUSTLEVEL)
                .arg(BIND_DEFAULT_CHANGETRUSTLEVEL)
                .arg(BIND_DEFAULT_CONSUMERPERMISSION)
                .arg(BIND_POSSIBLE_CONSUMERPERMISSIONS)
                .arg(BIND_POSSIBLE_TRUSTLEVELS)
                .arg(BIND_POSSIBLE_CHANGETRUSTLEVELS);

const QString LocalDomainAccessStore::DELETE_MASTER_ACE =
        QString("DELETE FROM MasterACL "
                "WHERE uid = %1 AND domain = %2 AND interfaceName = %3"
                " AND operation = %4")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION);

const QString LocalDomainAccessStore::GET_EDITABLE_MASTER_ACES =
        QString("SELECT acl.* FROM DomainRole as role, MasterACL as acl "
                "WHERE role.uid = %1 AND role.role = %2"
                " AND role.domain = acl.domain")
                .arg(BIND_UID)
                .arg(BIND_ROLE);

const QString LocalDomainAccessStore::GET_UID_MEDIATOR_ACES =
        QString("SELECT * FROM MediatorACL "
                "WHERE uid IN (%1 , '*')").arg(BIND_UID);

const QString LocalDomainAccessStore::GET_DOMAIN_INTERFACE_MEDIATOR_ACES =
        QString("SELECT * FROM MediatorACL "
                "WHERE domain = %1 AND interfaceName = %2")
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE);

const QString LocalDomainAccessStore::GET_UID_DOMAIN_INTERFACE_MEDIATOR_ACES =
        QString("SELECT * FROM MediatorACL "
                "WHERE uid IN (%1 , '*') AND domain = %2 AND interfaceName = %3")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE);

const QString LocalDomainAccessStore::GET_MEDIATOR_ACE =
        QString("SELECT * FROM MediatorACL "
                "WHERE uid IN (%1 , '*') AND domain = %2 AND interfaceName = %3"
                " AND operation IN (%4 , '*')"
                "ORDER BY CASE uid"
                " WHEN '*' THEN 2"
                " ELSE 1 "
                "END, "
                "CASE operation"
                " WHEN '*' THEN 2"
                " ELSE 1 "
                "END")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION);

const QString LocalDomainAccessStore::UPDATE_MEDIATOR_ACE =
        QString("INSERT OR REPLACE INTO MediatorACL "
                "(uid, domain, interfaceName, operation,"
                " defaultRequiredTrustLevel, defaultRequiredControlEntryTrustLevel,"
                " defaultConsumerPermission, possibleConsumerPermissions,"
                " possibleTrustLevels, possibleChangeTrustLevels) "
                "VALUES(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10)")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION)
                .arg(BIND_DEFAULT_TRUSTLEVEL)
                .arg(BIND_DEFAULT_CHANGETRUSTLEVEL)
                .arg(BIND_DEFAULT_CONSUMERPERMISSION)
                .arg(BIND_POSSIBLE_CONSUMERPERMISSIONS)
                .arg(BIND_POSSIBLE_TRUSTLEVELS)
                .arg(BIND_POSSIBLE_CHANGETRUSTLEVELS);

const QString LocalDomainAccessStore::DELETE_MEDIATOR_ACE =
        QString("DELETE FROM MediatorACL "
                "WHERE uid = %1 AND domain = %2 AND interfaceName = %3"
                " AND operation = %4")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION);

const QString LocalDomainAccessStore::GET_EDITABLE_MEDIATOR_ACES =
        QString("SELECT acl.* FROM DomainRole as role, MediatorACL as acl "
                "WHERE role.uid = %1 AND role.role = %2"
                " AND role.domain = acl.domain")
                .arg(BIND_UID)
                .arg(BIND_ROLE);

const QString LocalDomainAccessStore::GET_UID_OWNER_ACES =
        QString("SELECT * from OwnerACL "
                "WHERE uid IN (%1, '*')").arg(BIND_UID);

const QString LocalDomainAccessStore::GET_DOMAIN_INTERFACE_OWNER_ACES =
        QString("SELECT * from OwnerACL "
                "WHERE domain = %1 AND interfaceName = %2")
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE);

const QString LocalDomainAccessStore::GET_UID_DOMAIN_INTERFACE_OWNER_ACES =
        QString("SELECT * from OwnerACL "
                "WHERE uid IN (%1 , '*') AND domain = %2 AND interfaceName = %3")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE);

const QString LocalDomainAccessStore::GET_OWNER_ACE =
        QString("SELECT * from OwnerACL "
                "WHERE uid IN (%1 , '*') AND domain = %2 AND interfaceName = %3"
                " AND operation IN (%4 , '*')"
                "ORDER BY CASE uid"
                " WHEN '*' THEN 2"
                " ELSE 1 "
                "END, "
                "CASE operation"
                " WHEN '*' THEN 2"
                " ELSE 1 "
                "END")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION);

const QString LocalDomainAccessStore::UPDATE_OWNER_ACE =
        QString("INSERT OR REPLACE INTO OwnerACL "
                "(uid, domain, interfaceName, operation, requiredTrustLevel,"
                " requiredAceChangeTrustLevel, consumerPermission) "
                "VALUES(%1, %2, %3, %4, %5, %6, %7)")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION)
                .arg(BIND_REQUIRED_TRUSTLEVEL)
                .arg(BIND_REQUIRED_CHANGETRUSTLEVEL)
                .arg(BIND_CONSUMERPERMISSION);

const QString LocalDomainAccessStore::DELETE_OWNER_ACE =
        QString("DELETE FROM OwnerACL "
                "WHERE uid = %1 AND domain = %2 AND interfaceName = %3"
                " AND operation = %4")
                .arg(BIND_UID)
                .arg(BIND_DOMAIN)
                .arg(BIND_INTERFACE)
                .arg(BIND_OPERATION);

const QString LocalDomainAccessStore::GET_EDITABLE_OWNER_ACES =
        QString("SELECT acl.* FROM DomainRole as role, OwnerACL as acl "
                "WHERE role.uid = %1 AND role.role = %2"
                " AND role.domain = acl.domain")
                .arg(BIND_UID)
                .arg(BIND_ROLE);

//--- Generic serialization functions --------------------------------------------

template <typename T>
QList<T> LocalDomainAccessStore::deserializeEnumList(const QByteArray& serializedEnumList)
{
    // Deserialize the blob into a QList
    QList<int> enumAsIntList;
    QDataStream stream(serializedEnumList);
    stream >> enumAsIntList;

    return Util::convertIntListToEnumList<T>(enumAsIntList);
}

template <typename T>
QByteArray LocalDomainAccessStore::serializeEnumList(const QList<T>& enumList)
{
    // Convert to an int list
    QList<int> ints = Util::convertEnumListToIntList<T>(enumList);

    // Serialize to a bytearray
    QByteArray serializedEnumList;
    QDataStream stream(&serializedEnumList, QIODevice::WriteOnly);
    stream << ints;

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
    // Its a compilation problem if Qt SQLite is unavailable
    assert(db.isValid());

    LOG_DEBUG(logger, "Called LocalDomainAccessStore");

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
        assert(createDomainRole.exec());

        QSqlQuery createDomainRoleIndex("CREATE INDEX IF NOT EXISTS "
                                        "DomainRoleIdx ON DomainRole (uid, role)",
                                        db);
        assert(createDomainRoleIndex.exec());

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
        assert(createMasterACL.exec());

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
        assert(createMediatorACL.exec());

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
        assert(createOwnerACL.exec());
    }
    LOG_DEBUG(logger, QString("Connection to SQLite DB opened"));
}

LocalDomainAccessStore::~LocalDomainAccessStore()
{
    if (db.isOpen()) {
        db.close();
        LOG_DEBUG(logger, QString("Connection to SQLite DB closed"));
    }
}

QList<QtDomainRoleEntry> LocalDomainAccessStore::getDomainRoles(const QString& userId)
{
    LOG_DEBUG(logger, QString("execute: entering getDomainRoleEntries with userId %1").arg(userId));

    QList<QtDomainRoleEntry> domainRoles;
    Optional<QtDomainRoleEntry> masterDre = getDomainRole(userId, QtRole::MASTER);
    if (masterDre) {
        // add dre to resultset only if it defines role for some domain
        domainRoles.append(masterDre.getValue());
    }

    Optional<QtDomainRoleEntry> ownerDre = getDomainRole(userId, QtRole::OWNER);
    if (ownerDre) {
        // add dre to resultset only if it defines role for some domain
        domainRoles.append(ownerDre.getValue());
    }

    return domainRoles;
}

Optional<QtDomainRoleEntry> joynr::LocalDomainAccessStore::getDomainRole(const QString& uid,
                                                                         QtRole::Enum role)
{
    // Execute a query to get the domain role entry
    QSqlQuery query;
    assert(query.prepare(SELECT_DRE));
    query.bindValue(BIND_UID, uid);
    query.bindValue(BIND_ROLE, role);
    assert(query.exec());

    int domainField = query.record().indexOf("domain");

    // Extract the results
    QList<QString> domains;
    while (query.next()) {
        domains.append(query.value(domainField).toString());
    }

    if (domains.isEmpty()) {
        return Optional<QtDomainRoleEntry>::createNull();
    }

    return QtDomainRoleEntry(uid, domains, role);
}

bool LocalDomainAccessStore::updateDomainRole(const QtDomainRoleEntry& updatedEntry)
{
    LOG_DEBUG(logger,
              QString("execute: entering updateDomainRoleEntry with uId %1")
                      .arg(updatedEntry.getUid()));

    bool updateSuccess = insertDomainRoleEntry(
            updatedEntry.getUid(), updatedEntry.getRole(), updatedEntry.getDomains());
    return updateSuccess;
}

bool LocalDomainAccessStore::removeDomainRole(const QString& userId, QtRole::Enum role)
{
    LOG_DEBUG(logger, QString("execute: entering removeDomainRoleEntry with uId %1").arg(userId));

    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(DELETE_DRE);
    query.bindValue(BIND_UID, userId);
    query.bindValue(BIND_ROLE, role);

    bool removeSuccess = query.exec();
    return removeSuccess;
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const QString& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_MASTER_ACES, uid);
    assert(query.exec());

    return extractMasterAces(query);
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getEditableMasterAccessControlEntries(
        const QString& userId)
{
    LOG_DEBUG(logger,
              QString("execute: entering getEditableMasterAccessControlEntry with uId %1")
                      .arg(userId));

    // Get all the Master ACEs for the domains where the user is master
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_MASTER_ACES, userId, QtRole::MASTER);
    assert(query.exec());

    return extractMasterAces(query);
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_MASTER_ACES, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_MASTER_ACES, uid, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

Optional<QtMasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntry(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName,
        const QString& operation)
{
    QSqlQuery query = createGetAceQuery(GET_MASTER_ACE, uid, domain, interfaceName, operation);
    assert(query.exec());

    QList<QtMasterAccessControlEntry> masterAceList = extractMasterAces(query);
    return firstEntry(masterAceList);
}

bool LocalDomainAccessStore::updateMasterAccessControlEntry(
        const QtMasterAccessControlEntry& updatedMasterAce)
{
    LOG_DEBUG(logger,
              QString("execute: entering updateMasterAce with uId %1")
                      .arg(updatedMasterAce.getUid()));

    // Add/update a master ACE
    QSqlQuery query = createUpdateMasterAceQuery(UPDATE_MASTER_ACE, updatedMasterAce);
    return query.exec();
}

bool LocalDomainAccessStore::removeMasterAccessControlEntry(const QString& userId,
                                                            const QString& domain,
                                                            const QString& interfaceName,
                                                            const QString& operation)
{
    QSqlQuery query =
            createRemoveAceQuery(DELETE_MASTER_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const QString& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_MEDIATOR_ACES, uid);
    assert(query.exec());

    return extractMasterAces(query);
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getEditableMediatorAccessControlEntries(
        const QString& userId)
{
    LOG_DEBUG(logger, QString("execute: entering getEditableMediatorAces with uId %1").arg(userId));

    // Get all the Mediator ACEs for the domains where the user is master
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_MEDIATOR_ACES, userId, QtRole::MASTER);
    assert(query.exec());

    return extractMasterAces(query);
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_MEDIATOR_ACES, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_MEDIATOR_ACES, uid, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

Optional<QtMasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntry(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName,
        const QString& operation)
{
    QSqlQuery query = createGetAceQuery(GET_MEDIATOR_ACE, uid, domain, interfaceName, operation);
    assert(query.exec());

    QList<QtMasterAccessControlEntry> mediatorAceList = extractMasterAces(query);
    return firstEntry(mediatorAceList);
}

bool LocalDomainAccessStore::updateMediatorAccessControlEntry(
        const QtMasterAccessControlEntry& updatedMediatorAce)
{
    LOG_DEBUG(logger,
              QString("execute: entering updateMediatorAce with uId %1")
                      .arg(updatedMediatorAce.getUid()));

    bool updateSuccess = false;

    Optional<QtMasterAccessControlEntry> masterAceOptional =
            getMasterAccessControlEntry(updatedMediatorAce.getUid(),
                                        updatedMediatorAce.getDomain(),
                                        updatedMediatorAce.getInterfaceName(),
                                        updatedMediatorAce.getOperation());
    AceValidator aceValidator(masterAceOptional,
                              Optional<QtMasterAccessControlEntry>(updatedMediatorAce),
                              Optional<QtOwnerAccessControlEntry>::createNull());

    if (aceValidator.isMediatorValid()) {
        // Add/update a mediator ACE
        QSqlQuery query = createUpdateMasterAceQuery(UPDATE_MEDIATOR_ACE, updatedMediatorAce);
        updateSuccess = query.exec();
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeMediatorAccessControlEntry(const QString& userId,
                                                              const QString& domain,
                                                              const QString& interfaceName,
                                                              const QString& operation)
{
    LOG_DEBUG(logger,
              QString("execute: entering removeMediatorAce with userId: %1, domain: %2, "
                      "interface: %3, operation: %4")
                      .arg(userId)
                      .arg(domain)
                      .arg(interfaceName)
                      .arg(operation));

    QSqlQuery query =
            createRemoveAceQuery(DELETE_MEDIATOR_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

QList<QtOwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const QString& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_OWNER_ACES, uid);
    assert(query.exec());

    return extractOwnerAces(query);
}

QList<QtOwnerAccessControlEntry> LocalDomainAccessStore::getEditableOwnerAccessControlEntries(
        const QString& userId)
{
    LOG_DEBUG(logger, QString("execute: entering getEditableOwnerAces with uId %1").arg(userId));

    // Get all the Owner ACEs for the domains owned by the user
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_OWNER_ACES, userId, QtRole::OWNER);
    assert(query.exec());

    return extractOwnerAces(query);
}

QList<QtOwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_OWNER_ACES, domain, interfaceName);
    assert(query.exec());

    return extractOwnerAces(query);
}

QList<QtOwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const QString& userId,
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_OWNER_ACES, userId, domain, interfaceName);
    assert(query.exec());

    return extractOwnerAces(query);
}

Optional<QtOwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntry(
        const QString& userId,
        const QString& domain,
        const QString& interfaceName,
        const QString& operation)
{
    QSqlQuery query = createGetAceQuery(GET_OWNER_ACE, userId, domain, interfaceName, operation);
    assert(query.exec());

    QList<QtOwnerAccessControlEntry> ownerAceList = extractOwnerAces(query);
    return firstEntry(ownerAceList);
}

bool LocalDomainAccessStore::updateOwnerAccessControlEntry(
        const QtOwnerAccessControlEntry& updatedOwnerAce)
{
    LOG_DEBUG(
            logger,
            QString("execute: entering updateOwnerAce with uId %1").arg(updatedOwnerAce.getUid()));

    bool updateSuccess = false;

    Optional<QtMasterAccessControlEntry> masterAceOptional =
            getMasterAccessControlEntry(updatedOwnerAce.getUid(),
                                        updatedOwnerAce.getDomain(),
                                        updatedOwnerAce.getInterfaceName(),
                                        updatedOwnerAce.getOperation());
    Optional<QtMasterAccessControlEntry> mediatorAceOptional =
            getMediatorAccessControlEntry(updatedOwnerAce.getUid(),
                                          updatedOwnerAce.getDomain(),
                                          updatedOwnerAce.getInterfaceName(),
                                          updatedOwnerAce.getOperation());
    AceValidator aceValidator(masterAceOptional,
                              mediatorAceOptional,
                              Optional<QtOwnerAccessControlEntry>(updatedOwnerAce));

    if (aceValidator.isOwnerValid()) {
        QSqlQuery query;
        query.prepare(UPDATE_OWNER_ACE);

        query.bindValue(BIND_UID, updatedOwnerAce.getUid());
        query.bindValue(BIND_DOMAIN, updatedOwnerAce.getDomain());
        query.bindValue(BIND_INTERFACE, updatedOwnerAce.getInterfaceName());
        query.bindValue(BIND_OPERATION, updatedOwnerAce.getOperation());

        query.bindValue(BIND_REQUIRED_TRUSTLEVEL, updatedOwnerAce.getRequiredTrustLevel());
        query.bindValue(
                BIND_REQUIRED_CHANGETRUSTLEVEL, updatedOwnerAce.getRequiredAceChangeTrustLevel());
        query.bindValue(BIND_CONSUMERPERMISSION, updatedOwnerAce.getConsumerPermission());
        updateSuccess = query.exec();
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeOwnerAccessControlEntry(const QString& userId,
                                                           const QString& domain,
                                                           const QString& interfaceName,
                                                           const QString& operation)
{
    LOG_DEBUG(logger,
              QString("execute: entering removeOwnerAce with userId: %1, domain: %2, interface: "
                      "%3, operation: %4")
                      .arg(userId)
                      .arg(domain)
                      .arg(interfaceName)
                      .arg(operation));

    QSqlQuery query =
            createRemoveAceQuery(DELETE_OWNER_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

void LocalDomainAccessStore::reset()
{
    LOG_DEBUG(logger, QString("execute: entering reset store"));

    QSqlQuery dropDomainRole("DROP TABLE IF EXISTS DomainRole", db);
    QSqlQuery dropMasterAcl("DROP TABLE IF EXISTS MasterACL", db);
    QSqlQuery dropMediatorAcl("DROP TABLE IF EXISTS MediatorACL", db);
    QSqlQuery dropOwnerAcl("DROP TABLE IF EXISTS OwnerACL", db);
    QSqlQuery vacuum("VACUUM", db);

    assert(dropDomainRole.exec());
    assert(dropMasterAcl.exec());
    assert(dropMediatorAcl.exec());
    assert(dropOwnerAcl.exec());
    assert(vacuum.exec());
}

bool LocalDomainAccessStore::insertDomainRoleEntry(const QString& userId,
                                                   QtRole::Enum role,
                                                   const QList<QString>& domains)
{

    // Loop through the domains
    QListIterator<QString> it(domains);
    while (it.hasNext()) {

        // Insert a record for each domain
        QSqlQuery query;
        QString domain = it.next();

        query.prepare(UPDATE_DRE);
        query.bindValue(BIND_UID, userId);
        query.bindValue(BIND_ROLE, role);
        query.bindValue(BIND_DOMAIN, domain);

        if (!query.exec()) {
            LOG_ERROR(logger,
                      QString("Could not add domain entry %1 %2 %3").arg(userId).arg(role).arg(
                              domain));
            return false;
        }
    }

    // Assume success
    return true;
}

QList<QtMasterAccessControlEntry> LocalDomainAccessStore::extractMasterAces(QSqlQuery& query)
{
    QList<QtMasterAccessControlEntry> masterAces;

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

        QtMasterAccessControlEntry entry;

        // Set the scalar fields
        entry.setUid(query.value(uidField).toString());
        entry.setDomain(query.value(domainField).toString());
        entry.setInterfaceName(query.value(interfaceNameField).toString());
        entry.setDefaultRequiredTrustLevel(
                getEnumField<QtTrustLevel::Enum>(query, defaultRTLField));
        entry.setDefaultRequiredControlEntryChangeTrustLevel(
                getEnumField<QtTrustLevel::Enum>(query, defaultRCETLField));
        entry.setOperation(query.value(operationField).toString());
        entry.setDefaultConsumerPermission(
                getEnumField<QtPermission::Enum>(query, defaultConsumerPermissionField));

        // Append the list fields
        setPossibleConsumerPermissions(entry, query, possibleConsumerPermissionsField);
        setPossibleRequiredTrustLevels(entry, query, possibleTrustLevelsField);
        setPossibleRequiredControlEntryChangeTrustLevels(
                entry, query, possibleChangeTrustLevelsField);

        // Append the result
        masterAces.append(entry);
    }

    return masterAces;
}

QList<QtOwnerAccessControlEntry> LocalDomainAccessStore::extractOwnerAces(QSqlQuery& query)
{
    QList<QtOwnerAccessControlEntry> ownerAces;

    int uidField = query.record().indexOf("uid");
    int domainField = query.record().indexOf("domain");
    int interfaceNameField = query.record().indexOf("interfaceName");
    int operationField = query.record().indexOf("operation");
    int requiredTrustLevelField = query.record().indexOf("requiredTrustLevel");
    int requiredACEChangeTLField = query.record().indexOf("requiredAceChangeTrustLevel");
    int consumerPermissionField = query.record().indexOf("consumerPermission");

    while (query.next()) {

        QtOwnerAccessControlEntry entry;

        entry.setUid(query.value(uidField).toString());
        entry.setDomain(query.value(domainField).toString());
        entry.setInterfaceName(query.value(interfaceNameField).toString());
        entry.setOperation(query.value(operationField).toString());

        entry.setRequiredTrustLevel(
                getEnumField<QtTrustLevel::Enum>(query, requiredTrustLevelField));
        entry.setRequiredAceChangeTrustLevel(
                getEnumField<QtTrustLevel::Enum>(query, requiredACEChangeTLField));
        entry.setConsumerPermission(
                getEnumField<QtPermission::Enum>(query, consumerPermissionField));

        // Append the result
        ownerAces.append(entry);
    }

    return ownerAces;
}

void LocalDomainAccessStore::setPossibleConsumerPermissions(QtMasterAccessControlEntry& entry,
                                                            QSqlQuery& query,
                                                            int field)
{
    // Create a list of permissions
    QByteArray value = query.value(field).toByteArray();
    QList<QtPermission::Enum> permissions = deserializeEnumList<QtPermission::Enum>(value);

    // Set the result
    entry.setPossibleConsumerPermissions(permissions);
}

void LocalDomainAccessStore::setPossibleRequiredTrustLevels(QtMasterAccessControlEntry& entry,
                                                            QSqlQuery& query,
                                                            int field)
{
    QByteArray value = query.value(field).toByteArray();
    QList<QtTrustLevel::Enum> trustLevels = deserializeEnumList<QtTrustLevel::Enum>(value);
    entry.setPossibleRequiredTrustLevels(trustLevels);
}

void LocalDomainAccessStore::setPossibleRequiredControlEntryChangeTrustLevels(
        QtMasterAccessControlEntry& entry,
        QSqlQuery& query,
        int field)
{
    QByteArray value = query.value(field).toByteArray();
    QList<QtTrustLevel::Enum> trustLevels = deserializeEnumList<QtTrustLevel::Enum>(value);
    entry.setPossibleRequiredControlEntryChangeTrustLevels(trustLevels);
}

QSqlQuery LocalDomainAccessStore::createUpdateMasterAceQuery(
        const QString& sqlQuery,
        const QtMasterAccessControlEntry& updatedMasterAce)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);

    // Add scalar fields
    query.bindValue(BIND_UID, updatedMasterAce.getUid());
    query.bindValue(BIND_DOMAIN, updatedMasterAce.getDomain());
    query.bindValue(BIND_INTERFACE, updatedMasterAce.getInterfaceName());
    query.bindValue(BIND_OPERATION, updatedMasterAce.getOperation());
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

QSqlQuery LocalDomainAccessStore::createRemoveAceQuery(const QString& sqlQuery,
                                                       const QString& uid,
                                                       const QString& domain,
                                                       const QString& interfaceName,
                                                       const QString& operation)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);
    query.bindValue(BIND_UID, uid);
    query.bindValue(BIND_DOMAIN, domain);
    query.bindValue(BIND_INTERFACE, interfaceName);
    query.bindValue(BIND_OPERATION, operation);

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetEditableAceQuery(const QString& sqlQuery,
                                                            const QString& uid,
                                                            QtRole::Enum role)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);
    query.bindValue(BIND_UID, uid);
    query.bindValue(BIND_ROLE, role);

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const QString& sqlQuery,
                                                    const QString& uid,
                                                    const QString& domain,
                                                    const QString& interfaceName,
                                                    const QString& operation)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);
    query.bindValue(BIND_UID, uid);
    query.bindValue(BIND_DOMAIN, domain);
    query.bindValue(BIND_INTERFACE, interfaceName);
    query.bindValue(BIND_OPERATION, operation);

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const QString& sqlQuery, const QString& uid)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);
    query.bindValue(BIND_UID, uid);

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const QString& sqlQuery,
                                                    const QString& domain,
                                                    const QString& interfaceName)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);
    query.bindValue(BIND_DOMAIN, domain);
    query.bindValue(BIND_INTERFACE, interfaceName);

    return query;
}

QSqlQuery LocalDomainAccessStore::createGetAceQuery(const QString& sqlQuery,
                                                    const QString& uid,
                                                    const QString& domain,
                                                    const QString& interfaceName)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);
    query.bindValue(BIND_UID, uid);
    query.bindValue(BIND_DOMAIN, domain);
    query.bindValue(BIND_INTERFACE, interfaceName);

    return query;
}

template <typename T>
Optional<T> LocalDomainAccessStore::firstEntry(const QList<T>& list)
{
    if (list.isEmpty()) {
        return Optional<T>::createNull();
    }

    return list.first();
}

} // namespace joynr

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
#include <QtSql/QSqlRecord>
#include <QDataStream>
#include "joynr/TypeUtil.h"
#include <cassert>
#include <QVector>

namespace joynr
{
using namespace infrastructure::DacTypes;
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
std::vector<T> LocalDomainAccessStore::deserializeEnumList(const QByteArray& serializedEnumList)
{
    // Deserialize the blob into a std::vector
    QList<int> temp;
    QDataStream stream(serializedEnumList);
    stream >> temp;

    return Util::convertIntListToEnumList<T>(temp.toVector().toStdVector());
}

template <typename T>
QByteArray LocalDomainAccessStore::serializeEnumList(const std::vector<T>& enumList)
{
    // Convert to an int list
    std::vector<int> ints = Util::convertEnumListToIntList<T>(enumList);

    // Serialize to a bytearray
    QByteArray serializedEnumList;
    QDataStream stream(&serializedEnumList, QIODevice::WriteOnly);
    stream << TypeUtil::toQt(ints);

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
    LOG_DEBUG(logger, "Connection to SQLite DB opened");
}

LocalDomainAccessStore::~LocalDomainAccessStore()
{
    if (db.isOpen()) {
        db.close();
        LOG_DEBUG(logger, "Connection to SQLite DB closed");
    }
}

std::vector<DomainRoleEntry> LocalDomainAccessStore::getDomainRoles(const QString& userId)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering getDomainRoleEntries with userId %1")
                      .arg(userId.toStdString())
                      .str());

    std::vector<DomainRoleEntry> domainRoles;
    Optional<DomainRoleEntry> masterDre = getDomainRole(userId, Role::MASTER);
    if (masterDre) {
        // add dre to resultset only if it defines role for some domain
        domainRoles.push_back(masterDre.getValue());
    }

    Optional<DomainRoleEntry> ownerDre = getDomainRole(userId, Role::OWNER);
    if (ownerDre) {
        // add dre to resultset only if it defines role for some domain
        domainRoles.push_back(ownerDre.getValue());
    }

    return domainRoles;
}

Optional<DomainRoleEntry> joynr::LocalDomainAccessStore::getDomainRole(const QString& uid,
                                                                       Role::Enum role)
{
    // Execute a query to get the domain role entry
    QSqlQuery query;
    assert(query.prepare(SELECT_DRE));
    query.bindValue(BIND_UID, uid);
    query.bindValue(BIND_ROLE, role);
    assert(query.exec());

    int domainField = query.record().indexOf("domain");

    // Extract the results
    std::vector<std::string> domains;
    while (query.next()) {
        domains.push_back(query.value(domainField).toString().toStdString());
    }

    if (domains.empty()) {
        return Optional<DomainRoleEntry>::createNull();
    }

    return DomainRoleEntry(uid.toStdString(), domains, role);
}

bool LocalDomainAccessStore::updateDomainRole(const DomainRoleEntry& updatedEntry)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering updateDomainRoleEntry with uId %1")
                      .arg(updatedEntry.getUid().toStdString())
                      .str());

    bool updateSuccess = insertDomainRoleEntry(QString::fromStdString(updatedEntry.getUid()),
                                               updatedEntry.getRole(),
                                               updatedEntry.getDomains());
    return updateSuccess;
}

bool LocalDomainAccessStore::removeDomainRole(const QString& userId, Role::Enum role)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering removeDomainRoleEntry with uId %1")
                      .arg(userId.toStdString())
                      .str());

    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(DELETE_DRE);
    query.bindValue(BIND_UID, userId);
    query.bindValue(BIND_ROLE, role);

    bool removeSuccess = query.exec();
    return removeSuccess;
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const QString& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_MASTER_ACES, uid);
    assert(query.exec());

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getEditableMasterAccessControlEntries(
        const QString& userId)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering getEditableMasterAccessControlEntry with uId %1")
                      .arg(userId.toStdString())
                      .str());

    // Get all the Master ACEs for the domains where the user is master
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_MASTER_ACES, userId, Role::MASTER);
    assert(query.exec());

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_MASTER_ACES, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_MASTER_ACES, uid, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

Optional<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntry(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName,
        const QString& operation)
{
    QSqlQuery query = createGetAceQuery(GET_MASTER_ACE, uid, domain, interfaceName, operation);
    assert(query.exec());

    std::vector<MasterAccessControlEntry> masterAceList = extractMasterAces(query);
    return firstEntry(masterAceList);
}

bool LocalDomainAccessStore::updateMasterAccessControlEntry(
        const MasterAccessControlEntry& updatedMasterAce)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering updateMasterAce with uId %1")
                      .arg(updatedMasterAce.getUid().toStdString())
                      .str());

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

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const QString& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_MEDIATOR_ACES, uid);
    assert(query.exec());

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::
        getEditableMediatorAccessControlEntries(const QString& userId)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering getEditableMediatorAces with uId %1")
                      .arg(userId.toStdString())
                      .str());

    // Get all the Mediator ACEs for the domains where the user is master
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_MEDIATOR_ACES, userId, Role::MASTER);
    assert(query.exec());

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_MEDIATOR_ACES, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_MEDIATOR_ACES, uid, domain, interfaceName);
    assert(query.exec());

    return extractMasterAces(query);
}

Optional<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntry(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName,
        const QString& operation)
{
    QSqlQuery query = createGetAceQuery(GET_MEDIATOR_ACE, uid, domain, interfaceName, operation);
    assert(query.exec());

    std::vector<MasterAccessControlEntry> mediatorAceList = extractMasterAces(query);
    return firstEntry(mediatorAceList);
}

bool LocalDomainAccessStore::updateMediatorAccessControlEntry(
        const MasterAccessControlEntry& updatedMediatorAce)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering updateMediatorAce with uId %1")
                      .arg(updatedMediatorAce.getUid().toStdString())
                      .str());

    bool updateSuccess = false;

    Optional<MasterAccessControlEntry> masterAceOptional = getMasterAccessControlEntry(
            QString::fromStdString(updatedMediatorAce.getUid()),
            QString::fromStdString(updatedMediatorAce.getDomain()),
            QString::fromStdString(updatedMediatorAce.getInterfaceName()),
            QString::fromStdString(updatedMediatorAce.getOperation()));
    AceValidator aceValidator(masterAceOptional,
                              Optional<MasterAccessControlEntry>(updatedMediatorAce),
                              Optional<OwnerAccessControlEntry>::createNull());

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
              FormatString("execute: entering removeMediatorAce with userId: %1, domain: %2, "
                           "interface: %3, operation: %4")
                      .arg(userId.toStdString())
                      .arg(domain.toStdString())
                      .arg(interfaceName.toStdString())
                      .arg(operation.toStdString())
                      .str());

    QSqlQuery query =
            createRemoveAceQuery(DELETE_MEDIATOR_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const QString& uid)
{
    QSqlQuery query = createGetAceQuery(GET_UID_OWNER_ACES, uid);
    assert(query.exec());

    return extractOwnerAces(query);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getEditableOwnerAccessControlEntries(
        const QString& userId)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering getEditableOwnerAces with uId %1")
                      .arg(userId.toStdString())
                      .str());

    // Get all the Owner ACEs for the domains owned by the user
    QSqlQuery query = createGetEditableAceQuery(GET_EDITABLE_OWNER_ACES, userId, Role::OWNER);
    assert(query.exec());

    return extractOwnerAces(query);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query = createGetAceQuery(GET_DOMAIN_INTERFACE_OWNER_ACES, domain, interfaceName);
    assert(query.exec());

    return extractOwnerAces(query);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const QString& userId,
        const QString& domain,
        const QString& interfaceName)
{
    QSqlQuery query =
            createGetAceQuery(GET_UID_DOMAIN_INTERFACE_OWNER_ACES, userId, domain, interfaceName);
    assert(query.exec());

    return extractOwnerAces(query);
}

Optional<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntry(
        const QString& userId,
        const QString& domain,
        const QString& interfaceName,
        const QString& operation)
{
    QSqlQuery query = createGetAceQuery(GET_OWNER_ACE, userId, domain, interfaceName, operation);
    assert(query.exec());

    std::vector<OwnerAccessControlEntry> ownerAceList = extractOwnerAces(query);
    return firstEntry(ownerAceList);
}

bool LocalDomainAccessStore::updateOwnerAccessControlEntry(
        const OwnerAccessControlEntry& updatedOwnerAce)
{
    LOG_DEBUG(logger,
              FormatString("execute: entering updateOwnerAce with uId %1")
                      .arg(updatedOwnerAce.getUid().toStdString())
                      .str());

    bool updateSuccess = false;

    Optional<MasterAccessControlEntry> masterAceOptional =
            getMasterAccessControlEntry(QString::fromStdString(updatedOwnerAce.getUid()),
                                        QString::fromStdString(updatedOwnerAce.getDomain()),
                                        QString::fromStdString(updatedOwnerAce.getInterfaceName()),
                                        QString::fromStdString(updatedOwnerAce.getOperation()));
    Optional<MasterAccessControlEntry> mediatorAceOptional = getMediatorAccessControlEntry(
            QString::fromStdString(updatedOwnerAce.getUid()),
            QString::fromStdString(updatedOwnerAce.getDomain()),
            QString::fromStdString(updatedOwnerAce.getInterfaceName()),
            QString::fromStdString(updatedOwnerAce.getOperation()));
    AceValidator aceValidator(masterAceOptional,
                              mediatorAceOptional,
                              Optional<OwnerAccessControlEntry>(updatedOwnerAce));

    if (aceValidator.isOwnerValid()) {
        QSqlQuery query;
        query.prepare(UPDATE_OWNER_ACE);

        query.bindValue(BIND_UID, QString::fromStdString(updatedOwnerAce.getUid()));
        query.bindValue(BIND_DOMAIN, QString::fromStdString(updatedOwnerAce.getDomain()));
        query.bindValue(BIND_INTERFACE, QString::fromStdString(updatedOwnerAce.getInterfaceName()));
        query.bindValue(BIND_OPERATION, QString::fromStdString(updatedOwnerAce.getOperation()));

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
    LOG_DEBUG(
            logger,
            FormatString("execute: entering removeOwnerAce with userId: %1, domain: %2, interface: "
                         "%3, operation: %4")
                    .arg(userId.toStdString())
                    .arg(domain.toStdString())
                    .arg(interfaceName.toStdString())
                    .arg(operation.toStdString())
                    .str());

    QSqlQuery query =
            createRemoveAceQuery(DELETE_OWNER_ACE, userId, domain, interfaceName, operation);
    return query.exec();
}

void LocalDomainAccessStore::reset()
{
    LOG_DEBUG(logger, "execute: entering reset store");

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
                                                   Role::Enum role,
                                                   const std::vector<std::string>& domains)
{

    // Loop through the domains
    auto it = domains.begin();
    while (it != domains.end()) {

        // Insert a record for each domain
        QSqlQuery query;
        QString domain = TypeUtil::toQt(*it);

        query.prepare(UPDATE_DRE);
        query.bindValue(BIND_UID, userId);
        query.bindValue(BIND_ROLE, role);
        query.bindValue(BIND_DOMAIN, domain);

        if (!query.exec()) {
            LOG_ERROR(logger,
                      FormatString("Could not add domain entry %1 %2 %3")
                              .arg(userId.toStdString())
                              .arg(role)
                              .arg(domain.toStdString())
                              .str());
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
        const QString& sqlQuery,
        const MasterAccessControlEntry& updatedMasterAce)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sqlQuery);

    // Add scalar fields
    query.bindValue(BIND_UID, QString::fromStdString(updatedMasterAce.getUid()));
    query.bindValue(BIND_DOMAIN, QString::fromStdString(updatedMasterAce.getDomain()));
    query.bindValue(BIND_INTERFACE, QString::fromStdString(updatedMasterAce.getInterfaceName()));
    query.bindValue(BIND_OPERATION, QString::fromStdString(updatedMasterAce.getOperation()));
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
                                                            Role::Enum role)
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
Optional<T> LocalDomainAccessStore::firstEntry(const std::vector<T>& list)
{
    if (list.empty()) {
        return Optional<T>::createNull();
    }

    return *list.begin();
}

} // namespace joynr

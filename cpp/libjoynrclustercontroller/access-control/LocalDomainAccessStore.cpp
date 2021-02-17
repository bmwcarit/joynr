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

#include "LocalDomainAccessStore.h"

#include <algorithm>

#include "joynr/Util.h"
#include "joynr/infrastructure/DacTypes/OwnerRegistrationControlEntry.h"

#include "Validator.h"

namespace joynr
{
using namespace infrastructure::DacTypes;

LocalDomainAccessStore::LocalDomainAccessStore() : persistenceFileName()
{
}

LocalDomainAccessStore::LocalDomainAccessStore(std::string fileName)
{
    if (fileName.empty()) {
        return;
    }

    persistenceFileName = std::move(fileName);

    try {
        joynr::serializer::deserializeFromJson(
                *this, joynr::util::loadStringFromFile(persistenceFileName));
        JOYNR_LOG_INFO(logger(), "Loading ACL/RCL templates from {}", persistenceFileName);
        logTables();
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger(), ex.what());
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(),
                        "Could not deserialize persisted access control entries from {}: {}",
                        persistenceFileName,
                        ex.what());
    }

    // insert all entries into wildcard storage
    applyForAllTables([this](auto& entryParam) { addToWildcardStorage(entryParam); });
}

LocalDomainAccessStore::~LocalDomainAccessStore() = default;

void LocalDomainAccessStore::logContent()
{
    JOYNR_LOG_DEBUG(logger(), "Full ACl/RCL content:");
    logTables();

    JOYNR_LOG_TRACE(
            logger(), "masterAccessTable: {}", serializer::serializeToJson(masterAccessTable));

    JOYNR_LOG_TRACE(
            logger(), "mediatorAccessTable: {}", serializer::serializeToJson(mediatorAccessTable));

    JOYNR_LOG_TRACE(
            logger(), "ownerAccessTable: {}", serializer::serializeToJson(ownerAccessTable));

    JOYNR_LOG_TRACE(logger(),
                    "masterRegistrationTable: {}",
                    serializer::serializeToJson(masterRegistrationTable));

    JOYNR_LOG_TRACE(logger(),
                    "mediatorRegistrationTable: {}",
                    serializer::serializeToJson(mediatorRegistrationTable));

    JOYNR_LOG_TRACE(logger(),
                    "ownerRegistrationTable: {}",
                    serializer::serializeToJson(ownerRegistrationTable));

    JOYNR_LOG_TRACE(logger(), "domainRoleTable: {}", serializer::serializeToJson(domainRoleTable));

    JOYNR_LOG_TRACE(logger(), "domainWildcardStorage: {}", domainWildcardStorage.toString());

    JOYNR_LOG_TRACE(logger(), "interfaceWildcardStorage: {}", interfaceWildcardStorage.toString());
}

bool LocalDomainAccessStore::mergeDomainAccessStore(const LocalDomainAccessStore& other)
{
    if (!mergeTable(other.domainRoleTable, domainRoleTable)) {
        JOYNR_LOG_ERROR(logger(), "Could not merge domainRoleTable");
        return false;
    }

    if (!mergeTable(other.masterAccessTable, masterAccessTable)) {
        JOYNR_LOG_ERROR(logger(), "Could not merge masterAccessTable");
        return false;
    }

    if (!mergeTable(other.mediatorAccessTable, mediatorAccessTable)) {
        JOYNR_LOG_ERROR(logger(), "Could not merge mediatorAccessTable");
        return false;
    }

    if (!mergeTable(other.ownerAccessTable, ownerAccessTable)) {
        JOYNR_LOG_ERROR(logger(), "Could not merge ownerAccessTable");
        return false;
    }

    if (!mergeTable(other.masterRegistrationTable, masterRegistrationTable)) {
        JOYNR_LOG_ERROR(logger(), "Could not merge masterRegistrationTable");
        return false;
    }

    if (!mergeTable(other.mediatorRegistrationTable, mediatorRegistrationTable)) {
        JOYNR_LOG_ERROR(logger(), "Could not merge mediatorRegistrationTable");
        return false;
    }

    if (!mergeTable(other.ownerRegistrationTable, ownerRegistrationTable)) {
        JOYNR_LOG_ERROR(logger(), "Could not merge ownerRegistrationTable");
        return false;
    }

    return true;
}

std::set<std::pair<std::string, std::string>> LocalDomainAccessStore::
        getUniqueDomainInterfaceCombinations() const
{
    std::set<std::pair<std::string, std::string>> result;

    auto insertInResult = [&result](const auto& entry) {
        result.insert(std::make_pair(entry.getDomain(), entry.getInterfaceName()));
    };

    for (const auto& masterACE : masterAccessTable) {
        insertInResult(masterACE);
    }

    for (const auto& mediatorACE : mediatorAccessTable) {
        insertInResult(mediatorACE);
    }

    for (const auto& ownerACE : ownerAccessTable) {
        insertInResult(ownerACE);
    }

    return result;
}

std::vector<DomainRoleEntry> LocalDomainAccessStore::getDomainRoles(const std::string& userId)
{
    JOYNR_LOG_TRACE(logger(), "getDomainRoleEntries uid: {}", userId);

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
    return lookupOptional(domainRoleTable, uid, role);
}

bool LocalDomainAccessStore::updateDomainRole(const DomainRoleEntry& updatedEntry)
{
    JOYNR_LOG_TRACE(logger(), "updateDomainRole uid: {}", updatedEntry.getUid());

    bool updateSuccess = insertOrReplace(domainRoleTable, updatedEntry);
    if (updateSuccess) {
        logRoleEntry(updatedEntry);
    }
    return updateSuccess;
}

bool LocalDomainAccessStore::removeDomainRole(const std::string& userId, Role::Enum role)
{
    JOYNR_LOG_DEBUG(logger(), "removeDomainRoleEntry uid: {}", userId);
    return removeFromTable(domainRoleTable, userId, role);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const std::string& uid) const
{
    return getEqualRangeWithUidWildcard(masterAccessTable, uid);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const std::string& domain,
        const std::string& interfaceName) const
{
    return getEqualRange(masterAccessTable.get<access_control::tags::DomainAndInterface>(),
                         domain,
                         interfaceName);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntries(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    return getEqualRangeWithUidWildcard(masterAccessTable, uid, domain, interfaceName);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getEditableMasterAccessControlEntries(
        const std::string& userId)
{
    JOYNR_LOG_TRACE(logger(), "getEditableMasterAccessControlEntry uid: {}", userId);

    return getEntries(masterAccessTable, userId, Role::MASTER);
}

boost::optional<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    // ignoring operation as not yet supported
    std::ignore = operation;
    JOYNR_LOG_TRACE(logger(),
                    "getMasterAccessControlEntry uid: {}, domain:{}, "
                    "interfaceName: {}",
                    uid,
                    domain,
                    interfaceName);
    return lookupOptionalWithWildcard(masterAccessTable, uid, domain, interfaceName);
}

bool LocalDomainAccessStore::updateMasterAccessControlEntry(
        const MasterAccessControlEntry& updatedMasterAce)
{
    JOYNR_LOG_TRACE(logger(),
                    "updateMasterAccessControlEntry uid: {}, domain:{}, "
                    "interfaceName: {}",
                    updatedMasterAce.getUid(),
                    updatedMasterAce.getDomain(),
                    updatedMasterAce.getInterfaceName());
    bool updateSuccess = insertOrReplace(masterAccessTable, updatedMasterAce);
    if (updateSuccess) {
        logControlEntry(updatedMasterAce, "MasterAccessControlEntry");
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeMasterAccessControlEntry(const std::string& userId,
                                                            const std::string& domain,
                                                            const std::string& interfaceName,
                                                            const std::string& operation)
{
    JOYNR_LOG_DEBUG(logger(),
                    "removeMasterAccessControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}, operation: {}",
                    userId,
                    domain,
                    interfaceName,
                    operation);
    return removeFromTable(masterAccessTable, userId, domain, interfaceName, operation);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& uid)
{
    return convertMediator(getEqualRangeWithUidWildcard(mediatorAccessTable, uid));
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& domain,
        const std::string& interfaceName)
{
    return convertMediator(
            getEqualRange(mediatorAccessTable.get<access_control::tags::DomainAndInterface>(),
                          domain,
                          interfaceName));
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    return convertMediator(
            getEqualRangeWithUidWildcard(mediatorAccessTable, uid, domain, interfaceName));
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::
        getEditableMediatorAccessControlEntries(const std::string& userId)
{
    JOYNR_LOG_TRACE(logger(), "getEditableMediatorAces uid: {}", userId);

    // Get all the Mediator ACEs for the domains where the user is master
    return convertMediator(getEntries(mediatorAccessTable, userId, Role::MASTER));
}

boost::optional<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    // ignoring operation as not yet supported
    std::ignore = operation;
    JOYNR_LOG_TRACE(logger(),
                    "getMediatorAccessControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    uid,
                    domain,
                    interfaceName);
    return convertMediator(
            lookupOptionalWithWildcard(mediatorAccessTable, uid, domain, interfaceName));
}

bool LocalDomainAccessStore::updateMediatorAccessControlEntry(
        const MasterAccessControlEntry& updatedMediatorAce)
{
    JOYNR_LOG_TRACE(logger(),
                    "updateMediatorAccessControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    updatedMediatorAce.getUid(),
                    updatedMediatorAce.getDomain(),
                    updatedMediatorAce.getInterfaceName());
    bool updateSuccess = false;

    boost::optional<MasterAccessControlEntry> masterAceOptional =
            getMasterAccessControlEntry(updatedMediatorAce.getUid(),
                                        updatedMediatorAce.getDomain(),
                                        updatedMediatorAce.getInterfaceName(),
                                        updatedMediatorAce.getOperation());
    AceValidator aceValidator(masterAceOptional, updatedMediatorAce, boost::none);

    if (aceValidator.isMediatorValid()) {
        // Add/update a mediator ACE
        updateSuccess = insertOrReplace(mediatorAccessTable, updatedMediatorAce);
        if (updateSuccess) {
            logControlEntry(updatedMediatorAce, "MediatorAccessControlEntry");
        }
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeMediatorAccessControlEntry(const std::string& userId,
                                                              const std::string& domain,
                                                              const std::string& interfaceName,
                                                              const std::string& operation)
{
    JOYNR_LOG_DEBUG(logger(),
                    "removeMediatorAccessControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}, operation: {}",
                    userId,
                    domain,
                    interfaceName,
                    operation);
    return removeFromTable(mediatorAccessTable, userId, domain, interfaceName, operation);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const std::string& uid)
{
    return getEqualRangeWithUidWildcard(ownerAccessTable, uid);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const std::string& domain,
        const std::string& interfaceName)
{
    return getEqualRange(ownerAccessTable.get<access_control::tags::DomainAndInterface>(),
                         domain,
                         interfaceName);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntries(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName)
{
    return getEqualRangeWithUidWildcard(ownerAccessTable, userId, domain, interfaceName);
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessStore::getEditableOwnerAccessControlEntries(
        const std::string& userId)
{
    JOYNR_LOG_TRACE(logger(), "getEditableOwnerAces uid: {}", userId);

    // Get all the Owner ACEs for the domains owned by the user
    return getEntries(ownerAccessTable, userId, Role::OWNER);
}

boost::optional<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntry(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    // ignoring operation as not yet supported
    std::ignore = operation;
    JOYNR_LOG_TRACE(logger(),
                    "getOwnerAccessControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    userId,
                    domain,
                    interfaceName);
    return lookupOptionalWithWildcard(ownerAccessTable, userId, domain, interfaceName);
}

bool LocalDomainAccessStore::updateOwnerAccessControlEntry(
        const OwnerAccessControlEntry& updatedOwnerAce)
{
    JOYNR_LOG_TRACE(logger(),
                    "updateOwnerAccessControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    updatedOwnerAce.getUid(),
                    updatedOwnerAce.getDomain(),
                    updatedOwnerAce.getInterfaceName());

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
    AceValidator aceValidator(masterAceOptional, mediatorAceOptional, updatedOwnerAce);

    if (aceValidator.isOwnerValid()) {
        updateSuccess = insertOrReplace(ownerAccessTable, updatedOwnerAce);
        if (updateSuccess) {
            logControlEntry(updatedOwnerAce, "OwnerAccessControlEntry");
        }
    }
    return updateSuccess;
}

bool LocalDomainAccessStore::removeOwnerAccessControlEntry(const std::string& userId,
                                                           const std::string& domain,
                                                           const std::string& interfaceName,
                                                           const std::string& operation)
{
    JOYNR_LOG_DEBUG(logger(),
                    "removeOwnerAccessControlEntry uid: {}, domain: {}, "
                    "interface: {}, "
                    "operation: {}",
                    userId,
                    domain,
                    interfaceName,
                    operation);

    return removeFromTable(ownerAccessTable, userId, domain, interfaceName, operation);
}

// Registration tables

// MasterRegistration

std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry> LocalDomainAccessStore::
        getMasterRegistrationControlEntries(const std::string& uid) const
{
    JOYNR_LOG_TRACE(logger(), "getMasterRegistrationControlEntries uid: {}", uid);
    return getEqualRangeWithUidWildcard(masterRegistrationTable, uid);
}

std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry> LocalDomainAccessStore::
        getEditableMasterRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(logger(), "getEditableMasterRegistrationControlEntry uid: {}", uid);

    return getEntries(masterRegistrationTable, uid, Role::MASTER);
}

boost::optional<MasterRegistrationControlEntry> LocalDomainAccessStore::
        getMasterRegistrationControlEntry(const std::string& uid,
                                          const std::string& domain,
                                          const std::string& interfaceName)
{
    JOYNR_LOG_TRACE(logger(),
                    "getMasterRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    uid,
                    domain,
                    interfaceName);
    return lookupOptionalWithWildcard(masterRegistrationTable, uid, domain, interfaceName);
}

bool LocalDomainAccessStore::updateMasterRegistrationControlEntry(
        const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMasterRce)
{
    JOYNR_LOG_TRACE(logger(),
                    "updateMasterRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    updatedMasterRce.getUid(),
                    updatedMasterRce.getDomain(),
                    updatedMasterRce.getInterfaceName());

    bool updateSuccess = insertOrReplace(masterRegistrationTable, updatedMasterRce);
    if (updateSuccess) {
        logControlEntry(updatedMasterRce, "MasterRegistrationControlEntry");
    }
    return updateSuccess;
}

bool LocalDomainAccessStore::removeMasterRegistrationControlEntry(const std::string& uid,
                                                                  const std::string& domain,
                                                                  const std::string& interfaceName)
{
    JOYNR_LOG_DEBUG(logger(),
                    "removeMasterRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    uid,
                    domain,
                    interfaceName);
    return removeFromTable(masterRegistrationTable, uid, domain, interfaceName);
}

// MediatorRegistration

std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry> LocalDomainAccessStore::
        getMediatorRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(logger(), "getMediatorRegistrationControlEntries uid: {}", uid);
    return convertMediator(getEqualRangeWithUidWildcard(mediatorRegistrationTable, uid));
}

std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry> LocalDomainAccessStore::
        getEditableMediatorRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(logger(), "getEditableMeditatorRegistrationControlEntry uid: {}", uid);

    return convertMediator(getEntries(mediatorRegistrationTable, uid, Role::MASTER));
}

boost::optional<MasterRegistrationControlEntry> LocalDomainAccessStore::
        getMediatorRegistrationControlEntry(const std::string& uid,
                                            const std::string& domain,
                                            const std::string& interfaceName)
{
    JOYNR_LOG_TRACE(logger(),
                    "getMediatorRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    uid,
                    domain,
                    interfaceName);
    return convertMediator(
            lookupOptionalWithWildcard(mediatorRegistrationTable, uid, domain, interfaceName));
}

bool LocalDomainAccessStore::updateMediatorRegistrationControlEntry(
        const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMediatorRce)
{
    JOYNR_LOG_TRACE(logger(),
                    "updateMediatorRegistrationControlEntry uid: {}, "
                    "domain: {}, interfaceName: {}",
                    updatedMediatorRce.getUid(),
                    updatedMediatorRce.getDomain(),
                    updatedMediatorRce.getInterfaceName());
    bool updateSuccess = false;

    boost::optional<MasterRegistrationControlEntry> masterRceOptional =
            getMasterRegistrationControlEntry(updatedMediatorRce.getUid(),
                                              updatedMediatorRce.getDomain(),
                                              updatedMediatorRce.getInterfaceName());
    RceValidator rceValidator(masterRceOptional, updatedMediatorRce, boost::none);

    if (rceValidator.isMediatorValid()) {
        // Add/update a mediator RCE
        updateSuccess = insertOrReplace(mediatorRegistrationTable, updatedMediatorRce);
        if (updateSuccess) {
            logControlEntry(updatedMediatorRce, "MediatorRegistrationControlEntry");
        }
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeMediatorRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    JOYNR_LOG_DEBUG(logger(),
                    "removeMediatorRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    uid,
                    domain,
                    interfaceName);
    return removeFromTable(mediatorRegistrationTable, uid, domain, interfaceName);
}

// OwnerRegistration

std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry> LocalDomainAccessStore::
        getOwnerRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(logger(), "getOwnerRegistrationControlEntries uid: {}", uid);
    return getEqualRangeWithUidWildcard(ownerRegistrationTable, uid);
}

std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry> LocalDomainAccessStore::
        getEditableOwnerRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(logger(), "getEditableOwnerRegistrationControlEntry uid: {}", uid);
    return getEntries(ownerRegistrationTable, uid, Role::OWNER);
}

boost::optional<OwnerRegistrationControlEntry> LocalDomainAccessStore::
        getOwnerRegistrationControlEntry(const std::string& userId,
                                         const std::string& domain,
                                         const std::string& interfaceName)
{
    JOYNR_LOG_TRACE(logger(),
                    "getOwnerRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    userId,
                    domain,
                    interfaceName);
    return lookupOptionalWithWildcard(ownerRegistrationTable, userId, domain, interfaceName);
}

bool LocalDomainAccessStore::updateOwnerRegistrationControlEntry(
        const infrastructure::DacTypes::OwnerRegistrationControlEntry& updatedOwnerRce)
{
    JOYNR_LOG_TRACE(logger(),
                    "updateOwnerRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    updatedOwnerRce.getUid(),
                    updatedOwnerRce.getDomain(),
                    updatedOwnerRce.getInterfaceName());

    bool updateSuccess = false;

    boost::optional<MasterRegistrationControlEntry> masterRceOptional =
            getMasterRegistrationControlEntry(updatedOwnerRce.getUid(),
                                              updatedOwnerRce.getDomain(),
                                              updatedOwnerRce.getInterfaceName());
    boost::optional<MasterRegistrationControlEntry> mediatorRceOptional =
            getMediatorRegistrationControlEntry(updatedOwnerRce.getUid(),
                                                updatedOwnerRce.getDomain(),
                                                updatedOwnerRce.getInterfaceName());
    RceValidator rceValidator(masterRceOptional, mediatorRceOptional, updatedOwnerRce);

    if (rceValidator.isOwnerValid()) {
        // Add/update a mediator RCE
        updateSuccess = insertOrReplace(ownerRegistrationTable, updatedOwnerRce);
        if (updateSuccess) {
            logControlEntry(updatedOwnerRce, "OwnerRegistrationControlEntry");
        }
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeOwnerRegistrationControlEntry(const std::string& uid,
                                                                 const std::string& domain,
                                                                 const std::string& interfaceName)
{
    JOYNR_LOG_DEBUG(logger(),
                    "removeOwnerRegistrationControlEntry uid: {}, domain: {}, "
                    "interfaceName: {}",
                    uid,
                    domain,
                    interfaceName);
    return removeFromTable(ownerRegistrationTable, uid, domain, interfaceName);
}

bool LocalDomainAccessStore::onlyWildcardOperations(const std::string& userId,
                                                    const std::string& domain,
                                                    const std::string& interfaceName)
{
    return checkOnlyWildcardOperations(masterAccessTable, userId, domain, interfaceName) &&
           checkOnlyWildcardOperations(mediatorAccessTable, userId, domain, interfaceName) &&
           checkOnlyWildcardOperations(ownerAccessTable, userId, domain, interfaceName);
}

void LocalDomainAccessStore::persistToFile() const
{
    if (persistenceFileName.empty()) {
        JOYNR_LOG_TRACE(logger(), "No persistency specified");
        return;
    }
    try {
        joynr::util::saveStringToFile(
                persistenceFileName, joynr::serializer::serializeToJson(*this));
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(), "serializing to JSON failed: {}", ex.what());
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }
}

bool LocalDomainAccessStore::endsWithWildcard(const std::string& value) const
{
    if (value.empty()) {
        return false;
    }
    return value.back() == *joynr::access_control::WILDCARD;
}

void LocalDomainAccessStore::logControlEntry(
        const MasterAccessControlEntry& masterAccessControlEntry,
        const std::string title)
{
    JOYNR_LOG_DEBUG(
            logger(),
            "{}: Uid: {}, Domain: {}, Interface: {}, Operation: {}, DefaultConsumerPermission: {}",
            title,
            masterAccessControlEntry.getUid(),
            masterAccessControlEntry.getDomain(),
            masterAccessControlEntry.getInterfaceName(),
            masterAccessControlEntry.getOperation(),
            Permission::getLiteral(masterAccessControlEntry.getDefaultConsumerPermission()));
}

void LocalDomainAccessStore::logControlEntry(const OwnerAccessControlEntry& ownerAccessControlEntry,
                                             const std::string title)
{
    JOYNR_LOG_DEBUG(
            logger(),
            "{}: Uid: {}, Domain: {}, Interface: {}, Operation: {}, ConsumerPermission: {} ",
            title,
            ownerAccessControlEntry.getUid(),
            ownerAccessControlEntry.getDomain(),
            ownerAccessControlEntry.getInterfaceName(),
            ownerAccessControlEntry.getOperation(),
            Permission::getLiteral(ownerAccessControlEntry.getConsumerPermission()));
}

void LocalDomainAccessStore::logControlEntry(
        const MasterRegistrationControlEntry& masterRegistrationControlEntry,
        const std::string title)
{
    JOYNR_LOG_DEBUG(
            logger(),
            "{}: Uid: {}, Domain: {}, Interface: {}, DefaultProviderPermission: {}",
            title,
            masterRegistrationControlEntry.getUid(),
            masterRegistrationControlEntry.getDomain(),
            masterRegistrationControlEntry.getInterfaceName(),
            Permission::getLiteral(masterRegistrationControlEntry.getDefaultProviderPermission()));
}

void LocalDomainAccessStore::logControlEntry(
        const OwnerRegistrationControlEntry& ownerRegistrationControlEntry,
        const std::string title)
{
    JOYNR_LOG_DEBUG(logger(),
                    "{}: Uid: {}, Domain: {}, Interface: {}, "
                    "ProviderPermission: {} ",
                    title,
                    ownerRegistrationControlEntry.getUid(),
                    ownerRegistrationControlEntry.getDomain(),
                    ownerRegistrationControlEntry.getInterfaceName(),
                    Permission::getLiteral(ownerRegistrationControlEntry.getProviderPermission()));
}

void LocalDomainAccessStore::logRoleEntry(const DomainRoleEntry& domainRoleEntry)
{
    JOYNR_LOG_DEBUG(logger(),
                    "DomainRoleEntry: Uid: {}, Domains: {}, Role: {}",
                    domainRoleEntry.getUid(),
                    boost::algorithm::join(domainRoleEntry.getDomains(), ", "),
                    Role::getLiteral(domainRoleEntry.getRole()));
}

void LocalDomainAccessStore::logTables()
{
    for (auto& entry : masterAccessTable) {
        logControlEntry(entry, "MasterAccessControlEntry");
    }
    for (auto& entry : mediatorAccessTable) {
        logControlEntry(entry, "MediatorAccessControlEntry");
    }
    for (auto& entry : ownerAccessTable) {
        logControlEntry(entry, "OwnerAccessControlEntry");
    }
    for (auto& entry : masterRegistrationTable) {
        logControlEntry(entry, "MasterRegistrationControlEntry");
    }
    for (auto& entry : mediatorRegistrationTable) {
        logControlEntry(entry, "MediatorRegistrationControlEntry");
    }
    for (auto& entry : ownerRegistrationTable) {
        logControlEntry(entry, "OwnerRegistrationControlEntry");
    }
    for (auto& entry : domainRoleTable) {
        logRoleEntry(entry);
    }
}

} // namespace joynr

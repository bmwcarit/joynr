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
#include <iterator>

#include "joynr/Util.h"
#include "libjoynrclustercontroller/access-control/Validator.h"

namespace joynr
{
using namespace infrastructure::DacTypes;

INIT_LOGGER(LocalDomainAccessStore);

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
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger,
                        "Could not deserialize persisted access control entries from {}: {}",
                        persistenceFileName,
                        ex.what());
    }
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
    JOYNR_LOG_TRACE(logger, "execute: entering getDomainRoleEntries with userId {}", userId);

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
    JOYNR_LOG_TRACE(
            logger, "execute: entering updateDomainRoleEntry with uId {}", updatedEntry.getUid());

    return insertOrReplace(domainRoleTable, updatedEntry);
}

bool LocalDomainAccessStore::removeDomainRole(const std::string& userId, Role::Enum role)
{
    JOYNR_LOG_TRACE(logger, "execute: entering removeDomainRoleEntry with uId {}", userId);
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
    JOYNR_LOG_TRACE(
            logger, "execute: entering getEditableMasterAccessControlEntry with uId {}", userId);

    return getEntries(masterAccessTable, userId, Role::MASTER);
}

boost::optional<MasterAccessControlEntry> LocalDomainAccessStore::getMasterAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    return lookupOptionalWithWildcard(masterAccessTable, uid, domain, interfaceName, operation);
}

bool LocalDomainAccessStore::updateMasterAccessControlEntry(
        const MasterAccessControlEntry& updatedMasterAce)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering updateMasterAce with uId {}", updatedMasterAce.getUid());

    return insertOrReplace(masterAccessTable, updatedMasterAce);
}

bool LocalDomainAccessStore::removeMasterAccessControlEntry(const std::string& userId,
                                                            const std::string& domain,
                                                            const std::string& interfaceName,
                                                            const std::string& operation)
{
    return removeFromTable(masterAccessTable, userId, domain, interfaceName, operation);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& uid)
{
    return getEqualRangeWithUidWildcard(mediatorAccessTable, uid);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& domain,
        const std::string& interfaceName)
{
    return getEqualRange(mediatorAccessTable.get<access_control::tags::DomainAndInterface>(),
                         domain,
                         interfaceName);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntries(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    return getEqualRangeWithUidWildcard(mediatorAccessTable, uid, domain, interfaceName);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessStore::
        getEditableMediatorAccessControlEntries(const std::string& userId)
{
    JOYNR_LOG_TRACE(logger, "execute: entering getEditableMediatorAces with uId {}", userId);

    // Get all the Mediator ACEs for the domains where the user is master
    return getEntries(mediatorAccessTable, userId, Role::MASTER);
}

boost::optional<MasterAccessControlEntry> LocalDomainAccessStore::getMediatorAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    return lookupOptionalWithWildcard(mediatorAccessTable, uid, domain, interfaceName, operation);
}

bool LocalDomainAccessStore::updateMediatorAccessControlEntry(
        const MasterAccessControlEntry& updatedMediatorAce)
{
    JOYNR_LOG_TRACE(
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
        updateSuccess = insertOrReplace(mediatorAccessTable, updatedMediatorAce);
    }

    return updateSuccess;
}

bool LocalDomainAccessStore::removeMediatorAccessControlEntry(const std::string& userId,
                                                              const std::string& domain,
                                                              const std::string& interfaceName,
                                                              const std::string& operation)
{
    JOYNR_LOG_TRACE(logger,
                    "execute: entering removeMediatorAce with userId: {}, domain: {}, "
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
    JOYNR_LOG_TRACE(logger, "execute: entering getEditableOwnerAces with uId {}", userId);

    // Get all the Owner ACEs for the domains owned by the user
    return getEntries(ownerAccessTable, userId, Role::OWNER);
}

boost::optional<OwnerAccessControlEntry> LocalDomainAccessStore::getOwnerAccessControlEntry(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation)
{
    return lookupOptionalWithWildcard(ownerAccessTable, userId, domain, interfaceName, operation);
}

bool LocalDomainAccessStore::updateOwnerAccessControlEntry(
        const OwnerAccessControlEntry& updatedOwnerAce)
{
    JOYNR_LOG_TRACE(
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
        updateSuccess = insertOrReplace(ownerAccessTable, updatedOwnerAce);
    }
    return updateSuccess;
}

bool LocalDomainAccessStore::removeOwnerAccessControlEntry(const std::string& userId,
                                                           const std::string& domain,
                                                           const std::string& interfaceName,
                                                           const std::string& operation)
{
    JOYNR_LOG_TRACE(logger,
                    "execute: entering removeOwnerAce with userId: {}, domain: {}, interface: {}, "
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
    JOYNR_LOG_TRACE(
            logger, "execute: entering getMasterRegistrationControlEntries with uid {}", uid);
    return getEqualRangeWithUidWildcard(masterRegistrationTable, uid);
}

std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry> LocalDomainAccessStore::
        getEditableMasterRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering getEditableMasterRegistrationControlEntry with uid {}", uid);

    return getEntries(masterRegistrationTable, uid, Role::MASTER);
}

boost::optional<MasterRegistrationControlEntry> LocalDomainAccessStore::
        getMasterRegistrationControlEntry(const std::string& uid,
                                          const std::string& domain,
                                          const std::string& interfaceName)
{
    return lookupOptionalWithWildcard(masterRegistrationTable, uid, domain, interfaceName);
}

bool LocalDomainAccessStore::updateMasterRegistrationControlEntry(
        const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMasterRce)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering updateMasterRce with uid {}", updatedMasterRce.getUid());

    return insertOrReplace(masterRegistrationTable, updatedMasterRce);
}

bool LocalDomainAccessStore::removeMasterRegistrationControlEntry(const std::string& uid,
                                                                  const std::string& domain,
                                                                  const std::string& interfaceName)
{
    return removeFromTable(masterRegistrationTable, uid, domain, interfaceName);
}

// MediatorRegistration

std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry> LocalDomainAccessStore::
        getMediatorRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering getMediatorRegistrationControlEntries with uid {}", uid);
    return getEqualRangeWithUidWildcard(mediatorRegistrationTable, uid);
}

std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry> LocalDomainAccessStore::
        getEditableMediatorRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(logger,
                    "execute: entering getEditableMeditatorRegistrationControlEntry with uid {}",
                    uid);

    return getEntries(mediatorRegistrationTable, uid, Role::MASTER);
}

boost::optional<MasterRegistrationControlEntry> LocalDomainAccessStore::
        getMediatorRegistrationControlEntry(const std::string& uid,
                                            const std::string& domain,
                                            const std::string& interfaceName)
{
    return lookupOptionalWithWildcard(mediatorRegistrationTable, uid, domain, interfaceName);
}

bool LocalDomainAccessStore::updateMediatorRegistrationControlEntry(
        const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMediatorRce)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering updateMediatorRce with uid {}", updatedMediatorRce.getUid());

    return insertOrReplace(mediatorRegistrationTable, updatedMediatorRce);
}

bool LocalDomainAccessStore::removeMediatorRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    return removeFromTable(mediatorRegistrationTable, uid, domain, interfaceName);
}

// OwnerRegistration

std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry> LocalDomainAccessStore::
        getOwnerRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering getOwnerRegistrationControlEntries with uid {}", uid);
    return getEqualRangeWithUidWildcard(ownerRegistrationTable, uid);
}

std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry> LocalDomainAccessStore::
        getEditableOwnerRegistrationControlEntries(const std::string& uid)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering getEditableOwnerRegistrationControlEntry with uid {}", uid);

    return getEntries(ownerRegistrationTable, uid, Role::MASTER);
}

boost::optional<OwnerRegistrationControlEntry> LocalDomainAccessStore::
        getOwnerRegistrationControlEntry(const std::string& userId,
                                         const std::string& domain,
                                         const std::string& interfaceName)
{
    return lookupOptionalWithWildcard(ownerRegistrationTable, userId, domain, interfaceName);
}

bool LocalDomainAccessStore::updateOwnerRegistrationControlEntry(
        const infrastructure::DacTypes::OwnerRegistrationControlEntry& updatedOwnerRce)
{
    JOYNR_LOG_TRACE(
            logger, "execute: entering updateOwnerRce with uid {}", updatedOwnerRce.getUid());

    return insertOrReplace(ownerRegistrationTable, updatedOwnerRce);
}

bool LocalDomainAccessStore::removeOwnerRegistrationControlEntry(const std::string& uid,
                                                                 const std::string& domain,
                                                                 const std::string& interfaceName)
{
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
        return;
    }
    try {
        joynr::util::saveStringToFile(
                persistenceFileName, joynr::serializer::serializeToJson(*this));
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger, "serializing to JSON failed: {}", ex.what());
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }
}

} // namespace joynr

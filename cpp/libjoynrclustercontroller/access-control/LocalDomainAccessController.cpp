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

#include "LocalDomainAccessController.h"

#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/DacTypes/DomainRoleEntry.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerRegistrationControlEntry.h"

#include "AccessControlAlgorithm.h"
#include "AccessControlUtils.h"
#include "LocalDomainAccessStore.h"

namespace joynr
{

using namespace infrastructure;
using namespace infrastructure::DacTypes;

LocalDomainAccessController::LocalDomainAccessController(
        std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore)
        : _localDomainAccessStore(std::move(localDomainAccessStore))
{
}

bool LocalDomainAccessController::hasRole(const std::string& userId,
                                          const std::string& domain,
                                          Role::Enum role)
{
    JOYNR_LOG_TRACE(logger(), "execute: entering hasRole");

    // See if the user has the given role
    bool hasRole = false;
    boost::optional<DomainRoleEntry> dre = _localDomainAccessStore->getDomainRole(userId, role);
    if (dre) {
        std::vector<std::string> domains = dre->getDomains();
        const std::string wildcard = "*";
        if (util::vectorContains(domains, domain) || util::vectorContains(domains, wildcard)) {
            hasRole = true;
        }
    }

    return hasRole;
}

void LocalDomainAccessController::getConsumerPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustLevel,
        std::shared_ptr<IGetPermissionCallback> callback)
{
    JOYNR_LOG_TRACE(logger(), "Entering getConsumerPermission with unknown operation");
    // The operations of the ACEs should only contain wildcards, if not
    // getConsumerPermission should be called with an operation
    if (!_localDomainAccessStore->onlyWildcardOperations(userId, domain, interfaceName)) {
        JOYNR_LOG_INFO(logger(), "Operation needed for ACL check.");
        callback->operationNeeded();
    } else {
        // The operations are all wildcards
        Permission::Enum permission = getConsumerPermission(
                userId, domain, interfaceName, access_control::WILDCARD, trustLevel);
        callback->permission(permission);
    }
}

Permission::Enum LocalDomainAccessController::getConsumerPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation,
        TrustLevel::Enum trustLevel)
{
    JOYNR_LOG_TRACE(logger(),
                    "Entering getConsumerPermission with userID={} domain={} interfaceName={}",
                    userId,
                    domain,
                    interfaceName);

    boost::optional<MasterAccessControlEntry> masterAceOptional =
            _localDomainAccessStore->getMasterAccessControlEntry(
                    userId, domain, interfaceName, operation);
    boost::optional<MasterAccessControlEntry> mediatorAceOptional =
            _localDomainAccessStore->getMediatorAccessControlEntry(
                    userId, domain, interfaceName, operation);
    boost::optional<OwnerAccessControlEntry> ownerAceOptional =
            _localDomainAccessStore->getOwnerAccessControlEntry(
                    userId, domain, interfaceName, operation);

    return _accessControlAlgorithm.getConsumerPermission(
            masterAceOptional, mediatorAceOptional, ownerAceOptional, trustLevel);
}

void LocalDomainAccessController::getProviderPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustLevel,
        std::shared_ptr<IGetPermissionCallback> callback)
{
    JOYNR_LOG_TRACE(logger(), "Entering getProviderPermission with callback");
    Permission::Enum permission = getProviderPermission(userId, domain, interfaceName, trustLevel);
    callback->permission(permission);
}

Permission::Enum LocalDomainAccessController::getProviderPermission(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustLevel)
{
    JOYNR_LOG_TRACE(logger(),
                    "Entering getProviderPermission with userID={} domain={} interfaceName={}",
                    uid,
                    domain,
                    interfaceName);

    boost::optional<MasterRegistrationControlEntry> masterRceOptional =
            _localDomainAccessStore->getMasterRegistrationControlEntry(uid, domain, interfaceName);
    boost::optional<MasterRegistrationControlEntry> mediatorRceOptional =
            _localDomainAccessStore->getMediatorRegistrationControlEntry(
                    uid, domain, interfaceName);
    boost::optional<OwnerRegistrationControlEntry> ownerRceOptional =
            _localDomainAccessStore->getOwnerRegistrationControlEntry(uid, domain, interfaceName);

    return _accessControlAlgorithm.getProviderPermission(
            masterRceOptional, mediatorRceOptional, ownerRceOptional, trustLevel);
}

} // namespace joynr

#pragma GCC diagnostic pop

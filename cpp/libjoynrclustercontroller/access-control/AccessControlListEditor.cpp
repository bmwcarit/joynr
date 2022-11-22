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

#include "AccessControlListEditor.h"

#include <tuple>

#include "joynr/CallContext.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerRegistrationControlEntry.h"
#include "joynr/infrastructure/IAccessControlListEditor.h"

#include "AccessController.h"
#include "LocalDomainAccessStore.h"

using namespace joynr::infrastructure::DacTypes;

namespace joynr
{

namespace exceptions
{
class ProviderRuntimeException;
}

AccessControlListEditor::AccessControlListEditor(
        std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore,
        std::shared_ptr<AccessController> accessController,
        bool auditMode)
        : _localDomainAccessStore(std::move(localDomainAccessStore)),
          _accessController(std::move(accessController)),
          _aclAudit(auditMode)
{
}

void AccessControlListEditor::updateMasterAccessControlEntry(
        const joynr::infrastructure::DacTypes::MasterAccessControlEntry& updatedMasterAce,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleMaster(updatedMasterAce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                _localDomainAccessStore->updateMasterAccessControlEntry(updatedMasterAce);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::removeMasterAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleMaster(domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = _localDomainAccessStore->removeMasterAccessControlEntry(
                uid, domain, interfaceName, operation);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::updateMediatorAccessControlEntry(
        const joynr::infrastructure::DacTypes::MasterAccessControlEntry& updatedMediatorAce,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    // TODO: which role is required here?
    if (!hasRoleMaster(updatedMediatorAce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                _localDomainAccessStore->updateMediatorAccessControlEntry(updatedMediatorAce);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::removeMediatorAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleMaster(domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = _localDomainAccessStore->removeMediatorAccessControlEntry(
                uid, domain, interfaceName, operation);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::updateOwnerAccessControlEntry(
        const joynr::infrastructure::DacTypes::OwnerAccessControlEntry& updatedOwnerAce,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleOwner(updatedOwnerAce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                _localDomainAccessStore->updateOwnerAccessControlEntry(updatedOwnerAce);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::removeOwnerAccessControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleOwner(domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = _localDomainAccessStore->removeOwnerAccessControlEntry(
                uid, domain, interfaceName, operation);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::updateMasterRegistrationControlEntry(
        const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMasterRce,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleMaster(updatedMasterRce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                _localDomainAccessStore->updateMasterRegistrationControlEntry(updatedMasterRce);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::removeMasterRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleMaster(domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = _localDomainAccessStore->removeMasterRegistrationControlEntry(
                uid, domain, interfaceName);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::updateMediatorRegistrationControlEntry(
        const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMediatorRce,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    // which role is required here?
    if (!hasRoleMaster(updatedMediatorRce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                _localDomainAccessStore->updateMediatorRegistrationControlEntry(updatedMediatorRce);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::removeMediatorRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleMaster(domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = _localDomainAccessStore->removeMediatorRegistrationControlEntry(
                uid, domain, interfaceName);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::updateOwnerRegistrationControlEntry(
        const joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry& updatedOwnerRce,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleOwner(updatedOwnerRce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                _localDomainAccessStore->updateOwnerRegistrationControlEntry(updatedOwnerRce);
        onSuccess(updateSuccess);
    }
}

void AccessControlListEditor::removeOwnerRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleOwner(domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = _localDomainAccessStore->removeOwnerRegistrationControlEntry(
                uid, domain, interfaceName);
        onSuccess(updateSuccess);
    }
}

bool AccessControlListEditor::hasRoleMaster(const std::string& domain)
{
    return hasRoleWorker(domain, Role::MASTER);
}

bool AccessControlListEditor::hasRoleOwner(const std::string& domain)
{
    return hasRoleWorker(domain, Role::OWNER);
}

bool AccessControlListEditor::hasRoleWorker(const std::string& domain,
                                            joynr::infrastructure::DacTypes::Role::Enum role)
{
    const CallContext& callContext = getCallContext();
    const std::string& uid = callContext.getPrincipal();
    JOYNR_LOG_TRACE(logger(),
                    "Lookup domain {} for userId {} and role {}",
                    domain,
                    uid,
                    joynr::infrastructure::DacTypes::Role::getLiteral(role));

    bool hasRole = _accessController->hasRole(uid, domain, role);

    if (_aclAudit) {
        if (!hasRole) {
            JOYNR_LOG_ERROR(logger(),
                            "ACL AUDIT: id '{}' does NOT have the roles to modify domain {}",
                            uid,
                            domain);
            hasRole = true;
        } else {
            JOYNR_LOG_TRACE(logger(),
                            "ACL AUDIT: id '{}' does have the roles to modify domain {}",
                            uid,
                            domain);
        }
    }

    return hasRole;
}

} // namespace joynr

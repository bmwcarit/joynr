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

#include "libjoynrclustercontroller/access-control/AccessControlListEditor.h"

#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessController.h"

using namespace joynr::infrastructure::DacTypes;

namespace joynr
{

AccessControlListEditor::AccessControlListEditor(
        std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore,
        std::shared_ptr<LocalDomainAccessController> localDomainAccessController)
        : localDomainAccessStore(std::move(localDomainAccessStore)),
          localDomainAccessController(std::move(localDomainAccessController))
{
}

void AccessControlListEditor::updateMasterAccessControlEntry(
        const joynr::infrastructure::DacTypes::MasterAccessControlEntry& updatedMasterAce,
        std::function<void(const bool& success)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    if (!hasRoleMaster(updatedMasterAce.getUid(), updatedMasterAce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                localDomainAccessStore->updateMasterAccessControlEntry(updatedMasterAce);
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
    if (!hasRoleMaster(uid, domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = localDomainAccessStore->removeMasterAccessControlEntry(
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
    if (!hasRoleMaster(updatedMediatorAce.getUid(), updatedMediatorAce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                localDomainAccessStore->updateMediatorAccessControlEntry(updatedMediatorAce);
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
    if (!hasRoleMaster(uid, domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = localDomainAccessStore->removeMediatorAccessControlEntry(
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
    if (!hasRoleOwner(updatedOwnerAce.getUid(), updatedOwnerAce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess = localDomainAccessStore->updateOwnerAccessControlEntry(updatedOwnerAce);
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
    if (!hasRoleOwner(uid, domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = localDomainAccessStore->removeOwnerAccessControlEntry(
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
    if (!hasRoleMaster(updatedMasterRce.getUid(), updatedMasterRce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                localDomainAccessStore->updateMasterRegistrationControlEntry(updatedMasterRce);
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
    if (!hasRoleMaster(uid, domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = localDomainAccessStore->removeMasterRegistrationControlEntry(
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
    if (!hasRoleMaster(updatedMediatorRce.getUid(), updatedMediatorRce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                localDomainAccessStore->updateMasterRegistrationControlEntry(updatedMediatorRce);
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
    if (!hasRoleMaster(uid, domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = localDomainAccessStore->removeMediatorRegistrationControlEntry(
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
    if (!hasRoleOwner(updatedOwnerRce.getUid(), updatedOwnerRce.getDomain())) {
        onSuccess(false);
    } else {
        bool updateSuccess =
                localDomainAccessStore->updateOwnerRegistrationControlEntry(updatedOwnerRce);
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
    if (!hasRoleOwner(uid, domain)) {
        onSuccess(false);
    } else {
        bool updateSuccess = localDomainAccessStore->removeOwnerRegistrationControlEntry(
                uid, domain, interfaceName);
        onSuccess(updateSuccess);
    }
}

bool AccessControlListEditor::hasRoleMaster(const std::string& uid, const std::string& domain)
{
    return localDomainAccessController->hasRole(uid, domain, Role::MASTER);
}

bool AccessControlListEditor::hasRoleOwner(const std::string& uid, const std::string& domain)
{
    return localDomainAccessController->hasRole(uid, domain, Role::OWNER);
}

} // namespace joynr

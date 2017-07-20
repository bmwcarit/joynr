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

#ifndef ACCESSCONTROLLISTEDITOR_H
#define ACCESSCONTROLLISTEDITOR_H

#include <memory>

#include "joynr/infrastructure/AccessControlListEditorAbstractProvider.h"

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{
class LocalDomainAccessStore;
class LocalDomainAccessController;

class JOYNRCLUSTERCONTROLLER_EXPORT AccessControlListEditor
        : public joynr::infrastructure::AccessControlListEditorAbstractProvider
{
public:
    explicit AccessControlListEditor(
            std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore,
            std::shared_ptr<LocalDomainAccessController> localDomainAccessController);

    void updateMasterAccessControlEntry(
            const joynr::infrastructure::DacTypes::MasterAccessControlEntry& updatedMasterAce,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void removeMasterAccessControlEntry(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& operation,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void updateMediatorAccessControlEntry(
            const joynr::infrastructure::DacTypes::MasterAccessControlEntry& updatedMediatorAce,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void removeMediatorAccessControlEntry(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& operation,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void updateOwnerAccessControlEntry(
            const joynr::infrastructure::DacTypes::OwnerAccessControlEntry& updatedOwnerAce,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void removeOwnerAccessControlEntry(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& operation,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void updateMasterRegistrationControlEntry(
            const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMasterRce,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void removeMasterRegistrationControlEntry(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void updateMediatorRegistrationControlEntry(
            const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry&
                    updatedMediatorRce,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void removeMediatorRegistrationControlEntry(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void updateOwnerRegistrationControlEntry(
            const joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry& updatedOwnerRce,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    void removeOwnerRegistrationControlEntry(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfaceName,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

private:
    DISALLOW_COPY_AND_ASSIGN(AccessControlListEditor);
    bool hasRoleMaster(const std::string& uid, const std::string& domain);
    bool hasRoleOwner(const std::string& uid, const std::string& domain);

    std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore;
    std::shared_ptr<LocalDomainAccessController> localDomainAccessController;
};

} // namespace joynr
#endif // ACCESSCONTROLLISTEDITOR_H

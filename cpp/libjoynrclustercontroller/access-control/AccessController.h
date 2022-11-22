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

#ifndef ACCESSCONTROLLER_H
#define ACCESSCONTROLLER_H

#include <memory>
#include <string>
#include <vector>

#include "AccessControlAlgorithm.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/access-control/IAccessController.h"
#include "joynr/infrastructure/DacTypes/Permission.h"
#include "joynr/infrastructure/DacTypes/Role.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"
#include "joynr/types/DiscoveryQos.h"

namespace joynr
{

class ImmutableMessage;
class LocalCapabilitiesDirectory;
class LocalDomainAccessStore;

/**
 * Object that controls access to providers
 */
class AccessController : public IAccessController,
                         public std::enable_shared_from_this<AccessController>
{
public:
    class IGetPermissionCallback
    {
    public:
        virtual ~IGetPermissionCallback() = default;

        // Called with the result of a consumer / provider permission request
        virtual void permission(infrastructure::DacTypes::Permission::Enum permission) = 0;

        // Called when an operation is needed to get the consumer permission
        virtual void operationNeeded() = 0;
    };

    AccessController(std::shared_ptr<LocalCapabilitiesDirectory> localCapabilitiesDirectory,
                     std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore);

    ~AccessController() override = default;

    //---IAccessController interface -------------------------------------------

    void hasConsumerPermission(std::shared_ptr<ImmutableMessage> message,
                               std::shared_ptr<IHasConsumerPermissionCallback> callback,
                               bool isLocalRecipient) override;

    bool hasProviderPermission(const std::string& userId,
                               infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                               const std::string& domain,
                               const std::string& interfaceName) override;

    void addParticipantToWhitelist(const std::string& participantId) override;

    /**
     * Check if user uid has role role for domain.
     * Used by an ACL editor app to verify whether the user is allowed to change ACEs or not
     *
     * @param userId The user accessing the interface
     * @param domain The trust level of the device accessing the interface
     * @param role The domain that is being accessed
     * @return Returns true, if user uid has role role for domain domain.
     */
    virtual bool hasRole(const std::string& userId,
                         const std::string& domain,
                         infrastructure::DacTypes::Role::Enum role);

protected:
    class LdacConsumerPermissionCallback;

    /**
     * Get consumer permission to access an interface
     *
     * @param userId        The user accessing the interface
     * @param domain        The domain that is being accessed
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     * @param callbacks     Object that will receive the result and then be deleted
     *
     * Use :
     *    getConsumerPermission(String, String, String, String, TrustLevel, callbacks)
     * to gain exact Permission on interface operation.
     */
    virtual void getConsumerPermission(const std::string& userId,
                                       const std::string& domain,
                                       const std::string& interfaceName,
                                       infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                                       std::shared_ptr<IGetPermissionCallback> callback);

    /**
     * Get consumer permission to access an interface operation
     *
     * @param userId        The user accessing the interface
     * @param domain        The domain that is being accessed
     * @param interfaceName The interface that is being accessed
     * @param operation     The operation user requests to execute on interface
     * @param trustLevel    The trust level of the device accessing the interface
     * @return the permission.
     *
     * This synchronous function assumes that the data to do ACL checks is available
     * and has been obtained through a call to getConsumerPermission()
     */
    virtual infrastructure::DacTypes::Permission::Enum getConsumerPermission(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& operation,
            infrastructure::DacTypes::TrustLevel::Enum trustLevel);

    /**
     * Get provider permission to register for an interface
     *
     * @param userId        The user registering for the interface
     * @param domain        The domain that is being registered for
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     * @param callbacks     Object that will receive the result and then be deleted
     *
     * Use :
     *    getProviderPermission(String, String, String, TrustLevel, callbacks)
     * to gain exact Permission on interface registration.
     */
    virtual void getProviderPermission(const std::string& userId,
                                       const std::string& domain,
                                       const std::string& interfaceName,
                                       infrastructure::DacTypes::TrustLevel::Enum trustLevel,
                                       std::shared_ptr<IGetPermissionCallback> callback);

    /**
     * Get provider permission to expose an interface
     *
     * @param uid        The userId of the provider exposing the interface
     * @param domain        The domain where interface belongs
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     */
    virtual infrastructure::DacTypes::Permission::Enum getProviderPermission(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfacename,
            infrastructure::DacTypes::TrustLevel::Enum trustLevel);

    AccessControlAlgorithm _accessControlAlgorithm;

    DISALLOW_COPY_AND_ASSIGN(AccessController);
    bool needsHasConsumerPermissionCheck(const ImmutableMessage& message) const;
    bool needsHasProviderPermissionCheck() const;

    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectory;
    std::shared_ptr<LocalDomainAccessStore> _localDomainAccessStore;
    std::vector<std::string> _whitelistParticipantIds;
    types::DiscoveryQos _discoveryQos;
    types::DiscoveryQos _discoveryQosWithLocalOnlyScope;

    ADD_LOGGER(AccessController)
};

} // namespace joynr
#endif // IACCESSCONTROLLER_H

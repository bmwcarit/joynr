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

#ifndef LOCALDOMAINACCESSCONTROLLER_H
#define LOCALDOMAINACCESSCONTROLLER_H

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <chrono>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/Permission.h"
#include "joynr/infrastructure/DacTypes/TrustLevel.h"
#include "joynr/infrastructure/DacTypes/Role.h"
#include "joynr/ISubscriptionListener.h"
#include "AccessControlAlgorithm.h"
#include "LocalDomainAccessStore.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"
#include "joynr/Future.h"

namespace joynr
{
namespace infrastructure
{
class GlobalDomainAccessControllerProxy;
} // namespace infrastructure

/**
 * Object that controls access to providers
 */
class JOYNRCLUSTERCONTROLLER_EXPORT LocalDomainAccessController
{
public:
    /**
     * The LocalDomainAccessController gets consumer permissions asynchronously.
     * When using the LocalDomainAccessController the caller provides a callback object.
     */
    class IGetConsumerPermissionCallback
    {
    public:
        virtual ~IGetConsumerPermissionCallback() = default;

        // Called with the result of a consumer permission request
        virtual void consumerPermission(infrastructure::DacTypes::Permission::Enum permission) = 0;

        // Called when an operation is needed to get the consumer permission
        virtual void operationNeeded() = 0;
    };

    explicit LocalDomainAccessController(
            std::unique_ptr<LocalDomainAccessStore> localDomainAccessStore);
    virtual ~LocalDomainAccessController() = default;
    /**
     * The init method has to be called first, only afterwards LocalDomainAccessController may be
     * used.
     */
    void init(std::shared_ptr<infrastructure::GlobalDomainAccessControllerProxy>
                      globalDomainAccessControllerProxy);

    /**
     * Check if user uid has role role for domain.
     * Used by an ACL editor app to verify whether the user is allowed to change ACEs or not
     *
     * @param userId The user accessing the interface
     * @param domain The trust level of the device accessing the interface
     * @param role The domain that is being accessed
     * @return Returns true, if user uid has role role for domain domain.
     */
    bool hasRole(const std::string& userId,
                 const std::string& domain,
                 infrastructure::DacTypes::Role::Enum role);

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
                                       std::shared_ptr<IGetConsumerPermissionCallback> callback);

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
     * Returns a list of entries that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * Used by an Master ACL GUI to show access rights of a user.
     *
     * @param uid The userId of the caller.
     * @return A list of master ACEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMasterAccessControlEntries(
            const std::string& uid);

    /**
     * Returns a list of editable master access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Master ACL GUI to show access rights of a user.
     *
     * @param uid The userId of the caller.
     * @return A list of editable master ACEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry>
    getEditableMasterAccessControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     * existent.
     *
     * @param updatedMasterAce The master ACE that has to be updated/added to the ACL store.
     * @return true if update succeeded.
     */
    bool updateMasterAccessControlEntry(
            const infrastructure::DacTypes::MasterAccessControlEntry& updatedMasterAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid The userId of the control entry.
     * @param domain The domain of the control entry.
     * @param interfaceName The interfaceName of the control entry.
     * @param operation The operation of the control entry.
     * @return true if remove succeeded.
     */
    bool removeMasterAccessControlEntry(const std::string& uid,
                                        const std::string& domain,
                                        const std::string& interfaceName,
                                        const std::string& operation);

    /**
     * Returns a list of entries that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * Used by an Mediator ACL GUI to show access rights of a user.
     *
     * @param uid The userId of the caller.
     * @return A list of mediator ACEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry> getMediatorAccessControlEntries(
            const std::string& uid);

    /**
     * Returns a list of editable mediator access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Mediator ACL GUI to show access rights of a user.
     * Calling this function blocks calling thread until update operation finish.
     *
     * @param uid The userId of the caller.
     * @return A list of editable mediator ACEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterAccessControlEntry>
    getEditableMediatorAccessControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * @param updatedMediatorAce The mediator ACE that has to be updated/added to the ACL store.
     * @return true if update succeeded.
     */
    bool updateMediatorAccessControlEntry(
            const infrastructure::DacTypes::MasterAccessControlEntry& updatedMediatorAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid The userId of the control entry.
     * @param domain The domain of the control entry.
     * @param interfaceName The interfaceName of the control entry.
     * @param operation The operation of the control entry.
     * @return true if remove succeeded.
     */
    bool removeMediatorAccessControlEntry(const std::string& uid,
                                          const std::string& domain,
                                          const std::string& interfaceName,
                                          const std::string& operation);

    /**
     * Returns a list of entries that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * Used by an Owner ACL GUI to show access rights of a user.
     *
     * @param uid The userId of the caller.
     * @return A list of owner ACEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::OwnerAccessControlEntry> getOwnerAccessControlEntries(
            const std::string& uid);

    /**
     * Returns a list of editable owner access control entries that apply to user uid,
     * i.e. the entries for which uid has role OWNER.
     * Used by an Owner ACL GUI to show access rights of a user.
     * Calling this function blocks calling thread until update operation finish.
     *
     * @param uid The userId of the caller.
     * @return A list of editable owner ACEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::OwnerAccessControlEntry>
    getEditableOwnerAccessControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * @param updatedOwnerAce The owner ACE that has to be updated/added to the ACL store.
     * @return true if update succeeded.
     */
    bool updateOwnerAccessControlEntry(
            const infrastructure::DacTypes::OwnerAccessControlEntry& updatedOwnerAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid The userId of the control entry.
     * @param domain The domain of the control entry.
     * @param interfaceName The interfaceName of the control entry.
     * @param operation The operation of the control entry.
     * @return true if remove succeeded.
     */
    bool removeOwnerAccessControlEntry(const std::string& uid,
                                       const std::string& domain,
                                       const std::string& interfaceName,
                                       const std::string& operation);

    /**
     * Get provider permission to expose an interface
     *
     * @param uid        The userId of the provider exposing the interface
     * @param domain        The domain where interface belongs
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     */
    infrastructure::DacTypes::Permission::Enum getProviderPermission(
            const std::string& uid,
            const std::string& domain,
            const std::string& interfacename,
            infrastructure::DacTypes::TrustLevel::Enum trustLevel);

    /**
     * Returns a list of master registration control entries that apply to provider uid, i.e. the
     *entries that define the registration rights of the provider uid.
     * Used by an Master RCL GUI to show registration rights of a provider.
     * Calling this function blocks calling thread until update operation finish.
     *
     * @param uid The provider userId.
     * @return A list of master RCEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getMasterRegistrationControlEntries(const std::string& uid);

    /**
     * Returns a list of editable master registration entries applying to domains the user uid has
     *role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Master ACL editor app.
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Master.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getEditableMasterRegistrationControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * @param updatedMasterRce The master RCE to be updated.
     * @return true if update succeeded.
     */
    bool updateMasterRegistrationControlEntry(
            const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMasterRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded.
     */
    bool removeMasterRegistrationControlEntry(const std::string& uid,
                                              const std::string& domain,
                                              const std::string& interfaceName);

    /**
     * Returns a list of mediator registration control entries that apply to provider uid, i.e. the
     *entries that define the registration rights of the provider uid.
     * Used by an Mediator RCL GUI to show registration rights of a provider.
     * Calling this function blocks calling thread until update operation finish.
     *
     * @param uid The provider userId.
     * @return A list of mediator RCEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getMediatorRegistrationControlEntries(const std::string& uid);

    /**
     * Returns a list of editable mediator registration entries applying to domains the user uid has
     *role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Mediator ACL editor app.
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Master.
     */
    std::vector<infrastructure::DacTypes::MasterRegistrationControlEntry>
    getEditableMediatorRegistrationControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * @param updatedMediatorRce The mediator RCE to be updated.
     * @return true if update succeeded.
     */
    bool updateMediatorRegistrationControlEntry(
            const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMediatorRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded.
     */
    bool removeMediatorRegistrationControlEntry(const std::string& uid,
                                                const std::string& domain,
                                                const std::string& interfaceName);

    /**
     * Returns a list of owner registration control entries that apply to provider uid, i.e. the
     *entries that define the registration rights of the provider uid.
     * Used by an Owner RCL GUI to show registration rights of a provider.
     * Calling this function blocks calling thread until update operation finish.
     *
     * @param uid The provider userId.
     * @return A list of owner RCEs for specified uid.
     */
    std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry>
    getOwnerRegistrationControlEntries(const std::string& uid);

    /**
     * Returns a list of editable owner registration entries applying to domains the user uid has
     *role Owner,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Owner.
     */
    std::vector<infrastructure::DacTypes::OwnerRegistrationControlEntry>
    getEditableOwnerRegistrationControlEntries(const std::string& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * @param updatedOwnerRce The owner RCE to be updated.
     * @return true if update succeeded.
     */
    bool updateOwnerRegistrationControlEntry(
            const infrastructure::DacTypes::OwnerRegistrationControlEntry& updatedOwnerRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded.
     */
    bool removeOwnerRegistrationControlEntry(const std::string& uid,
                                             const std::string& domain,
                                             const std::string& interfaceName);

    /**
     * Unregisters a domain/interface from access control.
     * @param domain The domain of the provider being unregistered.
     * @param interfaceName The interface of the provider being unregistered
     */
    void unregisterProvider(const std::string& domain, const std::string& interfaceName);

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessController);

    AccessControlAlgorithm accessControlAlgorithm;
    std::unordered_map<std::string, std::shared_ptr<Future<std::string>>> dreSubscriptions;

    struct AceSubscription
    {
        std::shared_ptr<Future<std::string>> masterAceSubscriptionIdFuture;
        std::shared_ptr<Future<std::string>> mediatorAceSubscriptionIdFuture;
        std::shared_ptr<Future<std::string>> ownerAceSubscriptionIdFuture;

        const std::string getMasterAceSubscriptionId()
        {
            std::string masterAceSubscriptionId;
            masterAceSubscriptionIdFuture->get(1000, masterAceSubscriptionId);
            return masterAceSubscriptionId;
        }

        const std::string getMediatorAceSubscriptionId()
        {
            std::string mediatorAceSubscriptionId;
            mediatorAceSubscriptionIdFuture->get(1000, mediatorAceSubscriptionId);
            return mediatorAceSubscriptionId;
        }

        const std::string getOwnerAceSubscriptionId()
        {
            std::string ownerAceSubscriptionId;
            ownerAceSubscriptionIdFuture->get(1000, ownerAceSubscriptionId);
            return ownerAceSubscriptionId;
        }

        AceSubscription()
                : masterAceSubscriptionIdFuture(),
                  mediatorAceSubscriptionIdFuture(),
                  ownerAceSubscriptionIdFuture()
        {
        }
    };

    std::unordered_map<std::string, AceSubscription> aceSubscriptions;

    std::shared_ptr<infrastructure::GlobalDomainAccessControllerProxy>
            globalDomainAccessControllerProxy;
    std::unique_ptr<LocalDomainAccessStore> localDomainAccessStore;

    ADD_LOGGER(LocalDomainAccessController);
    static std::chrono::milliseconds broadcastSubscriptionValidity;

    void initialiseLocalDomainAccessStore(const std::string& userId,
                                          const std::string& domain,
                                          const std::string& interfaceName);

    void initialised(const std::string& domain, const std::string& interfaceName);
    void abortInitialisation(const std::string& domain, const std::string& interfaceName);

    std::shared_ptr<Future<std::string>> subscribeForDreChange(const std::string& userId);
    AceSubscription subscribeForAceChange(const std::string& domain,
                                          const std::string& interfaceName);
    std::string createCompoundKey(const std::string& domain, const std::string& interfaceName);

    // Requests waiting to get consumer permission
    struct ConsumerPermissionRequest
    {
        std::string userId;
        std::string domain;
        std::string interfaceName;
        infrastructure::DacTypes::TrustLevel::Enum trustLevel;
        std::shared_ptr<IGetConsumerPermissionCallback> callbacks;
    };

    std::unordered_map<std::string, std::vector<ConsumerPermissionRequest>>
            consumerPermissionRequests;

    bool queueConsumerRequest(const std::string& key, const ConsumerPermissionRequest& request);
    void processConsumerRequests(const std::vector<ConsumerPermissionRequest>& requests);

    // Mutex that protects all member variables involved in initialisation
    // of data for a domain/interface
    // - aceSubscriptions
    // - consumerPermissionRequests
    std::mutex initStateMutex;

    // Class that keeps track of initialisation for a domain/interface
    class Initialiser;

    // Classes used to receive broadcasts

    class DomainRoleEntryChangedBroadcastListener;
    std::shared_ptr<DomainRoleEntryChangedBroadcastListener>
            domainRoleEntryChangedBroadcastListener;

    class MasterAccessControlEntryChangedBroadcastListener;
    std::shared_ptr<MasterAccessControlEntryChangedBroadcastListener>
            masterAccessControlEntryChangedBroadcastListener;

    class MediatorAccessControlEntryChangedBroadcastListener;
    std::shared_ptr<MediatorAccessControlEntryChangedBroadcastListener>
            mediatorAccessControlEntryChangedBroadcastListener;

    class OwnerAccessControlEntryChangedBroadcastListener;
    std::shared_ptr<OwnerAccessControlEntryChangedBroadcastListener>
            ownerAccessControlEntryChangedBroadcastListener;

    static std::string sanitizeForPartition(const std::string& value);
};
} // namespace joynr
#endif // LOCALDOMAINACCESSCONTROLLER_H

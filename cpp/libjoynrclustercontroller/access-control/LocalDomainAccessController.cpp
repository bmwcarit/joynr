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

#include <atomic>
#include <cassert>
#include <regex>
#include <tuple>

#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/infrastructure/DacTypes/DomainRoleEntry.h"
#include "joynr/infrastructure/GlobalDomainAccessControlListEditorProxy.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerProxy.h"
#include "joynr/infrastructure/GlobalDomainRoleControllerProxy.h"

#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

namespace joynr
{

using namespace infrastructure;
using namespace infrastructure::DacTypes;

// 10 years
std::chrono::milliseconds LocalDomainAccessController::broadcastSubscriptionValidity =
        std::chrono::hours(24) * 365 * 10;

//--- Declarations of nested classes -------------------------------------------

class LocalDomainAccessController::Initializer
{
public:
    Initializer(LocalDomainAccessController& parent,
                const std::string& domain,
                const std::string& interfaceName,
                const std::uint8_t steps,
                const bool handleAces,
                const bool handleRces)
            : counter(steps),
              aborted(false),
              parent(parent),
              domain(domain),
              interfaceName(interfaceName),
              handleAces(handleAces),
              handleRces(handleRces)
    {
    }

    // Called to indicate that some data has been initialized
    void update()
    {
        std::uint8_t prevValue = counter--;
        if (prevValue == 1) {
            // Initialization has finished
            if (aborted) {
                parent.abortInitialization(domain, interfaceName, handleAces, handleRces);
            } else {
                parent.initialized(domain, interfaceName, handleAces, handleRces);
            }
        }
    }

    // Called to abort the initialisation
    void abort()
    {
        aborted = true;
        update();
    }

private:
    std::atomic<std::uint8_t> counter;
    std::atomic<bool> aborted;
    LocalDomainAccessController& parent;

    const std::string domain;
    const std::string interfaceName;
    const bool handleAces;
    const bool handleRces;
};

class LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::ChangeType::Enum,
                                       infrastructure::DacTypes::DomainRoleEntry>
{
public:
    explicit DomainRoleEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onSubscribed(const std::string& subscriptionId) override;
    void onReceive(const infrastructure::DacTypes::ChangeType::Enum& changeType,
                   const infrastructure::DacTypes::DomainRoleEntry& changedDre) override;
    void onError(const exceptions::JoynrRuntimeException& error) override;

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::ChangeType::Enum,
                                       infrastructure::DacTypes::MasterAccessControlEntry>
{
public:
    explicit MasterAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onSubscribed(const std::string& subscriptionId) override;
    void onReceive(
            const infrastructure::DacTypes::ChangeType::Enum& changeType,
            const infrastructure::DacTypes::MasterAccessControlEntry& changedMasterAce) override;
    void onError(const exceptions::JoynrRuntimeException& error) override;

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::ChangeType::Enum,
                                       infrastructure::DacTypes::MasterAccessControlEntry>
{
public:
    explicit MediatorAccessControlEntryChangedBroadcastListener(
            LocalDomainAccessController& parent);
    void onSubscribed(const std::string& subscriptionId) override;
    void onReceive(
            const infrastructure::DacTypes::ChangeType::Enum& changeType,
            const infrastructure::DacTypes::MasterAccessControlEntry& changedMediatorAce) override;
    void onError(const exceptions::JoynrRuntimeException& error) override;

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::ChangeType::Enum,
                                       infrastructure::DacTypes::OwnerAccessControlEntry>
{
public:
    explicit OwnerAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onSubscribed(const std::string& subscriptionId) override;
    void onReceive(
            const infrastructure::DacTypes::ChangeType::Enum& changeType,
            const infrastructure::DacTypes::OwnerAccessControlEntry& changedOwnerAce) override;
    void onError(const exceptions::JoynrRuntimeException& error) override;

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MasterRegistrationControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::ChangeType::Enum,
                                       infrastructure::DacTypes::MasterRegistrationControlEntry>
{
public:
    explicit MasterRegistrationControlEntryChangedBroadcastListener(
            LocalDomainAccessController& parent);
    void onSubscribed(const std::string& subscriptionId) override;
    void onReceive(const infrastructure::DacTypes::ChangeType::Enum& changeType,
                   const infrastructure::DacTypes::MasterRegistrationControlEntry& changedMasterRce)
            override;
    void onError(const exceptions::JoynrRuntimeException& error) override;

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MediatorRegistrationControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::ChangeType::Enum,
                                       infrastructure::DacTypes::MasterRegistrationControlEntry>
{
public:
    explicit MediatorRegistrationControlEntryChangedBroadcastListener(
            LocalDomainAccessController& parent);
    void onSubscribed(const std::string& subscriptionId) override;
    void onReceive(const infrastructure::DacTypes::ChangeType::Enum& changeType,
                   const infrastructure::DacTypes::MasterRegistrationControlEntry&
                           changedMediatorRce) override;
    void onError(const exceptions::JoynrRuntimeException& error) override;

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::OwnerRegistrationControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::ChangeType::Enum,
                                       infrastructure::DacTypes::OwnerRegistrationControlEntry>
{
public:
    explicit OwnerRegistrationControlEntryChangedBroadcastListener(
            LocalDomainAccessController& parent);
    void onSubscribed(const std::string& subscriptionId) override;
    void onReceive(const infrastructure::DacTypes::ChangeType::Enum& changeType,
                   const infrastructure::DacTypes::OwnerRegistrationControlEntry& changedOwnerRce)
            override;
    void onError(const exceptions::JoynrRuntimeException& error) override;

private:
    LocalDomainAccessController& parent;
};

//--- LocalDomainAccessController ----------------------------------------------

LocalDomainAccessController::LocalDomainAccessController(
        std::shared_ptr<LocalDomainAccessStore> localDomainAccessStore,
        bool useOnlyLocalDomainAccessStore)
        : accessControlAlgorithm(),
          dreSubscriptions(),
          aceSubscriptions(),
          rceSubscriptions(),
          globalDomainAccessControllerProxy(),
          globalDomainAccessControlListEditorProxy(),
          globalDomainRoleControllerProxy(),
          localDomainAccessStore(std::move(localDomainAccessStore)),
          useOnlyLocalDomainAccessStore(useOnlyLocalDomainAccessStore),
          consumerPermissionRequests(),
          initStateMutex(),
          domainRoleEntryChangedBroadcastListener(
                  std::make_shared<DomainRoleEntryChangedBroadcastListener>(*this)),
          masterAccessControlEntryChangedBroadcastListener(
                  std::make_shared<MasterAccessControlEntryChangedBroadcastListener>(*this)),
          mediatorAccessControlEntryChangedBroadcastListener(
                  std::make_shared<MediatorAccessControlEntryChangedBroadcastListener>(*this)),
          ownerAccessControlEntryChangedBroadcastListener(
                  std::make_shared<OwnerAccessControlEntryChangedBroadcastListener>(*this)),
          masterRegistrationControlEntryChangedBroadcastListener(
                  std::make_shared<MasterRegistrationControlEntryChangedBroadcastListener>(*this)),
          mediatorRegistrationControlEntryChangedBroadcastListener(
                  std::make_shared<MediatorRegistrationControlEntryChangedBroadcastListener>(
                          *this)),
          ownerRegistrationControlEntryChangedBroadcastListener(
                  std::make_shared<OwnerRegistrationControlEntryChangedBroadcastListener>(*this)),
          multicastSubscriptionQos(std::make_shared<joynr::MulticastSubscriptionQos>(
                  broadcastSubscriptionValidity.count()))
{
}

void LocalDomainAccessController::setGlobalDomainAccessControllerProxy(
        std::shared_ptr<GlobalDomainAccessControllerProxy> globalDomainAccessControllerProxy)
{
    this->globalDomainAccessControllerProxy = std::move(globalDomainAccessControllerProxy);

    // Iterate over all domain/interfaces in LocalDomainAccessStore and create for each a
    // subscription
    auto uniqueDomainInterfaceCombinations =
            localDomainAccessStore->getUniqueDomainInterfaceCombinations();
    const bool restoringFromFile = true;
    const bool handleAces = true;
    const bool handleRces = true;
    for (const auto& domainInterfacePair : uniqueDomainInterfaceCombinations) {
        initialized(domainInterfacePair.first,
                    domainInterfacePair.second,
                    handleAces,
                    handleRces,
                    restoringFromFile);
    }
}

void LocalDomainAccessController::setGlobalDomainAccessControlListEditorProxy(std::shared_ptr<
        GlobalDomainAccessControlListEditorProxy> globalDomainAccessControlListEditorProxy)
{
    this->globalDomainAccessControlListEditorProxy =
            std::move(globalDomainAccessControlListEditorProxy);
}

void LocalDomainAccessController::setGlobalDomainRoleControllerProxy(
        std::shared_ptr<GlobalDomainRoleControllerProxy> globalDomainRoleControllerProxy)
{
    this->globalDomainRoleControllerProxy = std::move(globalDomainRoleControllerProxy);
}

bool LocalDomainAccessController::hasRole(const std::string& userId,
                                          const std::string& domain,
                                          Role::Enum role)
{
    JOYNR_LOG_TRACE(logger(), "execute: entering hasRole");

    // See if the user has the given role
    bool hasRole = false;
    boost::optional<DomainRoleEntry> dre = localDomainAccessStore->getDomainRole(userId, role);
    if (dre) {
        std::vector<std::string> domains = dre->getDomains();
        const std::string wildcard = "*";
        if (util::vectorContains(domains, domain) || util::vectorContains(domains, wildcard)) {
            hasRole = true;
        }
    }

    // Subscribe changes in the users roles
    if (!useOnlyLocalDomainAccessStore && dreSubscriptions.count(userId) == 0) {
        dreSubscriptions.insert(std::make_pair(userId, subscribeForDreChange(userId)));
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

    // If we only use the local domain access store, we assume that all required ACEs
    // were already provisioned. Otherwise we need to query them from the backend.
    if (!useOnlyLocalDomainAccessStore) {
        // Is the ACL for this domain/interface available?
        std::string compoundKey = createCompoundKey(domain, interfaceName);
        bool needsInit = false;
        {
            std::lock_guard<std::mutex> lock(initStateMutex);
            if (aceSubscriptions.count(compoundKey) == 0) {
                // Queue the request
                ConsumerPermissionRequest request = {
                        userId, domain, interfaceName, trustLevel, callback};

                if (queueConsumerRequest(compoundKey, request)) {
                    // There are requests already queued - do nothing
                    return;
                } else {
                    // There are no requests already queued - further
                    // initialisation is needed
                    needsInit = true;
                }
            }
        }

        // Do further initialisation outside of the mutex to prevent deadlocks
        if (needsInit) {
            // Get the data for this domain interface and do not wait for it
            initializeLocalDomainAccessStoreAces(domain, interfaceName);

            // Init domain roles as well
            initializeDomainRoleTable(userId);

            return;
        }
    }

    // If this point is reached the data for the ACL check is available

    // The operations of the ACEs should only contain wildcards, if not
    // getConsumerPermission should be called with an operation
    if (!localDomainAccessStore->onlyWildcardOperations(userId, domain, interfaceName)) {
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
            localDomainAccessStore->getMasterAccessControlEntry(
                    userId, domain, interfaceName, operation);
    boost::optional<MasterAccessControlEntry> mediatorAceOptional =
            localDomainAccessStore->getMediatorAccessControlEntry(
                    userId, domain, interfaceName, operation);
    boost::optional<OwnerAccessControlEntry> ownerAceOptional =
            localDomainAccessStore->getOwnerAccessControlEntry(
                    userId, domain, interfaceName, operation);

    return accessControlAlgorithm.getConsumerPermission(
            masterAceOptional, mediatorAceOptional, ownerAceOptional, trustLevel);
}

std::vector<MasterAccessControlEntry> LocalDomainAccessController::getMasterAccessControlEntries(
        const std::string& uid)
{
    std::vector<MasterAccessControlEntry> resultMasterAces;
    globalDomainAccessControllerProxy->getMasterAccessControlEntries(resultMasterAces, uid);
    return resultMasterAces;
}

std::vector<MasterAccessControlEntry> LocalDomainAccessController::
        getEditableMasterAccessControlEntries(const std::string& uid)
{
    std::vector<MasterAccessControlEntry> resultMasterAces;
    globalDomainAccessControlListEditorProxy->getEditableMasterAccessControlEntries(
            resultMasterAces, uid);
    return resultMasterAces;
}

bool LocalDomainAccessController::updateMasterAccessControlEntry(
        const MasterAccessControlEntry& updatedMasterAce)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->updateMasterAccessControlEntry(
            success, updatedMasterAce);
    return success;
}

bool LocalDomainAccessController::removeMasterAccessControlEntry(const std::string& uid,
                                                                 const std::string& domain,
                                                                 const std::string& interfaceName,
                                                                 const std::string& operation)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->removeMasterAccessControlEntry(
            success, uid, domain, interfaceName, operation);
    return success;
}

std::vector<MasterAccessControlEntry> LocalDomainAccessController::getMediatorAccessControlEntries(
        const std::string& uid)
{
    std::vector<MasterAccessControlEntry> resultMediatorAces;
    globalDomainAccessControllerProxy->getMediatorAccessControlEntries(resultMediatorAces, uid);
    return resultMediatorAces;
}

std::vector<MasterAccessControlEntry> LocalDomainAccessController::
        getEditableMediatorAccessControlEntries(const std::string& uid)
{
    std::vector<MasterAccessControlEntry> resultMediatorAces;
    globalDomainAccessControlListEditorProxy->getEditableMediatorAccessControlEntries(
            resultMediatorAces, uid);
    return resultMediatorAces;
}

bool LocalDomainAccessController::updateMediatorAccessControlEntry(
        const MasterAccessControlEntry& updatedMediatorAce)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->updateMediatorAccessControlEntry(
            success, updatedMediatorAce);
    return success;
}

bool LocalDomainAccessController::removeMediatorAccessControlEntry(const std::string& uid,
                                                                   const std::string& domain,
                                                                   const std::string& interfaceName,
                                                                   const std::string& operation)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->removeMediatorAccessControlEntry(
            success, uid, domain, interfaceName, operation);
    return success;
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessController::getOwnerAccessControlEntries(
        const std::string& uid)
{
    std::vector<OwnerAccessControlEntry> resultOwnerAces;
    globalDomainAccessControllerProxy->getOwnerAccessControlEntries(resultOwnerAces, uid);
    return resultOwnerAces;
}

std::vector<OwnerAccessControlEntry> LocalDomainAccessController::
        getEditableOwnerAccessControlEntries(const std::string& uid)
{
    std::vector<OwnerAccessControlEntry> resultOwnerAces;
    globalDomainAccessControlListEditorProxy->getEditableOwnerAccessControlEntries(
            resultOwnerAces, uid);
    return resultOwnerAces;
}

bool LocalDomainAccessController::updateOwnerAccessControlEntry(
        const OwnerAccessControlEntry& updatedOwnerAce)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->updateOwnerAccessControlEntry(
            success, updatedOwnerAce);
    return success;
}

bool LocalDomainAccessController::removeOwnerAccessControlEntry(const std::string& uid,
                                                                const std::string& domain,
                                                                const std::string& interfaceName,
                                                                const std::string& operation)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->removeOwnerAccessControlEntry(
            success, uid, domain, interfaceName, operation);
    return success;
}

// Returns true if other requests have already been queued
bool LocalDomainAccessController::queueProviderRequest(const std::string& key,
                                                       const ProviderPermissionRequest& request)
{
    // This function assumes that the initStateMutex has already been obtained

    if (providerPermissionRequests.count(key) != 0) {
        providerPermissionRequests[key].push_back(request);
        return true;
    } else {
        std::vector<ProviderPermissionRequest> requestList;
        requestList.push_back(request);
        providerPermissionRequests.insert(std::make_pair(key, requestList));
        return false;
    }
}

void LocalDomainAccessController::getProviderPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        TrustLevel::Enum trustLevel,
        std::shared_ptr<IGetPermissionCallback> callback)
{
    JOYNR_LOG_TRACE(logger(), "Entering getProviderPermission with callback");

    // If we only use the local domain access store, we assume that all required RCEs
    // were already provisioned. Otherwise we need to query them from the backend.
    if (!useOnlyLocalDomainAccessStore) {
        // Is the RCL for this domain/interface available?
        std::string compoundKey = createCompoundKey(domain, interfaceName);
        bool needsInit = false;
        {
            std::lock_guard<std::mutex> lock(initStateMutex);
            if (rceSubscriptions.count(compoundKey) == 0) {
                // Queue the request
                ProviderPermissionRequest request = {
                        userId, domain, interfaceName, trustLevel, callback};

                if (queueProviderRequest(compoundKey, request)) {
                    // There are requests already queued - do nothing
                    return;
                } else {
                    // There are no requests already queued - further
                    // initialisation is needed
                    needsInit = true;
                }
            }
        }

        // Do further initialisation outside of the mutex to prevent deadlocks
        if (needsInit) {
            // Get the data for this domain interface and do not wait for it
            initializeLocalDomainAccessStoreRces(userId, domain, interfaceName);

            // Init domain roles as well
            initializeDomainRoleTable(userId);

            return;
        }
    }

    // If this point is reached the data for the RCL check is available

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
            localDomainAccessStore->getMasterRegistrationControlEntry(uid, domain, interfaceName);
    boost::optional<MasterRegistrationControlEntry> mediatorRceOptional =
            localDomainAccessStore->getMediatorRegistrationControlEntry(uid, domain, interfaceName);
    boost::optional<OwnerRegistrationControlEntry> ownerRceOptional =
            localDomainAccessStore->getOwnerRegistrationControlEntry(uid, domain, interfaceName);

    return accessControlAlgorithm.getProviderPermission(
            masterRceOptional, mediatorRceOptional, ownerRceOptional, trustLevel);
}

std::vector<MasterRegistrationControlEntry> LocalDomainAccessController::
        getMasterRegistrationControlEntries(const std::string& uid)
{
    std::vector<MasterRegistrationControlEntry> resultMasterRces;
    globalDomainAccessControllerProxy->getMasterRegistrationControlEntries(resultMasterRces, uid);
    return resultMasterRces;
}

std::vector<MasterRegistrationControlEntry> LocalDomainAccessController::
        getEditableMasterRegistrationControlEntries(const std::string& uid)
{
    std::vector<MasterRegistrationControlEntry> resultMasterRces;
    globalDomainAccessControlListEditorProxy->getEditableMasterRegistrationControlEntries(
            resultMasterRces, uid);
    return resultMasterRces;
}

bool LocalDomainAccessController::updateMasterRegistrationControlEntry(
        const MasterRegistrationControlEntry& updatedMasterRce)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->updateMasterRegistrationControlEntry(
            success, updatedMasterRce);
    return success;
}

bool LocalDomainAccessController::removeMasterRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->removeMasterRegistrationControlEntry(
            success, uid, domain, interfaceName);
    return success;
}

std::vector<MasterRegistrationControlEntry> LocalDomainAccessController::
        getMediatorRegistrationControlEntries(const std::string& uid)
{
    std::vector<MasterRegistrationControlEntry> resultMasterRces;
    globalDomainAccessControllerProxy->getMediatorRegistrationControlEntries(resultMasterRces, uid);
    return resultMasterRces;
}

std::vector<MasterRegistrationControlEntry> LocalDomainAccessController::
        getEditableMediatorRegistrationControlEntries(const std::string& uid)
{
    std::vector<MasterRegistrationControlEntry> resultMasterRces;
    globalDomainAccessControlListEditorProxy->getEditableMasterRegistrationControlEntries(
            resultMasterRces, uid);
    return resultMasterRces;
}

bool LocalDomainAccessController::updateMediatorRegistrationControlEntry(
        const MasterRegistrationControlEntry& updatedMediatorRce)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->updateMediatorRegistrationControlEntry(
            success, updatedMediatorRce);
    return success;
}

bool LocalDomainAccessController::removeMediatorRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->removeMediatorRegistrationControlEntry(
            success, uid, domain, interfaceName);
    return success;
}

std::vector<OwnerRegistrationControlEntry> LocalDomainAccessController::
        getOwnerRegistrationControlEntries(const std::string& uid)
{
    std::vector<OwnerRegistrationControlEntry> resultOwnerRces;
    globalDomainAccessControllerProxy->getOwnerRegistrationControlEntries(resultOwnerRces, uid);
    return resultOwnerRces;
}

std::vector<OwnerRegistrationControlEntry> LocalDomainAccessController::
        getEditableOwnerRegistrationControlEntries(const std::string& uid)
{
    std::vector<OwnerRegistrationControlEntry> resultOwnerRces;
    globalDomainAccessControlListEditorProxy->getEditableOwnerRegistrationControlEntries(
            resultOwnerRces, uid);
    return resultOwnerRces;
}

bool LocalDomainAccessController::updateOwnerRegistrationControlEntry(
        const OwnerRegistrationControlEntry& updatedOwnerRce)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->updateOwnerRegistrationControlEntry(
            success, updatedOwnerRce);
    return success;
}

bool LocalDomainAccessController::removeOwnerRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    bool success = false;
    globalDomainAccessControlListEditorProxy->removeOwnerRegistrationControlEntry(
            success, uid, domain, interfaceName);
    return success;
}

void LocalDomainAccessController::unregisterProvider(const std::string& domain,
                                                     const std::string& interfaceName)
{
    if (useOnlyLocalDomainAccessStore) {
        return;
    }

    std::string compoundKey = createCompoundKey(domain, interfaceName);
    AceSubscription aceSubscriptionIds;
    bool subscriptionsFound = false;
    {
        std::lock_guard<std::mutex> lock(initStateMutex);
        if (aceSubscriptions.count(compoundKey) > 0) {
            // Get the subscription ids
            aceSubscriptionIds = aceSubscriptions[compoundKey];
            aceSubscriptions.erase(compoundKey);
            subscriptionsFound = true;
        }
    }
    if (subscriptionsFound) {
        JOYNR_LOG_TRACE(logger(),
                        "Unsubscribing from ACL broadcasts for domain {}, interface {}",
                        domain,
                        interfaceName);

        // Unsubscribe from ACE change subscriptions
        try {
            std::string masterAceSubscriptionId = aceSubscriptionIds.getMasterAceSubscriptionId();
            globalDomainAccessControllerProxy
                    ->unsubscribeFromMasterAccessControlEntryChangedBroadcast(
                            masterAceSubscriptionId);
        } catch (const exceptions::JoynrException& error) {
            JOYNR_LOG_ERROR(logger(),
                            "Unsubscribe from MasterAccessControlEntryChangedBroadcast failed: " +
                                    error.getMessage());
        }
        try {
            std::string mediatorAceSubscriptionId =
                    aceSubscriptionIds.getMediatorAceSubscriptionId();
            globalDomainAccessControllerProxy
                    ->unsubscribeFromMediatorAccessControlEntryChangedBroadcast(
                            mediatorAceSubscriptionId);
        } catch (const exceptions::JoynrException& error) {
            JOYNR_LOG_ERROR(logger(),
                            "Unsubscribe from MediatorAccessControlEntryChangedBroadcast failed: " +
                                    error.getMessage());
        }
        try {
            std::string ownerAceSubscriptionId = aceSubscriptionIds.getOwnerAceSubscriptionId();
            globalDomainAccessControllerProxy
                    ->unsubscribeFromOwnerAccessControlEntryChangedBroadcast(
                            ownerAceSubscriptionId);
        } catch (const exceptions::JoynrException& error) {
            JOYNR_LOG_ERROR(logger(),
                            "Unsubscribe from OwnerAccessControlEntryChangedBroadcast failed: " +
                                    error.getMessage());
        }
    }

    RceSubscription rceSubscriptionIds;
    subscriptionsFound = false;
    {
        std::lock_guard<std::mutex> lock(initStateMutex);
        if (rceSubscriptions.count(compoundKey) > 0) {
            // Get the subscription ids
            rceSubscriptionIds = rceSubscriptions[compoundKey];
            rceSubscriptions.erase(compoundKey);
            subscriptionsFound = true;
        }
    }
    if (subscriptionsFound) {
        JOYNR_LOG_TRACE(logger(),
                        "Unsubscribing from RCL broadcasts for domain {}, interface {}",
                        domain,
                        interfaceName);

        // Unsubscribe from RCE change subscriptions
        try {
            std::string masterRceSubscriptionId = rceSubscriptionIds.getMasterRceSubscriptionId();
            globalDomainAccessControllerProxy
                    ->unsubscribeFromMasterRegistrationControlEntryChangedBroadcast(
                            masterRceSubscriptionId);
        } catch (const exceptions::JoynrException& error) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "Unsubscribe from MasterRegistrationControlEntryChangedBroadcast failed: " +
                            error.getMessage());
        }
        try {
            std::string mediatorRceSubscriptionId =
                    rceSubscriptionIds.getMediatorRceSubscriptionId();
            globalDomainAccessControllerProxy
                    ->unsubscribeFromMediatorRegistrationControlEntryChangedBroadcast(
                            mediatorRceSubscriptionId);
        } catch (const exceptions::JoynrException& error) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "Unsubscribe from MediatorRegistrationControlEntryChangedBroadcast failed: " +
                            error.getMessage());
        }
        try {
            std::string ownerRceSubscriptionId = rceSubscriptionIds.getOwnerRceSubscriptionId();
            globalDomainAccessControllerProxy
                    ->unsubscribeFromOwnerRegistrationControlEntryChangedBroadcast(
                            ownerRceSubscriptionId);
        } catch (const exceptions::JoynrException& error) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "Unsubscribe from OwnerRegistrationControlEntryChangedBroadcast failed: " +
                            error.getMessage());
        }
    }
}

void LocalDomainAccessController::initializeDomainRoleTable(const std::string& userId)
{
    // Initialize domain roles from global data
    std::function<void(const std::vector<DomainRoleEntry>& domainRoleEntries)> domainRoleOnSuccess =
            [this](const std::vector<DomainRoleEntry>& domainRoleEntries) {
        // Add the results
        for (const DomainRoleEntry& dre : domainRoleEntries) {
            localDomainAccessStore->updateDomainRole(dre);
        }
    };

    std::function<void(const exceptions::JoynrException&)> domainRoleOnError =
            [](const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Aborting ACL initialisation due to communication error:\n{}",
                        error.getMessage());
    };

    globalDomainRoleControllerProxy->getDomainRolesAsync(
            userId, std::move(domainRoleOnSuccess), std::move(domainRoleOnError));
}

void LocalDomainAccessController::initializeLocalDomainAccessStoreAces(
        const std::string& domain,
        const std::string& interfaceName)
{
    // Create an object to keep track of the initialisation
    // steps == 3, because there are 3 init operations (MasterACE, MediatorACE, OwnerACE)
    const std::uint8_t steps = 3;
    const bool handleAces = true;
    const bool handleRces = false;
    auto initializer = std::make_shared<Initializer>(
            *this, domain, interfaceName, steps, handleAces, handleRces);

    std::function<void(const std::vector<MasterAccessControlEntry>& masterAces)>
            masterAceOnSuccess =
                    [this, initializer](const std::vector<MasterAccessControlEntry>& masterAces) {
        // Add the results
        for (const MasterAccessControlEntry& masterAce : masterAces) {
            localDomainAccessStore->updateMasterAccessControlEntry(masterAce);
        }
        initializer->update();
    };

    std::function<void(const exceptions::JoynrException& error)> masterAceOnError =
            [initializer](const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Aborting ACL initialisation due to communication error:\n{}",
                        error.getMessage());

        // Abort the initialisation
        initializer->abort();
    };

    globalDomainAccessControllerProxy->getMasterAccessControlEntriesAsync(
            domain, interfaceName, std::move(masterAceOnSuccess), std::move(masterAceOnError));

    // Initialize mediator access control entries from global data
    std::function<void(const std::vector<MasterAccessControlEntry>& mediatorAces)>
            mediatorAceOnSuccess =
                    [this, initializer](const std::vector<MasterAccessControlEntry>& mediatorAces) {
        // Add the results
        for (const MasterAccessControlEntry& mediatorAce : mediatorAces) {
            localDomainAccessStore->updateMediatorAccessControlEntry(mediatorAce);
        }
        initializer->update();
    };

    std::function<void(const exceptions::JoynrException& error)> mediatorAceOnError =
            [this, initializer](const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Aborting ACL initialisation due to communication error:\n{}",
                        error.getMessage());

        // Abort the initialisation
        initializer->abort();
    };

    globalDomainAccessControllerProxy->getMediatorAccessControlEntriesAsync(
            domain, interfaceName, std::move(mediatorAceOnSuccess), std::move(mediatorAceOnError));

    // Initialize owner access control entries from global data
    std::function<void(const std::vector<OwnerAccessControlEntry>& ownerAces)> ownerAceOnSuccess =
            [this, initializer](const std::vector<OwnerAccessControlEntry>& ownerAces) {
        // Add the results
        for (const OwnerAccessControlEntry& ownerAce : ownerAces) {
            localDomainAccessStore->updateOwnerAccessControlEntry(ownerAce);
        }
        initializer->update();
    };

    std::function<void(const exceptions::JoynrException& error)> ownerAceOnError =
            [initializer](const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Aborting ACL initialisation due to communication error:\n{}",
                        error.getMessage());

        // Abort the initialisation
        initializer->abort();
    };

    globalDomainAccessControllerProxy->getOwnerAccessControlEntriesAsync(
            domain, interfaceName, std::move(ownerAceOnSuccess), std::move(ownerAceOnError));
}

void LocalDomainAccessController::initializeLocalDomainAccessStoreRces(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName)
{
    // Create an object to keep track of the initialisation
    // steps == 3, because there are 3 init operations (MasterRCE, MediatorRCE, OwnerRCE)
    const std::uint8_t steps = 3;
    const bool handleAces = false;
    const bool handleRces = true;
    auto initializer = std::make_shared<Initializer>(
            *this, domain, interfaceName, steps, handleAces, handleRces);

    std::function<void(const std::vector<MasterRegistrationControlEntry>& masterRces)>
            masterRceOnSuccess = [this, initializer](
                    const std::vector<MasterRegistrationControlEntry>& masterRces) {
        // Add the results
        for (const MasterRegistrationControlEntry& masterRce : masterRces) {
            localDomainAccessStore->updateMasterRegistrationControlEntry(masterRce);
        }
        initializer->update();
    };

    std::function<void(const exceptions::JoynrException& error)> masterRceOnError =
            [initializer](const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Aborting RCL initialisation due to communication error:\n{}",
                        error.getMessage());

        // Abort the initialisation
        initializer->abort();
    };

    globalDomainAccessControllerProxy->getMasterRegistrationControlEntriesAsync(
            userId, std::move(masterRceOnSuccess), std::move(masterRceOnError));

    // Initialize mediator access control entries from global data
    std::function<void(const std::vector<MasterRegistrationControlEntry>& mediatorRces)>
            mediatorRceOnSuccess = [this, initializer](
                    const std::vector<MasterRegistrationControlEntry>& mediatorRces) {
        // Add the results
        for (const MasterRegistrationControlEntry& mediatorRce : mediatorRces) {
            localDomainAccessStore->updateMediatorRegistrationControlEntry(mediatorRce);
        }
        initializer->update();
    };

    std::function<void(const exceptions::JoynrException& error)> mediatorRceOnError =
            [this, initializer](const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Aborting RCL initialisation due to communication error:\n{}",
                        error.getMessage());

        // Abort the initialisation
        initializer->abort();
    };

    globalDomainAccessControllerProxy->getMediatorRegistrationControlEntriesAsync(
            userId, std::move(mediatorRceOnSuccess), std::move(mediatorRceOnError));

    // Initialize owner registration control entries from global data
    std::function<
            void(const std::vector<OwnerRegistrationControlEntry>& ownerRces)> ownerRceOnSuccess =
            [this, initializer](const std::vector<OwnerRegistrationControlEntry>& ownerRces) {
        // Add the results
        for (const OwnerRegistrationControlEntry& ownerRce : ownerRces) {
            localDomainAccessStore->updateOwnerRegistrationControlEntry(ownerRce);
        }
        initializer->update();
    };

    std::function<void(const exceptions::JoynrException& error)> ownerRceOnError =
            [initializer](const exceptions::JoynrException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Aborting RCL initialisation due to communication error:\n{}",
                        error.getMessage());

        // Abort the initialisation
        initializer->abort();
    };

    globalDomainAccessControllerProxy->getOwnerRegistrationControlEntriesAsync(
            userId, std::move(ownerRceOnSuccess), std::move(ownerRceOnError));
}

// Called when the data for the given domain/interface has been obtained from the GDAC
// or from loading the accessStore from disk.
void LocalDomainAccessController::initialized(const std::string& domain,
                                              const std::string& interfaceName,
                                              const bool handleAces,
                                              const bool handleRces,
                                              bool restoringFromFile)
{
    std::string compoundKey = createCompoundKey(domain, interfaceName);

    if (handleAces) {
        std::vector<ConsumerPermissionRequest> consumerRequests;

        {
            std::lock_guard<std::mutex> lock(initStateMutex);

            if (!useOnlyLocalDomainAccessStore) {
                // Subscribe to ACL broadcasts about this domain/interface
                aceSubscriptions.insert(
                        std::make_pair(compoundKey, subscribeForAceChange(domain, interfaceName)));
            }

            if (!restoringFromFile) {
                // Remove requests for processing
                auto it = consumerPermissionRequests.find(compoundKey);
                if (it != consumerPermissionRequests.cend()) {
                    consumerRequests = it->second;
                    consumerPermissionRequests.erase(it);
                }
            }
        }
        // Handle any queued requests for this domain/interface
        processConsumerRequests(consumerRequests);
    }

    if (handleRces) {
        std::vector<ProviderPermissionRequest> providerRequests;

        {
            std::lock_guard<std::mutex> lock(initStateMutex);

            if (!useOnlyLocalDomainAccessStore) {
                // Subscribe to RCL broadcasts about this domain/interface
                rceSubscriptions.insert(
                        std::make_pair(compoundKey, subscribeForRceChange(domain, interfaceName)));
            }

            if (!restoringFromFile) {
                // Remove requests for processing
                auto it = providerPermissionRequests.find(compoundKey);
                if (it != providerPermissionRequests.cend()) {
                    providerRequests = it->second;
                    providerPermissionRequests.erase(it);
                }
            }
        }
        // Handle any queued requests for this domain/interface
        processProviderRequests(providerRequests);
    }
}

void LocalDomainAccessController::abortInitialization(const std::string& domain,
                                                      const std::string& interfaceName,
                                                      const bool handleAces,
                                                      const bool handleRces)
{
    JOYNR_LOG_TRACE(logger(),
                    "Removing outstanding ACL requests for domain {}, interface {}",
                    domain,
                    interfaceName);

    std::string compoundKey = createCompoundKey(domain, interfaceName);

    if (handleAces) {
        std::vector<ConsumerPermissionRequest> requests;
        std::lock_guard<std::mutex> lock(initStateMutex);

        // Remove requests that cannot be processed
        auto it = consumerPermissionRequests.find(compoundKey);
        if (it != consumerPermissionRequests.cend()) {
            requests = it->second;
            consumerPermissionRequests.erase(it);
        }

        // Mark all the requests as failed - we have no information from the Global
        // Domain Access Controller
        for (const ConsumerPermissionRequest& request : requests) {
            request.callbacks->permission(Permission::NO);
        }
    }

    if (handleRces) {
        std::vector<ProviderPermissionRequest> requests;
        std::lock_guard<std::mutex> lock(initStateMutex);

        // Remove requests that cannot be processed
        auto it = providerPermissionRequests.find(compoundKey);
        if (it != providerPermissionRequests.cend()) {
            requests = it->second;
            providerPermissionRequests.erase(it);
        }

        // Mark all the requests as failed - we have no information from the Global
        // Domain Access Controller
        for (const ProviderPermissionRequest& request : requests) {
            request.callbacks->permission(Permission::NO);
        }
    }
}

// Returns true if other requests have already been queued
bool LocalDomainAccessController::queueConsumerRequest(const std::string& key,
                                                       const ConsumerPermissionRequest& request)
{
    // This function assumes that the initStateMutex has already been obtained

    if (consumerPermissionRequests.count(key) != 0) {
        consumerPermissionRequests[key].push_back(request);
        return true;
    } else {
        std::vector<ConsumerPermissionRequest> requestList;
        requestList.push_back(request);
        consumerPermissionRequests.insert(std::make_pair(key, requestList));
        return false;
    }
}

void LocalDomainAccessController::processConsumerRequests(
        const std::vector<ConsumerPermissionRequest>& requests)
{
    for (const ConsumerPermissionRequest& request : requests) {
        getConsumerPermission(request.userId,
                              request.domain,
                              request.interfaceName,
                              request.trustLevel,
                              request.callbacks);
    }
}

void LocalDomainAccessController::processProviderRequests(
        const std::vector<ProviderPermissionRequest>& requests)
{
    for (const ProviderPermissionRequest& request : requests) {
        getProviderPermission(request.userId,
                              request.domain,
                              request.interfaceName,
                              request.trustLevel,
                              request.callbacks);
    }
}

std::string LocalDomainAccessController::sanitizeForPartition(const std::string& value)
{
    static const std::regex nonAlphaNumeric("[^a-zA-Z0-9]");
    return std::regex_replace(value, nonAlphaNumeric, "");
}

std::shared_ptr<Future<std::string>> LocalDomainAccessController::subscribeForDreChange(
        const std::string& userId)
{
    std::vector<std::string> partitions = {sanitizeForPartition(userId)};

    return globalDomainRoleControllerProxy->subscribeToDomainRoleEntryChangedBroadcast(
            std::static_pointer_cast<ISubscriptionListener<ChangeType::Enum, DomainRoleEntry>>(
                    domainRoleEntryChangedBroadcastListener),
            multicastSubscriptionQos,
            partitions);
}

std::vector<std::string> LocalDomainAccessController::createPartitionsVector(
        const std::string& domain,
        const std::string& interfaceName)
{
    static const std::string wildcardForUserId = "+";
    return {wildcardForUserId, sanitizeForPartition(domain), sanitizeForPartition(interfaceName)};
}

LocalDomainAccessController::AceSubscription LocalDomainAccessController::subscribeForAceChange(
        const std::string& domain,
        const std::string& interfaceName)
{
    AceSubscription subscriptionIds;
    std::vector<std::string> partitions = createPartitionsVector(domain, interfaceName);
    subscriptionIds.masterAceSubscriptionIdFuture =
            globalDomainAccessControllerProxy->subscribeToMasterAccessControlEntryChangedBroadcast(
                    std::static_pointer_cast<
                            ISubscriptionListener<ChangeType::Enum, MasterAccessControlEntry>>(
                            masterAccessControlEntryChangedBroadcastListener),
                    multicastSubscriptionQos,
                    partitions);

    subscriptionIds.mediatorAceSubscriptionIdFuture =
            globalDomainAccessControllerProxy
                    ->subscribeToMediatorAccessControlEntryChangedBroadcast(
                            std::static_pointer_cast<
                                    ISubscriptionListener<ChangeType::Enum,
                                                          MasterAccessControlEntry>>(
                                    mediatorAccessControlEntryChangedBroadcastListener),
                            multicastSubscriptionQos,
                            partitions);

    subscriptionIds.ownerAceSubscriptionIdFuture =
            globalDomainAccessControllerProxy->subscribeToOwnerAccessControlEntryChangedBroadcast(
                    std::static_pointer_cast<
                            ISubscriptionListener<ChangeType::Enum, OwnerAccessControlEntry>>(
                            ownerAccessControlEntryChangedBroadcastListener),
                    multicastSubscriptionQos,
                    partitions);

    return subscriptionIds;
}

LocalDomainAccessController::RceSubscription LocalDomainAccessController::subscribeForRceChange(
        const std::string& domain,
        const std::string& interfaceName)
{
    RceSubscription subscriptionIds;
    std::vector<std::string> partitions = createPartitionsVector(domain, interfaceName);
    subscriptionIds.masterRceSubscriptionIdFuture =
            globalDomainAccessControllerProxy
                    ->subscribeToMasterRegistrationControlEntryChangedBroadcast(
                            std::static_pointer_cast<
                                    ISubscriptionListener<ChangeType::Enum,
                                                          MasterRegistrationControlEntry>>(
                                    masterRegistrationControlEntryChangedBroadcastListener),
                            multicastSubscriptionQos,
                            partitions);

    subscriptionIds.mediatorRceSubscriptionIdFuture =
            globalDomainAccessControllerProxy
                    ->subscribeToMediatorRegistrationControlEntryChangedBroadcast(
                            std::static_pointer_cast<
                                    ISubscriptionListener<ChangeType::Enum,
                                                          MasterRegistrationControlEntry>>(
                                    mediatorRegistrationControlEntryChangedBroadcastListener),
                            multicastSubscriptionQos,
                            partitions);

    subscriptionIds.ownerRceSubscriptionIdFuture =
            globalDomainAccessControllerProxy
                    ->subscribeToOwnerRegistrationControlEntryChangedBroadcast(
                            std::static_pointer_cast<
                                    ISubscriptionListener<ChangeType::Enum,
                                                          OwnerRegistrationControlEntry>>(
                                    ownerRegistrationControlEntryChangedBroadcastListener),
                            multicastSubscriptionQos,
                            partitions);

    return subscriptionIds;
}

std::string LocalDomainAccessController::createCompoundKey(const std::string& domain,
                                                           const std::string& interfaceName)
{
    std::string subscriptionMapKey(domain);
    subscriptionMapKey.push_back('\x1e'); // ascii record separator
    subscriptionMapKey.insert(0, interfaceName);
    return subscriptionMapKey;
}

//--- Implementation of DomainRoleEntryChangedBroadcastListener ----------------

LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener::
        DomainRoleEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener::onSubscribed(
        const std::string& subscriptionId)
{
    std::ignore = subscriptionId;
}

void LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener::onReceive(
        const ChangeType::Enum& changeType,
        const DomainRoleEntry& changedDre)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateDomainRole(changedDre);
    } else {
        parent.localDomainAccessStore->removeDomainRole(changedDre.getUid(), changedDre.getRole());
    }
    JOYNR_LOG_TRACE(parent.logger(), "Changed DRE: {}", changedDre.toString());
}

void LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener::onError(
        const exceptions::JoynrRuntimeException& error)
{
    std::ignore = error;
    JOYNR_LOG_ERROR(parent.logger(), "Change of DRE failed!");
}

LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener::
        MasterAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener::onSubscribed(
        const std::string& subscriptionId)
{
    std::ignore = subscriptionId;
}

void LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener::onReceive(
        const ChangeType::Enum& changeType,
        const MasterAccessControlEntry& changedMasterAce)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateMasterAccessControlEntry(changedMasterAce);
        JOYNR_LOG_TRACE(parent.logger(), "Changed MasterAce: {}", changedMasterAce.toString());
    } else {
        parent.localDomainAccessStore->removeMasterAccessControlEntry(
                changedMasterAce.getUid(),
                changedMasterAce.getDomain(),
                changedMasterAce.getInterfaceName(),
                changedMasterAce.getOperation());
        JOYNR_LOG_TRACE(parent.logger(), "Removed MasterAce: {}", changedMasterAce.toString());
    }
}

void LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener::onError(
        const exceptions::JoynrRuntimeException& error)
{
    std::ignore = error;
    JOYNR_LOG_ERROR(parent.logger(), "Change of MasterAce failed!");
}

LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener::
        MediatorAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener::onSubscribed(
        const std::string& subscriptionId)
{
    std::ignore = subscriptionId;
}

void LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener::onReceive(
        const ChangeType::Enum& changeType,
        const MasterAccessControlEntry& changedMediatorAce)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateMediatorAccessControlEntry(changedMediatorAce);
    } else {
        parent.localDomainAccessStore->removeMediatorAccessControlEntry(
                changedMediatorAce.getUid(),
                changedMediatorAce.getDomain(),
                changedMediatorAce.getInterfaceName(),
                changedMediatorAce.getOperation());
    }
    JOYNR_LOG_TRACE(parent.logger(), "Changed MediatorAce: {}", changedMediatorAce.toString());
}

void LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener::onError(
        const exceptions::JoynrRuntimeException& error)
{
    std::ignore = error;
    JOYNR_LOG_ERROR(parent.logger(), "Change of MediatorAce failed!");
}

LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener::
        OwnerAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener::onSubscribed(
        const std::string& subscriptionId)
{
    std::ignore = subscriptionId;
}

void LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener::onReceive(
        const ChangeType::Enum& changeType,
        const OwnerAccessControlEntry& changedOwnerAce)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateOwnerAccessControlEntry(changedOwnerAce);
    } else {
        parent.localDomainAccessStore->removeOwnerAccessControlEntry(
                changedOwnerAce.getUid(),
                changedOwnerAce.getDomain(),
                changedOwnerAce.getInterfaceName(),
                changedOwnerAce.getOperation());
    }
    JOYNR_LOG_TRACE(parent.logger(), "Changed OwnerAce: {}", changedOwnerAce.toString());
}

void LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener::onError(
        const exceptions::JoynrRuntimeException& error)
{
    std::ignore = error;
    JOYNR_LOG_ERROR(parent.logger(), "Change of OwnerAce failed!");
}

LocalDomainAccessController::MasterRegistrationControlEntryChangedBroadcastListener::
        MasterRegistrationControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::MasterRegistrationControlEntryChangedBroadcastListener::
        onSubscribed(const std::string& subscriptionId)
{
    std::ignore = subscriptionId;
}

void LocalDomainAccessController::MasterRegistrationControlEntryChangedBroadcastListener::onReceive(
        const ChangeType::Enum& changeType,
        const MasterRegistrationControlEntry& changedMasterRce)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateMasterRegistrationControlEntry(changedMasterRce);
        JOYNR_LOG_TRACE(parent.logger(), "Changed MasterAce: {}", changedMasterRce.toString());
    } else {
        parent.localDomainAccessStore->removeMasterRegistrationControlEntry(
                changedMasterRce.getUid(),
                changedMasterRce.getDomain(),
                changedMasterRce.getInterfaceName());
        JOYNR_LOG_TRACE(parent.logger(), "Removed MasterAce: {}", changedMasterRce.toString());
    }
}

void LocalDomainAccessController::MasterRegistrationControlEntryChangedBroadcastListener::onError(
        const exceptions::JoynrRuntimeException& error)
{
    std::ignore = error;
    JOYNR_LOG_ERROR(parent.logger(), "Change of MasterAce failed!");
}

LocalDomainAccessController::MediatorRegistrationControlEntryChangedBroadcastListener::
        MediatorRegistrationControlEntryChangedBroadcastListener(
                LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::MediatorRegistrationControlEntryChangedBroadcastListener::
        onSubscribed(const std::string& subscriptionId)
{
    std::ignore = subscriptionId;
}

void LocalDomainAccessController::MediatorRegistrationControlEntryChangedBroadcastListener::
        onReceive(const ChangeType::Enum& changeType,
                  const MasterRegistrationControlEntry& changedMediatorRce)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateMediatorRegistrationControlEntry(changedMediatorRce);
    } else {
        parent.localDomainAccessStore->removeMediatorRegistrationControlEntry(
                changedMediatorRce.getUid(),
                changedMediatorRce.getDomain(),
                changedMediatorRce.getInterfaceName());
    }
    JOYNR_LOG_TRACE(parent.logger(), "Changed MediatorRce: {}", changedMediatorRce.toString());
}

void LocalDomainAccessController::MediatorRegistrationControlEntryChangedBroadcastListener::onError(
        const exceptions::JoynrRuntimeException& error)
{
    std::ignore = error;
    JOYNR_LOG_ERROR(parent.logger(), "Change of MediatorRce failed!");
}

LocalDomainAccessController::OwnerRegistrationControlEntryChangedBroadcastListener::
        OwnerRegistrationControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::OwnerRegistrationControlEntryChangedBroadcastListener::
        onSubscribed(const std::string& subscriptionId)
{
    std::ignore = subscriptionId;
}

void LocalDomainAccessController::OwnerRegistrationControlEntryChangedBroadcastListener::onReceive(
        const ChangeType::Enum& changeType,
        const OwnerRegistrationControlEntry& changedOwnerRce)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateOwnerRegistrationControlEntry(changedOwnerRce);
    } else {
        parent.localDomainAccessStore->removeOwnerRegistrationControlEntry(
                changedOwnerRce.getUid(),
                changedOwnerRce.getDomain(),
                changedOwnerRce.getInterfaceName());
    }
    JOYNR_LOG_TRACE(parent.logger(), "Changed OwnerRce: {}", changedOwnerRce.toString());
}

void LocalDomainAccessController::OwnerRegistrationControlEntryChangedBroadcastListener::onError(
        const exceptions::JoynrRuntimeException& error)
{
    std::ignore = error;
    JOYNR_LOG_ERROR(parent.logger(), "Change of OwnerRce failed!");
}

} // namespace joynr

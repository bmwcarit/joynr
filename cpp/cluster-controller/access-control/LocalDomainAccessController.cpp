/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "LocalDomainAccessStore.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerProxy.h"

#include "joynr/infrastructure/QtDomainRoleEntry.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerDomainRoleEntryChangedBroadcastFilterParameters.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerMasterAccessControlEntryChangedBroadcastFilterParameters.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerOwnerAccessControlEntryChangedBroadcastFilterParameters.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerMediatorAccessControlEntryChangedBroadcastFilterParameters.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/RequestStatus.h"
#include "joynr/StdOnChangeSubscriptionQos.h"
#include <vector>
#include "joynr/joynrlogging.h"
#include "joynr/TypeUtil.h"

#include <cassert>
#include <QAtomicInt>

namespace joynr
{

using namespace infrastructure;
using namespace infrastructure::DacTypes;
using namespace joynr_logging;

Logger* LocalDomainAccessController::logger =
        Logging::getInstance()->getLogger("MSG", "LocalDomainAccessController");

int64_t LocalDomainAccessController::broadcastMinIntervalMs = 1 * 1000;
int64_t LocalDomainAccessController::broadcastSubscriptionValidityMs =
        10 * 365 * 24 * 3600 * 1000LL; // 10 years
int64_t LocalDomainAccessController::broadcastPublicationTtlMs = 5 * 1000;

//--- Declarations of nested classes -------------------------------------------

class LocalDomainAccessController::Initialiser
{
public:
    Initialiser(LocalDomainAccessController& parent,
                const std::string& domain,
                const std::string& interfaceName);

    // Called to indicate that some data has been initialised
    void update();

    // Called to abort the initialisation
    void abort();

private:
    QAtomicInt counter;
    QAtomicInt aborted;
    LocalDomainAccessController& parent;
    std::string domain;
    std::string interfaceName;
};

class LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::StdChangeType::Enum,
                                       infrastructure::DacTypes::StdDomainRoleEntry>
{
public:
    DomainRoleEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::DacTypes::StdChangeType::Enum& changeType,
                   const infrastructure::DacTypes::StdDomainRoleEntry& changedDre);
    void onError();

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::StdChangeType::Enum,
                                       infrastructure::DacTypes::StdMasterAccessControlEntry>
{
public:
    MasterAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::DacTypes::StdChangeType::Enum& changeType,
                   const infrastructure::DacTypes::StdMasterAccessControlEntry& changedMasterAce);
    void onError();

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::StdChangeType::Enum,
                                       infrastructure::DacTypes::StdMasterAccessControlEntry>
{
public:
    MediatorAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::DacTypes::StdChangeType::Enum& changeType,
                   const infrastructure::DacTypes::StdMasterAccessControlEntry& changedMediatorAce);
    void onError();

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::DacTypes::StdChangeType::Enum,
                                       infrastructure::DacTypes::StdOwnerAccessControlEntry>
{
public:
    OwnerAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::DacTypes::StdChangeType::Enum& changeType,
                   const infrastructure::DacTypes::StdOwnerAccessControlEntry& changedOwnerAce);
    void onError();

private:
    LocalDomainAccessController& parent;
};

//--- LocalDomainAccessController ----------------------------------------------

LocalDomainAccessController::LocalDomainAccessController(
        LocalDomainAccessStore* localDomainAccessStore)
        : accessControlAlgorithm(),
          dreSubscriptions(),
          aceSubscriptions(),
          globalDomainAccessControllerProxy(),
          localDomainAccessStore(localDomainAccessStore),
          consumerPermissionRequests(),
          initStateMutex(),
          domainRoleEntryChangedBroadcastListener(
                  new DomainRoleEntryChangedBroadcastListener(*this)),
          masterAccessControlEntryChangedBroadcastListener(
                  new MasterAccessControlEntryChangedBroadcastListener(*this)),
          mediatorAccessControlEntryChangedBroadcastListener(
                  new MediatorAccessControlEntryChangedBroadcastListener(*this)),
          ownerAccessControlEntryChangedBroadcastListener(
                  new OwnerAccessControlEntryChangedBroadcastListener(*this))
{
}

LocalDomainAccessController::~LocalDomainAccessController()
{
    delete localDomainAccessStore;
}

void LocalDomainAccessController::init(
        QSharedPointer<GlobalDomainAccessControllerProxy> globalDomainAccessControllerProxy)
{
    this->globalDomainAccessControllerProxy = globalDomainAccessControllerProxy;
}

bool LocalDomainAccessController::hasRole(const QString& userId,
                                          const QString& domain,
                                          QtRole::Enum role)
{
    LOG_DEBUG(logger, QString("execute: entering hasRole"));

    // See if the user has the given role
    bool hasRole = false;
    Optional<QtDomainRoleEntry> dre = localDomainAccessStore->getDomainRole(userId, role);
    if (dre) {
        QList<QString> domains = dre.getValue().getDomains();
        if (domains.contains(domain)) {
            hasRole = true;
        }
    }

    // Subscribe changes in the users roles
    if (!dreSubscriptions.contains(userId)) {
        dreSubscriptions.insert(userId, subscribeForDreChange(userId));
    }

    return hasRole;
}

void LocalDomainAccessController::getConsumerPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        StdTrustLevel::Enum trustLevel,
        QSharedPointer<IGetConsumerPermissionCallback> callback)
{
    LOG_DEBUG(logger, QString("Entering getConsumerPermission with unknown operation"));

    // Is the ACL for this domain/interface available?
    std::string compoundKey = createCompoundKey(domain, interfaceName);
    bool needsInit = false;
    {
        QMutexLocker lock(&initStateMutex);
        if (!aceSubscriptions.contains(QString::fromStdString(compoundKey))) {
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
        initialiseLocalDomainAccessStore(userId, domain, interfaceName);
        return;
    }

    // If this point is reached the data for the ACL check is available
    QList<QtMasterAccessControlEntry> masterAces =
            localDomainAccessStore->getMasterAccessControlEntries(
                    QString::fromStdString(userId),
                    QString::fromStdString(domain),
                    QString::fromStdString(interfaceName));
    QList<QtMasterAccessControlEntry> mediatorAces =
            localDomainAccessStore->getMediatorAccessControlEntries(
                    QString::fromStdString(userId),
                    QString::fromStdString(domain),
                    QString::fromStdString(interfaceName));
    QList<QtOwnerAccessControlEntry> ownerAces =
            localDomainAccessStore->getOwnerAccessControlEntries(
                    QString::fromStdString(userId),
                    QString::fromStdString(domain),
                    QString::fromStdString(interfaceName));

    // The operations of the ACEs should only contain wildcards, if not
    // getConsumerPermission should be called with an operation
    if (!(onlyWildcardOperations(masterAces) && onlyWildcardOperations(mediatorAces) &&
          onlyWildcardOperations(ownerAces))) {
        callback->operationNeeded();
    } else {
        // The operations are all wildcards
        QtPermission::Enum permission = getConsumerPermission(
                userId, domain, interfaceName, LocalDomainAccessStore::WILDCARD, trustLevel);
        callback->consumerPermission(QtPermission::createStd(permission));
    }
}

QtPermission::Enum LocalDomainAccessController::getConsumerPermission(
        const std::string& userId,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& operation,
        StdTrustLevel::Enum trustLevel)
{
    LOG_DEBUG(logger, QString("Entering getConsumerPermission with known operation"));

    Optional<QtMasterAccessControlEntry> masterAceOptional =
            localDomainAccessStore->getMasterAccessControlEntry(
                    QString::fromStdString(userId),
                    QString::fromStdString(domain),
                    QString::fromStdString(interfaceName),
                    QString::fromStdString(operation));
    Optional<QtMasterAccessControlEntry> mediatorAceOptional =
            localDomainAccessStore->getMediatorAccessControlEntry(
                    QString::fromStdString(userId),
                    QString::fromStdString(domain),
                    QString::fromStdString(interfaceName),
                    QString::fromStdString(operation));
    Optional<QtOwnerAccessControlEntry> ownerAceOptional =
            localDomainAccessStore->getOwnerAccessControlEntry(
                    QString::fromStdString(userId),
                    QString::fromStdString(domain),
                    QString::fromStdString(interfaceName),
                    QString::fromStdString(operation));

    return accessControlAlgorithm.getConsumerPermission(masterAceOptional,
                                                        mediatorAceOptional,
                                                        ownerAceOptional,
                                                        QtTrustLevel::createQt(trustLevel));
}

std::vector<StdMasterAccessControlEntry> LocalDomainAccessController::getMasterAccessControlEntries(
        const std::string& uid)
{
    RequestStatus rs;
    std::vector<StdMasterAccessControlEntry> resultMasterAces;
    globalDomainAccessControllerProxy->getMasterAccessControlEntries(rs, resultMasterAces, uid);

    return resultMasterAces;
}

std::vector<StdMasterAccessControlEntry> LocalDomainAccessController::
        getEditableMasterAccessControlEntries(const std::string& uid)
{
    RequestStatus rs;
    std::vector<StdMasterAccessControlEntry> resultMasterAces;
    globalDomainAccessControllerProxy->getEditableMasterAccessControlEntries(
            rs, resultMasterAces, uid);

    return resultMasterAces;
}

bool LocalDomainAccessController::updateMasterAccessControlEntry(
        const StdMasterAccessControlEntry& updatedMasterAce)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->updateMasterAccessControlEntry(
            rs, success, updatedMasterAce);

    return success;
}

bool LocalDomainAccessController::removeMasterAccessControlEntry(const std::string& uid,
                                                                 const std::string& domain,
                                                                 const std::string& interfaceName,
                                                                 const std::string& operation)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->removeMasterAccessControlEntry(
            rs, success, uid, domain, interfaceName, operation);

    return success;
}

std::vector<StdMasterAccessControlEntry> LocalDomainAccessController::
        getMediatorAccessControlEntries(const std::string& uid)
{
    RequestStatus rs;
    std::vector<StdMasterAccessControlEntry> resultMediatorAces;
    globalDomainAccessControllerProxy->getMediatorAccessControlEntries(rs, resultMediatorAces, uid);

    return resultMediatorAces;
}

std::vector<StdMasterAccessControlEntry> LocalDomainAccessController::
        getEditableMediatorAccessControlEntries(const std::string& uid)
{
    RequestStatus rs;
    std::vector<StdMasterAccessControlEntry> resultMediatorAces;
    globalDomainAccessControllerProxy->getEditableMediatorAccessControlEntries(
            rs, resultMediatorAces, uid);

    return resultMediatorAces;
}

bool LocalDomainAccessController::updateMediatorAccessControlEntry(
        const StdMasterAccessControlEntry& updatedMediatorAce)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->updateMediatorAccessControlEntry(
            rs, success, updatedMediatorAce);

    return success;
}

bool LocalDomainAccessController::removeMediatorAccessControlEntry(const std::string& uid,
                                                                   const std::string& domain,
                                                                   const std::string& interfaceName,
                                                                   const std::string& operation)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->removeMediatorAccessControlEntry(
            rs, success, uid, domain, interfaceName, operation);

    return success;
}

std::vector<StdOwnerAccessControlEntry> LocalDomainAccessController::getOwnerAccessControlEntries(
        const std::string& uid)
{
    RequestStatus rs;
    std::vector<StdOwnerAccessControlEntry> resultOwnerAces;
    globalDomainAccessControllerProxy->getOwnerAccessControlEntries(rs, resultOwnerAces, uid);

    return resultOwnerAces;
}

std::vector<StdOwnerAccessControlEntry> LocalDomainAccessController::
        getEditableOwnerAccessControlEntries(const std::string& uid)
{
    RequestStatus rs;
    std::vector<StdOwnerAccessControlEntry> resultOwnerAces;
    globalDomainAccessControllerProxy->getEditableOwnerAccessControlEntries(
            rs, resultOwnerAces, uid);

    return resultOwnerAces;
}

bool LocalDomainAccessController::updateOwnerAccessControlEntry(
        const StdOwnerAccessControlEntry& updatedOwnerAce)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->updateOwnerAccessControlEntry(rs, success, updatedOwnerAce);

    return success;
}

bool LocalDomainAccessController::removeOwnerAccessControlEntry(const std::string& uid,
                                                                const std::string& domain,
                                                                const std::string& interfaceName,
                                                                const std::string& operation)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->removeOwnerAccessControlEntry(
            rs, success, uid, domain, interfaceName, operation);

    return success;
}

StdPermission::Enum LocalDomainAccessController::getProviderPermission(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName,
        StdTrustLevel::Enum trustLevel)
{
    Q_ASSERT_X(false, "getProviderPermission", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return QtPermission::createStd(accessControlAlgorithm.getProviderPermission(
            Optional<QtMasterAccessControlEntry>::createNull(),
            Optional<QtMasterAccessControlEntry>::createNull(),
            Optional<QtOwnerAccessControlEntry>::createNull(),
            QtTrustLevel::createQt(trustLevel)));
}

std::vector<StdMasterRegistrationControlEntry> LocalDomainAccessController::
        getMasterRegistrationControlEntries(const std::string& uid)
{
    Q_ASSERT_X(false, "getMasterRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return std::vector<StdMasterRegistrationControlEntry>();
}

//---- Unused methods copied from Java implementation --------------------------

std::vector<StdMasterRegistrationControlEntry> LocalDomainAccessController::
        getEditableMasterRegistrationControlEntries(const std::string& uid)
{
    Q_ASSERT_X(false, "getEditableMasterRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return std::vector<StdMasterRegistrationControlEntry>();
}

bool LocalDomainAccessController::updateMasterRegistrationControlEntry(
        const StdMasterRegistrationControlEntry& updatedMasterRce)
{
    Q_ASSERT_X(false, "updateMasterRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(updatedMasterRce);

    return false;
}

bool LocalDomainAccessController::removeMasterRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    Q_ASSERT_X(false, "removeMasterRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return false;
}

std::vector<StdMasterRegistrationControlEntry> LocalDomainAccessController::
        getMediatorRegistrationControlEntries(const std::string& uid)
{
    Q_ASSERT_X(false, "getMediatorRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return std::vector<StdMasterRegistrationControlEntry>();
}

std::vector<StdMasterRegistrationControlEntry> LocalDomainAccessController::
        getEditableMediatorRegistrationControlEntries(const std::string& uid)
{
    Q_ASSERT_X(false, "getEditableMediatorRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return std::vector<StdMasterRegistrationControlEntry>();
}

bool LocalDomainAccessController::updateMediatorRegistrationControlEntry(
        const StdMasterRegistrationControlEntry& updatedMediatorRce)
{
    Q_ASSERT_X(false, "updateMediatorRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(updatedMediatorRce);

    return false;
}

bool LocalDomainAccessController::removeMediatorRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    Q_ASSERT_X(false, "removeMediatorRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return false;
}

std::vector<StdOwnerRegistrationControlEntry> LocalDomainAccessController::
        getOwnerRegistrationControlEntries(const std::string& uid)
{
    Q_ASSERT_X(false, "getOwnerRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return std::vector<StdOwnerRegistrationControlEntry>();
}

std::vector<StdOwnerRegistrationControlEntry> LocalDomainAccessController::
        getEditableOwnerRegistrationControlEntries(const std::string& uid)
{
    Q_ASSERT_X(false, "getEditableOwnerRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return std::vector<StdOwnerRegistrationControlEntry>();
}

bool LocalDomainAccessController::updateOwnerRegistrationControlEntry(
        const StdOwnerRegistrationControlEntry& updatedOwnerRce)
{
    Q_ASSERT_X(false, "updateOwnerRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(updatedOwnerRce);

    return false;
}

bool LocalDomainAccessController::removeOwnerRegistrationControlEntry(
        const std::string& uid,
        const std::string& domain,
        const std::string& interfaceName)
{
    Q_ASSERT_X(false, "removeOwnerRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return false;
}

void LocalDomainAccessController::unregisterProvider(const std::string& domain,
                                                     const std::string& interfaceName)
{
    // Get the subscription ids
    std::string compoundKey = createCompoundKey(domain, interfaceName);
    AceSubscription subscriptionIds;
    {
        QMutexLocker lock(&initStateMutex);
        if (!aceSubscriptions.contains(QString::fromStdString(compoundKey))) {
            return;
        }
        subscriptionIds = aceSubscriptions.value(QString::fromStdString(compoundKey));
    }

    LOG_DEBUG(logger,
              QString("Unsubscribing from ACL broadcasts for domain %1, interface %2")
                      .arg(QString::fromStdString(domain))
                      .arg(QString::fromStdString(interfaceName)));

    // Unsubscribe from ACE change subscriptions
    globalDomainAccessControllerProxy->unsubscribeFromMasterAccessControlEntryChangedBroadcast(
            subscriptionIds.masterAceSubscriptionId);
    globalDomainAccessControllerProxy->unsubscribeFromMediatorAccessControlEntryChangedBroadcast(
            subscriptionIds.mediatorAceSubscriptionId);
    globalDomainAccessControllerProxy->unsubscribeFromOwnerAccessControlEntryChangedBroadcast(
            subscriptionIds.ownerAceSubscriptionId);
}

// Initialise the data for the given data/interface. This function is non-blocking
void LocalDomainAccessController::initialiseLocalDomainAccessStore(const std::string& userId,
                                                                   const std::string& domain,
                                                                   const std::string& interfaceName)
{
    // Create an object to keep track of the initialisation
    QSharedPointer<Initialiser> initialiser(new Initialiser(*this, domain, interfaceName));

    // Initialise domain roles from global data
    // TODO: confirm that this is needed
    std::function<void(
            const RequestStatus& status, const std::vector<StdDomainRoleEntry>& domainRoleEntries)>
            domainRoleCallbackFct =
                    [this, initialiser](const RequestStatus& status,
                                        const std::vector<StdDomainRoleEntry>& domainRoleEntries) {
        if (status.successful()) {
            // Add the results
            foreach (const StdDomainRoleEntry& dre, domainRoleEntries) {
                localDomainAccessStore->updateDomainRole(QtDomainRoleEntry::createQt(dre));
            }
            initialiser->update();
        } else {
            QString description = status.getDescription().join("\n");
            LOG_ERROR(logger,
                      QString("Aborting ACL initialisation due to communication error:\n%1")
                              .arg(description));

            // Abort the initialisation
            initialiser->abort();
        }
    };
    globalDomainAccessControllerProxy->getDomainRoles(userId, domainRoleCallbackFct);

    std::function<void(const RequestStatus& status,
                       const std::vector<StdMasterAccessControlEntry>& masterAces)>
            masterAceCallbackFct = [this, initialiser](
                    const RequestStatus& status,
                    const std::vector<StdMasterAccessControlEntry>& masterAces) {
        if (status.successful()) {
            // Add the results
            foreach (const StdMasterAccessControlEntry& masterAce, masterAces) {
                localDomainAccessStore->updateMasterAccessControlEntry(
                        QtMasterAccessControlEntry::createQt(masterAce));
            }
            initialiser->update();
        } else {
            QString description = status.getDescription().join("\n");
            LOG_ERROR(logger,
                      QString("Aborting ACL initialisation due to communication error:\n%1")
                              .arg(description));

            // Abort the initialisation
            initialiser->abort();
        }
    };
    globalDomainAccessControllerProxy->getMasterAccessControlEntries(
            domain, interfaceName, masterAceCallbackFct);

    // Initialise mediator access control entries from global data
    std::function<void(const RequestStatus& status,
                       const std::vector<StdMasterAccessControlEntry>& mediatorAces)>
            mediatorAceCallbackFct = [this, initialiser](
                    const RequestStatus& status,
                    const std::vector<StdMasterAccessControlEntry>& mediatorAces) {
        if (status.successful()) {
            // Add the results
            foreach (const StdMasterAccessControlEntry& mediatorAce, mediatorAces) {
                localDomainAccessStore->updateMediatorAccessControlEntry(
                        QtMasterAccessControlEntry::createQt(mediatorAce));
            }
            initialiser->update();
        } else {
            QString description = status.getDescription().join("\n");
            LOG_ERROR(logger,
                      QString("Aborting ACL initialisation due to communication error:\n%1")
                              .arg(description));

            // Abort the initialisation
            initialiser->abort();
        }
    };
    globalDomainAccessControllerProxy->getMediatorAccessControlEntries(
            domain, interfaceName, mediatorAceCallbackFct);

    // Initialise owner access control entries from global data
    std::function<void(
            const RequestStatus& status, const std::vector<StdOwnerAccessControlEntry>& ownerAces)>
            ownerAceCallbackFct =
                    [this, initialiser](const RequestStatus& status,
                                        const std::vector<StdOwnerAccessControlEntry>& ownerAces) {
        if (status.successful()) {
            // Add the results
            foreach (const StdOwnerAccessControlEntry& ownerAce, ownerAces) {
                localDomainAccessStore->updateOwnerAccessControlEntry(
                        QtOwnerAccessControlEntry::createQt(ownerAce));
            }
            initialiser->update();
        } else {
            QString description = status.getDescription().join("\n");
            LOG_ERROR(logger,
                      QString("Aborting ACL initialisation due to communication error:\n%1")
                              .arg(description));

            // Abort the initialisation
            initialiser->abort();
        }
    };
    globalDomainAccessControllerProxy->getOwnerAccessControlEntries(
            domain, interfaceName, ownerAceCallbackFct);
}

// Called when the data for the given domain/interface has been obtained from the GDAC
void LocalDomainAccessController::initialised(const std::string& domain,
                                              const std::string& interfaceName)
{
    std::string compoundKey = createCompoundKey(domain, interfaceName);
    QList<ConsumerPermissionRequest> requests;

    {
        QMutexLocker lock(&initStateMutex);

        // Subscribe to ACL broadcasts about this domain/interface
        aceSubscriptions.insert(QString::fromStdString(compoundKey),
                                subscribeForAceChange(QString::fromStdString(domain),
                                                      QString::fromStdString(interfaceName)));

        // Remove requests for processing
        requests = consumerPermissionRequests.take(QString::fromStdString(compoundKey));
    }

    // Handle any queued requests for this domain/interface
    processConsumerRequests(requests);
}

void LocalDomainAccessController::abortInitialisation(const std::string& domain,
                                                      const std::string& interfaceName)
{
    LOG_INFO(logger,
             QString("Removing outstanding ACL requests for domain %1, interface %2")
                     .arg(QString::fromStdString(domain))
                     .arg(QString::fromStdString(interfaceName)));

    std::string compoundKey = createCompoundKey(domain, interfaceName);
    QList<ConsumerPermissionRequest> requests;

    {
        QMutexLocker lock(&initStateMutex);

        // Remove requests that cannot be processed
        requests = consumerPermissionRequests.take(QString::fromStdString(compoundKey));
    }

    // Mark all the requests as failed - we have no information from the Global
    // Domain Access Controller
    foreach (const ConsumerPermissionRequest& request, requests) {
        request.callbacks->consumerPermission(StdPermission::NO);
    }
}

// Returns true if other requests have already been queued
bool LocalDomainAccessController::queueConsumerRequest(const std::string& key,
                                                       const ConsumerPermissionRequest& request)
{
    // This function assumes that the initStateMutex has already been obtained

    if (consumerPermissionRequests.contains(QString::fromStdString(key))) {
        consumerPermissionRequests[QString::fromStdString(key)].append(request);
        return true;
    } else {
        QList<ConsumerPermissionRequest> requestList;
        requestList << request;
        consumerPermissionRequests.insert(QString::fromStdString(key), requestList);
        return false;
    }
}

void LocalDomainAccessController::processConsumerRequests(
        const QList<ConsumerPermissionRequest>& requests)
{
    foreach (const ConsumerPermissionRequest& request, requests) {
        getConsumerPermission(request.userId,
                              request.domain,
                              request.interfaceName,
                              request.trustLevel,
                              request.callbacks);
    }
}

QString LocalDomainAccessController::subscribeForDreChange(const QString& userId)
{
    StdOnChangeSubscriptionQos broadcastSubscriptionQos;
    broadcastSubscriptionQos.setMinInterval(broadcastMinIntervalMs);
    broadcastSubscriptionQos.setValidity(broadcastSubscriptionValidityMs);
    broadcastSubscriptionQos.setPublicationTtl(broadcastPublicationTtlMs);
    GlobalDomainAccessControllerDomainRoleEntryChangedBroadcastFilterParameters
            domainRoleFilterParameters;
    domainRoleFilterParameters.setUserIdOfInterest(TypeUtil::toStd(userId));

    std::string subscriptionId =
            globalDomainAccessControllerProxy->subscribeToDomainRoleEntryChangedBroadcast(
                    domainRoleFilterParameters,
                    std::static_pointer_cast<
                            ISubscriptionListener<StdChangeType::Enum, StdDomainRoleEntry>>(
                            domainRoleEntryChangedBroadcastListener),
                    broadcastSubscriptionQos);

    return QString::fromStdString(subscriptionId);
}

LocalDomainAccessController::AceSubscription LocalDomainAccessController::subscribeForAceChange(
        const QString& domain,
        const QString& interfaceName)
{
    StdOnChangeSubscriptionQos broadcastSubscriptionQos;

    broadcastSubscriptionQos.setMinInterval(broadcastMinIntervalMs);
    broadcastSubscriptionQos.setValidity(broadcastSubscriptionValidityMs);
    broadcastSubscriptionQos.setPublicationTtl(broadcastPublicationTtlMs);

    AceSubscription subscriptionIds;

    GlobalDomainAccessControllerMasterAccessControlEntryChangedBroadcastFilterParameters
            masterAcefilterParameters;
    masterAcefilterParameters.setDomainOfInterest(TypeUtil::toStd(domain));
    masterAcefilterParameters.setInterfaceOfInterest(TypeUtil::toStd(interfaceName));
    subscriptionIds.masterAceSubscriptionId =
            globalDomainAccessControllerProxy->subscribeToMasterAccessControlEntryChangedBroadcast(
                    masterAcefilterParameters,
                    std::static_pointer_cast<ISubscriptionListener<StdChangeType::Enum,
                                                                   StdMasterAccessControlEntry>>(
                            masterAccessControlEntryChangedBroadcastListener),
                    broadcastSubscriptionQos);

    GlobalDomainAccessControllerMediatorAccessControlEntryChangedBroadcastFilterParameters
            mediatorAceFilterParameters;
    mediatorAceFilterParameters.setDomainOfInterest(TypeUtil::toStd(domain));
    mediatorAceFilterParameters.setInterfaceOfInterest(TypeUtil::toStd(interfaceName));
    subscriptionIds.mediatorAceSubscriptionId =
            globalDomainAccessControllerProxy
                    ->subscribeToMediatorAccessControlEntryChangedBroadcast(
                            mediatorAceFilterParameters,
                            std::static_pointer_cast<
                                    ISubscriptionListener<StdChangeType::Enum,
                                                          StdMasterAccessControlEntry>>(
                                    mediatorAccessControlEntryChangedBroadcastListener),
                            broadcastSubscriptionQos);

    GlobalDomainAccessControllerOwnerAccessControlEntryChangedBroadcastFilterParameters
            ownerAceFilterParameters;
    ownerAceFilterParameters.setDomainOfInterest(TypeUtil::toStd(domain));
    ownerAceFilterParameters.setInterfaceOfInterest(TypeUtil::toStd(interfaceName));
    subscriptionIds.ownerAceSubscriptionId =
            globalDomainAccessControllerProxy->subscribeToOwnerAccessControlEntryChangedBroadcast(
                    ownerAceFilterParameters,
                    std::static_pointer_cast<
                            ISubscriptionListener<StdChangeType::Enum, StdOwnerAccessControlEntry>>(
                            ownerAccessControlEntryChangedBroadcastListener),
                    broadcastSubscriptionQos);

    return subscriptionIds;
}

QString LocalDomainAccessController::createCompoundKey(const QString& domain,
                                                       const QString& interfaceName)
{
    QString subscriptionMapKey(domain);
    subscriptionMapKey.append('\x1e'); // ascii record separator
    subscriptionMapKey.append(interfaceName);

    return subscriptionMapKey;
}

std::string LocalDomainAccessController::createCompoundKey(const std::string& domain,
                                                           const std::string& interfaceName)
{
    return createCompoundKey(QString::fromStdString(domain), QString::fromStdString(interfaceName))
            .toStdString();
}

//--- Implementation of Initialiser --------------------------------------------

LocalDomainAccessController::Initialiser::Initialiser(LocalDomainAccessController& parent,
                                                      const std::string& domain,
                                                      const std::string& interfaceName)
        : counter(4), aborted(0), parent(parent), domain(domain), interfaceName(interfaceName)
// counter == 4, because there are 4 init operations (DRT, MasterACE, MediatorACE, OwnerACE)
{
}

void LocalDomainAccessController::Initialiser::update()
{
    if (counter.deref() == false) {
        // Initialisation has finished
        if (aborted.load() == 1) {
            parent.abortInitialisation(domain, interfaceName);
        } else {
            parent.initialised(domain, interfaceName);
        }
    }
}

void LocalDomainAccessController::Initialiser::abort()
{
    aborted.store(1);
    update();
}

//--- Implementation of DomainRoleEntryChangedBroadcastListener ----------------

LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener::
        DomainRoleEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener::onReceive(
        const StdChangeType::Enum& changeType,
        const StdDomainRoleEntry& changedDre)
{
    if (changeType != StdChangeType::REMOVE) {
        parent.localDomainAccessStore->updateDomainRole(QtDomainRoleEntry::createQt(changedDre));
    } else {
        parent.localDomainAccessStore->removeDomainRole(QString::fromStdString(changedDre.getUid()),
                                                        QtRole::createQt(changedDre.getRole()));
    }
    LOG_DEBUG(parent.logger,
              QString("Changed DRE: %1").arg(QString::fromStdString(changedDre.toString())));
}

void LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener::onError()
{
    LOG_ERROR(parent.logger, QString("Change of DRE failed!"));
}

LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener::
        MasterAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener::onReceive(
        const StdChangeType::Enum& changeType,
        const StdMasterAccessControlEntry& changedMasterAce)
{
    if (changeType != StdChangeType::REMOVE) {
        parent.localDomainAccessStore->updateMasterAccessControlEntry(
                QtMasterAccessControlEntry::createQt(changedMasterAce));
        LOG_DEBUG(parent.logger,
                  QString("Changed MasterAce: %1")
                          .arg(QString::fromStdString(changedMasterAce.toString())));
    } else {
        parent.localDomainAccessStore->removeMasterAccessControlEntry(
                QString::fromStdString(changedMasterAce.getUid()),
                QString::fromStdString(changedMasterAce.getDomain()),
                QString::fromStdString(changedMasterAce.getInterfaceName()),
                QString::fromStdString(changedMasterAce.getOperation()));
        LOG_DEBUG(parent.logger,
                  QString("Removed MasterAce: %1")
                          .arg(QString::fromStdString(changedMasterAce.toString())));
    }
}

void LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener::onError()
{
    LOG_ERROR(parent.logger, QString("Change of MasterAce failed!"));
}

LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener::
        MediatorAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener::onReceive(
        const StdChangeType::Enum& changeType,
        const StdMasterAccessControlEntry& changedMediatorAce)
{
    if (changeType != StdChangeType::REMOVE) {
        parent.localDomainAccessStore->updateMediatorAccessControlEntry(
                QtMasterAccessControlEntry::createQt(changedMediatorAce));
    } else {
        parent.localDomainAccessStore->removeMediatorAccessControlEntry(
                QString::fromStdString(changedMediatorAce.getUid()),
                QString::fromStdString(changedMediatorAce.getDomain()),
                QString::fromStdString(changedMediatorAce.getInterfaceName()),
                QString::fromStdString(changedMediatorAce.getOperation()));
    }
    LOG_DEBUG(parent.logger,
              QString("Changed MediatorAce: %1")
                      .arg(QString::fromStdString(changedMediatorAce.toString())));
}

void LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener::onError()
{
    LOG_ERROR(parent.logger, QString("Change of MediatorAce failed!"));
}

LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener::
        OwnerAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent)
        : parent(parent)
{
}

void LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener::onReceive(
        const StdChangeType::Enum& changeType,
        const StdOwnerAccessControlEntry& changedOwnerAce)
{
    if (changeType != StdChangeType::REMOVE) {
        parent.localDomainAccessStore->updateOwnerAccessControlEntry(
                QtOwnerAccessControlEntry::createQt(changedOwnerAce));
    } else {
        parent.localDomainAccessStore->removeOwnerAccessControlEntry(
                QString::fromStdString(changedOwnerAce.getUid()),
                QString::fromStdString(changedOwnerAce.getDomain()),
                QString::fromStdString(changedOwnerAce.getInterfaceName()),
                QString::fromStdString(changedOwnerAce.getOperation()));
    }
    LOG_DEBUG(parent.logger,
              QString("Changed OwnerAce: %1")
                      .arg(QString::fromStdString(changedOwnerAce.toString())));
}

void LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener::onError()
{
    LOG_ERROR(parent.logger, QString("Change of OwnerAce failed!"));
}

template <typename T>
bool LocalDomainAccessController::onlyWildcardOperations(const QList<T> aceEntries)
{
    if (aceEntries.isEmpty()) {
        return true;
    }

    if (aceEntries.size() > 1) {
        return false;
    }

    return aceEntries.first().getOperation() == LocalDomainAccessStore::WILDCARD;
}

} // namespace joynr

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

#include "joynr/infrastructure/DomainRoleEntry.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerDomainRoleEntryChangedBroadcastFilterParameters.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerMasterAccessControlEntryChangedBroadcastFilterParameters.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerOwnerAccessControlEntryChangedBroadcastFilterParameters.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerMediatorAccessControlEntryChangedBroadcastFilterParameters.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/RequestStatus.h"
#include "joynr/OnChangeSubscriptionQos.h"

#include "joynr/joynrlogging.h"

#include <cassert>
#include <QAtomicInt>

namespace joynr
{

using namespace infrastructure;
using namespace joynr_logging;

Logger* LocalDomainAccessController::logger =
        Logging::getInstance()->getLogger("MSG", "LocalDomainAccessController");

qint64 LocalDomainAccessController::broadcastMinIntervalMs = 1 * 1000;
qint64 LocalDomainAccessController::broadcastSubscriptionValidityMs =
        10 * 365 * 24 * 3600 * 1000LL; // 10 years
qint64 LocalDomainAccessController::broadcastPublicationTtlMs = 5 * 1000;

//--- Declarations of nested classes -------------------------------------------

class LocalDomainAccessController::Initialiser
{
public:
    Initialiser(LocalDomainAccessController& parent,
                const QString& domain,
                const QString& interfaceName);

    // Called to indicate that some data has been initialised
    void update();

    // Called to abort the initialisation
    void abort();

private:
    QAtomicInt counter;
    QAtomicInt aborted;
    LocalDomainAccessController& parent;
    QString domain;
    QString interfaceName;
};

class LocalDomainAccessController::DomainRoleEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::ChangeType::Enum,
                                       infrastructure::DomainRoleEntry>
{
public:
    DomainRoleEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::ChangeType::Enum& changeType,
                   const infrastructure::DomainRoleEntry& changedDre);
    void onError();

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MasterAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::ChangeType::Enum,
                                       infrastructure::MasterAccessControlEntry>
{
public:
    MasterAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::ChangeType::Enum& changeType,
                   const infrastructure::MasterAccessControlEntry& changedMasterAce);
    void onError();

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::MediatorAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::ChangeType::Enum,
                                       infrastructure::MasterAccessControlEntry>
{
public:
    MediatorAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::ChangeType::Enum& changeType,
                   const infrastructure::MasterAccessControlEntry& changedMediatorAce);
    void onError();

private:
    LocalDomainAccessController& parent;
};

class LocalDomainAccessController::OwnerAccessControlEntryChangedBroadcastListener
        : public ISubscriptionListener<infrastructure::ChangeType::Enum,
                                       infrastructure::OwnerAccessControlEntry>
{
public:
    OwnerAccessControlEntryChangedBroadcastListener(LocalDomainAccessController& parent);
    void onReceive(const infrastructure::ChangeType::Enum& changeType,
                   const infrastructure::OwnerAccessControlEntry& changedOwnerAce);
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
                                          Role::Enum role)
{
    LOG_DEBUG(logger, QString("execute: entering hasRole"));

    // See if the user has the given role
    bool hasRole = false;
    Optional<DomainRoleEntry> dre = localDomainAccessStore->getDomainRole(userId, role);
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
        const QString& userId,
        const QString& domain,
        const QString& interfaceName,
        TrustLevel::Enum trustLevel,
        QSharedPointer<IGetConsumerPermissionCallback> callback)
{
    LOG_DEBUG(logger, QString("Entering getConsumerPermission with unknown operation"));

    // Is the ACL for this domain/interface available?
    QString compoundKey = createCompoundKey(domain, interfaceName);
    bool needsInit = false;
    {
        QMutexLocker lock(&initStateMutex);
        if (!aceSubscriptions.contains(compoundKey)) {
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
    QList<MasterAccessControlEntry> masterAces =
            localDomainAccessStore->getMasterAccessControlEntries(userId, domain, interfaceName);
    QList<MasterAccessControlEntry> mediatorAces =
            localDomainAccessStore->getMediatorAccessControlEntries(userId, domain, interfaceName);
    QList<OwnerAccessControlEntry> ownerAces =
            localDomainAccessStore->getOwnerAccessControlEntries(userId, domain, interfaceName);

    // The operations of the ACEs should only contain wildcards, if not
    // getConsumerPermission should be called with an operation
    if (!(onlyWildcardOperations(masterAces) && onlyWildcardOperations(mediatorAces) &&
          onlyWildcardOperations(ownerAces))) {
        callback->operationNeeded();
    } else {
        // The operations are all wildcards
        Permission::Enum permission = getConsumerPermission(
                userId, domain, interfaceName, LocalDomainAccessStore::WILDCARD, trustLevel);
        callback->consumerPermission(permission);
    }
}

Permission::Enum LocalDomainAccessController::getConsumerPermission(const QString& userId,
                                                                    const QString& domain,
                                                                    const QString& interfaceName,
                                                                    const QString& operation,
                                                                    TrustLevel::Enum trustLevel)
{
    LOG_DEBUG(logger, QString("Entering getConsumerPermission with known operation"));

    Optional<MasterAccessControlEntry> masterAceOptional =
            localDomainAccessStore->getMasterAccessControlEntry(
                    userId, domain, interfaceName, operation);
    Optional<MasterAccessControlEntry> mediatorAceOptional =
            localDomainAccessStore->getMediatorAccessControlEntry(
                    userId, domain, interfaceName, operation);
    Optional<OwnerAccessControlEntry> ownerAceOptional =
            localDomainAccessStore->getOwnerAccessControlEntry(
                    userId, domain, interfaceName, operation);

    return accessControlAlgorithm.getConsumerPermission(
            masterAceOptional, mediatorAceOptional, ownerAceOptional, trustLevel);
}

QList<MasterAccessControlEntry> LocalDomainAccessController::getMasterAccessControlEntries(
        const QString& uid)
{
    RequestStatus rs;
    QList<MasterAccessControlEntry> resultMasterAces;
    globalDomainAccessControllerProxy->getMasterAccessControlEntries(
            rs, resultMasterAces, uid.toStdString());

    return resultMasterAces;
}

QList<MasterAccessControlEntry> LocalDomainAccessController::getEditableMasterAccessControlEntries(
        const QString& uid)
{
    RequestStatus rs;
    QList<MasterAccessControlEntry> resultMasterAces;
    globalDomainAccessControllerProxy->getEditableMasterAccessControlEntries(
            rs, resultMasterAces, uid.toStdString());

    return resultMasterAces;
}

bool LocalDomainAccessController::updateMasterAccessControlEntry(
        const MasterAccessControlEntry& updatedMasterAce)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->updateMasterAccessControlEntry(
            rs, success, updatedMasterAce);

    return success;
}

bool LocalDomainAccessController::removeMasterAccessControlEntry(const QString& uid,
                                                                 const QString& domain,
                                                                 const QString& interfaceName,
                                                                 const QString& operation)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->removeMasterAccessControlEntry(rs,
                                                                      success,
                                                                      uid.toStdString(),
                                                                      domain.toStdString(),
                                                                      interfaceName.toStdString(),
                                                                      operation.toStdString());

    return success;
}

QList<MasterAccessControlEntry> LocalDomainAccessController::getMediatorAccessControlEntries(
        const QString& uid)
{
    RequestStatus rs;
    QList<MasterAccessControlEntry> resultMediatorAces;
    globalDomainAccessControllerProxy->getMediatorAccessControlEntries(
            rs, resultMediatorAces, uid.toStdString());

    return resultMediatorAces;
}

QList<MasterAccessControlEntry> LocalDomainAccessController::
        getEditableMediatorAccessControlEntries(const QString& uid)
{
    RequestStatus rs;
    QList<MasterAccessControlEntry> resultMediatorAces;
    globalDomainAccessControllerProxy->getEditableMediatorAccessControlEntries(
            rs, resultMediatorAces, uid.toStdString());

    return resultMediatorAces;
}

bool LocalDomainAccessController::updateMediatorAccessControlEntry(
        const MasterAccessControlEntry& updatedMediatorAce)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->updateMediatorAccessControlEntry(
            rs, success, updatedMediatorAce);

    return success;
}

bool LocalDomainAccessController::removeMediatorAccessControlEntry(const QString& uid,
                                                                   const QString& domain,
                                                                   const QString& interfaceName,
                                                                   const QString& operation)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->removeMediatorAccessControlEntry(rs,
                                                                        success,
                                                                        uid.toStdString(),
                                                                        domain.toStdString(),
                                                                        interfaceName.toStdString(),
                                                                        operation.toStdString());

    return success;
}

QList<OwnerAccessControlEntry> LocalDomainAccessController::getOwnerAccessControlEntries(
        const QString& uid)
{
    RequestStatus rs;
    QList<OwnerAccessControlEntry> resultOwnerAces;
    globalDomainAccessControllerProxy->getOwnerAccessControlEntries(
            rs, resultOwnerAces, uid.toStdString());

    return resultOwnerAces;
}

QList<OwnerAccessControlEntry> LocalDomainAccessController::getEditableOwnerAccessControlEntries(
        const QString& uid)
{
    RequestStatus rs;
    QList<OwnerAccessControlEntry> resultOwnerAces;
    globalDomainAccessControllerProxy->getEditableOwnerAccessControlEntries(
            rs, resultOwnerAces, uid.toStdString());

    return resultOwnerAces;
}

bool LocalDomainAccessController::updateOwnerAccessControlEntry(
        const OwnerAccessControlEntry& updatedOwnerAce)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->updateOwnerAccessControlEntry(rs, success, updatedOwnerAce);

    return success;
}

bool LocalDomainAccessController::removeOwnerAccessControlEntry(const QString& uid,
                                                                const QString& domain,
                                                                const QString& interfaceName,
                                                                const QString& operation)
{
    RequestStatus rs;
    bool success;
    globalDomainAccessControllerProxy->removeOwnerAccessControlEntry(rs,
                                                                     success,
                                                                     uid.toStdString(),
                                                                     domain.toStdString(),
                                                                     interfaceName.toStdString(),
                                                                     operation.toStdString());

    return success;
}

Permission::Enum LocalDomainAccessController::getProviderPermission(const QString& uid,
                                                                    const QString& domain,
                                                                    const QString& interfaceName,
                                                                    TrustLevel::Enum trustLevel)
{
    Q_ASSERT_X(false, "getProviderPermission", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return accessControlAlgorithm.getProviderPermission(
            Optional<MasterAccessControlEntry>::createNull(),
            Optional<MasterAccessControlEntry>::createNull(),
            Optional<OwnerAccessControlEntry>::createNull(),
            trustLevel);
}

QList<MasterRegistrationControlEntry> LocalDomainAccessController::
        getMasterRegistrationControlEntries(const QString& uid)
{
    Q_ASSERT_X(false, "getMasterRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return QList<MasterRegistrationControlEntry>();
}

//---- Unused methods copied from Java implementation --------------------------

QList<MasterRegistrationControlEntry> LocalDomainAccessController::
        getEditableMasterRegistrationControlEntries(const QString& uid)
{
    Q_ASSERT_X(false, "getEditableMasterRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return QList<MasterRegistrationControlEntry>();
}

bool LocalDomainAccessController::updateMasterRegistrationControlEntry(
        const MasterRegistrationControlEntry& updatedMasterRce)
{
    Q_ASSERT_X(false, "updateMasterRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(updatedMasterRce);

    return false;
}

bool LocalDomainAccessController::removeMasterRegistrationControlEntry(const QString& uid,
                                                                       const QString& domain,
                                                                       const QString& interfaceName)
{
    Q_ASSERT_X(false, "removeMasterRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return false;
}

QList<MasterRegistrationControlEntry> LocalDomainAccessController::
        getMediatorRegistrationControlEntries(const QString& uid)
{
    Q_ASSERT_X(false, "getMediatorRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return QList<MasterRegistrationControlEntry>();
}

QList<MasterRegistrationControlEntry> LocalDomainAccessController::
        getEditableMediatorRegistrationControlEntries(const QString& uid)
{
    Q_ASSERT_X(false, "getEditableMediatorRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return QList<MasterRegistrationControlEntry>();
}

bool LocalDomainAccessController::updateMediatorRegistrationControlEntry(
        const MasterRegistrationControlEntry& updatedMediatorRce)
{
    Q_ASSERT_X(false, "updateMediatorRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(updatedMediatorRce);

    return false;
}

bool LocalDomainAccessController::removeMediatorRegistrationControlEntry(
        const QString& uid,
        const QString& domain,
        const QString& interfaceName)
{
    Q_ASSERT_X(false, "removeMediatorRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return false;
}

QList<OwnerRegistrationControlEntry> LocalDomainAccessController::
        getOwnerRegistrationControlEntries(const QString& uid)
{
    Q_ASSERT_X(false, "getOwnerRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return QList<OwnerRegistrationControlEntry>();
}

QList<OwnerRegistrationControlEntry> LocalDomainAccessController::
        getEditableOwnerRegistrationControlEntries(const QString& uid)
{
    Q_ASSERT_X(false, "getEditableOwnerRegistrationControlEntries", "Not implemented yet");
    Q_UNUSED(uid);

    return QList<OwnerRegistrationControlEntry>();
}

bool LocalDomainAccessController::updateOwnerRegistrationControlEntry(
        const OwnerRegistrationControlEntry& updatedOwnerRce)
{
    Q_ASSERT_X(false, "updateOwnerRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(updatedOwnerRce);

    return false;
}

bool LocalDomainAccessController::removeOwnerRegistrationControlEntry(const QString& uid,
                                                                      const QString& domain,
                                                                      const QString& interfaceName)
{
    Q_ASSERT_X(false, "removeOwnerRegistrationControlEntry", "Not implemented yet");
    Q_UNUSED(uid);
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);

    return false;
}

void LocalDomainAccessController::unregisterProvider(const QString& domain,
                                                     const QString& interfaceName)
{
    // Get the subscription ids
    QString compoundKey = createCompoundKey(domain, interfaceName);
    AceSubscription subscriptionIds;
    {
        QMutexLocker lock(&initStateMutex);
        if (!aceSubscriptions.contains(compoundKey)) {
            return;
        }
        subscriptionIds = aceSubscriptions.value(compoundKey);
    }

    LOG_DEBUG(logger,
              QString("Unsubscribing from ACL broadcasts for domain %1, interface %2")
                      .arg(domain)
                      .arg(interfaceName));

    // Unsubscribe from ACE change subscriptions
    globalDomainAccessControllerProxy->unsubscribeFromMasterAccessControlEntryChangedBroadcast(
            subscriptionIds.masterAceSubscriptionId);
    globalDomainAccessControllerProxy->unsubscribeFromMediatorAccessControlEntryChangedBroadcast(
            subscriptionIds.mediatorAceSubscriptionId);
    globalDomainAccessControllerProxy->unsubscribeFromOwnerAccessControlEntryChangedBroadcast(
            subscriptionIds.ownerAceSubscriptionId);
}

// Initialise the data for the given data/interface. This function is non-blocking
void LocalDomainAccessController::initialiseLocalDomainAccessStore(const QString& userId,
                                                                   const QString& domain,
                                                                   const QString& interfaceName)
{
    // Create an object to keep track of the initialisation
    QSharedPointer<Initialiser> initialiser(new Initialiser(*this, domain, interfaceName));

    // Initialise domain roles from global data
    // TODO: confirm that this is needed
    std::function<void(const RequestStatus& status,
                       const QList<DomainRoleEntry>& domainRoleEntries)> domainRoleCallbackFct =
            [this, initialiser](
                    const RequestStatus& status, const QList<DomainRoleEntry>& domainRoleEntries) {
        if (status.successful()) {
            // Add the results
            foreach (const DomainRoleEntry& dre, domainRoleEntries) {
                localDomainAccessStore->updateDomainRole(dre);
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
    globalDomainAccessControllerProxy->getDomainRoles(userId.toStdString(), domainRoleCallbackFct);

    std::function<void(const RequestStatus& status,
                       const QList<MasterAccessControlEntry>& masterAces)> masterAceCallbackFct =
            [this, initialiser](const RequestStatus& status,
                                const QList<MasterAccessControlEntry>& masterAces) {
        if (status.successful()) {
            // Add the results
            foreach (const MasterAccessControlEntry& masterAce, masterAces) {
                localDomainAccessStore->updateMasterAccessControlEntry(masterAce);
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
            domain.toStdString(), interfaceName.toStdString(), masterAceCallbackFct);

    // Initialise mediator access control entries from global data
    std::function<
            void(const RequestStatus& status, const QList<MasterAccessControlEntry>& mediatorAces)>
            mediatorAceCallbackFct =
                    [this, initialiser](const RequestStatus& status,
                                        const QList<MasterAccessControlEntry>& mediatorAces) {
        if (status.successful()) {
            // Add the results
            foreach (const MasterAccessControlEntry& mediatorAce, mediatorAces) {
                localDomainAccessStore->updateMediatorAccessControlEntry(mediatorAce);
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
            domain.toStdString(), interfaceName.toStdString(), mediatorAceCallbackFct);

    // Initialise owner access control entries from global data
    std::function<void(const RequestStatus& status,
                       const QList<OwnerAccessControlEntry>& ownerAces)> ownerAceCallbackFct =
            [this, initialiser](
                    const RequestStatus& status, const QList<OwnerAccessControlEntry>& ownerAces) {
        if (status.successful()) {
            // Add the results
            foreach (const OwnerAccessControlEntry& ownerAce, ownerAces) {
                localDomainAccessStore->updateOwnerAccessControlEntry(ownerAce);
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
            domain.toStdString(), interfaceName.toStdString(), ownerAceCallbackFct);
}

// Called when the data for the given domain/interface has been obtained from the GDAC
void LocalDomainAccessController::initialised(const QString& domain, const QString& interfaceName)
{
    QString compoundKey = createCompoundKey(domain, interfaceName);
    QList<ConsumerPermissionRequest> requests;

    {
        QMutexLocker lock(&initStateMutex);

        // Subscribe to ACL broadcasts about this domain/interface
        aceSubscriptions.insert(compoundKey, subscribeForAceChange(domain, interfaceName));

        // Remove requests for processing
        requests = consumerPermissionRequests.take(compoundKey);
    }

    // Handle any queued requests for this domain/interface
    processConsumerRequests(requests);
}

void LocalDomainAccessController::abortInitialisation(const QString& domain,
                                                      const QString& interfaceName)
{
    LOG_INFO(logger,
             QString("Removing outstanding ACL requests for domain %1, interface %2")
                     .arg(domain)
                     .arg(interfaceName));

    QString compoundKey = createCompoundKey(domain, interfaceName);
    QList<ConsumerPermissionRequest> requests;

    {
        QMutexLocker lock(&initStateMutex);

        // Remove requests that cannot be processed
        requests = consumerPermissionRequests.take(compoundKey);
    }

    // Mark all the requests as failed - we have no information from the Global
    // Domain Access Controller
    foreach (const ConsumerPermissionRequest& request, requests) {
        request.callbacks->consumerPermission(Permission::NO);
    }
}

// Returns true if other requests have already been queued
bool LocalDomainAccessController::queueConsumerRequest(const QString& key,
                                                       const ConsumerPermissionRequest& request)
{
    // This function assumes that the initStateMutex has already been obtained

    if (consumerPermissionRequests.contains(key)) {
        consumerPermissionRequests[key].append(request);
        return true;
    } else {
        QList<ConsumerPermissionRequest> requestList;
        requestList << request;
        consumerPermissionRequests.insert(key, requestList);
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
    QSharedPointer<OnChangeSubscriptionQos> broadcastSubscriptionQos(new OnChangeSubscriptionQos());
    broadcastSubscriptionQos->setMinInterval(broadcastMinIntervalMs);
    broadcastSubscriptionQos->setValidity(broadcastSubscriptionValidityMs);
    broadcastSubscriptionQos->setPublicationTtl(broadcastPublicationTtlMs);
    GlobalDomainAccessControllerDomainRoleEntryChangedBroadcastFilterParameters
            domainRoleFilterParameters;
    domainRoleFilterParameters.setUserIdOfInterest(userId);

    std::string subscriptionId =
            globalDomainAccessControllerProxy->subscribeToDomainRoleEntryChangedBroadcast(
                    domainRoleFilterParameters,
                    domainRoleEntryChangedBroadcastListener
                            .staticCast<ISubscriptionListener<ChangeType::Enum, DomainRoleEntry>>(),
                    broadcastSubscriptionQos);

    return QString::fromStdString(subscriptionId);
}

LocalDomainAccessController::AceSubscription LocalDomainAccessController::subscribeForAceChange(
        const QString& domain,
        const QString& interfaceName)
{
    QSharedPointer<OnChangeSubscriptionQos> broadcastSubscriptionQos(new OnChangeSubscriptionQos());
    broadcastSubscriptionQos->setMinInterval(broadcastMinIntervalMs);
    broadcastSubscriptionQos->setValidity(broadcastSubscriptionValidityMs);
    broadcastSubscriptionQos->setPublicationTtl(broadcastPublicationTtlMs);

    AceSubscription subscriptionIds;

    GlobalDomainAccessControllerMasterAccessControlEntryChangedBroadcastFilterParameters
            masterAcefilterParameters;
    masterAcefilterParameters.setDomainOfInterest(domain);
    masterAcefilterParameters.setInterfaceOfInterest(interfaceName);
    subscriptionIds.masterAceSubscriptionId =
            globalDomainAccessControllerProxy->subscribeToMasterAccessControlEntryChangedBroadcast(
                    masterAcefilterParameters,
                    masterAccessControlEntryChangedBroadcastListener.staticCast<
                            ISubscriptionListener<ChangeType::Enum, MasterAccessControlEntry>>(),
                    broadcastSubscriptionQos);

    GlobalDomainAccessControllerMediatorAccessControlEntryChangedBroadcastFilterParameters
            mediatorAceFilterParameters;
    mediatorAceFilterParameters.setDomainOfInterest(domain);
    mediatorAceFilterParameters.setInterfaceOfInterest(interfaceName);
    subscriptionIds.mediatorAceSubscriptionId =
            globalDomainAccessControllerProxy
                    ->subscribeToMediatorAccessControlEntryChangedBroadcast(
                            mediatorAceFilterParameters,
                            mediatorAccessControlEntryChangedBroadcastListener
                                    .staticCast<ISubscriptionListener<ChangeType::Enum,
                                                                      MasterAccessControlEntry>>(),
                            broadcastSubscriptionQos);

    GlobalDomainAccessControllerOwnerAccessControlEntryChangedBroadcastFilterParameters
            ownerAceFilterParameters;
    ownerAceFilterParameters.setDomainOfInterest(domain);
    ownerAceFilterParameters.setInterfaceOfInterest(interfaceName);
    subscriptionIds.ownerAceSubscriptionId =
            globalDomainAccessControllerProxy->subscribeToOwnerAccessControlEntryChangedBroadcast(
                    ownerAceFilterParameters,
                    ownerAccessControlEntryChangedBroadcastListener.staticCast<
                            ISubscriptionListener<ChangeType::Enum, OwnerAccessControlEntry>>(),
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

//--- Implementation of Initialiser --------------------------------------------

LocalDomainAccessController::Initialiser::Initialiser(LocalDomainAccessController& parent,
                                                      const QString& domain,
                                                      const QString& interfaceName)
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
        const ChangeType::Enum& changeType,
        const DomainRoleEntry& changedDre)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateDomainRole(changedDre);
    } else {
        parent.localDomainAccessStore->removeDomainRole(changedDre.getUid(), changedDre.getRole());
    }
    LOG_DEBUG(parent.logger, QString("Changed DRE: %1").arg(changedDre.toString()));
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
        const ChangeType::Enum& changeType,
        const MasterAccessControlEntry& changedMasterAce)
{
    if (changeType != ChangeType::REMOVE) {
        parent.localDomainAccessStore->updateMasterAccessControlEntry(changedMasterAce);
        LOG_DEBUG(parent.logger, QString("Changed MasterAce: %1").arg(changedMasterAce.toString()));
    } else {
        parent.localDomainAccessStore->removeMasterAccessControlEntry(
                changedMasterAce.getUid(),
                changedMasterAce.getDomain(),
                changedMasterAce.getInterfaceName(),
                changedMasterAce.getOperation());
        LOG_DEBUG(parent.logger, QString("Removed MasterAce: %1").arg(changedMasterAce.toString()));
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
    LOG_DEBUG(parent.logger, QString("Changed MediatorAce: %1").arg(changedMediatorAce.toString()));
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
    LOG_DEBUG(parent.logger, QString("Changed OwnerAce: %1").arg(changedOwnerAce.toString()));
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

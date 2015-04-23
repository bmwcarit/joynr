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

#ifndef LOCALDOMAINACCESSCONTROLLER_H
#define LOCALDOMAINACCESSCONTROLLER_H

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/infrastructure/MasterAccessControlEntry.h"
#include "joynr/infrastructure/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/OwnerRegistrationControlEntry.h"
#include "joynr/infrastructure/Permission.h"
#include "joynr/infrastructure/TrustLevel.h"
#include "joynr/infrastructure/Role.h"
#include "joynr/ISubscriptionListener.h"
#include "AccessControlAlgorithm.h"
#include "joynr/PrivateCopyAssign.h"

#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QMutex>

namespace joynr
{
namespace joynr_logging
{
class Logger;
}

namespace infrastructure
{
class GlobalDomainAccessControllerProxy;
}

class LocalDomainAccessStore;

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
        virtual ~IGetConsumerPermissionCallback()
        {
        }

        // Called with the result of a consumer permission request
        virtual void consumerPermission(infrastructure::Permission::Enum permission) = 0;

        // Called when an operation is needed to get the consumer permission
        virtual void operationNeeded() = 0;
    };

    LocalDomainAccessController(LocalDomainAccessStore* localDomainAccessStore);
    virtual ~LocalDomainAccessController();
    /**
     * The init method has to be called first, only afterwards LocalDomainAccessController may be
     * used.
     */
    virtual void init(QSharedPointer<infrastructure::GlobalDomainAccessControllerProxy>
                              globalDomainAccessControllerProxy);

    /**
     * Check if user uid has role role for domain.
     * Used by an ACL editor app to verify whether the user is allowed to change ACEs or not
     *
     * \param userId The user accessing the interface
     * \param domain The trust level of the device accessing the interface
     * \param role The domain that is being accessed
     * \return Returns true, if user uid has role role for domain domain.
     */
    virtual bool hasRole(const QString& userId,
                         const QString& domain,
                         infrastructure::Role::Enum role);

    /**
      * Get consumer permission to access an interface
      *
      * \param userId        The user accessing the interface
      * \param domain        The domain that is being accessed
      * \param interfaceName The interface that is being accessed
      * \param trustLevel    The trust level of the device accessing the interface
      * \param callbacks     Object that will receive the result and then be deleted
      *
      * Use :
      *    getConsumerPermission(String, String, String, String, TrustLevel, callbacks)
      * to gain exact Permission on interface operation.
      */
    virtual void getConsumerPermission(const QString& userId,
                                       const QString& domain,
                                       const QString& interfaceName,
                                       infrastructure::TrustLevel::Enum trustLevel,
                                       QSharedPointer<IGetConsumerPermissionCallback> callback);

    /**
      * Get consumer permission to access an interface operation
      *
      * \param userId        The user accessing the interface
      * \param domain        The domain that is being accessed
      * \param interfaceName The interface that is being accessed
      * \param operation     The operation user requests to execute on interface
      * \param trustLevel    The trust level of the device accessing the interface
      * \return the permission.
      *
      * This synchronous function assumes that the data to do ACL checks is available
      * and has been obtained through a call to getConsumerPermission()
      */
    virtual infrastructure::Permission::Enum getConsumerPermission(
            const QString& userId,
            const QString& domain,
            const QString& interfaceName,
            const QString& operation,
            infrastructure::TrustLevel::Enum trustLevel);

    /**
     * Returns a list of entries that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * Used by an Master ACL GUI to show access rights of a user.
     *
     * \param uid The userId of the caller.
     * \return A list of master ACEs for specified uid.
     */
    virtual QList<infrastructure::MasterAccessControlEntry> getMasterAccessControlEntries(
            const QString& uid);

    /**
     * Returns a list of editable master access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Master ACL GUI to show access rights of a user.
     *
     * \param uid The userId of the caller.
     * \return A list of editable master ACEs for specified uid.
     */
    virtual QList<infrastructure::MasterAccessControlEntry> getEditableMasterAccessControlEntries(
            const QString& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     * existent.
     *
     * \param updatedMasterAce The master ACE that has to be updated/added to the ACL store.
     * \return true if update succeeded.
     */
    virtual bool updateMasterAccessControlEntry(
            const infrastructure::MasterAccessControlEntry& updatedMasterAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * \param uid The userId of the control entry.
     * \param domain The domain of the control entry.
     * \param interfaceName The interfaceName of the control entry.
     * \param operation The operation of the control entry.
     * \return true if remove succeeded.
     */
    virtual bool removeMasterAccessControlEntry(const QString& uid,
                                                const QString& domain,
                                                const QString& interfaceName,
                                                const QString& operation);

    /**
     * Returns a list of entries that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * Used by an Mediator ACL GUI to show access rights of a user.
     *
     * \param uid The userId of the caller.
     * \return A list of mediator ACEs for specified uid.
     */
    virtual QList<infrastructure::MasterAccessControlEntry> getMediatorAccessControlEntries(
            const QString& uid);

    /**
     * Returns a list of editable mediator access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Mediator ACL GUI to show access rights of a user.
     * Calling this function blocks calling thread until update operation finish.
     *
     * \param uid The userId of the caller.
     * \return A list of editable mediator ACEs for specified uid.
     */
    virtual QList<infrastructure::MasterAccessControlEntry> getEditableMediatorAccessControlEntries(
            const QString& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * \param updatedMediatorAce The mediator ACE that has to be updated/added to the ACL store.
     * \return true if update succeeded.
     */
    virtual bool updateMediatorAccessControlEntry(
            const infrastructure::MasterAccessControlEntry& updatedMediatorAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * \param uid The userId of the control entry.
     * \param domain The domain of the control entry.
     * \param interfaceName The interfaceName of the control entry.
     * \param operation The operation of the control entry.
     * \return true if remove succeeded.
     */
    virtual bool removeMediatorAccessControlEntry(const QString& uid,
                                                  const QString& domain,
                                                  const QString& interfaceName,
                                                  const QString& operation);

    /**
     * Returns a list of entries that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * Used by an Owner ACL GUI to show access rights of a user.
     *
     * \param uid The userId of the caller.
     * \return A list of owner ACEs for specified uid.
     */
    virtual QList<infrastructure::OwnerAccessControlEntry> getOwnerAccessControlEntries(
            const QString& uid);

    /**
     * Returns a list of editable owner access control entries that apply to user uid,
     * i.e. the entries for which uid has role OWNER.
     * Used by an Owner ACL GUI to show access rights of a user.
     * Calling this function blocks calling thread until update operation finish.
     *
     * \param uid The userId of the caller.
     * \return A list of editable owner ACEs for specified uid.
     */
    virtual QList<infrastructure::OwnerAccessControlEntry> getEditableOwnerAccessControlEntries(
            const QString& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * \param updatedOwnerAce The owner ACE that has to be updated/added to the ACL store.
     * \return true if update succeeded.
     */
    virtual bool updateOwnerAccessControlEntry(
            const infrastructure::OwnerAccessControlEntry& updatedOwnerAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * \param uid The userId of the control entry.
     * \param domain The domain of the control entry.
     * \param interfaceName The interfaceName of the control entry.
     * \param operation The operation of the control entry.
     * \return true if remove succeeded.
     */
    virtual bool removeOwnerAccessControlEntry(const QString& uid,
                                               const QString& domain,
                                               const QString& interfaceName,
                                               const QString& operation);

    /**
     * Get provider permission to expose an interface
     *
     * \param uid        The userId of the provider exposing the interface
     * \param domain        The domain where interface belongs
     * \param interfaceName The interface that is being accessed
     * \param trustLevel    The trust level of the device accessing the interface
     */
    virtual infrastructure::Permission::Enum getProviderPermission(
            const QString& uid,
            const QString& domain,
            const QString& interfaceName,
            infrastructure::TrustLevel::Enum trustLevel);

    /**
     * Returns a list of master registration control entries that apply to provider uid, i.e. the
     *entries that define the registration rights of the provider uid.
     * Used by an Master RCL GUI to show registration rights of a provider.
     * Calling this function blocks calling thread until update operation finish.
     *
     * \param uid The provider userId.
     * \return A list of master RCEs for specified uid.
     */
    virtual QList<infrastructure::MasterRegistrationControlEntry>
    getMasterRegistrationControlEntries(const QString& uid);

    /**
     * Returns a list of editable master registration entries applying to domains the user uid has
     *role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Master ACL editor app.
     *
     * \param uid The userId of the caller.
     * \return A list of entries applying to domains the user uid has role Master.
     */
    virtual QList<infrastructure::MasterRegistrationControlEntry>
    getEditableMasterRegistrationControlEntries(const QString& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * \param updatedMasterRce The master RCE to be updated.
     * \return true if update succeeded.
     */
    virtual bool updateMasterRegistrationControlEntry(
            const infrastructure::MasterRegistrationControlEntry& updatedMasterRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * \param uid Provider userId.
     * \param domain Domain where provider has been registered.
     * \param interfaceName Provider interface.
     * \return true if remove succeeded.
     */
    virtual bool removeMasterRegistrationControlEntry(const QString& uid,
                                                      const QString& domain,
                                                      const QString& interfaceName);

    /**
     * Returns a list of mediator registration control entries that apply to provider uid, i.e. the
     *entries that define the registration rights of the provider uid.
     * Used by an Mediator RCL GUI to show registration rights of a provider.
     * Calling this function blocks calling thread until update operation finish.
     *
     * \param uid The provider userId.
     * \return A list of mediator RCEs for specified uid.
     */
    virtual QList<infrastructure::MasterRegistrationControlEntry>
    getMediatorRegistrationControlEntries(const QString& uid);

    /**
     * Returns a list of editable mediator registration entries applying to domains the user uid has
     *role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Mediator ACL editor app.
     *
     * \param uid The userId of the caller.
     * \return A list of entries applying to domains the user uid has role Master.
     */
    virtual QList<infrastructure::MasterRegistrationControlEntry>
    getEditableMediatorRegistrationControlEntries(const QString& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * \param updatedMediatorRce The mediator RCE to be updated.
     * \return true if update succeeded.
     */
    virtual bool updateMediatorRegistrationControlEntry(
            const infrastructure::MasterRegistrationControlEntry& updatedMediatorRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * \param uid Provider userId.
     * \param domain Domain where provider has been registered.
     * \param interfaceName Provider interface.
     * \return true if remove succeeded.
     */
    virtual bool removeMediatorRegistrationControlEntry(const QString& uid,
                                                        const QString& domain,
                                                        const QString& interfaceName);

    /**
     * Returns a list of owner registration control entries that apply to provider uid, i.e. the
     *entries that define the registration rights of the provider uid.
     * Used by an Owner RCL GUI to show registration rights of a provider.
     * Calling this function blocks calling thread until update operation finish.
     *
     * \param uid The provider userId.
     * \return A list of owner RCEs for specified uid.
     */
    virtual QList<infrastructure::OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(
            const QString& uid);

    /**
     * Returns a list of editable owner registration entries applying to domains the user uid has
     *role Owner,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     *
     * \param uid The userId of the caller.
     * \return A list of entries applying to domains the user uid has role Owner.
     */
    virtual QList<infrastructure::OwnerRegistrationControlEntry>
    getEditableOwnerRegistrationControlEntries(const QString& uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already
     *existent.
     *
     * \param updatedOwnerRce The owner RCE to be updated.
     * \return true if update succeeded.
     */
    virtual bool updateOwnerRegistrationControlEntry(
            const infrastructure::OwnerRegistrationControlEntry& updatedOwnerRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * \param uid Provider userId.
     * \param domain Domain where provider has been registered.
     * \param interfaceName Provider interface.
     * \return true if remove succeeded.
     */
    virtual bool removeOwnerRegistrationControlEntry(const QString& uid,
                                                     const QString& domain,
                                                     const QString& interfaceName);

    /**
     * Unregisters a domain/interface from access control.
     * \param domain The domain of the provider being unregistered.
     * \param interfaceName The interface of the provider being unregistered
     */
    virtual void unregisterProvider(const QString& domain, const QString& interfaceName);

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessController);

    AccessControlAlgorithm accessControlAlgorithm;
    QHash<QString, QString> dreSubscriptions;

    struct AceSubscription
    {
        QString masterAceSubscriptionId;
        QString mediatorAceSubscriptionId;
        QString ownerAceSubscriptionId;

        AceSubscription()
                : masterAceSubscriptionId(), mediatorAceSubscriptionId(), ownerAceSubscriptionId()
        {
        }
    };

    QHash<QString, AceSubscription> aceSubscriptions;

    QSharedPointer<infrastructure::GlobalDomainAccessControllerProxy>
            globalDomainAccessControllerProxy;
    LocalDomainAccessStore* localDomainAccessStore;

    static joynr_logging::Logger* logger;
    static qint64 broadcastMinIntervalMs;
    static qint64 broadcastSubscriptionValidityMs;
    static qint64 broadcastPublicationTtlMs;

    void initialiseLocalDomainAccessStore(const QString& userId,
                                          const QString& domain,
                                          const QString& interfaceName);

    void initialised(const QString& domain, const QString& interfaceName);
    void abortInitialisation(const QString& domain, const QString& interfaceName);

    QString subscribeForDreChange(const QString& userId);
    AceSubscription subscribeForAceChange(const QString& domain, const QString& interfaceName);
    QString createCompoundKey(const QString& domain, const QString& interfaceName);

    template <typename T>
    bool onlyWildcardOperations(const QList<T> aceEntries);

    // Requests waiting to get consumer permission
    struct ConsumerPermissionRequest
    {
        QString userId;
        QString domain;
        QString interfaceName;
        infrastructure::TrustLevel::Enum trustLevel;
        QSharedPointer<IGetConsumerPermissionCallback> callbacks;
    };

    QHash<QString, QList<ConsumerPermissionRequest>> consumerPermissionRequests;

    bool queueConsumerRequest(const QString& key, const ConsumerPermissionRequest& request);
    void processConsumerRequests(const QList<ConsumerPermissionRequest>& requests);

    // Mutex that protects all member variables involved in initialisation
    // of data for a domain/interface
    // - aceSubscriptions
    // - consumerPermissionRequests
    QMutex initStateMutex;

    // Class that keeps track of initialisation for a domain/interface
    class Initialiser;

    // Classes used to receive broadcasts

    class DomainRoleEntryChangedBroadcastListener;
    QSharedPointer<DomainRoleEntryChangedBroadcastListener> domainRoleEntryChangedBroadcastListener;

    class MasterAccessControlEntryChangedBroadcastListener;
    QSharedPointer<MasterAccessControlEntryChangedBroadcastListener>
            masterAccessControlEntryChangedBroadcastListener;

    class MediatorAccessControlEntryChangedBroadcastListener;
    QSharedPointer<MediatorAccessControlEntryChangedBroadcastListener>
            mediatorAccessControlEntryChangedBroadcastListener;

    class OwnerAccessControlEntryChangedBroadcastListener;
    QSharedPointer<OwnerAccessControlEntryChangedBroadcastListener>
            ownerAccessControlEntryChangedBroadcastListener;
};
} // namespace joynr
#endif // LOCALDOMAINACCESSCONTROLLER_H

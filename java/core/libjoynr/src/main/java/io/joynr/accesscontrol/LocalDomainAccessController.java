package io.joynr.accesscontrol;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import joynr.infrastructure.Role;
import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.Permission;
import joynr.infrastructure.MasterRegistrationControlEntry;
import joynr.infrastructure.OwnerRegistrationControlEntry;
import joynr.infrastructure.TrustLevel;

import javax.annotation.CheckForNull;
import java.util.List;

/**
 * Manages access control lists for local providers.
 */
public interface LocalDomainAccessController {

    /**
     * Check if user uid has role role for domain.
     * Used by an ACL editor app to verify whether the user is allowed to change ACEs or not
     *
     * @param userId        The user accessing the interface
     * @param domain    The trust level of the device accessing the interface
     * @param role        The domain that is being accessed
     * @return Returns true, if user uid has role role for domain domain.
     */
    boolean hasRole(String userId, String domain, Role role);

    /**
     * Get consumer permission to access an interface
     *
     * @param userId        The user accessing the interface
     * @param domain        The domain that is being accessed
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     * @return Permission to access given interface, or NULL if there is more than one ACE for given uid, domain, interfaceName.
     * If function returns NULL, use {@link io.joynr.accesscontrol.LocalDomainAccessController#getConsumerPermission(String, String, String, String, joynr.infrastructure.TrustLevel)} to gain Permission on interface operation.
     */
    @CheckForNull
    Permission getConsumerPermission(String userId, String domain, String interfaceName, TrustLevel trustLevel);

    /**
     * Get consumer permission to access an interface operation
     *
     * @param userId        The user accessing the interface
     * @param domain        The domain that is being accessed
     * @param interfaceName The interface that is being accessed
     * @param operation    The operation user requests to execute on interface
     * @param trustLevel    The trust level of the device accessing the interface
     * @return Permission to access given interface operation.
     */
    Permission getConsumerPermission(String userId,
                                     String domain,
                                     String interfaceName,
                                     String operation,
                                     TrustLevel trustLevel);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access rights of the user uid.
     * Used by an Master ACL GUI to show access rights of a user.
     *
     * @param uid The userId of the caller.
     * @return A list of master ACEs for specified uid.
     */
    List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already existent.
     *
     * @param updatedMasterAce The master ACE that has to be updated/added to the ACL store.
     * @return true if update succeeded.
     */
    boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid The userId of the control entry.
     * @param domain The domain of the control entry.
     * @param interfaceName The interfaceName of the control entry.
     * @param operation The operation of the control entry.
     * @return true if remove succeeded.
     */
    boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access rights of the user uid.
     * Used by an Mediator ACL GUI to show access rights of a user.
     *
     * @param uid The userId of the caller.
     * @return A list of mediator ACEs for specified uid.
     */
    List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already existent.
     *
     * @param updatedMediatorAce The mediator ACE that has to be updated/added to the ACL store.
     * @return true if update succeeded.
     */
    boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid The userId of the control entry.
     * @param domain The domain of the control entry.
     * @param interfaceName The interfaceName of the control entry.
     * @param operation The operation of the control entry.
     * @return true if remove succeeded.
     */
    boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access rights of the user uid.
     * Used by an Owner ACL GUI to show access rights of a user.
     *
     * @param uid The userId of the caller.
     * @return A list of owner ACEs for specified uid.
     */
    List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already existent.
     *
     * @param updatedOwnerAce The owner ACE that has to be updated/added to the ACL store.
     * @return true if update succeeded.
     */
    boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid The userId of the control entry.
     * @param domain The domain of the control entry.
     * @param interfaceName The interfaceName of the control entry.
     * @param operation The operation of the control entry.
     * @return true if remove succeeded.
     */
    boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation);

    /**
     * Get provider permission to expose an interface
     *
     * @param uid        The userId of the provider exposing the interface
     * @param domain        The domain where interface belongs
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     */
    Permission getProviderPermission(String uid, String domain, String interfaceName, TrustLevel trustLevel);

    /**
     * Returns a list of editable master registration entries applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Master ACL editor app.
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Master.
     */
    List<MasterAccessControlEntry> getEditableMasterRegistrationControlEntries(String uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already existent.
     *
     * @param updatedMasterRce The master RCE to be updated.
     * @return true if update succeeded.
     */
    boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded.
     */
    boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName);

    /**
     * Returns a list of editable mediator registration entries applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Mediator ACL editor app.
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Master.
     */
    List<MasterAccessControlEntry> getEditableMediatorRegistrationControlEntries(String uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already existent.
     *
     * @param updatedMediatorRce The mediator RCE to be updated.
     * @return true if update succeeded.
     */
    boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded.
     */
    boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName);

    /**
     * Returns a list of editable owner registration entries applying to domains the user uid has role Owner,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     *
     * @param uid The userId of the caller.
     * @return A list of entries applying to domains the user uid has role Owner.
     */
    List<OwnerAccessControlEntry> getEditableOwnerRegistrationControlEntries(String uid);

    /**
     * Updates an existing entry (according to primary key) or adds a new entry if not already existent.
     *
     * @param updatedOwnerRce The owner RCE to be updated.
     * @return true if update succeeded.
     */
    boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce);

    /**
     * Removes an existing entry (according to primary key).
     *
     * @param uid Provider userId.
     * @param domain Domain where provider has been registered.
     * @param interfaceName Provider interface.
     * @return true if remove succeeded.
     */
    boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName);
}

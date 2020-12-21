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
package io.joynr.accesscontrol;

import java.util.List;

import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

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
     * @param callback      Will be called when the permission is available. The callback's
     *                      argument is the permission to access the given interface, or NULL if
     *                      there is more than one ACE for given uid, domain, interfaceName.
     *                      If the callback returns NULL, use {@link #getConsumerPermission(String,
     *                      String, String, String, TrustLevel)} to gain Permission on interface operation.
     */
    void getConsumerPermission(String userId,
                               String domain,
                               String interfaceName,
                               TrustLevel trustLevel,
                               GetConsumerPermissionCallback callback);

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
     * Returns a list of editable master access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Master ACL GUI to show access rights of a user.
     * Calling this function blocks the calling thread until the update operation is finished.
     *
     * @param uid The userId of the caller.
     * @return A list of editable master ACEs for specified uid.
     */
    List<MasterAccessControlEntry> getEditableMasterAccessControlEntries(String uid);

    /**
     * Returns a list of editable master access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Master ACL GUI to show access rights of a user.
     * Calling this function doesn't block the calling thread.
     *
     * @param uid The userId of the caller.
     * @param callback function result handler
     * @return Future object whose shared state is made ready when the execution of the function ends.
     * Shared state is a list of editable master ACEs for specified uid.
     */
    Future<List<MasterAccessControlEntry>> getEditableMasterAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                                 String uid);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access rights of the user uid.
     * Used by an Mediator ACL GUI to show access rights of a user.
     * Calling this function doesn't block the calling thread.
     *
     * @param uid The userId of the caller.
     * @param callback function result handler
     * @return Future object whose shared state is made ready when the execution of the function ends.
     * Shared state is a list of mediator ACEs for specified uid.
     */
    Future<List<MasterAccessControlEntry>> getMediatorAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                           String uid);

    /**
     * Returns a list of editable mediator access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Mediator ACL GUI to show access rights of a user.
     * Calling this function blocks the calling thread until the update operation is finished.
     *
     * @param uid The userId of the caller.
     * @return A list of editable mediator ACEs for specified uid.
     */
    List<MasterAccessControlEntry> getEditableMediatorAccessControlEntries(String uid);

    /**
     * Returns a list of editable mediator access control entries that apply to user uid,
     * i.e. the entries for which uid has role MASTER.
     * Used by an Mediator ACL GUI to show access rights of a user.
     * Calling this function doesn't block the calling thread.
     *
     * @param uid The userId of the caller.
     * @param callback function result handler
     * @return Future object whose shared state is made ready when the execution of the function ends.
     * Shared state is a list of editable mediator ACEs for specified uid.
     */
    Future<List<MasterAccessControlEntry>> getEditableMediatorAccessControlEntries(Callback<List<MasterAccessControlEntry>> callback,
                                                                                   String uid);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access rights of the user uid.
     * Used by an Owner ACL GUI to show access rights of a user.
     * Calling this function doesn't block the calling thread.
     *
     * @param uid The userId of the caller.
     * @param callback function result handler
     * @return Future object whose shared state is made ready when the execution of the function ends.
     * Shared state is a list of owner ACEs for specified uid.
     */
    Future<List<OwnerAccessControlEntry>> getOwnerAccessControlEntries(Callback<List<OwnerAccessControlEntry>> callback,
                                                                       String uid);

    /**
     * Returns a list of editable owner access control entries that apply to user uid,
     * i.e. the entries for which uid has role OWNER.
     * Used by an Owner ACL GUI to show access rights of a user.
     * Calling this function blocks the calling thread until the update operation is finished.
     *
     * @param uid The userId of the caller.
     * @return A list of editable owner ACEs for specified uid.
     */
    List<OwnerAccessControlEntry> getEditableOwnerAccessControlEntries(String uid);

    /**
     * Returns a list of editable owner access control entries that apply to user uid,
     * i.e. the entries for which uid has role OWNER.
     * Used by an Owner ACL GUI to show access rights of a user.
     * Calling this function doesn't block the calling thread.
     *
     * @param uid The userId of the caller.
     * @param callback function result handler
     * @return Future object whose shared state is made ready when the execution of the function ends.
     * Shared state is a list of editable owner ACEs for specified uid.
     */
    Future<List<OwnerAccessControlEntry>> getEditableOwnerAccessControlEntries(Callback<List<OwnerAccessControlEntry>> callback,
                                                                               String uid);

    /**
     * Get provider permission to expose an interface
     *
     * @param uid        The userId of the provider exposing the interface
     * @param domain        The domain where interface belongs
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     * @return provider permission
     */
    Permission getProviderPermission(String uid, String domain, String interfaceName, TrustLevel trustLevel);

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

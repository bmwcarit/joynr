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

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;

/**
 * The DomainAccessControlStore interface stores Access Control Lists.
 * At the first stage Registration Control Lists are not supported.
 */
public interface DomainAccessControlStore {

    /**
     * Get the domain roles for the given user.
     *
     * @param uid The user to get the domain roles for. No wildcards supported.
     * @return List of Domain Role Entries that apply to the user uid.
     * The list contains max two entries (since we only have two roles Master and Owner).
     * Used to get domain roles when a user logs in.
     */
    List<DomainRoleEntry> getDomainRoles(String uid);

    /**
     * Get the domain role for the given user and role.
     *
     * @param uid The user to get the domain role for. No wildcards supported.
     * @param role The user to get the domain role for. No wildcards supported.
     * @return Domain Role Entry that apply to the user uid and role.
     */
    DomainRoleEntry getDomainRole(String uid, Role role);

    /**
     * Updates given domain role entry. If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedEntry Entry that has to be updated.
     * @return If operation succeeded return true.
     */
    Boolean updateDomainRole(DomainRoleEntry updatedEntry);

    /**
     * Removes an entry according to the specified primary key.
     *
     * @param uid UserId whose DRE is going to be removed.
     * @param role UserId role that builds with uid primary key that identifies DRE that has to be removed.
     * @return If operation succeeded return true.
     */
    Boolean removeDomainRole(String uid, Role role);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MasterAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master ACEs with entries owned by the user.
     * If no entry has been found for specified uid, then returns master ACE with uid "*".
     */
    List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid);

    /**
     * Returns a list of maste ACEs applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Master ACL editor app.
     *
     * @param uid The user id that owns the domains.
     * @return List of master ACEs with entries owned by the user.
     * In case uid has no domains with role MASTER, this function returns emty list.
     * In case when this uid owns no domains with role MASTER, should this function return master ACE for uid "*"?
     */
    List<MasterAccessControlEntry> getEditableMasterAccessControlEntries(String uid);

    /**
     * Returns a list of master ACEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return List of master ACEs associated to given domain and interface.
     */
    List<MasterAccessControlEntry> getMasterAccessControlEntries(String domain, String interfaceName);

    /**
     * Returns a list of master ACEs that apply to the userId, domain and interface combination.
     *
     * @param uid The userId you search ACE's for.
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return List of master ACEs associated to given domain and interface.
     */
    List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid, String domain, String interfaceName);

    /**
     * Returns a master ACEs that apply to the given uid, domain, interface and operation combination.
     *
     * @param uid The userId you search ACE for.
     * @param domain The domain you search ACE for.
     * @param interfaceName The interface you search ACE for.
     * @param operation The operation you search ACE for.
     * @return Master ACE associated to given uid domain, interface and operation.
     */
    MasterAccessControlEntry getMasterAccessControlEntry(String uid,
                                                         String domain,
                                                         String interfaceName,
                                                         String operation);

    /**
     * Update given master access control entry.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedMasterAce The entry to add
     * @return false if update fails.
     */
    Boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce);

    /**
     * Remove master access control entry uniquely identified with uid, domain, interface and operation.
     *
     * @param uid The user id for which the entry should be removed.
     * @param domain The domain for which the entry should be removed.
     * @param interfaceName The interface for which the entry should be removed.
     * @param operation The operation for which the entry should be removed.
     * @return false if remove fails or master ACE that match given parameters was not found.
     */
    Boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation);

    /**
     * Returns a list of maste ACEs from Mediator ACL that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MediatorAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master ACEs with entries owned by the uid.
     * If no entry has been found for specified uid, then returns master ACE from Mediator ACL with uid "*".
     */
    List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid);

    /**
     * Returns a list of maste ACEs from Mediator ACL applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Mediator ACL editor app.
     *
     * @param uid The user id that owns the domains.
     * @return List of master ACEs with entries owned by the user.
     * In case uid has no domains with role MASTER, this function returns emty list.
     * In case when this uid owns no domains with role MASTER, should this function return master ACE for uid "*"?
     */
    List<MasterAccessControlEntry> getEditableMediatorAccessControlEntries(String uid);

    /**
     * Returns a list of master ACEs from Mediator ACL that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return List of master ACEs associated to given domain and interface.
     */
    List<MasterAccessControlEntry> getMediatorAccessControlEntries(String domain, String interfaceName);

    /**
     * Returns a list of master ACEs from Mediator ACL that apply to the userId, domain and interface combination.
     *
     * @param uid The userId you search ACE's for.
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return List of master ACEs from Mediator ACL associated to given userId, domain and interface.
     */
    List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid, String domain, String interfaceName);

    /**
     * Returns a master ACE from Mediator ACL that apply to the given uid, domain, interface and operation combination.
     *
     * @param uid The userId you search ACE for.
     * @param domain The domain you search ACE for.
     * @param interfaceName The interface you search ACE for.
     * @param operation The operation you search ACE for.
     * @return Master ACE from Mediator ACL associated to given uid domain, interface and operation.
     */
    MasterAccessControlEntry getMediatorAccessControlEntry(String uid,
                                                           String domain,
                                                           String interfaceName,
                                                           String operation);

    /**
     * Update given master ACE in MediatorACL.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedMediatorAce The entry to add
     * @return false if update fails.
     */
    Boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce);

    /**
     * Remove master ACE from MediatorACL identified with uid, domain, interface and operation.
     *
     * @param uid The user id for which the master ACE should be removed.
     * @param domain The domain for which the master ACE should be removed.
     * @param interfaceName The interface for which the master ACE should be removed.
     * @param operation The operation for which the master ACE should be removed.
     * @return false if remove fails or master ACE that match given parameters was not found.
     */
    Boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation);

    /**
     * Returns a list of owner ACEs that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master ACEs with entries owned by the user.
     */
    List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid);

    /**
     * Returns a list of owner ACEs from Owner ACL applying to domains the user uid has role OWNER,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of owner ACEs with entries owned by the user.
     * In case uid has no domains with role OWNER, this function returns list of all uid owner ACEs.
     * In case when this uid owns no domains with role OWNER, what should this function return (owner ACE for uid "*" ?).
     */
    List<OwnerAccessControlEntry> getEditableOwnerAccessControlEntries(String uid);

    /**
     * Returns a list of owner ACEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return List of owner ACEs associated to given domain and interface.
     */
    List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String domain, String interfaceName);

    /**
     * Returns a list of owner ACEs that apply to the userId, domain and interface combination.
     *
     * @param uid The userId you search ACE's for.
     * @param domain The domain you search ACE's for.
     * @param interfaceName The interface you search ACE's for.
     * @return List of owner ACEs associated to given userId, domain and interface.
     */
    List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid, String domain, String interfaceName);

    /**
     * Returns a owner ACE that apply to the given uid, domain, interface and operation combination.
     *
     * @param uid The userId you search ACE for.
     * @param domain The domain you search ACE for.
     * @param interfaceName The interface you search ACE for.
     * @param operation The operation you search ACE for.
     * @return Owner ACE associated to given uid, domain, interface and operation.
     */
    OwnerAccessControlEntry getOwnerAccessControlEntry(String uid,
                                                       String domain,
                                                       String interfaceName,
                                                       String operation);

    /**
     * Update given owner ACE.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedOwnerAce The entry to add
     * @return false if update fails.
     */
    Boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce);

    /**
     * Remove ownerAce ACE identified with uid, domain, interface and operation.
     *
     * @param uid The user id for which the ownerACE should be removed.
     * @param domain The domain for which the ownerACE should be removed.
     * @param interfaceName The interface for which the ownerACE should be removed.
     * @param operation The operation for which the ownerACE should be removed.
     * @return false if remove fails or ownerAce ACE that match given parameters was not found.
     */
    Boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation);

    /**
     * Returns a list of entries that apply to user uid, i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MasterAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master RCEs with entries owned by the user.
     * If no entry has been found for specified uid, then returns master RCE with uid "*".
     */
    List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String uid);

    /**
     * Returns a list of master AREs applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Master ACL editor app.
     *
     * @param uid The user id that owns the domains.
     * @return List of master RCEs with entries owned by the user.
     * In case uid has no domains with role MASTER, this function returns emty list.
     * In case when this uid owns no domains with role MASTER, should this function return master RCE for uid "*"?
     */
    List<MasterRegistrationControlEntry> getEditableMasterRegistrationControlEntries(String uid);

    /**
     * Returns a list of master RCEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search RCE's for.
     * @param interfaceName The interface you search RCE's for.
     * @return List of master RCEs associated to given domain and interface.
     */
    List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String domain, String interfaceName);

    /**
     * Returns a list of master RCEs that apply to the userId, domain and interface combination.
     *
     * @param uid The userId you search RCE's for.
     * @param domain The domain you search RCE's for.
     * @param interfaceName The interface you search RCE's for.
     * @return List of master RCEs associated to given domain and interface.
     */
    List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String uid,
                                                                             String domain,
                                                                             String interfaceName);

    /**
     * Returns a master RCEs that apply to the given uid, domain, interface and operation combination.
     *
     * @param uid The userId you search RCE for.
     * @param domain The domain you search RCE for.
     * @param interfaceName The interface you search RCE for.
     * @return Master RCE associated to given uid domain, interface and operation.
     */
    MasterRegistrationControlEntry getMasterRegistrationControlEntry(String uid, String domain, String interfaceName);

    /**
     * Update given master access control entry.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedMasterRce The entry to add
     * @return false if update fails.
     */
    Boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce);

    /**
     * Remove master access control entry uniquely identified with uid, domain, interface and operation.
     *
     * @param uid The user id for which the entry should be removed.
     * @param domain The domain for which the entry should be removed.
     * @param interfaceName The interface for which the entry should be removed.
     * @return false if remove fails or master RCE that match given parameters was not found.
     */
    Boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName);

    /**
     * Returns a list of maste RCEs from Mediator ACL that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache MediatorAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master RCEs with entries owned by the uid.
     * If no entry has been found for specified uid, then returns master RCE from Mediator ACL with uid "*".
     */
    List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String uid);

    /**
     * Returns a list of master RCEs from Mediator ACL applying to domains the user uid has role Master,
     * i.e. the entries the user uid is allowed to edit. Used by an Mediator ACL editor app.
     *
     * @param uid The user id that owns the domains.
     * @return List of master RCEs with entries owned by the user.
     * In case uid has no domains with role MASTER, this function returns emty list.
     * In case when this uid owns no domains with role MASTER, should this function return master RCE for uid "*"?
     */
    List<MasterRegistrationControlEntry> getEditableMediatorRegistrationControlEntries(String uid);

    /**
     * Returns a list of master RCEs from Mediator ACL that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search RCE's for.
     * @param interfaceName The interface you search RCE's for.
     * @return List of master RCEs associated to given domain and interface.
     */
    List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String domain, String interfaceName);

    /**
     * Returns a list of master RCEs from Mediator ACL that apply to the userId, domain and interface combination.
     *
     * @param uid The userId you search RCE's for.
     * @param domain The domain you search RCE's for.
     * @param interfaceName The interface you search RCE's for.
     * @return List of master RCEs from Mediator ACL associated to given userId, domain and interface.
     */
    List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String uid,
                                                                               String domain,
                                                                               String interfaceName);

    /**
     * Returns a master RCE from Mediator ACL that apply to the given uid, domain, interface and operation combination.
     *
     * @param uid The userId you search RCE for.
     * @param domain The domain you search RCE for.
     * @param interfaceName The interface you search RCE for.
     * @return Master RCE from Mediator ACL associated to given uid domain, interface and operation.
     */
    MasterRegistrationControlEntry getMediatorRegistrationControlEntry(String uid, String domain, String interfaceName);

    /**
     * Update given master RCE in MediatorACL.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedMediatorRce The entry to add
     * @return false if update fails.
     */
    Boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce);

    /**
     * Remove master RCE from MediatorACL identified with uid, domain, interface and operation.
     *
     * @param uid The user id for which the master RCE should be removed.
     * @param domain The domain for which the master RCE should be removed.
     * @param interfaceName The interface for which the master RCE should be removed.
     * @return false if remove fails or master RCE that match given parameters was not found.
     */
    Boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName);

    /**
     * Returns a list of owner RCEs that apply to user uid,
     * i.e. the entries that define the access rights of the user uid.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of master RCEs with entries owned by the user.
     */
    List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String uid);

    /**
     * Returns a list of owner RCEs from Owner ACL applying to domains the user uid has role OWNER,
     * i.e. the entries the user uid is allowed to edit. Used by an Owner ACL editor app.
     * This method is called when a user logs in and a client wishes to cache OwnerAcl for that user.
     *
     * @param uid The user id that owns the domains.
     * @return List of owner RCEs with entries owned by the user.
     * In case uid has no domains with role OWNER, this function returns list of all uid owner RCEs.
     * In case when this uid owns no domains with role OWNER, what should this function return (owner RCE for uid "*" ?).
     */
    List<OwnerRegistrationControlEntry> getEditableOwnerRegistrationControlEntries(String uid);

    /**
     * Returns a list of owner RCEs that apply to the domain and interface combination.
     * Used when a provider is registered to prefetch applying entries.
     *
     * @param domain The domain you search RCE's for.
     * @param interfaceName The interface you search RCE's for.
     * @return List of owner RCEs associated to given domain and interface.
     */
    List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String domain, String interfaceName);

    /**
     * Returns a list of owner RCEs that apply to the userId, domain and interface combination.
     *
     * @param uid The userId you search RCE's for.
     * @param domain The domain you search RCE's for.
     * @param interfaceName The interface you search RCE's for.
     * @return List of owner RCEs associated to given userId, domain and interface.
     */
    List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String uid,
                                                                           String domain,
                                                                           String interfaceName);

    /**
     * Returns a owner RCE that apply to the given uid, domain, interface and operation combination.
     *
     * @param uid The userId you search RCE for.
     * @param domain The domain you search RCE for.
     * @param interfaceName The interface you search RCE for.
     * @return Owner RCE associated to given uid, domain, interface and operation.
     */
    OwnerRegistrationControlEntry getOwnerRegistrationControlEntry(String uid, String domain, String interfaceName);

    /**
     * Update given owner RCE.
     * If such doesn't already exist in the store, it will be added to the store.
     *
     * @param updatedOwnerRce The entry to add
     * @return false if update fails.
     */
    Boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce);

    /**
     * Remove ownerRce RCE identified with uid, domain, interface and operation.
     *
     * @param uid The user id for which the ownerRCE should be removed.
     * @param domain The domain for which the ownerRCE should be removed.
     * @param interfaceName The interface for which the ownerRCE should be removed.
     * @return false if remove fails or ownerRce RCE that match given parameters was not found.
     */
    Boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName);
}

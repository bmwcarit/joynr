package io.joynr.accesscontrol.global;

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

import io.joynr.accesscontrol.DomainAccessControlStore;
import io.joynr.provider.Promise;
import joynr.infrastructure.DomainRoleEntry;
import joynr.infrastructure.GlobalDomainAccessControllerAbstractProvider;
import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.MasterRegistrationControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.OwnerRegistrationControlEntry;
import joynr.infrastructure.Role;

import com.google.inject.Inject;

/**
 * Manages the Access Control Lists for all providers.
 */
public class GlobalDomainAccessControllerProviderImpl extends GlobalDomainAccessControllerAbstractProvider {

    private final DomainAccessControlStore domainAccessStore;

    @Inject
    public GlobalDomainAccessControllerProviderImpl(DomainAccessControlStore domainAccessStore) {
        this.domainAccessStore = domainAccessStore;
    }

    @Override
    public Promise<GetDomainRolesDeferred> getDomainRoles(String uid) {
        GetDomainRolesDeferred deferred = new GetDomainRolesDeferred();
        deferred.resolve(domainAccessStore.getDomainRoles(uid));
        return new Promise<GetDomainRolesDeferred>(deferred);
    }

    @Override
    public Promise<UpdateDomainRoleDeferred> updateDomainRole(DomainRoleEntry updatedEntry) {
        UpdateDomainRoleDeferred deferred = new UpdateDomainRoleDeferred();
        deferred.resolve(domainAccessStore.updateDomainRole(updatedEntry));
        return new Promise<UpdateDomainRoleDeferred>(deferred);
    }

    @Override
    public Promise<RemoveDomainRoleDeferred> removeDomainRole(String uid, Role role) {
        RemoveDomainRoleDeferred deferred = new RemoveDomainRoleDeferred();
        deferred.resolve(domainAccessStore.removeDomainRole(uid, role));
        return new Promise<RemoveDomainRoleDeferred>(deferred);
    }

    @Override
    public Promise<GetMasterAccessControlEntries1Deferred> getMasterAccessControlEntries(String uid) {
        GetMasterAccessControlEntries1Deferred deferred = new GetMasterAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMasterAccessControlEntries(uid));
        return new Promise<GetMasterAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetEditableMasterAccessControlEntriesDeferred> getEditableMasterAccessControlEntries(String uid) {
        GetEditableMasterAccessControlEntriesDeferred deferred = new GetEditableMasterAccessControlEntriesDeferred();
        deferred.resolve(domainAccessStore.getEditableMasterAccessControlEntries(uid));
        return new Promise<GetEditableMasterAccessControlEntriesDeferred>(deferred);
    }

    @Override
    public Promise<GetMasterAccessControlEntries1Deferred> getMasterAccessControlEntries(String domain,
                                                                                         String interfaceName) {
        GetMasterAccessControlEntries1Deferred deferred = new GetMasterAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMasterAccessControlEntries(domain, interfaceName));
        return new Promise<GetMasterAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<UpdateMasterAccessControlEntryDeferred> updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAccessControlEntry) {
        UpdateMasterAccessControlEntryDeferred deferred = new UpdateMasterAccessControlEntryDeferred();
        deferred.resolve(domainAccessStore.updateMasterAccessControlEntry(updatedMasterAccessControlEntry));
        return new Promise<UpdateMasterAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<RemoveMasterAccessControlEntryDeferred> removeMasterAccessControlEntry(String uid,
                                                                                          String domain,
                                                                                          String interfaceName,
                                                                                          String operation) {
        RemoveMasterAccessControlEntryDeferred deferred = new RemoveMasterAccessControlEntryDeferred();
        deferred.resolve(domainAccessStore.removeMasterAccessControlEntry(uid, domain, interfaceName, operation));
        return new Promise<RemoveMasterAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<GetMediatorAccessControlEntries1Deferred> getMediatorAccessControlEntries(String uid) {
        GetMediatorAccessControlEntries1Deferred deferred = new GetMediatorAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMediatorAccessControlEntries(uid));
        return new Promise<GetMediatorAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetEditableMediatorAccessControlEntriesDeferred> getEditableMediatorAccessControlEntries(String uid) {
        GetEditableMediatorAccessControlEntriesDeferred deferred = new GetEditableMediatorAccessControlEntriesDeferred();
        deferred.resolve(domainAccessStore.getEditableMediatorAccessControlEntries(uid));
        return new Promise<GetEditableMediatorAccessControlEntriesDeferred>(deferred);
    }

    @Override
    public Promise<GetMediatorAccessControlEntries1Deferred> getMediatorAccessControlEntries(String domain,
                                                                                             String interfaceName) {
        GetMediatorAccessControlEntries1Deferred deferred = new GetMediatorAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMediatorAccessControlEntries(domain, interfaceName));
        return new Promise<GetMediatorAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<UpdateMediatorAccessControlEntryDeferred> updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAccessControlEntry) {
        UpdateMediatorAccessControlEntryDeferred deferred = new UpdateMediatorAccessControlEntryDeferred();
        deferred.resolve(domainAccessStore.updateMediatorAccessControlEntry(updatedMediatorAccessControlEntry));
        return new Promise<UpdateMediatorAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<RemoveMediatorAccessControlEntryDeferred> removeMediatorAccessControlEntry(String uid,
                                                                                              String domain,
                                                                                              String interfaceName,
                                                                                              String operation) {
        RemoveMediatorAccessControlEntryDeferred deferred = new RemoveMediatorAccessControlEntryDeferred();
        deferred.resolve(domainAccessStore.removeMediatorAccessControlEntry(uid, domain, interfaceName, operation));
        return new Promise<RemoveMediatorAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<GetOwnerAccessControlEntries1Deferred> getOwnerAccessControlEntries(String uid) {
        GetOwnerAccessControlEntries1Deferred deferred = new GetOwnerAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getOwnerAccessControlEntries(uid));
        return new Promise<GetOwnerAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetOwnerAccessControlEntries1Deferred> getOwnerAccessControlEntries(String domain,
                                                                                       String interfaceName) {
        GetOwnerAccessControlEntries1Deferred deferred = new GetOwnerAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getOwnerAccessControlEntries(domain, interfaceName));
        return new Promise<GetOwnerAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetEditableOwnerAccessControlEntriesDeferred> getEditableOwnerAccessControlEntries(String uid) {
        GetEditableOwnerAccessControlEntriesDeferred deferred = new GetEditableOwnerAccessControlEntriesDeferred();
        deferred.resolve(domainAccessStore.getEditableOwnerAccessControlEntries(uid));
        return new Promise<GetEditableOwnerAccessControlEntriesDeferred>(deferred);
    }

    @Override
    public Promise<UpdateOwnerAccessControlEntryDeferred> updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAccessControlEntry) {
        UpdateOwnerAccessControlEntryDeferred deferred = new UpdateOwnerAccessControlEntryDeferred();
        deferred.resolve(domainAccessStore.updateOwnerAccessControlEntry(updatedOwnerAccessControlEntry));
        return new Promise<UpdateOwnerAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<RemoveOwnerAccessControlEntryDeferred> removeOwnerAccessControlEntry(String uid,
                                                                                        String domain,
                                                                                        String interfaceName,
                                                                                        String operation) {
        RemoveOwnerAccessControlEntryDeferred deferred = new RemoveOwnerAccessControlEntryDeferred();
        deferred.resolve(domainAccessStore.removeOwnerAccessControlEntry(uid, domain, interfaceName, operation));
        return new Promise<RemoveOwnerAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<GetMasterRegistrationControlEntriesDeferred> getMasterRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<GetEditableMasterRegistrationControlEntriesDeferred> getEditableMasterRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<UpdateMasterRegistrationControlEntryDeferred> updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<RemoveMasterRegistrationControlEntryDeferred> removeMasterRegistrationControlEntry(String uid,
                                                                                                      String domain,
                                                                                                      String interfaceName) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<GetMediatorRegistrationControlEntriesDeferred> getMediatorRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<GetEditableMediatorRegistrationControlEntriesDeferred> getEditableMediatorRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<UpdateMediatorRegistrationControlEntryDeferred> updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<RemoveMediatorRegistrationControlEntryDeferred> removeMediatorRegistrationControlEntry(String uid,
                                                                                                          String domain,
                                                                                                          String interfaceName) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<GetOwnerRegistrationControlEntriesDeferred> getOwnerRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<GetEditableOwnerRegistrationControlEntriesDeferred> getEditableOwnerRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<UpdateOwnerRegistrationControlEntryDeferred> updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<RemoveOwnerRegistrationControlEntryDeferred> removeOwnerRegistrationControlEntry(String uid,
                                                                                                    String domain,
                                                                                                    String interfaceName) {
        assert false : "method not implemented";
        return null;
    }
}

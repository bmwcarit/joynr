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

import com.google.inject.Inject;
import io.joynr.accesscontrol.DomainAccessControlStore;
import joynr.infrastructure.GlobalDomainAccessControllerAbstractProvider;
import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.DomainRoleEntry;
import joynr.infrastructure.MasterRegistrationControlEntry;
import joynr.infrastructure.OwnerRegistrationControlEntry;
import joynr.infrastructure.Role;

import java.util.List;

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
    public List<DomainRoleEntry> getDomainRoles(String uid) {
        return domainAccessStore.getDomainRoles(uid);
    }

    @Override
    public Boolean updateDomainRole(DomainRoleEntry updatedEntry) {
        return domainAccessStore.updateDomainRole(updatedEntry);
    }

    @Override
    public Boolean removeDomainRole(String uid, Role role) {
        return domainAccessStore.removeDomainRole(uid, role);
    }

    @Override
    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid) {
        return domainAccessStore.getMasterAccessControlEntries(uid);
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMasterAccessControlEntries(String uid) {
        return domainAccessStore.getEditableMasterAccessControlEntries(uid);
    }

    @Override
    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String domain, String interfaceName) {
        return domainAccessStore.getMasterAccessControlEntries(domain, interfaceName);
    }

    @Override
    public Boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAccessControlEntry) {
        return domainAccessStore.updateMasterAccessControlEntry(updatedMasterAccessControlEntry);
    }

    @Override
    public Boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return domainAccessStore.removeMasterAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid) {
        return domainAccessStore.getMediatorAccessControlEntries(uid);
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMediatorAccessControlEntries(String uid) {
        return domainAccessStore.getEditableMediatorAccessControlEntries(uid);
    }

    @Override
    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String domain, String interfaceName) {
        return domainAccessStore.getMediatorAccessControlEntries(domain, interfaceName);
    }

    @Override
    public Boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAccessControlEntry) {
        return domainAccessStore.updateMediatorAccessControlEntry(updatedMediatorAccessControlEntry);
    }

    @Override
    public Boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return domainAccessStore.removeMediatorAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid) {
        return domainAccessStore.getOwnerAccessControlEntries(uid);
    }

    @Override
    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String domain, String interfaceName) {
        return domainAccessStore.getOwnerAccessControlEntries(domain, interfaceName);
    }

    @Override
    public List<OwnerAccessControlEntry> getEditableOwnerAccessControlEntries(String uid) {
        return domainAccessStore.getEditableOwnerAccessControlEntries(uid);
    }

    @Override
    public Boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAccessControlEntry) {
        return domainAccessStore.updateOwnerAccessControlEntry(updatedOwnerAccessControlEntry);
    }

    @Override
    public Boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return domainAccessStore.removeOwnerAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public List<MasterRegistrationControlEntry> getEditableMasterRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        assert false : "method not implemented";
        return false;
    }

    @Override
    public Boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        assert false : "method not implemented";
        return false;
    }

    @Override
    public List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public List<MasterRegistrationControlEntry> getEditableMediatorRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        assert false : "method not implemented";
        return false;
    }

    @Override
    public Boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName) {
        assert false : "method not implemented";
        return false;
    }

    @Override
    public List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public List<OwnerRegistrationControlEntry> getEditableOwnerRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        assert false : "method not implemented";
        return false;
    }

    @Override
    public Boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        assert false : "method not implemented";
        return false;
    }
}

/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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

import com.google.inject.Inject;

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;

/**
 * Uses CqEngine to implement a DomainAccessControlStore.
 * Add/Remove operations can be expensive. Get operations should be fast.
 */
public class DomainAccessControlStoreCqEngine implements DomainAccessControlStore {
    private static final String WILDCARD = "*";
    private DomainRoleEntryStore domainRoleEntryStore;
    private MasterAccessControlEntryStore masterAccessControlEntryStore;
    private MasterAccessControlEntryStore mediatorAccessControlEntryStore;
    private OwnerAccessControlEntryStore ownerAccessControlEntryStore;

    private MasterRegistrationControlEntryStore masterRegistrationControlEntryStore;
    private MasterRegistrationControlEntryStore mediatorRegistrationControlEntryStore;
    private OwnerRegistrationControlEntryStore ownerRegistrationControlEntryStore;

    @Inject
    public DomainAccessControlStoreCqEngine(DomainAccessControlProvisioning domainAccessControlProvisioning) {

        domainRoleEntryStore = new DomainRoleEntryStore(domainAccessControlProvisioning);

        masterRegistrationControlEntryStore = new MasterRegistrationControlEntryStore(domainRoleEntryStore);

        mediatorRegistrationControlEntryStore = new MasterRegistrationControlEntryStore(domainRoleEntryStore);

        ownerRegistrationControlEntryStore = new OwnerRegistrationControlEntryStore(domainRoleEntryStore);

        masterAccessControlEntryStore = new MasterAccessControlEntryStore(domainAccessControlProvisioning,
                                                                          domainRoleEntryStore);
        mediatorAccessControlEntryStore = new MasterAccessControlEntryStore(new DefaultDomainAccessControlProvisioning(),
                                                                            domainRoleEntryStore);
        ownerAccessControlEntryStore = new OwnerAccessControlEntryStore(domainRoleEntryStore);
    }

    @Override
    public synchronized List<DomainRoleEntry> getDomainRoles(String uid) {
        return domainRoleEntryStore.getDomainRoles(uid);
    }

    @Override
    public synchronized DomainRoleEntry getDomainRole(String uid, Role role) {
        return domainRoleEntryStore.getDomainRole(uid, role);
    }

    @Override
    public synchronized Boolean updateDomainRole(DomainRoleEntry updatedEntry) {
        return domainRoleEntryStore.updateDomainRole(updatedEntry);
    }

    @Override
    public synchronized Boolean removeDomainRole(String uid, Role role) {
        return domainRoleEntryStore.removeDomainRole(uid, role);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid) {
        return masterAccessControlEntryStore.getControlEntries(uid);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getEditableMasterAccessControlEntries(String uid) {
        return masterAccessControlEntryStore.getEditableAces(uid, Role.MASTER);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getMasterAccessControlEntries(String domain,
                                                                                     String interfaceName) {
        return masterAccessControlEntryStore.getControlEntries(domain, interfaceName);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid,
                                                                                     String domain,
                                                                                     String interfaceName) {
        return masterAccessControlEntryStore.getControlEntries(uid, domain, interfaceName);
    }

    @Override
    public synchronized MasterAccessControlEntry getMasterAccessControlEntry(String uid,
                                                                             String domain,
                                                                             String interfaceName,
                                                                             String operation) {
        return masterAccessControlEntryStore.getControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public synchronized Boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        return masterAccessControlEntryStore.updateControlEntry(updatedMasterAce);
    }

    @Override
    public synchronized Boolean removeMasterAccessControlEntry(String uid,
                                                               String domain,
                                                               String interfaceName,
                                                               String operation) {
        return masterAccessControlEntryStore.removeControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid) {
        return mediatorAccessControlEntryStore.getControlEntries(uid);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getEditableMediatorAccessControlEntries(String uid) {
        return mediatorAccessControlEntryStore.getEditableAces(uid, Role.MASTER);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getMediatorAccessControlEntries(String domain,
                                                                                       String interfaceName) {
        return mediatorAccessControlEntryStore.getControlEntries(domain, interfaceName);
    }

    @Override
    public synchronized List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid,
                                                                                       String domain,
                                                                                       String interfaceName) {
        return mediatorAccessControlEntryStore.getControlEntries(uid, domain, interfaceName);
    }

    @Override
    public synchronized MasterAccessControlEntry getMediatorAccessControlEntry(String uid,
                                                                               String domain,
                                                                               String interfaceName,
                                                                               String operation) {
        MasterAccessControlEntry mediatorAce = mediatorAccessControlEntryStore.getControlEntry(uid,
                                                                                               domain,
                                                                                               interfaceName,
                                                                                               operation);
        if (mediatorAce == null) {
            mediatorAce = mediatorAccessControlEntryStore.getControlEntry(uid, domain, interfaceName, WILDCARD);
        }

        return mediatorAce;
    }

    @Override
    public synchronized Boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {

        boolean updateSuccess = false;
        MasterAccessControlEntry masterAce = getMasterAccessControlEntry(updatedMediatorAce.getUid(),
                                                                         updatedMediatorAce.getDomain(),
                                                                         updatedMediatorAce.getInterfaceName(),
                                                                         updatedMediatorAce.getOperation());

        AceValidator aceValidator = new AceValidator(masterAce, updatedMediatorAce, null);
        if (aceValidator.isMediatorValid()) {
            updateSuccess = mediatorAccessControlEntryStore.updateControlEntry(updatedMediatorAce);
        }

        return updateSuccess;
    }

    @Override
    public synchronized Boolean removeMediatorAccessControlEntry(String uid,
                                                                 String domain,
                                                                 String interfaceName,
                                                                 String operation) {
        return mediatorAccessControlEntryStore.removeControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public synchronized List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid) {
        return ownerAccessControlEntryStore.getControlEntries(uid);
    }

    @Override
    public synchronized List<OwnerAccessControlEntry> getEditableOwnerAccessControlEntries(String uid) {
        return ownerAccessControlEntryStore.getEditableAces(uid, Role.OWNER);
    }

    @Override
    public synchronized List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String domain,
                                                                                   String interfaceName) {
        return ownerAccessControlEntryStore.getControlEntries(domain, interfaceName);
    }

    @Override
    public synchronized List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid,
                                                                                   String domain,
                                                                                   String interfaceName) {
        return ownerAccessControlEntryStore.getControlEntries(uid, domain, interfaceName);
    }

    @Override
    public synchronized OwnerAccessControlEntry getOwnerAccessControlEntry(String uid,
                                                                           String domain,
                                                                           String interfaceName,
                                                                           String operation) {
        OwnerAccessControlEntry ownerAce = ownerAccessControlEntryStore.getControlEntry(uid,
                                                                                        domain,
                                                                                        interfaceName,
                                                                                        operation);
        if (ownerAce == null) {
            ownerAce = ownerAccessControlEntryStore.getControlEntry(uid, domain, interfaceName, WILDCARD);
        }

        return ownerAce;
    }

    @Override
    public synchronized Boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        boolean updateSuccess = false;
        MasterAccessControlEntry masterAce = getMasterAccessControlEntry(updatedOwnerAce.getUid(),
                                                                         updatedOwnerAce.getDomain(),
                                                                         updatedOwnerAce.getInterfaceName(),
                                                                         updatedOwnerAce.getOperation());

        MasterAccessControlEntry mediatorAce = getMediatorAccessControlEntry(updatedOwnerAce.getUid(),
                                                                             updatedOwnerAce.getDomain(),
                                                                             updatedOwnerAce.getInterfaceName(),
                                                                             updatedOwnerAce.getOperation());

        AceValidator aceValidator = new AceValidator(masterAce, mediatorAce, updatedOwnerAce);
        if (aceValidator.isOwnerValid()) {
            updateSuccess = ownerAccessControlEntryStore.updateControlEntry(updatedOwnerAce);
        }

        return updateSuccess;
    }

    @Override
    public synchronized Boolean removeOwnerAccessControlEntry(String uid,
                                                              String domain,
                                                              String interfaceName,
                                                              String operation) {
        return ownerAccessControlEntryStore.removeControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public synchronized List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryStore.getControlEntries(uid);
    }

    @Override
    public synchronized List<MasterRegistrationControlEntry> getEditableMasterRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryStore.getEditableAces(uid, Role.MASTER);
    }

    @Override
    public synchronized List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String domain,
                                                                                                 String interfaceName) {
        return masterRegistrationControlEntryStore.getControlEntries(domain, interfaceName);
    }

    @Override
    public synchronized MasterRegistrationControlEntry getMasterRegistrationControlEntry(String uid,
                                                                                         String domain,
                                                                                         String interfaceName) {
        return masterRegistrationControlEntryStore.getControlEntry(uid, domain, interfaceName);
    }

    @Override
    public synchronized Boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        return masterRegistrationControlEntryStore.updateControlEntry(updatedMasterRce);
    }

    @Override
    public synchronized Boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        return masterRegistrationControlEntryStore.removeControlEntry(uid, domain, interfaceName);
    }

    @Override
    public synchronized List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String uid) {
        return mediatorRegistrationControlEntryStore.getControlEntries(uid);
    }

    @Override
    public synchronized List<MasterRegistrationControlEntry> getEditableMediatorRegistrationControlEntries(String uid) {
        return mediatorRegistrationControlEntryStore.getEditableAces(uid, Role.MASTER);
    }

    @Override
    public synchronized List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String domain,
                                                                                                   String interfaceName) {
        return mediatorRegistrationControlEntryStore.getControlEntries(domain, interfaceName);
    }

    @Override
    public synchronized MasterRegistrationControlEntry getMediatorRegistrationControlEntry(String uid,
                                                                                           String domain,
                                                                                           String interfaceName) {
        return mediatorRegistrationControlEntryStore.getControlEntry(uid, domain, interfaceName);
    }

    @Override
    public synchronized Boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        boolean updateSuccess = false;
        MasterRegistrationControlEntry masterRce = getMasterRegistrationControlEntry(updatedMediatorRce.getUid(),
                                                                                     updatedMediatorRce.getDomain(),
                                                                                     updatedMediatorRce.getInterfaceName());

        RceValidator rceValidator = new RceValidator(masterRce, updatedMediatorRce, null);
        if (rceValidator.isMediatorValid()) {
            updateSuccess = mediatorRegistrationControlEntryStore.updateControlEntry(updatedMediatorRce);
        }

        return updateSuccess;
    }

    @Override
    public synchronized Boolean removeMediatorRegistrationControlEntry(String uid,
                                                                       String domain,
                                                                       String interfaceName) {
        return mediatorRegistrationControlEntryStore.removeControlEntry(uid, domain, interfaceName);
    }

    @Override
    public synchronized List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String uid) {
        return ownerRegistrationControlEntryStore.getControlEntries(uid);
    }

    @Override
    public synchronized List<OwnerRegistrationControlEntry> getEditableOwnerRegistrationControlEntries(String uid) {
        return ownerRegistrationControlEntryStore.getEditableAces(uid, Role.OWNER);
    }

    @Override
    public synchronized List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String domain,
                                                                                               String interfaceName) {
        return ownerRegistrationControlEntryStore.getControlEntries(domain, interfaceName);
    }

    @Override
    public synchronized OwnerRegistrationControlEntry getOwnerRegistrationControlEntry(String uid,
                                                                                       String domain,
                                                                                       String interfaceName) {
        return ownerRegistrationControlEntryStore.getControlEntry(uid, domain, interfaceName);
    }

    @Override
    public synchronized Boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        boolean updateSuccess = false;
        MasterRegistrationControlEntry masterRce = getMasterRegistrationControlEntry(updatedOwnerRce.getUid(),
                                                                                     updatedOwnerRce.getDomain(),
                                                                                     updatedOwnerRce.getInterfaceName());

        MasterRegistrationControlEntry mediatorRce = getMediatorRegistrationControlEntry(updatedOwnerRce.getUid(),
                                                                                         updatedOwnerRce.getDomain(),
                                                                                         updatedOwnerRce.getInterfaceName());

        RceValidator rceValidator = new RceValidator(masterRce, mediatorRce, updatedOwnerRce);
        if (rceValidator.isOwnerValid()) {
            updateSuccess = ownerRegistrationControlEntryStore.updateControlEntry(updatedOwnerRce);
        }

        return updateSuccess;
    }

    @Override
    public synchronized Boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        return ownerRegistrationControlEntryStore.removeControlEntry(uid, domain, interfaceName);
    }
}

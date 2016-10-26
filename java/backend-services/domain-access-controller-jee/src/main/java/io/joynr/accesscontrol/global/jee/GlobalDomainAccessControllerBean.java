package io.joynr.accesscontrol.global.jee;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MASTER;
import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MEDIATOR;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.transaction.Transactional;

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.GlobalDomainAccessControllerSubscriptionPublisher;
import joynr.infrastructure.GlobalDomainAccessControllerSync;

@Stateless
@ServiceProvider(serviceInterface = GlobalDomainAccessControllerSync.class)
@Transactional
public class GlobalDomainAccessControllerBean implements GlobalDomainAccessControllerSync {

    private GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher;

    private DomainRoleEntryManager domainRoleEntryManager;

    private MasterAccessControlEntryManager masterAccessControlEntryManager;

    private OwnerAccessControlEntryManager ownerAccessControlEntryManager;

    private MasterRegistrationControlEntryManager masterRegistrationControlEntryManager;

    private OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager;

    // Only required for testing
    protected GlobalDomainAccessControllerBean() {
    }

    @Inject
    public GlobalDomainAccessControllerBean(@SubscriptionPublisher GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher,
                                            DomainRoleEntryManager domainRoleEntryManager,
                                            MasterAccessControlEntryManager masterAccessControlEntryManager,
                                            OwnerAccessControlEntryManager ownerAccessControlEntryManager,
                                            MasterRegistrationControlEntryManager masterRegistrationControlEntryManager,
                                            OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager) {
        this.globalDomainAccessControllerSubscriptionPublisher = globalDomainAccessControllerSubscriptionPublisher;
        this.domainRoleEntryManager = domainRoleEntryManager;
        this.masterAccessControlEntryManager = masterAccessControlEntryManager;
        this.ownerAccessControlEntryManager = ownerAccessControlEntryManager;
        this.masterRegistrationControlEntryManager = masterRegistrationControlEntryManager;
        this.ownerRegistrationControlEntryManager = ownerRegistrationControlEntryManager;
    }

    private String sanitizeForPartition(String value) {
        return value.replaceAll("[^a-zA-Z0-9]", "");
    }

    @Override
    public DomainRoleEntry[] getDomainRoles(String uid) {
        return domainRoleEntryManager.findByUserId(uid);
    }

    @Override
    public Boolean updateDomainRole(DomainRoleEntry updatedEntry) {
        CreateOrUpdateResult<DomainRoleEntry> result = domainRoleEntryManager.createOrUpdate(updatedEntry);
        globalDomainAccessControllerSubscriptionPublisher.fireDomainRoleEntryChanged(result.getChangeType(),
                                                                                     result.getEntry(),
                                                                                     sanitizeForPartition(result.getEntry()
                                                                                                                .getUid()));
        return true;
    }

    @Override
    public Boolean removeDomainRole(String uid, Role role) {
        DomainRoleEntry removedEntry = domainRoleEntryManager.removeByUserIdAndRole(uid, role);
        if (removedEntry != null) {
            globalDomainAccessControllerSubscriptionPublisher.fireDomainRoleEntryChanged(ChangeType.REMOVE,
                                                                                         removedEntry,
                                                                                         sanitizeForPartition(uid));
            return true;
        }
        return false;
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserId(uid, MASTER);
    }

    @Override
    public MasterAccessControlEntry[] getEditableMasterAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserIdThatAreEditable(uid, MASTER);
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String domain, String interfaceName) {
        return masterAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName, MASTER);
    }

    @Override
    public Boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        CreateOrUpdateResult<MasterAccessControlEntry> result = masterAccessControlEntryManager.createOrUpdate(updatedMasterAce,
                                                                                                               MASTER);
        if (result != null) {
            MasterAccessControlEntry persistedAce = result.getEntry();
            globalDomainAccessControllerSubscriptionPublisher.fireMasterAccessControlEntryChanged(result.getChangeType(),
                                                                                                  persistedAce,
                                                                                                  sanitizeForPartition(persistedAce.getUid()),
                                                                                                  sanitizeForPartition(persistedAce.getDomain()),
                                                                                                  sanitizeForPartition(persistedAce.getInterfaceName()),
                                                                                                  sanitizeForPartition(persistedAce.getOperation()));
            return true;
        }
        return false;
    }

    @Override
    public Boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        MasterAccessControlEntry removedEntry = masterAccessControlEntryManager.removeByUserIdDomainInterfaceNameAndOperation(uid,
                                                                                                                              domain,
                                                                                                                              interfaceName,
                                                                                                                              operation,
                                                                                                                              MASTER);
        if (removedEntry != null) {
            globalDomainAccessControllerSubscriptionPublisher.fireMasterAccessControlEntryChanged(ChangeType.REMOVE,
                                                                                                  removedEntry,
                                                                                                  sanitizeForPartition(uid),
                                                                                                  sanitizeForPartition(domain),
                                                                                                  sanitizeForPartition(interfaceName),
                                                                                                  sanitizeForPartition(operation));
            return true;
        }
        return false;
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserId(uid, MEDIATOR);
    }

    @Override
    public MasterAccessControlEntry[] getEditableMediatorAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserIdThatAreEditable(uid, MEDIATOR);
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String domain, String interfaceName) {
        return masterAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName, MEDIATOR);
    }

    @Override
    public Boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {
        CreateOrUpdateResult<MasterAccessControlEntry> result = masterAccessControlEntryManager.createOrUpdate(updatedMediatorAce,
                                                                                                               MEDIATOR);
        if (result != null) {
            MasterAccessControlEntry persistedEntry = result.getEntry();
            globalDomainAccessControllerSubscriptionPublisher.fireMediatorAccessControlEntryChanged(result.getChangeType(),
                                                                                                    persistedEntry,
                                                                                                    sanitizeForPartition(persistedEntry.getUid()),
                                                                                                    sanitizeForPartition(persistedEntry.getDomain()),
                                                                                                    sanitizeForPartition(persistedEntry.getInterfaceName()),
                                                                                                    sanitizeForPartition(persistedEntry.getOperation()));
            return true;
        }
        return false;
    }

    @Override
    public Boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        MasterAccessControlEntry removedEntry = masterAccessControlEntryManager.removeByUserIdDomainInterfaceNameAndOperation(uid,
                                                                                                                              domain,
                                                                                                                              interfaceName,
                                                                                                                              operation,
                                                                                                                              MEDIATOR);
        if (removedEntry != null) {
            globalDomainAccessControllerSubscriptionPublisher.fireMediatorAccessControlEntryChanged(ChangeType.REMOVE,
                                                                                                    removedEntry,
                                                                                                    sanitizeForPartition(uid),
                                                                                                    sanitizeForPartition(domain),
                                                                                                    sanitizeForPartition(interfaceName),
                                                                                                    sanitizeForPartition(operation));
            return true;
        }
        return false;
    }

    @Override
    public OwnerAccessControlEntry[] getOwnerAccessControlEntries(String uid) {
        return ownerAccessControlEntryManager.findByUserId(uid);
    }

    @Override
    public OwnerAccessControlEntry[] getOwnerAccessControlEntries(String domain, String interfaceName) {
        return ownerAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName);
    }

    @Override
    public OwnerAccessControlEntry[] getEditableOwnerAccessControlEntries(String uid) {
        return ownerAccessControlEntryManager.findByUserIdThatAreEditable(uid);
    }

    @Override
    public Boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        CreateOrUpdateResult<OwnerAccessControlEntry> result = ownerAccessControlEntryManager.createOrUpdate(updatedOwnerAce);
        OwnerAccessControlEntry entry = result.getEntry();
        globalDomainAccessControllerSubscriptionPublisher.fireOwnerAccessControlEntryChanged(result.getChangeType(),
                                                                                             entry,
                                                                                             sanitizeForPartition(entry.getUid()),
                                                                                             sanitizeForPartition(entry.getDomain()),
                                                                                             sanitizeForPartition(entry.getInterfaceName()),
                                                                                             sanitizeForPartition(entry.getOperation()));
        return true;
    }

    @Override
    public Boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        OwnerAccessControlEntry removedEntry = ownerAccessControlEntryManager.removeByUserIdDomainInterfaceNameAndOperation(uid,
                                                                                                                            domain,
                                                                                                                            interfaceName,
                                                                                                                            operation);
        if (removedEntry != null) {
            globalDomainAccessControllerSubscriptionPublisher.fireOwnerAccessControlEntryChanged(ChangeType.REMOVE,
                                                                                                 removedEntry,
                                                                                                 sanitizeForPartition(removedEntry.getUid()),
                                                                                                 sanitizeForPartition(removedEntry.getDomain()),
                                                                                                 sanitizeForPartition(removedEntry.getInterfaceName()));
            return true;
        }
        return false;
    }

    @Override
    public MasterRegistrationControlEntry[] getMasterRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndType(uid, ControlEntryType.MASTER);
    }

    @Override
    public MasterRegistrationControlEntry[] getEditableMasterRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndThatAreEditable(uid, ControlEntryType.MASTER);
    }

    @Override
    public Boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        CreateOrUpdateResult<MasterRegistrationControlEntry> result = masterRegistrationControlEntryManager.createOrUpdate(updatedMasterRce,
                                                                                                                           ControlEntryType.MASTER);
        MasterRegistrationControlEntry entry = result.getEntry();
        globalDomainAccessControllerSubscriptionPublisher.fireMasterRegistrationControlEntryChanged(result.getChangeType(),
                                                                                                    entry,
                                                                                                    sanitizeForPartition(entry.getUid()),
                                                                                                    sanitizeForPartition(entry.getDomain()),
                                                                                                    sanitizeForPartition(entry.getInterfaceName()));
        return true;
    }

    @Override
    public Boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        MasterRegistrationControlEntry removedEntry = masterRegistrationControlEntryManager.removeByUserIdDomainInterfaceNameAndType(uid,
                                                                                                                                     domain,
                                                                                                                                     interfaceName,
                                                                                                                                     ControlEntryType.MASTER);
        if (removedEntry != null) {
            globalDomainAccessControllerSubscriptionPublisher.fireMasterRegistrationControlEntryChanged(ChangeType.REMOVE,
                                                                                                        removedEntry,
                                                                                                        sanitizeForPartition(removedEntry.getUid()),
                                                                                                        sanitizeForPartition(removedEntry.getDomain()),
                                                                                                        sanitizeForPartition(removedEntry.getInterfaceName()));
            return true;
        }
        return false;
    }

    @Override
    public MasterRegistrationControlEntry[] getMediatorRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndType(uid, ControlEntryType.MEDIATOR);
    }

    @Override
    public MasterRegistrationControlEntry[] getEditableMediatorRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndThatAreEditable(uid, ControlEntryType.MEDIATOR);
    }

    @Override
    public Boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        CreateOrUpdateResult<MasterRegistrationControlEntry> result = masterRegistrationControlEntryManager.createOrUpdate(updatedMediatorRce,
                                                                                                                           ControlEntryType.MEDIATOR);
        MasterRegistrationControlEntry entry = result.getEntry();
        globalDomainAccessControllerSubscriptionPublisher.fireMediatorRegistrationControlEntryChanged(result.getChangeType(),
                                                                                                      entry,
                                                                                                      sanitizeForPartition(entry.getUid()),
                                                                                                      sanitizeForPartition(entry.getDomain()),
                                                                                                      sanitizeForPartition(entry.getInterfaceName()));
        return true;
    }

    @Override
    public Boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName) {
        MasterRegistrationControlEntry removedEntry = masterRegistrationControlEntryManager.removeByUserIdDomainInterfaceNameAndType(uid,
                                                                                                                                     domain,
                                                                                                                                     interfaceName,
                                                                                                                                     ControlEntryType.MEDIATOR);
        if (removedEntry != null) {
            globalDomainAccessControllerSubscriptionPublisher.fireMediatorRegistrationControlEntryChanged(ChangeType.REMOVE,
                                                                                                          removedEntry,
                                                                                                          sanitizeForPartition(removedEntry.getUid()),
                                                                                                          sanitizeForPartition(removedEntry.getDomain()),
                                                                                                          sanitizeForPartition(removedEntry.getInterfaceName()));
            return true;
        }
        return false;
    }

    @Override
    public OwnerRegistrationControlEntry[] getOwnerRegistrationControlEntries(String uid) {
        return ownerRegistrationControlEntryManager.findByUserId(uid);
    }

    @Override
    public OwnerRegistrationControlEntry[] getEditableOwnerRegistrationControlEntries(String uid) {
        return ownerRegistrationControlEntryManager.findByUserIdAndThatIsEditable(uid);
    }

    @Override
    public Boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        CreateOrUpdateResult<OwnerRegistrationControlEntry> result = ownerRegistrationControlEntryManager.createOrUpdate(updatedOwnerRce);
        OwnerRegistrationControlEntry entry = result.getEntry();
        globalDomainAccessControllerSubscriptionPublisher.fireOwnerRegistrationControlEntryChanged(result.getChangeType(),
                                                                                                   entry,
                                                                                                   sanitizeForPartition(entry.getUid()),
                                                                                                   sanitizeForPartition(entry.getDomain()),
                                                                                                   sanitizeForPartition(entry.getInterfaceName()));
        return true;
    }

    @Override
    public Boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        OwnerRegistrationControlEntry removedEntry = ownerRegistrationControlEntryManager.removeByUserIdDomainAndInterfaceName(uid,
                                                                                                                               domain,
                                                                                                                               interfaceName);
        if (removedEntry != null) {
            globalDomainAccessControllerSubscriptionPublisher.fireOwnerRegistrationControlEntryChanged(ChangeType.REMOVE,
                                                                                                       removedEntry,
                                                                                                       sanitizeForPartition(removedEntry.getUid()),
                                                                                                       sanitizeForPartition(removedEntry.getDomain()),
                                                                                                       sanitizeForPartition(removedEntry.getInterfaceName()));
            return true;
        }
        return false;
    }
}

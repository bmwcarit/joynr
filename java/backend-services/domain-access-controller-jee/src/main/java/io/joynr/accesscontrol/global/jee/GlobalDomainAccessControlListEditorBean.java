package io.joynr.accesscontrol.global.jee;

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

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import io.joynr.jeeintegration.api.ServiceProvider;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.GlobalDomainAccessControlListEditorSync;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.transaction.Transactional;

import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MASTER;
import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MEDIATOR;

@Stateless
@ServiceProvider(serviceInterface = GlobalDomainAccessControlListEditorSync.class)
@Transactional
public class GlobalDomainAccessControlListEditorBean implements GlobalDomainAccessControlListEditorSync {

    private MasterAccessControlEntryManager masterAccessControlEntryManager;

    private OwnerAccessControlEntryManager ownerAccessControlEntryManager;

    private MasterRegistrationControlEntryManager masterRegistrationControlEntryManager;

    private OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager;

    private GlobalDomainAccessControllerLocal globalDomainAccessControllerBean;

    // Only required for testing
    protected GlobalDomainAccessControlListEditorBean() {
    }

    @Inject
    public GlobalDomainAccessControlListEditorBean(MasterAccessControlEntryManager masterAccessControlEntryManager,
                                            OwnerAccessControlEntryManager ownerAccessControlEntryManager,
                                            MasterRegistrationControlEntryManager masterRegistrationControlEntryManager,
                                            OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager,
                                            GlobalDomainAccessControllerLocal globalDomainAccessControllerBean) {
        this.masterAccessControlEntryManager = masterAccessControlEntryManager;
        this.ownerAccessControlEntryManager = ownerAccessControlEntryManager;
        this.masterRegistrationControlEntryManager = masterRegistrationControlEntryManager;
        this.ownerRegistrationControlEntryManager = ownerRegistrationControlEntryManager;
        this.globalDomainAccessControllerBean = globalDomainAccessControllerBean;
    }

    @Override
    public MasterAccessControlEntry[] getEditableMasterAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserIdThatAreEditable(uid, MASTER);
    }

    @Override
    public Boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        CreateOrUpdateResult<MasterAccessControlEntry> result = masterAccessControlEntryManager.createOrUpdate(updatedMasterAce,
                                                                                                               MASTER);
        if (result != null) {
            MasterAccessControlEntry persistedAce = result.getEntry();
            globalDomainAccessControllerBean.doFireMasterAccessControlEntryChanged(result.getChangeType(), persistedAce);
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
            globalDomainAccessControllerBean.doFireMasterAccessControlEntryChanged(ChangeType.REMOVE, removedEntry);
            return true;
        }
        return false;
    }

    @Override
    public MasterAccessControlEntry[] getEditableMediatorAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserIdThatAreEditable(uid, MEDIATOR);
    }

    @Override
    public Boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {
        CreateOrUpdateResult<MasterAccessControlEntry> result = masterAccessControlEntryManager.createOrUpdate(updatedMediatorAce,
                                                                                                               MEDIATOR);
        if (result != null) {
            MasterAccessControlEntry persistedEntry = result.getEntry();
            globalDomainAccessControllerBean.doFireMediatorAccessControlEntryChanged(result.getChangeType(), persistedEntry);
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
            globalDomainAccessControllerBean.doFireMediatorAccessControlEntryChanged(ChangeType.REMOVE, removedEntry);
            return true;
        }
        return false;
    }

    @Override
    public OwnerAccessControlEntry[] getEditableOwnerAccessControlEntries(String uid) {
        return ownerAccessControlEntryManager.findByUserIdThatAreEditable(uid);
    }

    @Override
    public Boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        CreateOrUpdateResult<OwnerAccessControlEntry> result = ownerAccessControlEntryManager.createOrUpdate(updatedOwnerAce);
        if (result != null) {
            OwnerAccessControlEntry entry = result.getEntry();
            globalDomainAccessControllerBean.doFireOwnerAccessControlEntryChanged(result.getChangeType(), entry);
            return true;
        }
        return false;
    }

    @Override
    public Boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        OwnerAccessControlEntry removedEntry = ownerAccessControlEntryManager.removeByUserIdDomainInterfaceNameAndOperation(uid,
                                                                                                                            domain,
                                                                                                                            interfaceName,
                                                                                                                            operation);
        if (removedEntry != null) {
            globalDomainAccessControllerBean.doFireOwnerAccessControlEntryChanged(ChangeType.REMOVE, removedEntry);
            return true;
        }
        return false;
    }

    @Override
    public MasterRegistrationControlEntry[] getEditableMasterRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndThatAreEditable(uid, ControlEntryType.MASTER);
    }

    @Override
    public Boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        CreateOrUpdateResult<MasterRegistrationControlEntry> result = masterRegistrationControlEntryManager.createOrUpdate(updatedMasterRce,
                                                                                                                           ControlEntryType.MASTER);
        if (result != null) {
            MasterRegistrationControlEntry entry = result.getEntry();
            globalDomainAccessControllerBean.doFireMasterRegistrationControlEntryChanged(result.getChangeType(), entry);
            return true;
        }
        return false;
    }

    @Override
    public Boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        MasterRegistrationControlEntry removedEntry = masterRegistrationControlEntryManager.removeByUserIdDomainInterfaceNameAndType(uid,
                                                                                                                                     domain,
                                                                                                                                     interfaceName,
                                                                                                                                     ControlEntryType.MASTER);
        if (removedEntry != null) {
            globalDomainAccessControllerBean.doFireMasterRegistrationControlEntryChanged(ChangeType.REMOVE, removedEntry);
            return true;
        }
        return false;
    }

    @Override
    public MasterRegistrationControlEntry[] getEditableMediatorRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndThatAreEditable(uid, ControlEntryType.MEDIATOR);
    }

    @Override
    public Boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        CreateOrUpdateResult<MasterRegistrationControlEntry> result = masterRegistrationControlEntryManager.createOrUpdate(updatedMediatorRce,
                                                                                                                           ControlEntryType.MEDIATOR);
        if (result != null) {
            MasterRegistrationControlEntry entry = result.getEntry();
            globalDomainAccessControllerBean.doFireMediatorRegistrationControlEntryChanged(result.getChangeType(), entry);
            return true;
        }
        return false;
    }

    @Override
    public Boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName) {
        MasterRegistrationControlEntry removedEntry = masterRegistrationControlEntryManager.removeByUserIdDomainInterfaceNameAndType(uid,
                                                                                                                                     domain,
                                                                                                                                     interfaceName,
                                                                                                                                     ControlEntryType.MEDIATOR);
        if (removedEntry != null) {
            globalDomainAccessControllerBean.doFireMediatorRegistrationControlEntryChanged(ChangeType.REMOVE, removedEntry);
            return true;
        }
        return false;
    }

    @Override
    public OwnerRegistrationControlEntry[] getEditableOwnerRegistrationControlEntries(String uid) {
        return ownerRegistrationControlEntryManager.findByUserIdAndThatIsEditable(uid);
    }

    @Override
    public Boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        CreateOrUpdateResult<OwnerRegistrationControlEntry> result = ownerRegistrationControlEntryManager.createOrUpdate(updatedOwnerRce);
        if (result != null) {
            OwnerRegistrationControlEntry entry = result.getEntry();
            globalDomainAccessControllerBean.doFireOwnerRegistrationControlEntryChanged(result.getChangeType(), entry);
            return true;
        }
        return false;
    }

    @Override
    public Boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        OwnerRegistrationControlEntry removedEntry = ownerRegistrationControlEntryManager.removeByUserIdDomainAndInterfaceName(uid,
                                                                                                                               domain,
                                                                                                                               interfaceName);
        if (removedEntry != null) {
            globalDomainAccessControllerBean.doFireOwnerRegistrationControlEntryChanged(ChangeType.REMOVE, removedEntry);
            return true;
        }
        return false;
    }
}

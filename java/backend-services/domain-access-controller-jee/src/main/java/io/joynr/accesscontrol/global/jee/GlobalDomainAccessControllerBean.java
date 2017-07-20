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

import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MASTER;
import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MEDIATOR;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.transaction.Transactional;

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.GlobalDomainAccessControllerSubscriptionPublisher;
import joynr.infrastructure.GlobalDomainAccessControllerSync;

@Stateless
@ServiceProvider(serviceInterface = GlobalDomainAccessControllerSync.class)
@Transactional
public class GlobalDomainAccessControllerBean implements GlobalDomainAccessControllerLocal {

    private MasterAccessControlEntryManager masterAccessControlEntryManager;

    private OwnerAccessControlEntryManager ownerAccessControlEntryManager;

    private MasterRegistrationControlEntryManager masterRegistrationControlEntryManager;

    private OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager;

    private GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher;

    // Only required for testing
    protected GlobalDomainAccessControllerBean() {
    }

    @Inject
    public GlobalDomainAccessControllerBean(@SubscriptionPublisher GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher,
                                            MasterAccessControlEntryManager masterAccessControlEntryManager,
                                            OwnerAccessControlEntryManager ownerAccessControlEntryManager,
                                            MasterRegistrationControlEntryManager masterRegistrationControlEntryManager,
                                            OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager) {
        this.globalDomainAccessControllerSubscriptionPublisher = globalDomainAccessControllerSubscriptionPublisher;
        this.masterAccessControlEntryManager = masterAccessControlEntryManager;
        this.ownerAccessControlEntryManager = ownerAccessControlEntryManager;
        this.masterRegistrationControlEntryManager = masterRegistrationControlEntryManager;
        this.ownerRegistrationControlEntryManager = ownerRegistrationControlEntryManager;
    }

    private String sanitizeForPartition(String value) {
        return value.replaceAll("[^a-zA-Z0-9]", "");
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserId(uid, MASTER);
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String domain, String interfaceName) {
        return masterAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName, MASTER);
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserId(uid, MEDIATOR);
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String domain, String interfaceName) {
        return masterAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName, MEDIATOR);
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
    public MasterRegistrationControlEntry[] getMasterRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndType(uid, ControlEntryType.MASTER);
    }

    @Override
    public MasterRegistrationControlEntry[] getMediatorRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndType(uid, ControlEntryType.MEDIATOR);
    }

    @Override
    public OwnerRegistrationControlEntry[] getOwnerRegistrationControlEntries(String uid) {
        return ownerRegistrationControlEntryManager.findByUserId(uid);
    }

    public void doFireMasterAccessControlEntryChanged(ChangeType changeType, MasterAccessControlEntry persistedAce) {
        globalDomainAccessControllerSubscriptionPublisher.fireMasterAccessControlEntryChanged(changeType,
                persistedAce,
                sanitizeForPartition(persistedAce.getUid()),
                sanitizeForPartition(persistedAce.getDomain()),
                sanitizeForPartition(persistedAce.getInterfaceName()));
    }

    public void doFireMediatorAccessControlEntryChanged(ChangeType changeType, MasterAccessControlEntry persistedAce) {
        globalDomainAccessControllerSubscriptionPublisher.fireMediatorAccessControlEntryChanged(changeType,
                persistedAce,
                sanitizeForPartition(persistedAce.getUid()),
                sanitizeForPartition(persistedAce.getDomain()),
                sanitizeForPartition(persistedAce.getInterfaceName()));
    }

    public void doFireOwnerAccessControlEntryChanged(ChangeType changeType, OwnerAccessControlEntry persistedAce) {
        globalDomainAccessControllerSubscriptionPublisher.fireOwnerAccessControlEntryChanged(changeType,
                persistedAce,
                sanitizeForPartition(persistedAce.getUid()),
                sanitizeForPartition(persistedAce.getDomain()),
                sanitizeForPartition(persistedAce.getInterfaceName()));
    }

    public void doFireMasterRegistrationControlEntryChanged(ChangeType changeType, MasterRegistrationControlEntry persistedAce) {
        globalDomainAccessControllerSubscriptionPublisher.fireMasterRegistrationControlEntryChanged(changeType,
                persistedAce,
                sanitizeForPartition(persistedAce.getUid()),
                sanitizeForPartition(persistedAce.getDomain()),
                sanitizeForPartition(persistedAce.getInterfaceName()));
    }

    public void doFireMediatorRegistrationControlEntryChanged(ChangeType changeType, MasterRegistrationControlEntry persistedAce) {
        globalDomainAccessControllerSubscriptionPublisher.fireMediatorRegistrationControlEntryChanged(changeType,
                persistedAce,
                sanitizeForPartition(persistedAce.getUid()),
                sanitizeForPartition(persistedAce.getDomain()),
                sanitizeForPartition(persistedAce.getInterfaceName()));
    }

    public void doFireOwnerRegistrationControlEntryChanged(ChangeType changeType, OwnerRegistrationControlEntry persistedAce) {
        globalDomainAccessControllerSubscriptionPublisher.fireOwnerRegistrationControlEntryChanged(changeType,
                persistedAce,
                sanitizeForPartition(persistedAce.getUid()),
                sanitizeForPartition(persistedAce.getDomain()),
                sanitizeForPartition(persistedAce.getInterfaceName()));
    }

}

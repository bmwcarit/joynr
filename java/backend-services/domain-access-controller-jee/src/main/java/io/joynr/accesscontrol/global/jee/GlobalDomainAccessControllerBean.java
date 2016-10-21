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

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.transaction.Transactional;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
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

    private final GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher;

    private DomainRoleEntryManager domainRoleEntryManager;

    @Inject
    public GlobalDomainAccessControllerBean(@SubscriptionPublisher GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher,
                                            DomainRoleEntryManager domainRoleEntryManager) {
        this.globalDomainAccessControllerSubscriptionPublisher = globalDomainAccessControllerSubscriptionPublisher;
        this.domainRoleEntryManager = domainRoleEntryManager;
    }

    @Override
    public DomainRoleEntry[] getDomainRoles(String uid) {
        return domainRoleEntryManager.findByUserId(uid);
    }

    @Override
    public Boolean updateDomainRole(DomainRoleEntry updatedEntry) {
        domainRoleEntryManager.createOrUpdate(updatedEntry);
        return true;
    }

    @Override
    public Boolean removeDomainRole(String uid, Role role) {
        return domainRoleEntryManager.removeByUserIdAndRole(uid, role);
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String uid) {
        return new MasterAccessControlEntry[0];
    }

    @Override
    public MasterAccessControlEntry[] getEditableMasterAccessControlEntries(String uid) {
        return new MasterAccessControlEntry[0];
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String domain, String interfaceName) {
        return new MasterAccessControlEntry[0];
    }

    @Override
    public Boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        return false;
    }

    @Override
    public Boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return false;
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String uid) {
        return new MasterAccessControlEntry[0];
    }

    @Override
    public MasterAccessControlEntry[] getEditableMediatorAccessControlEntries(String uid) {
        return new MasterAccessControlEntry[0];
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String domain, String interfaceName) {
        return new MasterAccessControlEntry[0];
    }

    @Override
    public Boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {
        return false;
    }

    @Override
    public Boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return false;
    }

    @Override
    public OwnerAccessControlEntry[] getOwnerAccessControlEntries(String uid) {
        return new OwnerAccessControlEntry[0];
    }

    @Override
    public OwnerAccessControlEntry[] getOwnerAccessControlEntries(String domain, String interfaceName) {
        return new OwnerAccessControlEntry[0];
    }

    @Override
    public OwnerAccessControlEntry[] getEditableOwnerAccessControlEntries(String uid) {
        return new OwnerAccessControlEntry[0];
    }

    @Override
    public Boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        return false;
    }

    @Override
    public Boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return false;
    }

    @Override
    public MasterRegistrationControlEntry[] getMasterRegistrationControlEntries(String uid) {
        return new MasterRegistrationControlEntry[0];
    }

    @Override
    public MasterRegistrationControlEntry[] getEditableMasterRegistrationControlEntries(String uid) {
        return new MasterRegistrationControlEntry[0];
    }

    @Override
    public Boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        return false;
    }

    @Override
    public Boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        return false;
    }

    @Override
    public MasterRegistrationControlEntry[] getMediatorRegistrationControlEntries(String uid) {
        return new MasterRegistrationControlEntry[0];
    }

    @Override
    public MasterRegistrationControlEntry[] getEditableMediatorRegistrationControlEntries(String uid) {
        return new MasterRegistrationControlEntry[0];
    }

    @Override
    public Boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        return false;
    }

    @Override
    public Boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName) {
        return false;
    }

    @Override
    public OwnerRegistrationControlEntry[] getOwnerRegistrationControlEntries(String uid) {
        return new OwnerRegistrationControlEntry[0];
    }

    @Override
    public OwnerRegistrationControlEntry[] getEditableOwnerRegistrationControlEntries(String uid) {
        return new OwnerRegistrationControlEntry[0];
    }

    @Override
    public Boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        return false;
    }

    @Override
    public Boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        return false;
    }
}

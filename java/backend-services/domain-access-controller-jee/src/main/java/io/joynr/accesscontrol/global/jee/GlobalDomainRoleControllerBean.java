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


import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.transaction.Transactional;

import static io.joynr.dispatching.subscription.MulticastIdUtil.sanitizeForPartition;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.GlobalDomainRoleControllerSubscriptionPublisher;
import joynr.infrastructure.GlobalDomainRoleControllerSync;

@Stateless
@ServiceProvider(serviceInterface = GlobalDomainRoleControllerSync.class)
@Transactional
public class GlobalDomainRoleControllerBean implements GlobalDomainRoleControllerSync {

    private GlobalDomainRoleControllerSubscriptionPublisher globalDomainRoleControllerSubscriptionPublisher;

    private DomainRoleEntryManager domainRoleEntryManager;

    // Only required for testing
    protected GlobalDomainRoleControllerBean() {
    }

    @Inject
    public GlobalDomainRoleControllerBean(@SubscriptionPublisher GlobalDomainRoleControllerSubscriptionPublisher globalDomainRoleControllerSubscriptionPublisher,
                                            DomainRoleEntryManager domainRoleEntryManager) {
        this.globalDomainRoleControllerSubscriptionPublisher = globalDomainRoleControllerSubscriptionPublisher;
        this.domainRoleEntryManager = domainRoleEntryManager;
    }

    @Override
    public DomainRoleEntry[] getDomainRoles(String uid) {
        return domainRoleEntryManager.findByUserId(uid);
    }

    @Override
    public Boolean updateDomainRole(DomainRoleEntry updatedEntry) {
        CreateOrUpdateResult<DomainRoleEntry> result = domainRoleEntryManager.createOrUpdate(updatedEntry);
        globalDomainRoleControllerSubscriptionPublisher.fireDomainRoleEntryChanged(result.getChangeType(),
                                                                                     result.getEntry(),
                                                                                     sanitizeForPartition(result.getEntry()
                                                                                                                .getUid()));
        return true;
    }

    @Override
    public Boolean removeDomainRole(String uid, Role role) {
        DomainRoleEntry removedEntry = domainRoleEntryManager.removeByUserIdAndRole(uid, role);
        if (removedEntry != null) {
            globalDomainRoleControllerSubscriptionPublisher.fireDomainRoleEntryChanged(ChangeType.REMOVE,
                                                                                         removedEntry,
                                                                                         sanitizeForPartition(uid));
            return true;
        }
        return false;
    }
}

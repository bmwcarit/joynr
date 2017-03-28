package io.joynr.accesscontrol.global;

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

import io.joynr.accesscontrol.DomainAccessControlStore;
import io.joynr.provider.Promise;

import joynr.infrastructure.GlobalDomainRoleControllerAbstractProvider;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.Role;

import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * Manages the Role Control Lists for all providers.
 */
@Singleton
public class GlobalDomainRoleControllerProviderImpl extends GlobalDomainRoleControllerAbstractProvider {
    private final DomainAccessControlStore domainAccessStore;

    @Inject
    public GlobalDomainRoleControllerProviderImpl(DomainAccessControlStore domainAccessStore) {
        this.domainAccessStore = domainAccessStore;
    }

    @Override
    public Promise<GetDomainRolesDeferred> getDomainRoles(String uid) {
        GetDomainRolesDeferred deferred = new GetDomainRolesDeferred();
        deferred.resolve(domainAccessStore.getDomainRoles(uid).toArray(new DomainRoleEntry[0]));
        return new Promise<GetDomainRolesDeferred>(deferred);
    }

    @Override
    public Promise<UpdateDomainRoleDeferred> updateDomainRole(DomainRoleEntry updatedEntry) {
        UpdateDomainRoleDeferred deferred = new UpdateDomainRoleDeferred();
        boolean updateSuccess = domainAccessStore.updateDomainRole(updatedEntry);
        if (updateSuccess) {
            fireDomainRoleEntryChanged(ChangeType.UPDATE, updatedEntry);
        }
        deferred.resolve(updateSuccess);
        return new Promise<UpdateDomainRoleDeferred>(deferred);
    }

    @Override
    public Promise<RemoveDomainRoleDeferred> removeDomainRole(String uid, Role role) {
        RemoveDomainRoleDeferred deferred = new RemoveDomainRoleDeferred();
        boolean removeSuccess = domainAccessStore.removeDomainRole(uid, role);
        if (removeSuccess) {
            // To notify DRE removal only primary keys (user ID and role) must
            // be set. All other fields are undefined.
            DomainRoleEntry removedEntry = new DomainRoleEntry(uid, null, role);
            fireDomainRoleEntryChanged(ChangeType.REMOVE, removedEntry);
        }
        deferred.resolve(removeSuccess);
        return new Promise<RemoveDomainRoleDeferred>(deferred);
    }
}

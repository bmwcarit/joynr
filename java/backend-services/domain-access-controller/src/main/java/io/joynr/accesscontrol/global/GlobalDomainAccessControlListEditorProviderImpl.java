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

import java.util.Arrays;

import joynr.infrastructure.GlobalDomainAccessControlListEditorAbstractProvider;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;

import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * Manages the Access Control Lists for all providers.
 */
@Singleton
public class GlobalDomainAccessControlListEditorProviderImpl extends
        GlobalDomainAccessControlListEditorAbstractProvider {

    private static final String DUMMY_USERID = "dummyUserId";

    private final DomainAccessControlStore domainAccessStore;
    private GlobalDomainAccessControllerProviderImpl domainAccessControllerProvider;

    @Inject
    public GlobalDomainAccessControlListEditorProviderImpl(DomainAccessControlStore domainAccessStore,
                                                           GlobalDomainAccessControllerProviderImpl domainAccessControllerProvider) {

        this.domainAccessStore = domainAccessStore;
        // required to fire broadcasts
        this.domainAccessControllerProvider = domainAccessControllerProvider;
    }

    @Override
    public Promise<GetEditableMasterAccessControlEntriesDeferred> getEditableMasterAccessControlEntries(String uid) {
        GetEditableMasterAccessControlEntriesDeferred deferred = new GetEditableMasterAccessControlEntriesDeferred();
        deferred.resolve(domainAccessStore.getEditableMasterAccessControlEntries(uid)
                                          .toArray(new MasterAccessControlEntry[0]));
        return new Promise<GetEditableMasterAccessControlEntriesDeferred>(deferred);
    }

    @Override
    public Promise<UpdateMasterAccessControlEntryDeferred> updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAccessControlEntry) {
        UpdateMasterAccessControlEntryDeferred deferred = new UpdateMasterAccessControlEntryDeferred();

        // Unless the userId has Role.MASTER, they may not change Master ACL
        // TODO: we need the user ID of the user that is updating the ACE
        if (!hasRoleMaster(DUMMY_USERID, updatedMasterAccessControlEntry.getDomain())) {
            deferred.resolve(false);
        } else {
            boolean updateSuccess = domainAccessStore.updateMasterAccessControlEntry(updatedMasterAccessControlEntry);
            if (updateSuccess) {
                domainAccessControllerProvider.fireMasterAccessControlEntryChanged(ChangeType.UPDATE,
                                                                                   updatedMasterAccessControlEntry);
            }
            deferred.resolve(updateSuccess);
        }
        return new Promise<UpdateMasterAccessControlEntryDeferred>(deferred);
    }

    // Indicates if the given user has master role for the given domain
    private boolean hasRoleMaster(String userId, String domain) {

        DomainRoleEntry domainRole = domainAccessStore.getDomainRole(userId, Role.MASTER);
        if (domainRole == null || !Arrays.asList(domainRole.getDomains()).contains(domain)) {
            return false;
        }

        return true;
    }

    @Override
    public Promise<RemoveMasterAccessControlEntryDeferred> removeMasterAccessControlEntry(String uid,
                                                                                          String domain,
                                                                                          String interfaceName,
                                                                                          String operation) {
        RemoveMasterAccessControlEntryDeferred deferred = new RemoveMasterAccessControlEntryDeferred();
        boolean removeSuccess = domainAccessStore.removeMasterAccessControlEntry(uid, domain, interfaceName, operation);
        if (removeSuccess) {
            MasterAccessControlEntry removedEntry = new MasterAccessControlEntry(uid,
                                                                                 domain,
                                                                                 interfaceName,
                                                                                 null,
                                                                                 null,
                                                                                 null,
                                                                                 null,
                                                                                 operation,
                                                                                 null,
                                                                                 null);
            domainAccessControllerProvider.fireMasterAccessControlEntryChanged(ChangeType.REMOVE, removedEntry);
        }
        deferred.resolve(removeSuccess);
        return new Promise<RemoveMasterAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<GetEditableMediatorAccessControlEntriesDeferred> getEditableMediatorAccessControlEntries(String uid) {
        GetEditableMediatorAccessControlEntriesDeferred deferred = new GetEditableMediatorAccessControlEntriesDeferred();
        deferred.resolve(domainAccessStore.getEditableMediatorAccessControlEntries(uid)
                                          .toArray(new MasterAccessControlEntry[0]));
        return new Promise<GetEditableMediatorAccessControlEntriesDeferred>(deferred);
    }

    @Override
    public Promise<UpdateMediatorAccessControlEntryDeferred> updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAccessControlEntry) {
        UpdateMediatorAccessControlEntryDeferred deferred = new UpdateMediatorAccessControlEntryDeferred();

        // Unless the userId has Role.MASTER, they may not change Mediator ACL
        // TODO: we need the user ID of the user that is updating the ACE
        if (!hasRoleMaster(DUMMY_USERID, updatedMediatorAccessControlEntry.getDomain())) {
            deferred.resolve(false);
        } else {
            boolean updateSuccess = domainAccessStore.updateMediatorAccessControlEntry(updatedMediatorAccessControlEntry);
            if (updateSuccess) {
                domainAccessControllerProvider.fireMediatorAccessControlEntryChanged(ChangeType.UPDATE,
                                                                                     updatedMediatorAccessControlEntry);
            }
            deferred.resolve(updateSuccess);
        }
        return new Promise<UpdateMediatorAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<RemoveMediatorAccessControlEntryDeferred> removeMediatorAccessControlEntry(String uid,
                                                                                              String domain,
                                                                                              String interfaceName,
                                                                                              String operation) {
        RemoveMediatorAccessControlEntryDeferred deferred = new RemoveMediatorAccessControlEntryDeferred();
        boolean removeSuccess = domainAccessStore.removeMediatorAccessControlEntry(uid,
                                                                                   domain,
                                                                                   interfaceName,
                                                                                   operation);
        if (removeSuccess) {
            MasterAccessControlEntry removedEntry = new MasterAccessControlEntry(uid,
                                                                                 domain,
                                                                                 interfaceName,
                                                                                 null,
                                                                                 null,
                                                                                 null,
                                                                                 null,
                                                                                 operation,
                                                                                 null,
                                                                                 null);
            domainAccessControllerProvider.fireMediatorAccessControlEntryChanged(ChangeType.REMOVE, removedEntry);
        }
        deferred.resolve(removeSuccess);
        return new Promise<RemoveMediatorAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<GetEditableOwnerAccessControlEntriesDeferred> getEditableOwnerAccessControlEntries(String uid) {
        GetEditableOwnerAccessControlEntriesDeferred deferred = new GetEditableOwnerAccessControlEntriesDeferred();
        deferred.resolve(domainAccessStore.getEditableOwnerAccessControlEntries(uid)
                                          .toArray(new OwnerAccessControlEntry[0]));
        return new Promise<GetEditableOwnerAccessControlEntriesDeferred>(deferred);
    }

    @Override
    public Promise<UpdateOwnerAccessControlEntryDeferred> updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAccessControlEntry) {
        UpdateOwnerAccessControlEntryDeferred deferred = new UpdateOwnerAccessControlEntryDeferred();

        // Unless the userId has Role.MASTER, they may not change Owner ACL
        // TODO: we need the user ID of the user that is updating the ACE
        if (!hasRoleMaster(DUMMY_USERID, updatedOwnerAccessControlEntry.getDomain())) {
            deferred.resolve(false);
        } else {
            boolean updateSuccess = domainAccessStore.updateOwnerAccessControlEntry(updatedOwnerAccessControlEntry);
            if (updateSuccess) {
                domainAccessControllerProvider.fireOwnerAccessControlEntryChanged(ChangeType.UPDATE,
                                                                                  updatedOwnerAccessControlEntry);
            }
            deferred.resolve(updateSuccess);
        }
        return new Promise<UpdateOwnerAccessControlEntryDeferred>(deferred);
    }

    @Override
    public Promise<RemoveOwnerAccessControlEntryDeferred> removeOwnerAccessControlEntry(String uid,
                                                                                        String domain,
                                                                                        String interfaceName,
                                                                                        String operation) {
        RemoveOwnerAccessControlEntryDeferred deferred = new RemoveOwnerAccessControlEntryDeferred();
        boolean removeSuccess = domainAccessStore.removeOwnerAccessControlEntry(uid, domain, interfaceName, operation);
        if (removeSuccess) {
            OwnerAccessControlEntry removedEntry = new OwnerAccessControlEntry(uid,
                                                                               domain,
                                                                               interfaceName,
                                                                               null,
                                                                               null,
                                                                               operation,
                                                                               null);
            domainAccessControllerProvider.fireOwnerAccessControlEntryChanged(ChangeType.REMOVE, removedEntry);
        }
        deferred.resolve(removeSuccess);
        return new Promise<RemoveOwnerAccessControlEntryDeferred>(deferred);
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

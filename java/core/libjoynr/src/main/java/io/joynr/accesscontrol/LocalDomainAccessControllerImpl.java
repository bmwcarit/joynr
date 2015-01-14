package io.joynr.accesscontrol;

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
import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.DomainRoleEntry;
import joynr.infrastructure.Role;
import joynr.infrastructure.TrustLevel;
import joynr.infrastructure.Permission;
import joynr.infrastructure.MasterRegistrationControlEntry;
import joynr.infrastructure.OwnerRegistrationControlEntry;

import javax.annotation.CheckForNull;
import java.util.List;

public class LocalDomainAccessControllerImpl implements LocalDomainAccessController {
    private final DomainAccessControlStore localDomainAccessStore;
    private final AccessControlAlgorithm accessControlAlgorithm;
    private static final String WILDCARD = "*";

    @Inject
    LocalDomainAccessControllerImpl(DomainAccessControlStore localDomainAccessStore,
                                    AccessControlAlgorithm accessControlAlgorithm) {
        this.localDomainAccessStore = localDomainAccessStore;
        this.accessControlAlgorithm = accessControlAlgorithm;
    }

    @Override
    public boolean hasRole(String userId, String domain, Role role) {
        boolean hasRole = false;
        DomainRoleEntry dre = localDomainAccessStore.getDomainRole(userId, role);
        if (dre != null && dre.getDomains().contains(domain)) {
            hasRole = true;
        }

        return hasRole;
    }

    @Override
    @CheckForNull
    public Permission getConsumerPermission(String userId, String domain, String interfaceName, TrustLevel trustLevel) {
        List<MasterAccessControlEntry> masterAces = localDomainAccessStore.getMasterAccessControlEntries(userId,
                                                                                                         domain,
                                                                                                         interfaceName);
        List<MasterAccessControlEntry> mediatorAces = localDomainAccessStore.getMediatorAccessControlEntries(userId,
                                                                                                             domain,
                                                                                                             interfaceName);
        List<OwnerAccessControlEntry> ownerAces = localDomainAccessStore.getOwnerAccessControlEntries(userId,
                                                                                                      domain,
                                                                                                      interfaceName);

        if (masterAces.size() > 1 || mediatorAces.size() > 1 || ownerAces.size() > 1) {
            return null;
        } else {
            return getConsumerPermission(userId, domain, interfaceName, WILDCARD, trustLevel);
        }
    }

    @Override
    public Permission getConsumerPermission(String userId,
                                            String domain,
                                            String interfaceName,
                                            String operation,
                                            TrustLevel trustLevel) {
        MasterAccessControlEntry masterAce = localDomainAccessStore.getMasterAccessControlEntry(userId,
                                                                                                domain,
                                                                                                interfaceName,
                                                                                                operation);
        MasterAccessControlEntry mediatorAce = localDomainAccessStore.getMediatorAccessControlEntry(userId,
                                                                                                    domain,
                                                                                                    interfaceName,
                                                                                                    operation);
        OwnerAccessControlEntry ownerAce = localDomainAccessStore.getOwnerAccessControlEntry(userId,
                                                                                             domain,
                                                                                             interfaceName,
                                                                                             operation);

        return accessControlAlgorithm.getConsumerPermission(masterAce, mediatorAce, ownerAce, trustLevel);
    }

    @Override
    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid) {
        return localDomainAccessStore.getMasterAccessControlEntries(uid);
    }

    @Override
    public boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        return localDomainAccessStore.updateMasterAccessControlEntry(updatedMasterAce);
    }

    @Override
    public boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return localDomainAccessStore.removeMasterAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid) {
        return localDomainAccessStore.getMediatorAccessControlEntries(uid);
    }

    @Override
    public boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {
        return localDomainAccessStore.updateMediatorAccessControlEntry(updatedMediatorAce);
    }

    @Override
    public boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return localDomainAccessStore.removeMediatorAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid) {
        return localDomainAccessStore.getOwnerAccessControlEntries(uid);
    }

    @Override
    public boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        return localDomainAccessStore.updateOwnerAccessControlEntry(updatedOwnerAce);
    }

    @Override
    public boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        return localDomainAccessStore.removeOwnerAccessControlEntry(uid, domain, interfaceName, operation);
    }

    @Override
    public Permission getProviderPermission(String uid, String domain, String interfaceName, TrustLevel trustLevel) {
        assert false : "Not implemented yet";
        return accessControlAlgorithm.getProviderPermission(null, null, null, trustLevel);
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMasterRegistrationControlEntries(String uid) {
        assert false : "Not implemented yet";
        return null;
    }

    @Override
    public boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        assert false : "Not implemented yet";
        return false;
    }

    @Override
    public boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        assert false : "Not implemented yet";
        return false;
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMediatorRegistrationControlEntries(String uid) {
        assert false : "Not implemented yet";
        return null;
    }

    @Override
    public boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        assert false : "Not implemented yet";
        return false;
    }

    @Override
    public boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName) {
        assert false : "Not implemented yet";
        return false;
    }

    @Override
    public List<OwnerAccessControlEntry> getEditableOwnerRegistrationControlEntries(String uid) {
        assert false : "Not implemented yet";
        return null;
    }

    @Override
    public boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        assert false : "Not implemented yet";
        return false;
    }

    @Override
    public boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        assert false : "Not implemented yet";
        return false;
    }
}

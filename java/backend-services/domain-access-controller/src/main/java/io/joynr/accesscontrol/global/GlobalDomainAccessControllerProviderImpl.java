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

import joynr.infrastructure.GlobalDomainAccessControllerAbstractProvider;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;

import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * Manages the Access Control Lists for all providers.
 */
@Singleton
public class GlobalDomainAccessControllerProviderImpl extends GlobalDomainAccessControllerAbstractProvider {

    private final DomainAccessControlStore domainAccessStore;

    @Inject
    public GlobalDomainAccessControllerProviderImpl(DomainAccessControlStore domainAccessStore) {
        this.domainAccessStore = domainAccessStore;
    }

    @Override
    public Promise<GetMasterAccessControlEntries1Deferred> getMasterAccessControlEntries(String uid) {
        GetMasterAccessControlEntries1Deferred deferred = new GetMasterAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMasterAccessControlEntries(uid).toArray(new MasterAccessControlEntry[0]));
        return new Promise<GetMasterAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetMasterAccessControlEntries1Deferred> getMasterAccessControlEntries(String domain,
                                                                                         String interfaceName) {
        GetMasterAccessControlEntries1Deferred deferred = new GetMasterAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMasterAccessControlEntries(domain, interfaceName)
                                          .toArray(new MasterAccessControlEntry[0]));
        return new Promise<GetMasterAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetMediatorAccessControlEntries1Deferred> getMediatorAccessControlEntries(String uid) {
        GetMediatorAccessControlEntries1Deferred deferred = new GetMediatorAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMediatorAccessControlEntries(uid)
                                          .toArray(new MasterAccessControlEntry[0]));
        return new Promise<GetMediatorAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetMediatorAccessControlEntries1Deferred> getMediatorAccessControlEntries(String domain,
                                                                                             String interfaceName) {
        GetMediatorAccessControlEntries1Deferred deferred = new GetMediatorAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getMediatorAccessControlEntries(domain, interfaceName)
                                          .toArray(new MasterAccessControlEntry[0]));
        return new Promise<GetMediatorAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetOwnerAccessControlEntries1Deferred> getOwnerAccessControlEntries(String uid) {
        GetOwnerAccessControlEntries1Deferred deferred = new GetOwnerAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getOwnerAccessControlEntries(uid).toArray(new OwnerAccessControlEntry[0]));
        return new Promise<GetOwnerAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetOwnerAccessControlEntries1Deferred> getOwnerAccessControlEntries(String domain,
                                                                                       String interfaceName) {
        GetOwnerAccessControlEntries1Deferred deferred = new GetOwnerAccessControlEntries1Deferred();
        deferred.resolve(domainAccessStore.getOwnerAccessControlEntries(domain, interfaceName)
                                          .toArray(new OwnerAccessControlEntry[0]));
        return new Promise<GetOwnerAccessControlEntries1Deferred>(deferred);
    }

    @Override
    public Promise<GetMasterRegistrationControlEntriesDeferred> getMasterRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<GetMediatorRegistrationControlEntriesDeferred> getMediatorRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }

    @Override
    public Promise<GetOwnerRegistrationControlEntriesDeferred> getOwnerRegistrationControlEntries(String uid) {
        assert false : "method not implemented";
        return null;
    }
}

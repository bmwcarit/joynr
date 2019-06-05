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
package io.joynr.capabilities.directory;

import java.util.Arrays;
import java.util.Collection;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.GlobalDiscoveryEntryPersistedStorePersisted;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;

/**
 * The capabilities directory implementation for server-side capabilities querying.
 * Capability informations are stored in a concurrentHashMap. Using a in memory database could be possible optimization.
 */

@Singleton
public class CapabilitiesDirectoryImpl extends GlobalCapabilitiesDirectoryAbstractProvider {
    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesDirectoryImpl.class);

    private GlobalDiscoveryEntryPersistedStorePersisted discoveryEntryStore;

    @Inject
    public CapabilitiesDirectoryImpl(GlobalDiscoveryEntryPersistedStorePersisted discoveryEntryStore) {
        this.discoveryEntryStore = discoveryEntryStore;
    }

    @Override
    public Promise<DeferredVoid> add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        DeferredVoid deferred = new DeferredVoid();
        Promise<DeferredVoid> promise = new Promise<DeferredVoid>(deferred);
        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
        String clusterControllerId;
        if (address instanceof MqttAddress) {
            clusterControllerId = ((MqttAddress) address).getTopic();
        } else if (address instanceof ChannelAddress) {
            clusterControllerId = ((ChannelAddress) address).getChannelId();
        } else {
            deferred.reject(new ProviderRuntimeException(""));
            return promise;
        }
        GlobalDiscoveryEntryPersisted discoveryEntry = new GlobalDiscoveryEntryPersisted(globalDiscoveryEntry,
                                                                                         clusterControllerId);
        logger.debug("registered discovery entry: {}", discoveryEntry);
        discoveryEntryStore.add(discoveryEntry);
        deferred.resolve();
        return promise;
    }

    @Override
    public Promise<Add1Deferred> add(GlobalDiscoveryEntry globalDiscoveryEntry, String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    public Promise<DeferredVoid> add(GlobalDiscoveryEntry[] globalDiscoveryEntries) {
        DeferredVoid deferred = new DeferredVoid();
        for (GlobalDiscoveryEntry globalDiscoveryEntry : globalDiscoveryEntries) {
            add(globalDiscoveryEntry);
        }
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> remove(String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        logger.debug("removed discovery entry with participantId: {}", participantId);
        discoveryEntryStore.remove(participantId);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Remove1Deferred> remove(String participantId, String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    public Promise<DeferredVoid> remove(String[] participantIds) {
        DeferredVoid deferred = new DeferredVoid();
        discoveryEntryStore.remove(Arrays.asList(participantIds));
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Lookup1Deferred> lookup(final String[] domains, final String interfaceName) {
        Lookup1Deferred deferred = new Lookup1Deferred();
        GlobalDiscoveryEntry[] globalDiscoveryEntries = lookupInternal(domains, interfaceName);
        deferred.resolve(globalDiscoveryEntries);
        return new Promise<Lookup1Deferred>(deferred);
    }

    private GlobalDiscoveryEntry[] lookupInternal(final String[] domains, final String interfaceName) {
        logger.debug("Searching for global discovery entries for domains: {} interfaceName: {}",
                     domains,
                     interfaceName);
        Collection<GlobalDiscoveryEntryPersisted> discoveryEntries = discoveryEntryStore.lookup(domains, interfaceName);
        GlobalDiscoveryEntry[] globalDiscoveryEntries = new GlobalDiscoveryEntry[discoveryEntries.size()];
        int index = 0;
        for (DiscoveryEntry discoveryEntry : discoveryEntries) {
            // entries from persisted store are of type GlobalDiscoveryEntryPersisted.
            // Copy required or else _typeName will be incorrect
            globalDiscoveryEntries[index] = new GlobalDiscoveryEntry((GlobalDiscoveryEntry) discoveryEntry);
            index++;
        }
        return globalDiscoveryEntries;
    }

    @Override
    public Promise<Lookup2Deferred> lookup(String[] domains, String interfaceName, String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    public Promise<Lookup3Deferred> lookup(String forParticipantId) {
        Lookup3Deferred deferred = new Lookup3Deferred();
        logger.debug("Searching discovery entries for participantId: {}", forParticipantId);
        DiscoveryEntry discoveryEntry = discoveryEntryStore.lookup(forParticipantId,
                                                                   DiscoveryQos.NO_FILTER.getCacheMaxAgeMs());
        if (discoveryEntry == null) {
            deferred.resolve(null);
        } else {
            deferred.resolve((GlobalDiscoveryEntry) discoveryEntry);
        }
        return new Promise<Lookup3Deferred>(deferred);
    }

    @Override
    public Promise<Lookup4Deferred> lookup(String participantId, String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    public Promise<DeferredVoid> touch(String clusterControllerId) {
        DeferredVoid deferred = new DeferredVoid();
        discoveryEntryStore.touch(clusterControllerId);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

}

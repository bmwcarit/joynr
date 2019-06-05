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
import com.google.inject.name.Named;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.GlobalDiscoveryEntryPersistedStorePersisted;
import io.joynr.capabilities.directory.util.GcdUtilities;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

/**
 * The capabilities directory implementation for server-side capabilities querying.
 */
@Singleton
public class CapabilitiesDirectoryImpl extends GlobalCapabilitiesDirectoryAbstractProvider {
    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesDirectoryImpl.class);
    public static final String GCD_GBID = "joynr.gcd.gbid";

    private GlobalDiscoveryEntryPersistedStorePersisted discoveryEntryStore;
    private String gcdGbId;

    @Inject
    public CapabilitiesDirectoryImpl(GlobalDiscoveryEntryPersistedStorePersisted discoveryEntryStore,
                                     @Named(GCD_GBID) String gcdGbId) {
        this.discoveryEntryStore = discoveryEntryStore;
        this.gcdGbId = gcdGbId;
    }

    @Override
    public Promise<DeferredVoid> add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        DeferredVoid deferred = new DeferredVoid();
        Promise<DeferredVoid> promise = new Promise<DeferredVoid>(deferred);
        try {
            addInternal(globalDiscoveryEntry, gcdGbId);
            deferred.resolve();
        } catch (ProviderRuntimeException e) {
            logger.error("Error adding DiscoveryEntry for {}: {}", globalDiscoveryEntry.getParticipantId(), e);
            deferred.reject(e);
        }
        return promise;
    }

    @Override
    public Promise<Add1Deferred> add(GlobalDiscoveryEntry globalDiscoveryEntry, String[] gbids) {
        Add1Deferred deferred = new Add1Deferred();
        Promise<Add1Deferred> promise = new Promise<Add1Deferred>(deferred);
        switch (GcdUtilities.validateGbids(gbids, gcdGbId)) {
        case INVALID:
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            try {
                addInternal(globalDiscoveryEntry, gbids);
                deferred.resolve();
            } catch (ProviderRuntimeException e) {
                logger.error("Error adding DiscoveryEntry: {}", e);
                deferred.reject(e);
            }
        }
        return promise;
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

    private void addInternal(GlobalDiscoveryEntry globalDiscoveryEntry, String... gbids) {
        assert (gbids.length > 0);
        String clusterControllerId = "";
        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
        if (address instanceof MqttAddress) {
            // not always the clusterControllerId. If a unicast topic prefix is set, this clusterControllerId is a part of the topic
            clusterControllerId = ((MqttAddress) address).getTopic();
            ((MqttAddress) address).setBrokerUri(gbids[0]);
            globalDiscoveryEntry.setAddress(RoutingTypesUtil.toAddressString(address));
        } else if (address instanceof ChannelAddress) {
            clusterControllerId = ((ChannelAddress) address).getChannelId();
        } else {
            throw new ProviderRuntimeException("Unable to add DiscoveryEntry for "
                    + globalDiscoveryEntry.getParticipantId() + ". Unknown address type: "
                    + globalDiscoveryEntry.getAddress());
        }
        GlobalDiscoveryEntryPersisted discoveryEntry = new GlobalDiscoveryEntryPersisted(globalDiscoveryEntry,
                                                                                         clusterControllerId);
        logger.debug("registered discovery entry: {}", discoveryEntry);
        discoveryEntryStore.add(discoveryEntry);
    }

    @Override
    public Promise<DeferredVoid> remove(String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        removeInternal(participantId, gcdGbId);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    private boolean removeInternal(String participantId, String... gbids) {
        assert (gbids.length > 0);
        logger.debug("removing discovery entry with participantId: {} in backends {}",
                     participantId,
                     Arrays.toString(gbids));
        return discoveryEntryStore.remove(participantId);
    }

    @Override
    public Promise<Remove1Deferred> remove(String participantId, String[] gbids) {
        Remove1Deferred deferred = new Remove1Deferred();
        Promise<Remove1Deferred> promise = new Promise<Remove1Deferred>(deferred);
        switch (GcdUtilities.validateGbids(gbids, gcdGbId)) {
        case INVALID:
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            if (removeInternal(participantId, gbids)) {
                deferred.resolve();
            } else {
                deferred.reject(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
            }
        }
        return promise;
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
        GlobalDiscoveryEntry[] globalDiscoveryEntries = lookupInternal(domains, interfaceName, gcdGbId);
        deferred.resolve(globalDiscoveryEntries);
        return new Promise<Lookup1Deferred>(deferred);
    }

    private GlobalDiscoveryEntry[] lookupInternal(final String[] domains, final String interfaceName, String... gbids) {
        assert (gbids.length > 0);
        logger.debug("Searching for global discovery entries for domains: {} interfaceName: {} in backends {}",
                     domains,
                     interfaceName,
                     Arrays.toString(gbids));
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
        Lookup2Deferred deferred = new Lookup2Deferred();
        Promise<Lookup2Deferred> promise = new Promise<Lookup2Deferred>(deferred);
        GlobalDiscoveryEntry[] globalDiscoveryEntries = null;
        switch (GcdUtilities.validateGbids(gbids, gcdGbId)) {
        case INVALID:
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            globalDiscoveryEntries = lookupInternal(domains, interfaceName, gbids);
            if (globalDiscoveryEntries.length == 0) {
                logger.debug("No Global Discovery Entries found for backends {} in domains: {} and for interface {}",
                             gbids,
                             domains,
                             interfaceName);
            }
            deferred.resolve(globalDiscoveryEntries);
            break;
        }
        return promise;
    }

    @Override
    public Promise<Lookup3Deferred> lookup(String participantId) {
        Lookup3Deferred deferred = new Lookup3Deferred();
        GlobalDiscoveryEntry discoveryEntry = lookupInternal(participantId, gcdGbId);
        if (discoveryEntry == null) {
            deferred.reject(new ProviderRuntimeException("No Entry found for participantId " + participantId));
        } else {
            deferred.resolve(discoveryEntry);
        }
        return new Promise<Lookup3Deferred>(deferred);
    }

    private GlobalDiscoveryEntry lookupInternal(String forParticipantId, String... gbids) {
        assert (gbids.length > 0);
        logger.debug("Searching discovery entries for participantId: {} in backends: {}...",
                     forParticipantId,
                     Arrays.toString(gbids));
        return (GlobalDiscoveryEntry) discoveryEntryStore.lookup(forParticipantId,
                                                                 DiscoveryQos.NO_FILTER.getCacheMaxAgeMs());
    }

    @Override
    public Promise<Lookup4Deferred> lookup(String participantId, String[] gbids) {
        Lookup4Deferred deferred = new Lookup4Deferred();
        Promise<Lookup4Deferred> promise = new Promise<Lookup4Deferred>(deferred);
        switch (GcdUtilities.validateGbids(gbids, gcdGbId)) {
        case INVALID:
            deferred.reject(DiscoveryError.INVALID_GBID);
            break;
        case UNKNOWN:
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
            break;
        case OK:
            GlobalDiscoveryEntry discoveryEntry = lookupInternal(participantId, gbids);
            if (discoveryEntry == null) {
                deferred.reject(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
            } else {
                deferred.resolve(discoveryEntry);
            }
        }
        return promise;
    }

    @Override
    public Promise<DeferredVoid> touch(String clusterControllerId) {
        DeferredVoid deferred = new DeferredVoid();
        discoveryEntryStore.touch(clusterControllerId);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

}

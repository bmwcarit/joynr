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
import io.joynr.capabilities.DiscoveryEntryStore;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
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

    private DiscoveryEntryStore discoveryEntryStore;
    private String gcdGbId;

    @Inject
    public CapabilitiesDirectoryImpl(@Persisted DiscoveryEntryStore discoveryEntryStore,
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
            logger.error("Error adding DiscoveryEntry: {}", e);
            deferred.reject(e);
        }
        return promise;
    }

    @Override
    public Promise<Add1Deferred> add(GlobalDiscoveryEntry globalDiscoveryEntry, String[] gbids) {
        Add1Deferred deferred = new Add1Deferred();
        Promise<Add1Deferred> promise = new Promise<Add1Deferred>(deferred);
        if (gbids.length == 0) {
            logger.error("INVALID_GBID: provided list of GBIDs is empty.");
            deferred.reject(DiscoveryError.INVALID_GBID);
        } else if (gbids.length > 1) {
            deferred.reject(new ProviderRuntimeException("MULTIPLE GBIDs ARE NOT PERMITTED FOR THE MOMENT"));
        } else if (gbids[0] == null || gbids[0].isEmpty()) {
            logger.error("INVALID_GBID: provided GBID is null or empty: {}.", gbids[0]);
            deferred.reject(DiscoveryError.INVALID_GBID);
        } else if (!gcdGbId.equals(gbids[0])) {
            logger.error("UNKNOWN_GBID: {}", gbids[0]);
            deferred.reject(DiscoveryError.UNKNOWN_GBID);
        } else {
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
        logger.debug("Searching channels for domains: {} interfaceName: {}", domains, interfaceName);
        Collection<DiscoveryEntry> discoveryEntries = discoveryEntryStore.lookup(domains, interfaceName);
        GlobalDiscoveryEntry[] globalDiscoveryEntries = new GlobalDiscoveryEntry[discoveryEntries.size()];
        int index = 0;
        for (DiscoveryEntry discoveryEntry : discoveryEntries) {
            // entries from persisted store are of type GlobalDiscoveryEntryPersisted.
            // Copy required or else _typeName will be incorrect
            globalDiscoveryEntries[index] = new GlobalDiscoveryEntry((GlobalDiscoveryEntry) discoveryEntry);
            index++;
        }
        deferred.resolve(globalDiscoveryEntries);
        return new Promise<Lookup1Deferred>(deferred);
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

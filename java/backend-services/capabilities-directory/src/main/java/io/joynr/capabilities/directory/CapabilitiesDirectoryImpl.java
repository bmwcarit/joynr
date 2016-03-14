package io.joynr.capabilities.directory;

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

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.CapabilitiesStore;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.CapabilityEntryPersisted;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;

import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.types.GlobalDiscoveryEntry;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * The capabilities directory implementation for server-side capabilities querying.
 * Capability informations are stored in a concurrentHashMap. Using a in memory database could be possible optimization.
 */

// TODO Evaluate pro /cons of a in memory database
// TODO Using the interfaceAddress as the key may increase performance in most
// requests.

@Singleton
public class CapabilitiesDirectoryImpl extends GlobalCapabilitiesDirectoryAbstractProvider {
    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesDirectoryImpl.class);
    private CapabilitiesStore capabiltiesStore;

    @Inject
    public CapabilitiesDirectoryImpl(CapabilitiesStore capabiltiesStore) {
        this.capabiltiesStore = capabiltiesStore;
    }

    @Override
    public Promise<DeferredVoid> add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        DeferredVoid deferred = new DeferredVoid();
        CapabilityEntry capabilityEntry = new CapabilityEntryPersisted(globalDiscoveryEntry);
        logger.debug("registered capability: {}", capabilityEntry);
        capabiltiesStore.add(capabilityEntry);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> add(GlobalDiscoveryEntry[] globalDiscoveryEntries) {
        DeferredVoid deferred = new DeferredVoid();
        // TODO check interfaces before adding them
        List<CapabilityEntry> capabilityEntries = Lists.newArrayList();
        for (GlobalDiscoveryEntry globalDiscoveryEntry : globalDiscoveryEntries) {
            capabilityEntries.add(new CapabilityEntryPersisted(globalDiscoveryEntry));
        }

        logger.debug("registered capabilities: interface {}", capabilityEntries.toString());

        capabiltiesStore.add(capabilityEntries);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> remove(String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        logger.debug("removed capability with participantId: {}", participantId);
        capabiltiesStore.remove(participantId);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> remove(String[] capabilities) {
        DeferredVoid deferred = new DeferredVoid();
        // TODO who is allowed to remove capabilities
        List<CapabilityEntry> capabilityEntries = Lists.newArrayList();
        logger.debug("Removing capabilities: Capabilities {}", capabilityEntries);
        capabiltiesStore.remove(Arrays.asList(capabilities));
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Lookup1Deferred> lookup(final String domain, final String interfaceName) {
        Lookup1Deferred deferred = new Lookup1Deferred();
        logger.debug("Searching channels for domain: " + domain + " interfaceName: " + interfaceName + " {}");
        Collection<CapabilityEntry> entryCollection = capabiltiesStore.lookup(domain, interfaceName);
        GlobalDiscoveryEntry[] globalDiscoveryEntries = new GlobalDiscoveryEntry[entryCollection.size()];
        int index = 0;
        for (CapabilityEntry entry : entryCollection) {
            globalDiscoveryEntries[index] = CapabilityUtils.capabilityEntry2GlobalDiscoveryEntry(entry);
            index++;
        }
        deferred.resolve(globalDiscoveryEntries);
        return new Promise<Lookup1Deferred>(deferred);
    }

    @Override
    public Promise<Lookup2Deferred> lookup(String forParticipantId) {
        Lookup2Deferred deferred = new Lookup2Deferred();
        logger.debug("Searching capabilities for participantId: {}", forParticipantId);
        CapabilityEntry capEntry = capabiltiesStore.lookup(forParticipantId, DiscoveryQos.NO_FILTER.getCacheMaxAge());
        if (capEntry == null) {
            deferred.resolve(null);
        } else {
            deferred.resolve(CapabilityUtils.capabilityEntry2GlobalDiscoveryEntry(capEntry));
        }
        return new Promise<Lookup2Deferred>(deferred);
    }
}

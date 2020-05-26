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
package io.joynr.capabilities;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;

public class DummyCapabilitiesDirectory extends AbstractLocalCapabilitiesDirectory {
    private static final Logger logger = LoggerFactory.getLogger(DummyCapabilitiesDirectory.class);
    private static final DummyCapabilitiesDirectory instance = new DummyCapabilitiesDirectory();

    private ArrayList<DiscoveryEntryWithMetaInfo> registeredCapabilities = new ArrayList<>();

    @Inject
    @Named("joynr.messaging.channelId")
    String myChannelId;

    public static DummyCapabilitiesDirectory getInstance() {
        return instance;
    }

    @Override
    public Promise<DeferredVoid> add(DiscoveryEntry discoveryEntry) {
        DeferredVoid deferred = new DeferredVoid();
        registeredCapabilities.add(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true, discoveryEntry));
        notifyCapabilityAdded(discoveryEntry);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> add(DiscoveryEntry discoveryEntry, Boolean awaitGlobalRegistration) {
        // awaitGlobalRegistration is currently ignored in Java
        return add(discoveryEntry);
    }

    @Override
    public Promise<Add1Deferred> add(DiscoveryEntry discoveryEntry, Boolean awaitGlobalRegistration, String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    public Promise<AddToAllDeferred> addToAll(DiscoveryEntry discoveryEntry, Boolean awaitGlobalRegistration) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    public Promise<Lookup1Deferred> lookup(String[] domains,
                                           String interfaceName,
                                           joynr.types.DiscoveryQos discoveryQos) {
        final Lookup1Deferred deferred = new Lookup1Deferred();
        CapabilitiesCallback callback = new CapabilitiesCallback() {
            @Override
            public void processCapabilitiesReceived(Optional<Collection<DiscoveryEntryWithMetaInfo>> capabilities) {
                if (capabilities.isPresent()) {
                    deferred.resolve(capabilities.get().toArray(new DiscoveryEntryWithMetaInfo[0]));
                } else {
                    deferred.reject(new ProviderRuntimeException("Received capabilities list was null"));
                }
            }

            @Override
            public void onError(Throwable e) {
                deferred.reject(new ProviderRuntimeException(e.toString()));
            }

            @Override
            public void onError(DiscoveryError error) {
                deferred.reject(new ProviderRuntimeException(error.toString()));
            }
        };
        DiscoveryScope discoveryScope = DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope().name());
        lookup(domains,
               interfaceName,
               new DiscoveryQos(30000, ArbitrationStrategy.NotSet, discoveryQos.getCacheMaxAge(), discoveryScope),
               new String[0],
               callback);

        return new Promise<Lookup1Deferred>(deferred);
    }

    @Override
    public Promise<Lookup3Deferred> lookup(String participantId) {
        logger.info("!!!!!!!!!!!!!!!getCapabilitiesForParticipantId");
        Lookup3Deferred deferred = new Lookup3Deferred();

        for (DiscoveryEntryWithMetaInfo entry : registeredCapabilities) {
            if (entry.getParticipantId().equals(participantId)) {
                deferred.resolve(entry);
                break;
            }
        }

        return new Promise<Lookup3Deferred>(deferred);
    }

    @Override
    public Promise<DeferredVoid> remove(String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        logger.info("!!!!!!!!!!!!!!!removeCapabilities");
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public void remove(DiscoveryEntry interfaces) {
        logger.info("!!!!!!!!!!!!!!!removeCapabilities");

    }

    public void lookup(String[] domains,
                       String interfaceName,
                       DiscoveryQos discoveryQos,
                       String[] gbids,
                       CapabilitiesCallback capabilitiesCallback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilities async");
        ArrayList<DiscoveryEntryWithMetaInfo> foundCapabilities = new ArrayList<>();
        for (String domain : domains) {
            for (DiscoveryEntryWithMetaInfo ce : registeredCapabilities) {
                if (ce.getDomain().equals(domain) && ce.getInterfaceName().equals(interfaceName)) {
                    foundCapabilities.add(ce);
                }
            }
        }
        capabilitiesCallback.processCapabilitiesReceived(Optional.of(foundCapabilities));
    }

    @Override
    public Promise<Lookup2Deferred> lookup(String[] domains,
                                           String interfaceName,
                                           joynr.types.DiscoveryQos discoveryQos,
                                           String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    public void lookup(String participantId, DiscoveryQos discoveryQos, String[] gbids, CapabilityCallback callback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilitiesForParticipantId");
    }

    @Override
    public Promise<Lookup4Deferred> lookup(String participantId,
                                           joynr.types.DiscoveryQos discoveryQos,
                                           String[] gbids) {
        // TODO
        throw new ProviderRuntimeException("NOT IMPLEMENTED");
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        registeredCapabilities.clear();
    }

    @Override
    public Set<DiscoveryEntry> listLocalCapabilities() {
        return new HashSet<DiscoveryEntry>(registeredCapabilities);
    }

    @Override
    public void removeStaleProvidersOfClusterController() {
        logger.info("!!!!!!!!!!!!!!!remove stale providers of cluster controller");
    }
}

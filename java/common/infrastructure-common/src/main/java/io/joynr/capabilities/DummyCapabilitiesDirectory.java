package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import javax.annotation.CheckForNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.types.DiscoveryEntry;

public class DummyCapabilitiesDirectory extends AbstractLocalCapabilitiesDirectory {
    private static final Logger logger = LoggerFactory.getLogger(DummyCapabilitiesDirectory.class);
    private static final DummyCapabilitiesDirectory instance = new DummyCapabilitiesDirectory();
    private ArrayList<DiscoveryEntry> registeredCapabilities = Lists.newArrayList();

    @Inject
    @Named("joynr.messaging.channelId")
    String myChannelId;

    public static DummyCapabilitiesDirectory getInstance() {
        return instance;
    }

    @Override
    public Promise<DeferredVoid> add(DiscoveryEntry discoveryEntry) {
        DeferredVoid deferred = new DeferredVoid();
        registeredCapabilities.add(discoveryEntry);
        notifyCapabilityAdded(discoveryEntry);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Lookup1Deferred> lookup(@JoynrRpcParam("domain") String domain,
                                           @JoynrRpcParam("interfaceName") String interfaceName,
                                           @JoynrRpcParam("discoveryQos") joynr.types.DiscoveryQos discoveryQos) {
        final Lookup1Deferred deferred = new Lookup1Deferred();
        CapabilitiesCallback callback = new CapabilitiesCallback() {
            @Override
            public void processCapabilitiesReceived(@CheckForNull Collection<DiscoveryEntry> capabilities) {
                if (capabilities != null) {
                    deferred.resolve(capabilities.toArray(new DiscoveryEntry[0]));
                } else {
                    deferred.reject(new ProviderRuntimeException("Received capabilities list was null"));
                }
            }

            @Override
            public void onError(Throwable e) {
                deferred.reject(new ProviderRuntimeException(e.toString()));
            }
        };
        DiscoveryScope discoveryScope = DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope().name());
        lookup(domain, interfaceName, new DiscoveryQos(30000,
                                                       ArbitrationStrategy.NotSet,
                                                       discoveryQos.getCacheMaxAge(),
                                                       discoveryScope), callback);

        return new Promise<Lookup1Deferred>(deferred);
    }

    @Override
    public Promise<Lookup2Deferred> lookup(@JoynrRpcParam("participantId") String participantId) {
        Lookup2Deferred deferred = new Lookup2Deferred();
        DiscoveryEntry discoveryEntry = lookup(participantId, DiscoveryQos.NO_FILTER);
        deferred.resolve(discoveryEntry);
        return new Promise<Lookup2Deferred>(deferred);
    }

    @Override
    public Promise<DeferredVoid> remove(@JoynrRpcParam("participantId") String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        logger.info("!!!!!!!!!!!!!!!removeCapabilities");
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public void remove(DiscoveryEntry interfaces) {
        logger.info("!!!!!!!!!!!!!!!removeCapabilities");

    }

    @Override
    public void lookup(String domain,
                       String interfaceName,
                       DiscoveryQos discoveryQos,
                       CapabilitiesCallback capabilitiesCallback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilities async");
        ArrayList<DiscoveryEntry> foundCapabilities = Lists.newArrayList();
        for (DiscoveryEntry ce : registeredCapabilities) {
            if (ce.getDomain().equals(domain) && ce.getInterfaceName().equals(interfaceName)) {
                foundCapabilities.add(ce);
            }
        }
        capabilitiesCallback.processCapabilitiesReceived(foundCapabilities);
    }

    @Override
    @CheckForNull
    public void lookup(String participantId, DiscoveryQos discoveryQos, CapabilityCallback callback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilitiesForParticipantId");
    }

    @Override
    @CheckForNull
    public DiscoveryEntry lookup(String participantId, DiscoveryQos discoveryQos) {
        logger.info("!!!!!!!!!!!!!!!getCapabilitiesForParticipantId");
        DiscoveryEntry retrievedDiscoveryEntry = null;
        for (DiscoveryEntry entry : registeredCapabilities) {
            if (entry.getParticipantId().equals(participantId)) {
                retrievedDiscoveryEntry = entry;
                break;
            }
        }
        return retrievedDiscoveryEntry;
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        registeredCapabilities.clear();
    }

    @Override
    public Set<DiscoveryEntry> listLocalCapabilities() {
        return new HashSet<DiscoveryEntry>(registeredCapabilities);
    }
}

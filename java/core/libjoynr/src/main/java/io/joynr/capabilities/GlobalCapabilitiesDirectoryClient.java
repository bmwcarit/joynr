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

import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.GlobalDiscoveryEntry;

/**
 * A wrapper class around the {@link GlobalCapabilitiesDirectoryProxy}. Used inside the cluster
 * controller to communicate with the GCD for adding/removing/looking up global providers.
 *
 * In the current architecture with multiple GCDs (aka. "multiple backend") it is needed to extend
 * the GCD proxy calls with a custom header indicating which GCD (in which backend) should receive
 * the call. This information is processed by the AddressManager in order to select the right
 * address of the GCD for sending the message.
 */
public class GlobalCapabilitiesDirectoryClient {
    private static final long DEFAULT_TTL_ADD_AND_REMOVE = 60L * 1000L;
    private final String domain;
    private final DiscoveryQos discoveryQos;
    private final ProxyBuilderFactory proxyBuilderFactory;
    private GlobalCapabilitiesDirectoryProxy touchProxy;
    private GlobalCapabilitiesDirectoryProxy addAndRemoveProxy;
    private final String[] allGbids; // index 0 is the default backend

    @Inject
    @Named(MessagingPropertyKeys.CHANNELID)
    private String localChannelId;
    @Inject
    @Named(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS)
    private long freshnessUpdateIntervalMs;

    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS)
    private long ttlAddAndRemoveMs = DEFAULT_TTL_ADD_AND_REMOVE;

    @Inject
    public GlobalCapabilitiesDirectoryClient(ProxyBuilderFactory proxyBuilderFactory,
                                             @Named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY) GlobalDiscoveryEntry capabilitiesDirectoryEntry,
                                             @Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbidsArray) {
        this.proxyBuilderFactory = proxyBuilderFactory;
        this.domain = capabilitiesDirectoryEntry.getDomain();
        this.discoveryQos = new DiscoveryQos(30000,
                                             ArbitrationStrategy.HighestPriority,
                                             DiscoveryQos.NO_MAX_AGE,
                                             DiscoveryScope.GLOBAL_ONLY);
        this.allGbids = gbidsArray.clone();
    }

    private GlobalCapabilitiesDirectoryProxy getProxy(long ttl) {
        ProxyBuilder<GlobalCapabilitiesDirectoryProxy> capabilitiesProxyBuilder = proxyBuilderFactory.get(domain,
                                                                                                          GlobalCapabilitiesDirectoryProxy.class);
        MessagingQos messagingQos = new MessagingQos(ttl);
        return capabilitiesProxyBuilder.setDiscoveryQos(discoveryQos).setMessagingQos(messagingQos).build();
    }

    private GlobalCapabilitiesDirectoryProxy getAddAndRemoveProxy() {
        if (addAndRemoveProxy == null) {
            addAndRemoveProxy = getProxy(ttlAddAndRemoveMs);
        }
        return addAndRemoveProxy;
    }

    // add methods
    public void add(Callback<Void> callback, GlobalDiscoveryEntry globalDiscoveryEntry) {
        add(callback, globalDiscoveryEntry, allGbids[0]);
    }

    public void add(Callback<Void> callback, GlobalDiscoveryEntry globalDiscoveryEntry, String targetGbid) {
    }

    // remove methods
    public void remove(Callback<Void> callback, String participantId) {
        remove(callback, participantId, allGbids[0]);
    }

    public void remove(Callback<Void> callback, String participantId, String targetGbid) {
    }

    public void remove(Callback<Void> callback, List<String> participantIds) {
        remove(callback, participantIds, allGbids[0]);
    }

    public void remove(Callback<Void> callback, List<String> participantIds, String targetGbid) {
    }

    // lookup methods
    public void lookup(Callback<GlobalDiscoveryEntry> callback, String participantId, long ttl) {
        lookup(callback, participantId, ttl, allGbids[0]);
    }

    public void lookup(Callback<GlobalDiscoveryEntry> callback, String participantId, long ttl, String targetGbid) {
    }

    public void lookup(final Callback<List<GlobalDiscoveryEntry>> callback,
                       String[] domains,
                       String interfaceName,
                       long ttl) {
        getProxy(ttl).lookup(new Callback<GlobalDiscoveryEntry[]>() {
            @Override
            public void onFailure(JoynrRuntimeException error) {
                callback.onFailure(error);
            }

            @Override
            public void onSuccess(GlobalDiscoveryEntry[] result) {
                List<GlobalDiscoveryEntry> globalDiscoveryEntryList;

                if (result == null) {
                    globalDiscoveryEntryList = new ArrayList<GlobalDiscoveryEntry>();
                } else {
                    globalDiscoveryEntryList = Arrays.asList(result);
                }
                callback.onSuccess(globalDiscoveryEntryList);
            }

        }, domains, interfaceName);

    }

    // touch methods
    public void touch() {
        for (String gbid : allGbids) {
            touch(gbid);
        }
    }

    private void touch(String targetGbid) {
    }
}

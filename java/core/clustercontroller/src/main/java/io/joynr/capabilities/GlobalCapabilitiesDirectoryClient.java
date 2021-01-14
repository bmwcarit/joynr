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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.Message;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.DiscoveryError;
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
    private static final Logger logger = LoggerFactory.getLogger(GlobalCapabilitiesDirectoryClient.class);
    private final String domain;
    private final DiscoveryQos discoveryQos;
    private final ProxyBuilderFactory proxyBuilderFactory;

    private GlobalCapabilitiesDirectoryProxy gcdProxy;

    @Inject
    @Named(MessagingPropertyKeys.CHANNELID)
    private String localChannelId;
    @Inject
    @Named(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS)
    private long freshnessUpdateIntervalMs;

    @Inject
    public GlobalCapabilitiesDirectoryClient(ProxyBuilderFactory proxyBuilderFactory,
                                             @Named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY) GlobalDiscoveryEntry capabilitiesDirectoryEntry) {
        this.proxyBuilderFactory = proxyBuilderFactory;
        this.domain = capabilitiesDirectoryEntry.getDomain();
        this.discoveryQos = new DiscoveryQos(30000,
                                             ArbitrationStrategy.HighestPriority,
                                             DiscoveryQos.NO_MAX_AGE,
                                             DiscoveryScope.GLOBAL_ONLY);
    }

    private synchronized GlobalCapabilitiesDirectoryProxy getGcdProxy() {
        if (this.gcdProxy == null) {
            ProxyBuilder<GlobalCapabilitiesDirectoryProxy> capabilitiesProxyBuilder = proxyBuilderFactory.get(domain,
                                                                                                              GlobalCapabilitiesDirectoryProxy.class);
            gcdProxy = capabilitiesProxyBuilder.setDiscoveryQos(discoveryQos).build();
        }
        return this.gcdProxy;
    }

    // add methods
    public void add(CallbackWithModeledError<Void, DiscoveryError> callbackWithModeledError,
                    GlobalDiscoveryEntry globalDiscoveryEntry,
                    long ttlMs,
                    String[] gbids) {
        MessagingQos qosWithGbidCustomHeader = new MessagingQos(ttlMs);
        qosWithGbidCustomHeader.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, gbids[0]);
        getGcdProxy().add(callbackWithModeledError, globalDiscoveryEntry, gbids, qosWithGbidCustomHeader);
    }

    // remove methods
    public void remove(CallbackWithModeledError<Void, DiscoveryError> callback,
                       String participantId,
                       String[] targetGbids) {
        if (null == targetGbids || targetGbids.length == 0) {
            logger.warn("Remove called without any target GBIDs! Gbids: {}", (Object[]) targetGbids);
            throw new IllegalStateException("GCDClient.remove called without any target GBIDs!");
        }
        MessagingQos qosWithGbidCustomHeader = new MessagingQos();
        qosWithGbidCustomHeader.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbids[0]);
        getGcdProxy().remove(callback, participantId, targetGbids, qosWithGbidCustomHeader);
    }

    // lookup methods
    public void lookup(CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError> callback,
                       String participantId,
                       long ttl,
                       String[] targetGbids) {
        MessagingQos qosWithGbidCustomHeader = new MessagingQos(ttl);
        qosWithGbidCustomHeader.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbids[0]);
        getGcdProxy().lookup(callback, participantId, targetGbids, qosWithGbidCustomHeader);
    }

    public void lookup(final CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError> callback,
                       String[] domains,
                       String interfaceName,
                       long ttl,
                       String[] targetGbids) {
        MessagingQos qosWithGbidCustomHeader = new MessagingQos(ttl);
        qosWithGbidCustomHeader.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbids[0]);

        getGcdProxy().lookup(new CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError>() {
            @Override
            public void onFailure(JoynrRuntimeException error) {
                callback.onFailure(error);
            }

            @Override
            public void onFailure(DiscoveryError errorEnum) {
                callback.onFailure(errorEnum);
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

        }, domains, interfaceName, targetGbids, qosWithGbidCustomHeader);

    }

    public void touch(Callback<Void> callback, String[] participantIds, String targetGbid) {
        MessagingQos messagingQos = new MessagingQos(freshnessUpdateIntervalMs);
        messagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);
        getGcdProxy().touch(callback, localChannelId, participantIds, messagingQos);
    }

    // Remove stale providers (registered with the same cluster controller id in a previous lifecycle)
    public void removeStale(Callback<Void> callback, long maxLastSeenDateMs, String gbid) {
        long removeStaleTtl = 60 * 60 * 1000L;
        MessagingQos messagingQos = new MessagingQos(removeStaleTtl);
        messagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, gbid);
        getGcdProxy().removeStale(callback, localChannelId, maxLastSeenDateMs, messagingQos);
    }
}

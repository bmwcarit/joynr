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

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import javax.annotation.CheckForNull;
import javax.annotation.Nonnull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Function;
import com.google.common.collect.Collections2;
import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.TransportReadyListener;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.GlobalAddressProvider;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.RoutingTypes.Address;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

@Singleton
public class LocalCapabilitiesDirectoryImpl extends AbstractLocalCapabilitiesDirectory implements
        TransportReadyListener {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryImpl.class);

    private DiscoveryEntryStore localDiscoveryEntryStore;
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;
    private DiscoveryEntryStore globalDiscoveryEntryCache;
    private static final long DEFAULT_DISCOVERYTIMEOUT = 30000;

    private MessageRouter messageRouter;

    private GlobalAddressProvider globalAddressProvider;

    private ObjectMapper objectMapper;

    private Address globalAddress;
    private Object globalAddressLock = new Object();

    private List<QueuedDiscoveryEntry> queuedDiscoveryEntries = new ArrayList<QueuedDiscoveryEntry>();

    static class QueuedDiscoveryEntry {
        private DiscoveryEntry discoveryEntry;
        private DeferredVoid deferred;

        public QueuedDiscoveryEntry(DiscoveryEntry discoveryEntry, DeferredVoid deferred) {
            this.discoveryEntry = discoveryEntry;
            this.deferred = deferred;
        }

        public DiscoveryEntry getDiscoveryEntry() {
            return discoveryEntry;
        }

        public DeferredVoid getDeferred() {
            return deferred;
        }
    }

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LocalCapabilitiesDirectoryImpl(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabiltitiesDirectoryAddress,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID) String domainAccessControllerParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS) Address domainAccessControllerAddress,
                                          GlobalAddressProvider globalAddressProvider,
                                          DiscoveryEntryStore localDiscoveryEntryStore,
                                          DiscoveryEntryStore globalDiscoveryEntryCache,
                                          MessageRouter messageRouter,
                                          ProxyBuilderFactory proxyBuilderFactory,
                                          ObjectMapper objectMapper) {
        this.globalAddressProvider = globalAddressProvider;
        // CHECKSTYLE:ON
        this.messageRouter = messageRouter;
        this.localDiscoveryEntryStore = localDiscoveryEntryStore;
        this.globalDiscoveryEntryCache = globalDiscoveryEntryCache;
        this.objectMapper = objectMapper;
        this.globalDiscoveryEntryCache.add(CapabilityUtils.newGlobalDiscoveryEntry(new Version(),
                                                                                   discoveryDirectoriesDomain,
                                                                                   GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                                   capabilitiesDirectoryParticipantId,
                                                                                   new ProviderQos(),
                                                                                   System.currentTimeMillis(),
                                                                                   capabiltitiesDirectoryAddress));
        this.globalDiscoveryEntryCache.add(CapabilityUtils.newGlobalDiscoveryEntry(new Version(),
                                                                                   discoveryDirectoriesDomain,
                                                                                   GlobalDomainAccessController.INTERFACE_NAME,
                                                                                   domainAccessControllerParticipantId,
                                                                                   new ProviderQos(),
                                                                                   System.currentTimeMillis(),
                                                                                   domainAccessControllerAddress));

        globalCapabilitiesClient = new GlobalCapabilitiesDirectoryClient(proxyBuilderFactory,
                                                                         discoveryDirectoriesDomain);

        this.providerQos.setScope(ProviderScope.LOCAL);
    }

    /**
     * Adds local capability to local and (depending on SCOPE) the global directory
     */
    @Override
    public Promise<DeferredVoid> add(final DiscoveryEntry discoveryEntry) {
        final DeferredVoid deferred = new DeferredVoid();

        if (localDiscoveryEntryStore.hasDiscoveryEntry(discoveryEntry)) {
            DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_AND_GLOBAL, DiscoveryQos.NO_MAX_AGE);
            if (discoveryEntry.getQos().getScope().equals(ProviderScope.LOCAL)
                    || globalDiscoveryEntryCache.lookup(discoveryEntry.getParticipantId(), discoveryQos.getCacheMaxAgeMs()) != null) {
                // in this case, no further need for global registration is required. Registration completed.
                deferred.resolve();
                return new Promise<>(deferred);
            }
            // in the other case, the global registration needs to be done
        } else {
            localDiscoveryEntryStore.add(discoveryEntry);
            notifyCapabilityAdded(discoveryEntry);
        }

        if (discoveryEntry.getQos().getScope().equals(ProviderScope.GLOBAL)) {
            registerGlobal(discoveryEntry, deferred);
        } else {
            deferred.resolve();
        }
        return new Promise<>(deferred);
    }

    private void registerGlobal(final DiscoveryEntry discoveryEntry, final DeferredVoid deferred) {
        synchronized (globalAddressLock) {
            try {
                globalAddress = globalAddressProvider.get();
            } catch (Exception e) {
                globalAddress = null;
            }

            if (globalAddress == null) {
                queuedDiscoveryEntries.add(new QueuedDiscoveryEntry(discoveryEntry, deferred));
                globalAddressProvider.registerGlobalAddressesReadyListener(this);
                return;
            }
        }

        final GlobalDiscoveryEntry globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                              globalAddress);
        if (globalDiscoveryEntry != null) {

            logger.info("starting global registration for " + globalDiscoveryEntry.getDomain() + " : "
                    + globalDiscoveryEntry.getInterfaceName());

            globalCapabilitiesClient.add(new Callback<Void>() {

                @Override
                public void onSuccess(Void nothing) {
                    logger.info("global registration for " + globalDiscoveryEntry.getDomain() + " : "
                            + globalDiscoveryEntry.getInterfaceName() + " completed");
                    deferred.resolve();
                    globalDiscoveryEntryCache.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                      globalAddress));
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    deferred.reject(new ProviderRuntimeException(exception.toString()));
                }
            }, globalDiscoveryEntry);
        }
    }

    @Override
    public void remove(final DiscoveryEntry discoveryEntry) {
        localDiscoveryEntryStore.remove(discoveryEntry.getParticipantId());
        notifyCapabilityRemoved(discoveryEntry);

        // Remove from the global capabilities directory if needed
        if (discoveryEntry.getQos().getScope() != ProviderScope.LOCAL) {

            Callback<Void> callback = new Callback<Void>() {

                @Override
                public void onSuccess(Void result) {
                    globalDiscoveryEntryCache.remove(discoveryEntry.getParticipantId());
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    //do nothing
                }
            };
            globalCapabilitiesClient.remove(callback, Arrays.asList(discoveryEntry.getParticipantId()));
        }

        // Remove endpoint addresses
        messageRouter.removeNextHop(discoveryEntry.getParticipantId());
    }

    @Override
    public void lookup(final String domain,
                       final String interfaceName,
                       final DiscoveryQos discoveryQos,
                       final CapabilitiesCallback capabilitiesCallback) {

        Collection<DiscoveryEntry> localDiscoveryEntries = localDiscoveryEntryStore.lookup(domain, interfaceName);
        Collection<DiscoveryEntry> globalDiscoveryEntries = null;
        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        switch (discoveryScope) {
        case LOCAL_ONLY:
            capabilitiesCallback.processCapabilitiesReceived(localDiscoveryEntries);
            break;
        case LOCAL_THEN_GLOBAL:
            if (localDiscoveryEntries.size() > 0) {
                capabilitiesCallback.processCapabilitiesReceived(localDiscoveryEntries);
            } else {
                globalDiscoveryEntries = globalDiscoveryEntryCache.lookup(domain,
                                                                          interfaceName,
                                                                          discoveryQos.getCacheMaxAgeMs());
                if (globalDiscoveryEntries.size() > 0) {
                    capabilitiesCallback.processCapabilitiesReceived(globalDiscoveryEntries);
                } else {
                    asyncGetGlobalCapabilitities(domain,
                                                 interfaceName,
                                                 null,
                                                 discoveryQos.getDiscoveryTimeoutMs(),
                                                 capabilitiesCallback);
                }
            }
            break;
        case GLOBAL_ONLY:
            globalDiscoveryEntries = globalDiscoveryEntryCache.lookup(domain,
                                                                      interfaceName,
                                                                      discoveryQos.getCacheMaxAgeMs());
            if (globalDiscoveryEntries.size() > 0) {
                capabilitiesCallback.processCapabilitiesReceived(globalDiscoveryEntries);
            } else {
                // in this case, no global only caps are included in the cache --> call glob cap dir
                asyncGetGlobalCapabilitities(domain,
                                             interfaceName,
                                             null,
                                             discoveryQos.getDiscoveryTimeoutMs(),
                                             capabilitiesCallback);
            }
            break;
        case LOCAL_AND_GLOBAL:
            globalDiscoveryEntries = globalDiscoveryEntryCache.lookup(domain,
                                                                      interfaceName,
                                                                      discoveryQos.getCacheMaxAgeMs());
            if (globalDiscoveryEntries.size() > 0) {
                for (DiscoveryEntry discoveryEntry : localDiscoveryEntries) {
                    if (!globalDiscoveryEntries.contains(discoveryEntry)) {
                        globalDiscoveryEntries.add(discoveryEntry);
                    }
                }
                capabilitiesCallback.processCapabilitiesReceived(globalDiscoveryEntries);
            } else {
                // in this case, no global only caps are included in the cache --> call glob cap dir
                asyncGetGlobalCapabilitities(domain,
                                             interfaceName,
                                             localDiscoveryEntries,
                                             discoveryQos.getDiscoveryTimeoutMs(),
                                             capabilitiesCallback);
            }
            break;
        default:
            break;

        }
    }

    @Override
    @CheckForNull
    public void lookup(final String participantId,
                       final DiscoveryQos discoveryQos,
                       final CapabilityCallback capabilityCallback) {

        final DiscoveryEntry localDiscoveryEntry = localDiscoveryEntryStore.lookup(participantId,
                                                                                   discoveryQos.getCacheMaxAgeMs());

        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        switch (discoveryScope) {
        case LOCAL_ONLY:
            capabilityCallback.processCapabilityReceived(localDiscoveryEntry);
            break;
        case LOCAL_THEN_GLOBAL:
        case LOCAL_AND_GLOBAL:
            if (localDiscoveryEntry != null) {
                capabilityCallback.processCapabilityReceived(localDiscoveryEntry);
            } else {
                asyncGetGlobalCapabilitity(participantId, discoveryQos, capabilityCallback);
            }
            break;
        case GLOBAL_ONLY:
            asyncGetGlobalCapabilitity(participantId, discoveryQos, capabilityCallback);
            break;
        default:
            break;
        }
    }

    @Override
    @CheckForNull
    public DiscoveryEntry lookup(String participantId, DiscoveryQos discoveryQos) {
        final Future<DiscoveryEntry> lookupFuture = new Future<>();
        lookup(participantId, discoveryQos, new CapabilityCallback() {

            @Override
            public void processCapabilityReceived(DiscoveryEntry capability) {
                lookupFuture.onSuccess(capability);
            }

            @Override
            public void onError(Throwable e) {
                lookupFuture.onFailure(new JoynrRuntimeException(e));
            }
        });
        DiscoveryEntry retrievedCapabilitiyEntry = null;

        try {
            retrievedCapabilitiyEntry = lookupFuture.get();
        } catch (InterruptedException e1) {
            logger.error("interrupted while retrieving capability entry by participant ID", e1);
        } catch (ApplicationException e1) {
            // should not be reachable since ApplicationExceptions are not used internally
            logger.error("ApplicationException while retrieving capability entry by participant ID", e1);
        }
        return retrievedCapabilitiyEntry;
    }

    private void registerIncomingEndpoints(Collection<GlobalDiscoveryEntry> caps) {
        for (GlobalDiscoveryEntry ce : caps) {
            // TODO when are entries purged from the messagingEndpointDirectory?
            if (ce.getParticipantId() != null && ce.getAddress() != null) {
                Address address;
                try {
                    address = objectMapper.readValue(ce.getAddress(), Address.class);
                } catch (IOException e) {
                    throw new JoynrRuntimeException(e);
                }
                messageRouter.addNextHop(ce.getParticipantId(), address);
            }
        }
    }

    private void asyncGetGlobalCapabilitity(final String participantId,
                                            DiscoveryQos discoveryQos,
                                            final CapabilityCallback capabilitiesCallback) {

        DiscoveryEntry cachedGlobalCapability = globalDiscoveryEntryCache.lookup(participantId,
                                                                                 discoveryQos.getCacheMaxAgeMs());

        if (cachedGlobalCapability != null) {
            capabilitiesCallback.processCapabilityReceived(cachedGlobalCapability);
        } else {
            globalCapabilitiesClient.lookup(new Callback<GlobalDiscoveryEntry>() {

                @Override
                public void onSuccess(@CheckForNull GlobalDiscoveryEntry newGlobalDiscoveryEntry) {
                    if (newGlobalDiscoveryEntry != null) {
                        registerIncomingEndpoints(Lists.newArrayList(newGlobalDiscoveryEntry));
                        globalDiscoveryEntryCache.add(newGlobalDiscoveryEntry);
                        capabilitiesCallback.processCapabilityReceived(newGlobalDiscoveryEntry);
                    } else {
                        capabilitiesCallback.onError(new NullPointerException("Received capabilities are null"));
                    }
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    capabilitiesCallback.onError((Exception) exception);

                }
            }, participantId, discoveryQos.getDiscoveryTimeoutMs());
        }

    }

    /**
     * mixes in the localDiscoveryEntries to global capabilities found by participantId
     */
    private void asyncGetGlobalCapabilitities(final String domain,
                                              final String interfaceName,
                                              Collection<DiscoveryEntry> localDiscoveryEntries2,
                                              long discoveryTimeout,
                                              final CapabilitiesCallback capabilitiesCallback) {

        final Collection<DiscoveryEntry> localDiscoveryEntries = localDiscoveryEntries2 == null ? new LinkedList<DiscoveryEntry>()
                : localDiscoveryEntries2;

        globalCapabilitiesClient.lookup(new Callback<List<GlobalDiscoveryEntry>>() {

            @Override
            public void onSuccess(List<GlobalDiscoveryEntry> globalDiscoverEntries) {
                if (globalDiscoverEntries != null) {
                    registerIncomingEndpoints(globalDiscoverEntries);
                    globalDiscoveryEntryCache.add(globalDiscoverEntries);
                    Collection<DiscoveryEntry> allDisoveryEntries = new ArrayList<DiscoveryEntry>(globalDiscoverEntries.size()
                            + localDiscoveryEntries.size());
                    allDisoveryEntries.addAll(globalDiscoverEntries);
                    allDisoveryEntries.addAll(localDiscoveryEntries);
                    capabilitiesCallback.processCapabilitiesReceived(allDisoveryEntries);
                } else {
                    capabilitiesCallback.onError(new NullPointerException("Received capabilities are null"));
                }
            }

            @Override
            public void onFailure(JoynrRuntimeException exception) {
                capabilitiesCallback.onError((Exception) exception);
            }
        },
                                        domain,
                                        interfaceName,
                                        discoveryTimeout);
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        if (unregisterAllRegisteredCapabilities) {
            Set<DiscoveryEntry> allDiscoveryEntries = localDiscoveryEntryStore.getAllDiscoveryEntries();

            List<DiscoveryEntry> discoveryEntries = new ArrayList<>(allDiscoveryEntries.size());

            for (DiscoveryEntry capabilityEntry : allDiscoveryEntries) {
                if (capabilityEntry.getQos().getScope() == ProviderScope.GLOBAL) {
                    discoveryEntries.add(capabilityEntry);
                }
            }

            if (discoveryEntries.size() > 0) {
                try {
                    Function<? super DiscoveryEntry, String> transfomerFct = new Function<DiscoveryEntry, String>() {

                        @Override
                        public String apply(DiscoveryEntry input) {
                            return input != null ? input.getParticipantId() : null;
                        }
                    };
                    Callback<Void> callback = new Callback<Void>() {

                        @Override
                        public void onFailure(JoynrRuntimeException error) {
                        }

                        @Override
                        public void onSuccess(Void result) {
                        }

                    };
                    globalCapabilitiesClient.remove(callback, Lists.newArrayList(Collections2.transform(discoveryEntries,
                            transfomerFct)));
                } catch (DiscoveryException e) {
                }
            }
        }
    }

    @Override
    public Promise<Lookup1Deferred> lookup(@JoynrRpcParam("domain") String domain,
                                           @JoynrRpcParam("interfaceName") String interfaceName,
                                           @JoynrRpcParam("discoveryQos") joynr.types.DiscoveryQos discoveryQos) {
        final Lookup1Deferred deferred = new Lookup1Deferred();
        CapabilitiesCallback callback = new CapabilitiesCallback() {
            @Override
            public void processCapabilitiesReceived(@CheckForNull Collection<DiscoveryEntry> capabilities) {
                if (capabilities == null) {
                    deferred.reject(new ProviderRuntimeException("Received capablities collection was null"));
                } else {
                    deferred.resolve(capabilities.toArray(new DiscoveryEntry[0]));
                }
            }

            @Override
            public void onError(Throwable e) {
                deferred.reject(new ProviderRuntimeException(e.toString()));
            }
        };
        DiscoveryScope discoveryScope = DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope().name());
        lookup(domain, interfaceName, new DiscoveryQos(DEFAULT_DISCOVERYTIMEOUT,
                                                       ArbitrationStrategy.NotSet,
                                                       discoveryQos.getCacheMaxAge(),
                                                       discoveryScope), callback);

        return new Promise<>(deferred);
    }

    @Override
    public Promise<Lookup2Deferred> lookup(@JoynrRpcParam("participantId") String participantId) {
        Lookup2Deferred deferred = new Lookup2Deferred();
        DiscoveryEntry discoveryEntry = lookup(participantId, DiscoveryQos.NO_FILTER);
        deferred.resolve(discoveryEntry);
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> remove(@JoynrRpcParam("participantId") String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        DiscoveryEntry entryToRemove = localDiscoveryEntryStore.lookup(participantId, Long.MAX_VALUE);
        if (entryToRemove != null) {
            remove(entryToRemove);
            deferred.resolve();
        } else {
            deferred.reject(new ProviderRuntimeException("Failed to remove participantId: " + participantId));
        }
        return new Promise<>(deferred);
    }

    @Override
    public Set<DiscoveryEntry> listLocalCapabilities() {
        return localDiscoveryEntryStore.getAllDiscoveryEntries();
    }

    @Override
    public void transportReady(@Nonnull Address address) {
        synchronized (globalAddressLock) {
            globalAddress = address;
        }
        for (QueuedDiscoveryEntry queuedDiscoveryEntry : queuedDiscoveryEntries) {
            registerGlobal(queuedDiscoveryEntry.getDiscoveryEntry(), queuedDiscoveryEntry.getDeferred());
        }
    }
}

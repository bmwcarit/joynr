package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import javax.annotation.CheckForNull;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
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
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.ChannelUrlDirectory;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.types.CapabilityInformation;
import joynr.types.CommunicationMiddleware;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@Singleton
public class LocalCapabilitiesDirectoryImpl extends AbstractLocalCapabilitiesDirectory {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryImpl.class);

    private CapabilitiesStore localCapabilitiesStore;
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;
    private CapabilitiesStore globalCapabilitiesCache;
    private static final long DEFAULT_DISCOVERYTIMEOUT = 30000;

    private MessageRouter messageRouter;

    private Address globalAddress;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LocalCapabilitiesDirectoryImpl(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID) String channelUrlDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabiltitiesDirectoryChannelId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID) String domainAccessControllerParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID) String domainAccessControllerChannelId,
                                          @Named(ClusterControllerRuntimeModule.GLOBAL_ADDRESS) Address globalAddress,
                                          CapabilitiesStore localCapabilitiesStore,
                                          CapabilitiesCache globalCapabilitiesCache,
                                          MessageRouter messageRouter,
                                          ProxyBuilderFactory proxyBuilderFactory) {
        this.globalAddress = globalAddress;
        // CHECKSTYLE:ON
        this.messageRouter = messageRouter;
        this.localCapabilitiesStore = localCapabilitiesStore;
        this.globalCapabilitiesCache = globalCapabilitiesCache;
        this.globalCapabilitiesCache.add(CapabilityUtils.discoveryEntry2CapEntry(new DiscoveryEntry(discoveryDirectoriesDomain,
                                                                                                    GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                                                    capabilitiesDirectoryParticipantId,
                                                                                                    new ProviderQos(),
                                                                                                    new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR }),
                                                                                 new ChannelAddress(capabiltitiesDirectoryChannelId)));
        this.globalCapabilitiesCache.add(CapabilityUtils.discoveryEntry2CapEntry(new DiscoveryEntry(discoveryDirectoriesDomain,
                                                                                                    ChannelUrlDirectory.INTERFACE_NAME,
                                                                                                    channelUrlDirectoryParticipantId,
                                                                                                    new ProviderQos(),
                                                                                                    new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR }),
                                                                                 new ChannelAddress(channelUrlDirectoryChannelId)));
        this.globalCapabilitiesCache.add(CapabilityUtils.discoveryEntry2CapEntry(new DiscoveryEntry(discoveryDirectoriesDomain,
                                                                                                    GlobalDomainAccessController.INTERFACE_NAME,
                                                                                                    domainAccessControllerParticipantId,
                                                                                                    new ProviderQos(),
                                                                                                    new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR }),
                                                                                 new ChannelAddress(domainAccessControllerChannelId)));

        globalCapabilitiesClient = new GlobalCapabilitiesDirectoryClient(proxyBuilderFactory,
                                                                         discoveryDirectoriesDomain);

    }

    /**
     * Adds local capability to local and (depending on SCOPE) the global directory
     */
    @Override
    public Promise<DeferredVoid> add(final DiscoveryEntry discoveryEntry) {
        final DeferredVoid deferred = new DeferredVoid();

        if (localCapabilitiesStore.hasCapability(CapabilityUtils.discoveryEntry2CapEntry(discoveryEntry, globalAddress))) {
            DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_AND_GLOBAL, DiscoveryQos.NO_MAX_AGE);
            if (discoveryEntry.getQos().getScope().equals(ProviderScope.LOCAL)
                    || globalCapabilitiesCache.lookup(discoveryEntry.getParticipantId(), discoveryQos.getCacheMaxAge()) != null) {
                // in this case, no further need for global registration is required. Registration completed.
                deferred.resolve();
                return new Promise<>(deferred);
            }
            // in the other case, the global registration needs to be done
        } else {
            localCapabilitiesStore.add(CapabilityUtils.discoveryEntry2CapEntry(discoveryEntry, globalAddress));
            notifyCapabilityAdded(discoveryEntry);
        }

        // Register globally
        if (discoveryEntry.getQos().getScope().equals(ProviderScope.GLOBAL)) {

            final CapabilityInformation capabilityInformation = CapabilityUtils.discoveryEntry2Information(discoveryEntry,
                                                                                                           globalAddress);
            if (capabilityInformation != null) {

                logger.info("starting global registration for " + capabilityInformation.getDomain() + " : "
                        + capabilityInformation.getInterfaceName());

                globalCapabilitiesClient.add(new Callback<Void>() {

                    @Override
                    public void onSuccess(Void nothing) {
                        logger.info("global registration for " + capabilityInformation.getDomain() + " : "
                                + capabilityInformation.getInterfaceName() + " completed");
                        deferred.resolve();
                        globalCapabilitiesCache.add(CapabilityUtils.discoveryEntry2CapEntry(discoveryEntry,
                                                                                            globalAddress));
                    }

                    @Override
                    public void onFailure(JoynrRuntimeException exception) {
                        deferred.reject(new ProviderRuntimeException(exception.toString()));
                    }
                }, capabilityInformation);
            }
        } else {
            deferred.resolve();
        }
        return new Promise<>(deferred);
    }

    @Override
    public void remove(final DiscoveryEntry discoveryEntry) {
        localCapabilitiesStore.remove(discoveryEntry.getParticipantId());
        notifyCapabilityRemoved(discoveryEntry);

        // Remove from the global capabilities directory if needed
        if (discoveryEntry.getQos().getScope() != ProviderScope.LOCAL) {

            Callback<Void> callback = new Callback<Void>() {

                @Override
                public void onSuccess(Void result) {
                    globalCapabilitiesCache.remove(discoveryEntry.getParticipantId());
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    //do nothing
                }
            };
            globalCapabilitiesClient.remove(callback, Arrays.asList(discoveryEntry.getParticipantId()));
        }

        // Remove endpoint addresses
        for (CommunicationMiddleware communicationMiddleware : discoveryEntry.getConnections()) {
            if (communicationMiddleware == CommunicationMiddleware.JOYNR) {
                messageRouter.removeNextHop(discoveryEntry.getParticipantId());
                break;
            }
        }
    }

    @Override
    public void lookup(final String domain,
                       final String interfaceName,
                       final DiscoveryQos discoveryQos,
                       final CapabilitiesCallback capabilitiesCallback) {

        Collection<CapabilityEntry> localCapabilities = localCapabilitiesStore.lookup(domain, interfaceName);
        Collection<CapabilityEntry> globalCapabilities = null;
        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        switch (discoveryScope) {
        case LOCAL_ONLY:
            capabilitiesCallback.processCapabilitiesReceived(CapabilityUtils.capabilityEntries2DiscoveryEntries(localCapabilities));
            break;
        case LOCAL_THEN_GLOBAL:
            if (localCapabilities.size() > 0) {
                capabilitiesCallback.processCapabilitiesReceived(CapabilityUtils.capabilityEntries2DiscoveryEntries(localCapabilities));
            } else {
                globalCapabilities = globalCapabilitiesCache.lookup(domain,
                                                                    interfaceName,
                                                                    discoveryQos.getCacheMaxAge());
                if (globalCapabilities.size() > 0) {
                    capabilitiesCallback.processCapabilitiesReceived(CapabilityUtils.capabilityEntries2DiscoveryEntries(globalCapabilities));
                } else {
                    asyncGetGlobalCapabilitities(domain,
                                                 interfaceName,
                                                 null,
                                                 discoveryQos.getDiscoveryTimeout(),
                                                 capabilitiesCallback);
                }
            }
            break;
        case GLOBAL_ONLY:
            globalCapabilities = globalCapabilitiesCache.lookup(domain, interfaceName, discoveryQos.getCacheMaxAge());
            if (globalCapabilities.size() > 0) {
                capabilitiesCallback.processCapabilitiesReceived(CapabilityUtils.capabilityEntries2DiscoveryEntries(globalCapabilities));
            } else {
                // in this case, no global only caps are included in the cache --> call glob cap dir
                asyncGetGlobalCapabilitities(domain,
                                             interfaceName,
                                             null,
                                             discoveryQos.getDiscoveryTimeout(),
                                             capabilitiesCallback);
            }
            break;
        case LOCAL_AND_GLOBAL:
            globalCapabilities = globalCapabilitiesCache.lookup(domain, interfaceName, discoveryQos.getCacheMaxAge());
            if (globalCapabilities.size() > 0) {
                for (CapabilityEntry capabilityEntry : localCapabilities) {
                    if (!globalCapabilities.contains(capabilityEntry)) {
                        globalCapabilities.add(capabilityEntry);
                    }
                }
                capabilitiesCallback.processCapabilitiesReceived(CapabilityUtils.capabilityEntries2DiscoveryEntries(globalCapabilities));
            } else {
                // in this case, no global only caps are included in the cache --> call glob cap dir
                asyncGetGlobalCapabilitities(domain,
                                             interfaceName,
                                             localCapabilities,
                                             discoveryQos.getDiscoveryTimeout(),
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

        final CapabilityEntry localCapability = localCapabilitiesStore.lookup(participantId,
                                                                              discoveryQos.getCacheMaxAge());

        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        switch (discoveryScope) {
        case LOCAL_ONLY:
            capabilityCallback.processCapabilityReceived(CapabilityUtils.capabilityEntry2DiscoveryEntry(localCapability));
            break;
        case LOCAL_THEN_GLOBAL:
        case LOCAL_AND_GLOBAL:
            if (localCapability != null) {
                capabilityCallback.processCapabilityReceived(CapabilityUtils.capabilityEntry2DiscoveryEntry(localCapability));
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

    private void registerIncomingEndpoints(Collection<CapabilityInformation> caps) {
        for (CapabilityInformation ce : caps) {
            // TODO can a CapabilityEntry coming from the GlobalCapabilityDirectoy have more than one
            // EndpointAddress?
            // TODO when are entries purged from the messagingEndpointDirectory?
            if (ce.getParticipantId() != null && ce.getChannelId() != null) {
                messageRouter.addNextHop(ce.getParticipantId(), RoutingTypesUtil.fromAddressString(ce.getChannelId()));
            }
        }
    }

    private void asyncGetGlobalCapabilitity(final String participantId,
                                            DiscoveryQos discoveryQos,
                                            final CapabilityCallback capabilitiesCallback) {

        CapabilityEntry cachedGlobalCapability = globalCapabilitiesCache.lookup(participantId,
                                                                                discoveryQos.getCacheMaxAge());

        if (cachedGlobalCapability != null) {
            capabilitiesCallback.processCapabilityReceived(CapabilityUtils.capabilityEntry2DiscoveryEntry(cachedGlobalCapability));
        } else {
            globalCapabilitiesClient.lookup(new Callback<CapabilityInformation>() {

                @Override
                public void onSuccess(@CheckForNull CapabilityInformation capInfo) {
                    if (capInfo != null) {
                        DiscoveryEntry discoveryEntry = CapabilityUtils.capabilitiesInfo2DiscoveryEntry(capInfo);
                        registerIncomingEndpoints(Lists.newArrayList(capInfo));
                        globalCapabilitiesCache.add(CapabilityUtils.discoveryEntry2CapEntry(discoveryEntry,
                                                                                            capInfo.getChannelId()));
                        capabilitiesCallback.processCapabilityReceived(discoveryEntry);
                    } else {
                        capabilitiesCallback.onError(new NullPointerException("Received capabilities are null"));
                    }
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    capabilitiesCallback.onError((Exception) exception);

                }
            }, participantId, discoveryQos.getDiscoveryTimeout());
        }

    }

    /**
     * mixes in the localCapabilities to global capabilities found by participantId
     */
    private void asyncGetGlobalCapabilitities(final String domain,
                                              final String interfaceName,
                                              Collection<CapabilityEntry> mixinCapabilities,
                                              long discoveryTimeout,
                                              final CapabilitiesCallback capabilitiesCallback) {

        final Collection<CapabilityEntry> localCapabilities = mixinCapabilities == null ? new LinkedList<CapabilityEntry>()
                : mixinCapabilities;

        globalCapabilitiesClient.lookup(new Callback<List<CapabilityInformation>>() {

            @Override
            public void onSuccess(List<CapabilityInformation> capInfo) {
                if (capInfo != null) {
                    Collection<CapabilityEntry> caps = CapabilityUtils.capabilityInformationList2Entries(capInfo);

                    registerIncomingEndpoints(capInfo);
                    globalCapabilitiesCache.add(caps);

                    caps.addAll(localCapabilities);
                    capabilitiesCallback.processCapabilitiesReceived(CapabilityUtils.capabilityEntries2DiscoveryEntries(caps));
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
            Set<CapabilityEntry> allCapabilities = localCapabilitiesStore.getAllCapabilities();

            List<CapabilityInformation> capInfoList = new ArrayList<>(allCapabilities.size());

            for (CapabilityEntry capabilityEntry : allCapabilities) {
                if (capabilityEntry.getProviderQos().getScope() == ProviderScope.GLOBAL) {
                    CapabilityInformation capabilityInformation = CapabilityUtils.capabilityEntry2Information(capabilityEntry);
                    capInfoList.add(capabilityInformation);
                }
            }

            if (capInfoList.size() > 0) {
                try {
                    Function<? super CapabilityInformation, String> transfomerFct = new Function<CapabilityInformation, String>() {

                        @Override
                        public String apply(CapabilityInformation input) {
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
                    globalCapabilitiesClient.remove(callback, Lists.newArrayList(Collections2.transform(capInfoList,
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
        CapabilityEntry entryToRemove = localCapabilitiesStore.lookup(participantId, Long.MAX_VALUE);
        if (entryToRemove != null) {
            remove(CapabilityUtils.capabilityEntry2DiscoveryEntry(entryToRemove));
            deferred.resolve();
        } else {
            deferred.reject(new ProviderRuntimeException("Failed to remove participantId: " + participantId));
        }
        return new Promise<>(deferred);
    }
}

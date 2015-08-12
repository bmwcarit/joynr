package io.joynr.capabilities;

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
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.dispatcher.MessagingEndpointDirectory;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyInvocationHandlerFactory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import javax.annotation.CheckForNull;

import joynr.infrastructure.ChannelUrlDirectory;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Function;
import com.google.common.collect.Collections2;
import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class LocalCapabilitiesDirectoryImpl extends AbstractLocalCapabilitiesDirectory {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryImpl.class);

    private MessagingEndpointDirectory messagingEndpointDirectory;
    private String localChannelId;
    private CapabilitiesStore localCapabilitiesStore;
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;
    private CapabilitiesStore globalCapabilitiesCache;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LocalCapabilitiesDirectoryImpl(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID) String channelUrlDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabiltitiesDirectoryChannelId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID) String domainAccessControllerParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID) String domainAccessControllerChannelId,
                                          @Named(MessagingPropertyKeys.CHANNELID) String localChannelId,
                                          MessagingEndpointDirectory messagingEndpointDirectory,
                                          CapabilitiesStore localCapabilitiesStore,
                                          CapabilitiesCache globalCapabilitiesCache,
                                          ProxyInvocationHandlerFactory proxyInvocationHandlerFactory) {
        // CHECKSTYLE:ON
        this.localChannelId = localChannelId;
        this.messagingEndpointDirectory = messagingEndpointDirectory;
        this.localCapabilitiesStore = localCapabilitiesStore;
        this.globalCapabilitiesCache = globalCapabilitiesCache;
        this.globalCapabilitiesCache.add(new CapabilityEntryImpl(discoveryDirectoriesDomain,
                                                                 GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                 new ProviderQos(),
                                                                 capabilitiesDirectoryParticipantId,
                                                                 System.currentTimeMillis(),
                                                                 new JoynrMessagingEndpointAddress(capabiltitiesDirectoryChannelId)));

        this.globalCapabilitiesCache.add(new CapabilityEntryImpl(discoveryDirectoriesDomain,
                                                                 ChannelUrlDirectory.INTERFACE_NAME,
                                                                 new ProviderQos(),
                                                                 channelUrlDirectoryParticipantId,
                                                                 System.currentTimeMillis(),
                                                                 new JoynrMessagingEndpointAddress(channelUrlDirectoryChannelId)));

        this.globalCapabilitiesCache.add(new CapabilityEntryImpl(discoveryDirectoriesDomain,
                                                                 GlobalDomainAccessController.INTERFACE_NAME,
                                                                 new ProviderQos(),
                                                                 domainAccessControllerParticipantId,
                                                                 System.currentTimeMillis(),
                                                                 new JoynrMessagingEndpointAddress(domainAccessControllerChannelId)));

        globalCapabilitiesClient = new GlobalCapabilitiesDirectoryClient(discoveryDirectoriesDomain,
                                                                         this,
                                                                         proxyInvocationHandlerFactory);
    }

    /**
     * Adds local capability to local and (depending on SCOPE) the global directory
     */
    @Override
    public RegistrationFuture add(final CapabilityEntry capabilityEntry) {
        JoynrMessagingEndpointAddress joynrMessagingEndpointAddress = new JoynrMessagingEndpointAddress(localChannelId);
        capabilityEntry.addEndpoint(joynrMessagingEndpointAddress);

        messagingEndpointDirectory.put(capabilityEntry.getParticipantId(), joynrMessagingEndpointAddress);

        final RegistrationFuture ret = new RegistrationFuture(capabilityEntry.getParticipantId());

        if (localCapabilitiesStore.hasCapability(capabilityEntry)) {
            DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.LOCAL_AND_GLOBAL, DiscoveryQos.NO_MAX_AGE);
            if (capabilityEntry.getProviderQos().getScope().equals(ProviderScope.LOCAL)
                    || globalCapabilitiesCache.lookup(capabilityEntry.getParticipantId(), discoveryQos.getCacheMaxAge()) != null) {
                // in this case, no further need for global registration is required. Registration completed.
                ret.setStatus(RegistrationStatus.DONE);
                return ret;
            }
            // in the other case, the global registration needs to be done
        } else {
            localCapabilitiesStore.add(capabilityEntry);
            notifyCapabilityAdded(capabilityEntry);
        }

        // Register globally
        if (capabilityEntry.getProviderQos().getScope().equals(ProviderScope.GLOBAL)) {

            ret.setStatus(RegistrationStatus.REGISTERING_GLOBALLY);

            final CapabilityInformation capabilityInformation = capabilityEntry2Information(capabilityEntry);
            if (capabilityInformation != null) {

                logger.info("starting global registration for " + capabilityInformation.getDomain() + " : "
                        + capabilityInformation.getInterfaceName());

                globalCapabilitiesClient.add(new Callback<Void>() {

                    @Override
                    public void onSuccess(Void nothing) {
                        logger.info("global registration for " + capabilityInformation.getDomain() + " : "
                                + capabilityInformation.getInterfaceName() + " completed");
                        ret.setStatus(RegistrationStatus.DONE);
                        globalCapabilitiesCache.add(capabilityEntry);
                    }

                    @Override
                    public void onFailure(JoynrRuntimeException exception) {
                        ret.setStatus(RegistrationStatus.ERROR);

                    }
                }, capabilityInformation);
            }
        } else {
            ret.setStatus(RegistrationStatus.DONE);
        }
        return ret;
    }

    @Override
    public void remove(final CapabilityEntry capEntry) {
        localCapabilitiesStore.remove(capEntry.getParticipantId());
        notifyCapabilityRemoved(capEntry);

        // Remove from the global capabilities directory if needed
        if (capEntry.getProviderQos().getScope() != ProviderScope.LOCAL) {

            // Currently Endpoint address needs to be added for remove on server side to work correctly.
            // TODO Modify removeCapability in CapDirImpl to accept any endpoint address list
            JoynrMessagingEndpointAddress joynrMessagingEndpointAddress = new JoynrMessagingEndpointAddress(localChannelId);
            capEntry.addEndpoint(joynrMessagingEndpointAddress);

            CapabilityInformation capabilityInformation = capabilityEntry2Information(capEntry);
            if (capabilityInformation != null) {
                Callback<Void> callback = new Callback<Void>() {

                    @Override
                    public void onSuccess(Void result) {
                        globalCapabilitiesCache.remove(capEntry.getParticipantId());
                    }

                    @Override
                    public void onFailure(JoynrRuntimeException error) {
                        //do nothing
                    }
                };
                globalCapabilitiesClient.remove(callback, Arrays.asList(capabilityInformation.getParticipantId()));
            }
        }

        // Remove endpoint addresses
        for (EndpointAddressBase ep : capEntry.getEndpointAddresses()) {
            if (ep instanceof JoynrMessagingEndpointAddress) {
                messagingEndpointDirectory.remove(capEntry.getParticipantId());
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
            capabilitiesCallback.processCapabilitiesReceived(localCapabilities);
            break;
        case LOCAL_THEN_GLOBAL:
            if (localCapabilities.size() > 0) {
                capabilitiesCallback.processCapabilitiesReceived(localCapabilities);
            } else {
                globalCapabilities = globalCapabilitiesCache.lookup(domain,
                                                                    interfaceName,
                                                                    discoveryQos.getCacheMaxAge());
                if (globalCapabilities.size() > 0) {
                    capabilitiesCallback.processCapabilitiesReceived(globalCapabilities);
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
                capabilitiesCallback.processCapabilitiesReceived(globalCapabilities);
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
                capabilitiesCallback.processCapabilitiesReceived(globalCapabilities);
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
            capabilityCallback.processCapabilityReceived(localCapability);
            break;
        case LOCAL_THEN_GLOBAL:
        case LOCAL_AND_GLOBAL:
            if (localCapability != null) {
                capabilityCallback.processCapabilityReceived(localCapability);
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
    public CapabilityEntry lookup(String participantId, DiscoveryQos discoveryQos) {
        final Future<CapabilityEntry> lookupFuture = new Future<CapabilityEntry>();
        lookup(participantId, discoveryQos, new CapabilityCallback() {

            @Override
            public void processCapabilityReceived(CapabilityEntry capability) {
                lookupFuture.onSuccess(capability);
            }

            @Override
            public void onError(Throwable e) {
                lookupFuture.onFailure(new JoynrRuntimeException(e));
            }
        });
        CapabilityEntry retrievedCapabilitiyEntry = null;

        try {
            retrievedCapabilitiyEntry = lookupFuture.getReply();
        } catch (InterruptedException e1) {
            logger.error("interrupted while retrieving capability entry by participant ID", e1);
        }
        return retrievedCapabilitiyEntry;
    }

    private void registerIncomingEndpoints(Collection<CapabilityEntry> caps) {
        for (CapabilityEntry ce : caps) {
            // TODO can a CapabilityEntry coming from the GlobalCapabilityDirectoy have more than one
            // EndpointAddress?
            // TODO when are entries purged from the messagingEndpointDirectory?
            if (ce.getParticipantId() != null && ce.getEndpointAddresses().size() > 0) {
                messagingEndpointDirectory.put(ce.getParticipantId(), ce.getEndpointAddresses().get(0));
            }
        }
    }

    private void asyncGetGlobalCapabilitity(final String participantId,
                                            DiscoveryQos discoveryQos,
                                            final CapabilityCallback capabilitiesCallback) {

        CapabilityEntry cachedGlobalCapability = globalCapabilitiesCache.lookup(participantId,
                                                                                discoveryQos.getCacheMaxAge());

        if (cachedGlobalCapability != null) {
            capabilitiesCallback.processCapabilityReceived(cachedGlobalCapability);
        } else {
            globalCapabilitiesClient.lookup(new Callback<CapabilityInformation>() {

                @Override
                public void onSuccess(CapabilityInformation capInfo) {
                    CapabilityEntry capEntry = new CapabilityEntryImpl(capInfo);
                    registerIncomingEndpoints(Lists.newArrayList(capEntry));
                    globalCapabilitiesCache.add(capEntry);

                    capabilitiesCallback.processCapabilityReceived(capEntry);
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    capabilitiesCallback.onError(exception);

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
                Collection<CapabilityEntry> caps = capabilityInformationList2Entries(capInfo);

                registerIncomingEndpoints(caps);
                globalCapabilitiesCache.add(caps);

                caps.addAll(localCapabilities);
                capabilitiesCallback.processCapabilitiesReceived(caps);
            }

            @Override
            public void onFailure(JoynrRuntimeException exception) {
                capabilitiesCallback.onError(exception);
            }
        }, domain, interfaceName, discoveryTimeout);
    }

    @CheckForNull
    private CapabilityInformation capabilityEntry2Information(CapabilityEntry capabilityEntry) {
        return capabilityEntry.toCapabilityInformation();
    }

    private Collection<CapabilityEntry> capabilityInformationList2Entries(List<CapabilityInformation> capInfoList) {
        Collection<CapabilityEntry> capEntryCollection = Lists.newArrayList();
        for (CapabilityInformation capInfo : capInfoList) {
            capEntryCollection.add(new CapabilityEntryImpl(capInfo));
        }
        return capEntryCollection;
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        if (unregisterAllRegisteredCapabilities) {
            Set<CapabilityEntry> allCapabilities = localCapabilitiesStore.getAllCapabilities();

            List<CapabilityInformation> capInfoList = new ArrayList<CapabilityInformation>(allCapabilities.size());

            for (CapabilityEntry capabilityEntry : allCapabilities) {
                if (capabilityEntry.getProviderQos().getScope() == ProviderScope.GLOBAL) {
                    CapabilityInformation capabilityInformation = capabilityEntry2Information(capabilityEntry);
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
                } catch (JoynrArbitrationException e) {
                    return;
                }
            }
        }
    }
}

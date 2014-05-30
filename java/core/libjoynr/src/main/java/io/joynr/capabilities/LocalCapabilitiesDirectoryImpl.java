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
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;

import javax.annotation.CheckForNull;

import joynr.infrastructure.ChannelUrlDirectory;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
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
public class LocalCapabilitiesDirectoryImpl implements LocalCapabilitiesDirectory {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryImpl.class);

    CapabilitiesStore localCapabilitiesStore;

    @Inject
    GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;

    @Inject
    MessagingEndpointDirectory messagingEndpointDirectory;

    @Inject
    @Named(MessagingPropertyKeys.CHANNELID)
    String localChannelId;

    private CapabilitiesStore globalCapabilitiesCache;

    public LocalCapabilitiesDirectoryImpl(CapabilitiesStore localCapabilitiesStore,
                                          CapabilitiesStore globalCapabilitiesCache) {
        this.localCapabilitiesStore = localCapabilitiesStore;
        this.globalCapabilitiesCache = globalCapabilitiesCache;
    }

    @Inject
    public LocalCapabilitiesDirectoryImpl(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID) String channelUrlDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabiltitiesDirectoryChannelId,
                                          CapabilitiesStore localCapabilitiesStore,
                                          CapabilitiesStore globalCapabilitiesCache) {

        this(localCapabilitiesStore, globalCapabilitiesCache);
        this.globalCapabilitiesCache.add(new CapabilityEntry(discoveryDirectoriesDomain,
                                                             GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                             new ProviderQos(),
                                                             new JoynrMessagingEndpointAddress(capabiltitiesDirectoryChannelId),
                                                             capabilitiesDirectoryParticipantId));

        this.globalCapabilitiesCache.add(new CapabilityEntry(discoveryDirectoriesDomain,
                                                             ChannelUrlDirectory.INTERFACE_NAME,
                                                             new ProviderQos(),
                                                             new JoynrMessagingEndpointAddress(channelUrlDirectoryChannelId),
                                                             channelUrlDirectoryParticipantId));
    }

    /**
     * Adds capability to local and global directories,
     * 
     * @return
     */
    @Override
    public RegistrationFuture add(CapabilityEntry capabilityEntry) {
        JoynrMessagingEndpointAddress joynrMessagingEndpointAddress = new JoynrMessagingEndpointAddress(localChannelId);
        capabilityEntry.endpointAddresses.add(joynrMessagingEndpointAddress);

        messagingEndpointDirectory.put(capabilityEntry.getParticipantId(), joynrMessagingEndpointAddress);

        final RegistrationFuture ret = new RegistrationFuture(capabilityEntry.getParticipantId());

        if (localCapabilitiesStore.hasCapability(capabilityEntry)) {
            ret.setStatus(RegistrationStatus.DONE);
            return ret;
        }

        localCapabilitiesStore.add(capabilityEntry);

        // Register globally
        if (capabilityEntry.providerQos.getScope().equals(ProviderScope.GLOBAL)) {

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

                    }

                    @Override
                    public void onFailure(JoynrException exception) {
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
    public void remove(CapabilityEntry capEntry) {
        localCapabilitiesStore.remove(capEntry.getParticipantId());

        // Remove from the global capabilities directory if needed
        if (capEntry.getProviderQos().getScope() != ProviderScope.LOCAL) {

            // Currently Endpoint address needs to be added for remove on server side to work correctly.
            // TODO Modify removeCapability in CapDirImpl to accept any endpoint address list
            JoynrMessagingEndpointAddress joynrMessagingEndpointAddress = new JoynrMessagingEndpointAddress(localChannelId);
            capEntry.endpointAddresses.add(joynrMessagingEndpointAddress);

            CapabilityInformation capabilityInformation = capabilityEntry2Information(capEntry);
            if (capabilityInformation != null) {
                globalCapabilitiesClient.remove(capabilityInformation.getParticipantId());
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
                    asyncGetGlobalCapabilitities(domain, interfaceName, null, capabilitiesCallback);
                }
            }
            break;
        case GLOBAL_ONLY:
            globalCapabilities = globalCapabilitiesCache.lookup(domain, interfaceName, discoveryQos.getCacheMaxAge());
            if (globalCapabilities.size() > 0) {
                capabilitiesCallback.processCapabilitiesReceived(globalCapabilities);
            } else {
                // in this case, no global only caps are included in the cache --> call glob cap dir
                asyncGetGlobalCapabilitities(domain, interfaceName, null, capabilitiesCallback);
            }
            break;
        case LOCAL_AND_GLOBAL:
            globalCapabilities = globalCapabilitiesCache.lookup(domain, interfaceName, discoveryQos.getCacheMaxAge());
            if (globalCapabilities.size() > 0) {
                globalCapabilities.addAll(localCapabilities);
                capabilitiesCallback.processCapabilitiesReceived(globalCapabilities);
            } else {
                // in this case, no global only caps are included in the cache --> call glob cap dir
                asyncGetGlobalCapabilitities(domain, interfaceName, localCapabilities, capabilitiesCallback);
            }
            break;
        default:
            break;

        }
    }

    @Override
    public void lookup(final String participantId,
                       final DiscoveryQos discoveryQos,
                       final CapabilityCallback capabilityCallback) {

        final CapabilityEntry localCapability = localCapabilitiesStore.lookup(participantId, discoveryQos);

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
                asyncGetGlobalCapabilitity(participantId, capabilityCallback);
            }
            break;
        case GLOBAL_ONLY:
            asyncGetGlobalCapabilitity(participantId, capabilityCallback);
            break;
        default:
            break;
        }
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

    /**
     * mixes in the localCapabilities to global capabilities found by domain/interface
     * 
     * @param localCapabilities
     * @param capabilitiesCallback
     * @param participantId
     */
    private void asyncGetGlobalCapabilitity(final String participantId, final CapabilityCallback capabilitiesCallback) {

        globalCapabilitiesClient.lookup(new Callback<CapabilityInformation>() {

            @Override
            public void onSuccess(CapabilityInformation capInfo) {
                CapabilityEntry capEntry = CapabilityEntry.fromCapabilityInformation(capInfo);
                registerIncomingEndpoints(Lists.newArrayList(capEntry));
                localCapabilitiesStore.add(capEntry);

                capabilitiesCallback.processCapabilityReceived(capEntry);
            }

            @Override
            public void onFailure(JoynrException exception) {
                capabilitiesCallback.onError(exception);

            }
        }, participantId);
    }

    /**
     * mixes in the localCapabilities to global capabilities found by participantId
     * 
     * @param localCapabilities
     * @param capabilitiesCallback
     * @param participantId
     */
    private void asyncGetGlobalCapabilitities(final String domain,
                                              final String interfaceName,
                                              Collection<CapabilityEntry> mixinCapabilities,
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
            public void onFailure(JoynrException exception) {
                capabilitiesCallback.onError(exception);
            }
        }, domain, interfaceName);
    }

    @CheckForNull
    private CapabilityInformation capabilityEntry2Information(CapabilityEntry capabilityEntry) {
        return capabilityEntry.toCapabilityInformation();
    }

    private Collection<CapabilityEntry> capabilityInformationList2Entries(List<CapabilityInformation> capInfoList) {
        Collection<CapabilityEntry> capEntryCollection = Lists.newArrayList();
        for (CapabilityInformation capInfo : capInfoList) {
            capEntryCollection.add(CapabilityEntry.fromCapabilityInformation(capInfo));
        }
        return capEntryCollection;
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        if (unregisterAllRegisteredCapabilities) {
            HashSet<CapabilityEntry> allCapabilities = localCapabilitiesStore.getAllCapabilities();

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
                    globalCapabilitiesClient.remove(Lists.newArrayList(Collections2.transform(capInfoList,
                                                                                              transfomerFct)));
                } catch (JoynrArbitrationException e) {
                    return;
                }
            }
        }
    }
}

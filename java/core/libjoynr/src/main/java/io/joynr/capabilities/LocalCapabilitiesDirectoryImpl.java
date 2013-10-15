package io.joynr.capabilities;

/*
 * #%L
 * joynr::java::core::libjoynr
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static io.joynr.arbitration.DiscoveryScope.GLOBAL_ONLY;
import static io.joynr.arbitration.DiscoveryScope.LOCAL_AND_GLOBAL;
import static io.joynr.arbitration.DiscoveryScope.LOCAL_THEN_GLOBAL;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.dispatcher.MessagingEndpointDirectory;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadFactory;

import javax.annotation.CheckForNull;

import joynr.infrastructure.ChannelUrlDirectory;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;
import joynr.types.ProviderQosRequirements;
import joynr.types.ProviderScope;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class LocalCapabilitiesDirectoryImpl implements LocalCapabilitiesDirectory {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryImpl.class);

    @Inject
    CapabilitiesStore localCapabilitiesStore;

    @Inject
    GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;

    @Inject
    MessagingEndpointDirectory messagingEndpointDirectory;

    @Inject
    @Named(MessagingPropertyKeys.CHANNELID)
    String localChannelId;

    private ExecutorService executionQueue;

    // @Inject
    // @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID)
    // String channelUrlDirectoryChannelId;

    // @Inject
    // @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID)
    // String channelUrlDirectoryParticipantId;

    // @Inject
    // @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_DOMAIN)
    // String channelUrlDirectoryDomain;

    @Inject
    public LocalCapabilitiesDirectoryImpl(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryirectoriesDomain,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID) String channelUrlDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabiltitiesDirectoryChannelId,
                                          @Named(MessagingPropertyKeys.JOYNR_PROPERTIES) Properties joynrProperties,
                                          CapabilitiesStore localCapabilitiesStore) {

        this.localCapabilitiesStore = localCapabilitiesStore;
        this.localCapabilitiesStore.registerCapability(new CapabilityEntry(discoveryirectoriesDomain,
                                                                           GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                           new ProviderQos(),
                                                                           new JoynrMessagingEndpointAddress(capabiltitiesDirectoryChannelId),
                                                                           capabilitiesDirectoryParticipantId,
                                                                           CapabilityScope.REMOTE));

        this.localCapabilitiesStore.registerCapability(new CapabilityEntry(discoveryirectoriesDomain,
                                                                           ChannelUrlDirectory.INTERFACE_NAME,
                                                                           new ProviderQos(),
                                                                           new JoynrMessagingEndpointAddress(channelUrlDirectoryChannelId),
                                                                           channelUrlDirectoryParticipantId,
                                                                           CapabilityScope.REMOTE));

        ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("CapabilitiesDirectoryLookup-"
                + System.currentTimeMillis() + "%d").build();
        executionQueue = Executors.newCachedThreadPool(namedThreadFactory);
    }

    // public void init() {
    // Object stubedCapabilitiesDirectory = JoynMessagingConnectorFactory.create(CapabilitiesDirectory.class,
    // this.dispatcher,
    // this.joynrMessageSender,
    // clientParticipantId,
    // CapabilitiesDirectory.CAPABILITIES_DIRECTORY_PARTICIPANTID,
    // qosSettings);
    //
    // }

    /**
     * Adds capability to local and global directories,
     * 
     * @return
     */
    @Override
    public RegistrationFuture addCapability(CapabilityEntry capabilityEntry) {
        JoynrMessagingEndpointAddress joynrMessagingEndpointAddress = new JoynrMessagingEndpointAddress(localChannelId);
        capabilityEntry.endpointAddresses.add(joynrMessagingEndpointAddress);

        // TODO replace this with inprocess address as soon as inprocess communication is available
        messagingEndpointDirectory.put(capabilityEntry.getParticipantId(), joynrMessagingEndpointAddress);

        return addCapabilityAsync(capabilityEntry);
    }

    // Registers capabilities asynchronously
    private RegistrationFuture addCapabilityAsync(final CapabilityEntry capabilityEntry) {

        final RegistrationFuture ret = new RegistrationFuture(capabilityEntry.getParticipantId());
        try {
            executionQueue.submit(new Runnable() {

                @Override
                public void run() {
                    // Register locally
                    if (localCapabilitiesStore.hasCapability(capabilityEntry)) {
                        ret.setStatus(RegistrationStatus.DONE);
                        return;
                    }
                    localCapabilitiesStore.registerCapability(capabilityEntry);

                    // Register globally
                    if (!capabilityEntry.providerQos.getScope().equals(ProviderScope.LOCAL)) {
                        ret.setStatus(RegistrationStatus.REGISTERING_GLOBALLY);
                        try {
                            CapabilityInformation capabilityInformation = capabilityEntry2Information(capabilityEntry);
                            if (capabilityInformation != null) {
                                logger.info("starting global registration for " + capabilityInformation.getDomain()
                                        + " : " + capabilityInformation.getInterfaceName());
                                globalCapabilitiesClient.registerCapability(capabilityInformation);
                                logger.info("global registration for " + capabilityInformation.getDomain() + " : "
                                        + capabilityInformation.getInterfaceName() + " completed");
                            }
                        } catch (JoynrArbitrationException e) {
                            ret.setStatus(RegistrationStatus.ERROR);
                            return;
                        }
                    }
                    ret.setStatus(RegistrationStatus.DONE);
                }
            });
        } catch (RejectedExecutionException e) {
            ret.setStatus(RegistrationStatus.ERROR);
        }

        return ret;
    }

    @Override
    public void removeCapability(CapabilityEntry capEntry) {
        localCapabilitiesStore.removeCapability(capEntry);

        // Remove from the global capabilities directory if needed
        if (capEntry.getProviderQos().getScope() != ProviderScope.LOCAL) {

            // Currently Endpoint address needs to be added for remove on server side to work correctly.
            // TODO Modify removeCapability in CapDirImpl to accept any endpoint address list
            JoynrMessagingEndpointAddress joynrMessagingEndpointAddress = new JoynrMessagingEndpointAddress(localChannelId);
            capEntry.endpointAddresses.add(joynrMessagingEndpointAddress);

            CapabilityInformation capabilityInformation = capabilityEntry2Information(capEntry);
            if (capabilityInformation != null) {
                globalCapabilitiesClient.unregisterCapability(capabilityInformation);
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
    public Collection<CapabilityEntry> getCapabilities(String domain,
                                                       String interfaceName,
                                                       ProviderQosRequirements requestedQos,
                                                       DiscoveryQos discoveryQos) {
        // assert (globalCapabilitiesClient != null);
        Collection<CapabilityEntry> caps = new LinkedList<CapabilityEntry>();
        DiscoveryScope scope = discoveryQos.getDiscoveryScope();

        caps.addAll(localCapabilitiesStore.findCapabilitiesForInterfaceAddress(domain,
                                                                               interfaceName,
                                                                               requestedQos,
                                                                               discoveryQos));

        if ((caps.size() == 0) && (scope == GLOBAL_ONLY || scope == LOCAL_AND_GLOBAL || scope == LOCAL_THEN_GLOBAL)) {
            if (globalCapabilitiesClient != null) {
                List<CapabilityInformation> capInfoList = globalCapabilitiesClient.lookupCapabilities(domain,
                                                                                                      interfaceName,
                                                                                                      requestedQos);
                Collection<CapabilityEntry> globalCaps = capabilityInformationList2Entries(capInfoList);
                for (CapabilityEntry ce : globalCaps) {
                    // TODO can a CapabilityEntry coming from the GlobalCapabilityDirectoy have more than one
                    // EndpointAddress?
                    // TODO when are entries purged from the messagingEndpointDirectory?
                    if (ce.getParticipantId() != null && ce.getEndpointAddresses().size() > 0) {
                        messagingEndpointDirectory.put(ce.getParticipantId(), ce.getEndpointAddresses().get(0));
                    }
                }
                caps.addAll(globalCaps);
            }
        }
        return caps;
    }

    @Override
    public Collection<CapabilityEntry> getCapabilities(String participantId, DiscoveryQos discoveryQos) {
        Collection<CapabilityEntry> caps = localCapabilitiesStore.findCapabilitiesForParticipantId(participantId,
                                                                                                   discoveryQos);
        if (caps.size() == 0 && !discoveryQos.isLocalOnly()) {
            caps = capabilityInformationList2Entries(globalCapabilitiesClient.getCapabilitiesForParticipantId(participantId));
        }

        return caps;
    }

    @Override
    public void getCapabilities(final String domain,
                                final String interfaceName,
                                final ProviderQosRequirements requestedQos,
                                final DiscoveryQos discoveryQos,
                                final CapabilitiesCallback capabilitiesCallback) {
        try {
            executionQueue.submit(new Runnable() {

                @Override
                public void run() {
                    Collection<CapabilityEntry> caps = null;
                    try {
                        caps = getCapabilities(domain, interfaceName, requestedQos, discoveryQos);
                        capabilitiesCallback.processCapabilitiesReceived(caps);
                    } catch (Throwable e) {
                        capabilitiesCallback.onError(e);
                    }
                }

            });

        } catch (RejectedExecutionException e) {
            capabilitiesCallback.onError(e);
        }
    }

    @Override
    public void getCapabilities(final String participantId,
                                final DiscoveryQos discoveryQos,
                                final CapabilitiesCallback capabilitiesCallback) {
        try {
            executionQueue.submit(new Runnable() {

                @Override
                public void run() {
                    try {
                        Collection<CapabilityEntry> caps = getCapabilities(participantId, discoveryQos);
                        capabilitiesCallback.processCapabilitiesReceived(caps);
                    } catch (Throwable e) {
                        capabilitiesCallback.onError(e);
                    }
                }

            });
        } catch (RejectedExecutionException e) {
            capabilitiesCallback.onError(e);
        }
    }

    @CheckForNull
    private CapabilityInformation capabilityEntry2Information(CapabilityEntry capabilityEntry) {
        return capabilityEntry.toCapabilityInformation();
    }

    private Collection<CapabilityEntry> capabilityInformationList2Entries(List<CapabilityInformation> capInfoList) {
        Collection<CapabilityEntry> capEntryCollection = Lists.newArrayList();
        for (CapabilityInformation capInfo : capInfoList) {
            capEntryCollection.add(new CapabilityEntry(capInfo.getDomain(),
                                                       capInfo.getInterfaceName(),
                                                       capInfo.getProviderQos(),
                                                       new JoynrMessagingEndpointAddress(capInfo.getChannelId()),
                                                       capInfo.getParticipantId(),
                                                       CapabilityScope.REMOTE));
        }
        return capEntryCollection;
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        if (unregisterAllRegisteredCapabilities) {
            HashSet<CapabilityEntry> allCapabilities = localCapabilitiesStore.getAllCapabilities();

            List<CapabilityInformation> capInfoList = new ArrayList<CapabilityInformation>(allCapabilities.size());

            for (CapabilityEntry capabilityEntry : allCapabilities) {
                if (capabilityEntry.isLocalRegisteredGlobally()) {
                    CapabilityInformation capabilityInformation = capabilityEntry2Information(capabilityEntry);
                    capInfoList.add(capabilityInformation);
                }
            }

            if (capInfoList.size() > 0) {
                try {
                    globalCapabilitiesClient.unregisterCapabilities(capInfoList);
                } catch (JoynrArbitrationException e) {
                    return;
                }
            }
        }

        executionQueue.shutdown();
    }
}

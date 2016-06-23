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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID;
import static io.joynr.messaging.MessagingPropertyKeys.CAPABILITYDIRECTORYURL;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.DISCOVERYDIRECTORYURL;

import java.util.HashMap;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.inprocess.InProcessAddress;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

/**
 * Reads the legacy properties for provisioning capabilities / discovery directory and domain access controller and
 * provides discovery entries created therefor.
 */
public class LegacyCapabilitiesProvisioning {

    private static final Logger logger = LoggerFactory.getLogger(LegacyCapabilitiesProvisioning.class);

    private Map<Class<?>, DiscoveryEntry> legacyDiscoveryEntries = new HashMap<>();
    private Map<Class<?>, Address> legacyAddresses = new HashMap<>();

    @Inject
    // CHECKSTYLE:OFF
    public LegacyCapabilitiesProvisioning(@Named(DISCOVERYDIRECTORYURL) String discoveryDirectoryUrl,
                                          @Named(CAPABILITYDIRECTORYURL) String deprecatedCapabilityDirectoryUrl,
                                          @Named(CHANNELID) String channelId,
                                          @Named(PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                          @Named(PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                          @Named(PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabilitiesDirectoryChannelId,
                                          @Named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID) String domainAccessControllerParticipantId,
                                          @Named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID) String domainAccessControllerChannelId) {
        // CHECKSTYLE:ON
        String urlToUse = deprecatedCapabilityDirectoryUrl == null || deprecatedCapabilityDirectoryUrl.isEmpty()
                ? discoveryDirectoryUrl : deprecatedCapabilityDirectoryUrl;
        createDiscoveryEntryFor(GlobalCapabilitiesDirectory.class,
                                GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                capabilitiesDirectoryChannelId,
                                capabilitiesDirectoryParticipantId,
                                urlToUse,
                                channelId,
                                discoveryDirectoriesDomain);
        createDiscoveryEntryFor(GlobalDomainAccessController.class,
                                GlobalDomainAccessController.INTERFACE_NAME,
                                domainAccessControllerChannelId,
                                domainAccessControllerParticipantId,
                                urlToUse,
                                channelId,
                                discoveryDirectoriesDomain);
    }

    private void createDiscoveryEntryFor(Class<?> interfaceClass,
                                         String interfaceName,
                                         String channelId,
                                         String participantId,
                                         String urlForAddress,
                                         String localChannelId,
                                         String domain) {
        Address address;
        if (localChannelId.equals(channelId)) {
            address = new InProcessAddress();
        } else if (urlForAddress.startsWith("tcp") || urlForAddress.startsWith("mqtt")) {
            address = new MqttAddress(urlForAddress, channelId + "/+");
        } else {
            address = new ChannelAddress(urlForAddress, channelId);
        }
        DiscoveryEntry discoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                                domain,
                                                                                interfaceName,
                                                                                participantId,
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                Long.MAX_VALUE,
                                                                                "",
                                                                                address);
        logger.debug("Created legacy discovery entry: {}", discoveryEntry);
        legacyDiscoveryEntries.put(interfaceClass, discoveryEntry);
        legacyAddresses.put(interfaceClass, address);
    }

    public DiscoveryEntry getDiscoveryEntryForInterface(Class<?> serviceInterface) {
        return legacyDiscoveryEntries.get(serviceInterface);
    }

    public Address getAddressForInterface(Class<?> serviceInterface) {
        return legacyAddresses.get(serviceInterface);
    }

}

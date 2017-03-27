package io.joynr.capabilities;

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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.DISCOVERYDIRECTORYURL;
import static io.joynr.messaging.MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL;
import static java.lang.String.format;

import java.util.HashMap;
import java.util.Map;

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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Reads the legacy properties for provisioning capabilities / discovery directory and domain access controller and
 * provides discovery entries created therefor.
 */
public class LegacyCapabilitiesProvisioning {

    private static final Logger logger = LoggerFactory.getLogger(LegacyCapabilitiesProvisioning.class);

    public static class LegacyProvisioningPropertiesHolder {

        @Inject(optional = true)
        @Named(DISCOVERYDIRECTORYURL)
        protected String discoveryDirectoryUrl;

        @Inject(optional = true)
        @Named(DOMAINACCESSCONTROLLERURL)
        protected String domainAccessControllerUrl;

        @Inject(optional = true)
        @Named(CHANNELID)
        protected String channelId;

        @Inject(optional = true)
        @Named(PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN)
        protected String discoveryDirectoriesDomain;

        @Inject(optional = true)
        @Named(PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID)
        protected String capabilitiesDirectoryParticipantId;

        @Inject(optional = true)
        @Named(PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID)
        protected String capabilitiesDirectoryChannelId;

        @Inject(optional = true)
        @Named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID)
        protected String domainAccessControllerParticipantId;

        @Inject(optional = true)
        @Named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID)
        protected String domainAccessControllerChannelId;

        public LegacyProvisioningPropertiesHolder() {
        }

        // @CHECKSTYLE:OFF
        public LegacyProvisioningPropertiesHolder(String discoveryDirectoryUrl,
                                                  String domainAccessControllerUrl,
                                                  String channelId,
                                                  String discoveryDirectoriesDomain,
                                                  String capabilitiesDirectoryParticipantId,
                                                  String capabilitiesDirectoryChannelId,
                                                  String domainAccessControllerParticipantId,
                                                  String domainAccessControllerChannelId) {
            // @CHECKSTYLE:ON
            this.discoveryDirectoryUrl = discoveryDirectoryUrl;
            this.domainAccessControllerUrl = domainAccessControllerUrl;
            this.channelId = channelId;
            this.discoveryDirectoriesDomain = discoveryDirectoriesDomain;
            this.capabilitiesDirectoryParticipantId = capabilitiesDirectoryParticipantId;
            this.capabilitiesDirectoryChannelId = capabilitiesDirectoryChannelId;
            this.domainAccessControllerParticipantId = domainAccessControllerParticipantId;
            this.domainAccessControllerChannelId = domainAccessControllerChannelId;
        }
    }

    private Map<Class<?>, DiscoveryEntry> legacyDiscoveryEntries = new HashMap<>();
    private Map<Class<?>, Address> legacyAddresses = new HashMap<>();

    @Inject
    public LegacyCapabilitiesProvisioning(LegacyProvisioningPropertiesHolder properties) {
        createDiscoveryEntryFor(GlobalCapabilitiesDirectory.class,
                                GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                properties.capabilitiesDirectoryChannelId,
                                properties.capabilitiesDirectoryParticipantId,
                                properties.discoveryDirectoryUrl,
                                properties.channelId,
                                properties.discoveryDirectoriesDomain);
        createDiscoveryEntryFor(GlobalDomainAccessController.class,
                                GlobalDomainAccessController.INTERFACE_NAME,
                                properties.domainAccessControllerChannelId,
                                properties.domainAccessControllerParticipantId,
                                properties.domainAccessControllerUrl,
                                properties.channelId,
                                properties.discoveryDirectoriesDomain);
    }

    private void createDiscoveryEntryFor(Class<?> interfaceClass,
                                         String interfaceName,
                                         String channelId,
                                         String participantId,
                                         String urlForAddress,
                                         String localChannelId,
                                         String domain) {
        boolean hasUrl = isPresent(urlForAddress);
        boolean hasParticipantId = isPresent(participantId);
        if (hasUrl && !hasParticipantId) {
            throw new IllegalArgumentException(format("When configuring the discovery directory or domain access controller "
                                                              + "via properties, you must provide both a URL and a participant ID per service.%n"
                                                              + "You provided the URL '%s' and the participant ID '%s' for the service %s.%n"
                                                              + "Please complete the configuration and restart the application.",
                                                      urlForAddress,
                                                      participantId,
                                                      interfaceName));
        }
        if (hasParticipantId && hasUrl && isPresent(channelId) && isPresent(domain)) {
            Address address;
            if (localChannelId.equals(channelId)) {
                address = new InProcessAddress();
            } else if (urlForAddress.startsWith("tcp") || urlForAddress.startsWith("mqtt")) {
                address = new MqttAddress(urlForAddress, channelId);
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
        } else {
            logger.trace("Insufficient properties data to create entry for interface {}", interfaceName);
        }
    }

    private boolean isPresent(String value) {
        return value != null && !value.trim().isEmpty();
    }

    public DiscoveryEntry getDiscoveryEntryForInterface(Class<?> serviceInterface) {
        return legacyDiscoveryEntries.get(serviceInterface);
    }

    public Address getAddressForInterface(Class<?> serviceInterface) {
        return legacyAddresses.get(serviceInterface);
    }

}

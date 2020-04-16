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

import static io.joynr.util.JoynrUtil.createUuidString;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.guice.LowerCaseProperties;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.PropertyLoader;
import io.joynr.runtime.SystemServicesSettings;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;
import joynr.infrastructure.GlobalDomainAccessControllerProvider;
import joynr.system.DiscoveryProvider;
import joynr.system.RoutingProvider;
import joynr.types.GlobalDiscoveryEntry;

@Singleton
public class PropertiesFileParticipantIdStorage implements ParticipantIdStorage {

    private static final Logger logger = LoggerFactory.getLogger(PropertiesFileParticipantIdStorage.class);
    private final GlobalDiscoveryEntry capabilitiesDirectoryEntry;
    private final GlobalDiscoveryEntry domainAccessControllerEntry;
    Properties persistedParticipantIds;
    private String persistenceFileName;
    private Properties joynrProperties;
    private String discoveryProviderParticipantId;
    private String routingProviderParticipantId;

    @Inject
    public PropertiesFileParticipantIdStorage(@Named(MessagingPropertyKeys.JOYNR_PROPERTIES) Properties joynrProperties,
                                              @Named(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE) String persistenceFileName,
                                              @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                              @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String routingProviderParticipantId,
                                              @Named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY) GlobalDiscoveryEntry capabilitiesDirectoryEntry,
                                              @Named(MessagingPropertyKeys.DOMAIN_ACCESS_CONTROLLER_DISCOVERY_ENTRY) GlobalDiscoveryEntry domainAccessControllerEntry) {
        this.joynrProperties = joynrProperties;
        this.persistenceFileName = persistenceFileName;
        this.capabilitiesDirectoryEntry = capabilitiesDirectoryEntry;
        this.domainAccessControllerEntry = domainAccessControllerEntry;
        this.discoveryProviderParticipantId = discoveryProviderParticipantId;
        this.routingProviderParticipantId = routingProviderParticipantId;
        File persistenceFile = new File(persistenceFileName);
        persistedParticipantIds = new LowerCaseProperties(PropertyLoader.loadProperties(persistenceFile));
    }

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.capabilities.ParticipantIdStorage#
     * getProviderParticipantId(java.lang.String, java.lang.Class, java.lang.String,
     * java.lang.String)
     */
    @Override
    public String getProviderParticipantId(String domain, String interfaceName, int majorVersion, String defaultValue) {

        String token = ParticipantIdKeyUtil.getProviderParticipantIdKey(domain, interfaceName, majorVersion);

        String participantId;

        // first see if participantId exists in the persistence file
        if (persistedParticipantIds.containsKey(token.toLowerCase())) {
            participantId = persistedParticipantIds.getProperty(token.toLowerCase());
            // if not, use the default value that was passed in via properties
        } else if (defaultValue != null) {
            participantId = defaultValue;
            // if no default value, generate one and save it to the persistence file
        } else if (ProviderAnnotations.getInterfaceName(GlobalCapabilitiesDirectoryProvider.class)
                                      .equals(interfaceName)) {
            participantId = capabilitiesDirectoryEntry.getParticipantId();
        } else if (ProviderAnnotations.getInterfaceName(GlobalDomainAccessControllerProvider.class)
                                      .equals(interfaceName)) {
            participantId = domainAccessControllerEntry.getParticipantId();
        } else if (ProviderAnnotations.getInterfaceName(DiscoveryProvider.class).equals(interfaceName)) {
            participantId = discoveryProviderParticipantId;
        } else if (ProviderAnnotations.getInterfaceName(RoutingProvider.class).equals(interfaceName)) {
            participantId = routingProviderParticipantId;
        } else {

            participantId = createUuidString();
            persistedParticipantIds.put(token, participantId);
            FileOutputStream fileOutputStream = null;
            try {
                fileOutputStream = new FileOutputStream(persistenceFileName);
                persistedParticipantIds.store(fileOutputStream, null);
            } catch (IOException e1) {
                logger.error("Error saving properties file for channelId", e1);

            } finally {
                if (fileOutputStream != null) {
                    try {
                        fileOutputStream.close();
                    } catch (IOException e) {
                        logger.debug("Error closing output stream", e);
                    }
                }
            }
        }
        return participantId;
    }

    @Override
    public String getProviderParticipantId(String domain, String interfaceName, int majorVersion) {
        String defaultParticipantId = null;
        String providerParticipantIdKey = ParticipantIdKeyUtil.getProviderParticipantIdKey(domain,
                                                                                           interfaceName,
                                                                                           majorVersion);
        if (joynrProperties.containsKey(providerParticipantIdKey)) {
            defaultParticipantId = joynrProperties.getProperty(providerParticipantIdKey);
        }
        return getProviderParticipantId(domain, interfaceName, majorVersion, defaultParticipantId);
    }

}

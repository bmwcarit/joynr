package io.joynr.capabilities;

/*
 * #%L
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

import io.joynr.guice.LowerCaseProperties;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.PropertyLoader;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Properties;
import java.util.UUID;

import io.joynr.runtime.SystemServicesSettings;
import joynr.infrastructure.ChannelUrlDirectoryProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;
import joynr.infrastructure.GlobalDomainAccessControllerProvider;

import joynr.system.DiscoveryProvider;
import joynr.system.RoutingProvider;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class PropertiesFileParticipantIdStorage implements ParticipantIdStorage {
    private static final Logger logger = LoggerFactory.getLogger(PropertiesFileParticipantIdStorage.class);
    Properties persistedParticipantIds;
    private String persistenceFileName;
    private Properties joynrProperties;
    private String channelUrlDirectoryParticipantId;
    private String capabilitiesDirectoryParticipantId;
    private String domainAccessControllerParticipantId;
    private String discoveryProviderParticipantId;
    private String routingProviderParticipantId;;

    @Inject
    public PropertiesFileParticipantIdStorage(@Named(MessagingPropertyKeys.JOYNR_PROPERTIES) Properties joynrProperties,
                                              @Named(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE) String persistenceFileName,
                                              @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID) String channelUrlDirectoryParticipantId,
                                              @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                              @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID) String domainAccessControllerParticipantId,
                                              @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                              @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String routingProviderParticipantId) {
        this.joynrProperties = joynrProperties;
        this.persistenceFileName = persistenceFileName;
        this.channelUrlDirectoryParticipantId = channelUrlDirectoryParticipantId;
        this.capabilitiesDirectoryParticipantId = capabilitiesDirectoryParticipantId;
        this.domainAccessControllerParticipantId = domainAccessControllerParticipantId;
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
    public String getProviderParticipantId(String domain, Class<?> providedInterface, String defaultValue) {

        String token = getProviderParticipantIdKey(domain, providedInterface);

        String participantId;

        // first see if participantId exists in the persistence file
        if (persistedParticipantIds.containsKey(token.toLowerCase())) {
            participantId = persistedParticipantIds.getProperty(token.toLowerCase());
            // if not, use the default value that was passed in via properties
        } else if (defaultValue != null) {
            participantId = defaultValue;
            // if no default value, generate one and save it to the persistence file
        } else if (ChannelUrlDirectoryProvider.class.isAssignableFrom(providedInterface)) {
            participantId = channelUrlDirectoryParticipantId;
        } else if (GlobalCapabilitiesDirectoryProvider.class.isAssignableFrom(providedInterface)) {
            participantId = capabilitiesDirectoryParticipantId;
        } else if (GlobalDomainAccessControllerProvider.class.isAssignableFrom(providedInterface)) {
            participantId = domainAccessControllerParticipantId;
        } else if (DiscoveryProvider.class.isAssignableFrom(providedInterface)) {
            participantId = discoveryProviderParticipantId;
        } else if (RoutingProvider.class.isAssignableFrom(providedInterface)) {
            participantId = routingProviderParticipantId;
        } else {

            participantId = UUID.randomUUID().toString();
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
                    }
                }
            }
        }
        return participantId;
    }

    private static String getProviderParticipantIdKey(String domain, Class<?> providedInterface) {
        String interfaceName = providedInterface.getName();
        try {
            if (providedInterface.getField("INTERFACE_NAME") != null) {
                interfaceName = providedInterface.getField("INTERFACE_NAME").get(null).toString();
            }
        } catch (Exception e) {
        }
        String token = "joynr.participant." + domain + "." + interfaceName;
        return token.replace('/', '.');
    }

    @Override
    public String getProviderParticipantId(String domain, Class<?> providedInterface) {
        String defaultParticipantId = null;
        String providerParticipantIdKey = getProviderParticipantIdKey(domain, providedInterface).toLowerCase();
        if (joynrProperties.containsKey(providerParticipantIdKey)) {
            defaultParticipantId = joynrProperties.getProperty(providerParticipantIdKey);
        }
        return getProviderParticipantId(domain, providedInterface, defaultParticipantId);
    }

}

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
package io.joynr.discovery.jee;

import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.GCD_GBID;
import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.VALID_GBIDS;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;
import static io.joynr.messaging.MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE;

import java.util.Map;
import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;
import javax.inject.Named;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.capabilities.directory.util.GcdUtilities;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.PropertyLoader;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;

/**
 * This is the singleton bean which will provide the configuration values at runtime for the
 * service, also allowing you to specify your own values via OS environment variables and
 * Java system properties to override the defaults provided here.
 * <p>
 *     In order to override values via environment variables, take the joynr property name
 *     (see the
 *     <a href="https://github.com/bmwcarit/joynr/blob/develop/wiki/JavaSettings.md">Java Configuration Reference</a>
 *     for details) and replace all period
 *     characters ('.') with underscores ('_'). Hence, <code>joynr.servlet.hostpath</code>
 *     becomes <code>joynr_servlet_hostpath</code>. Case is ignored, so you can feel free
 *     to use upper-case to make any of the longer names more readable in your setup.
 * </p>
 * <p>
 *     Values which you will most likely want to override to match your deployment setup are:
 *     <ul>
 *         <li>{@link io.joynr.messaging.ConfigurableMessagingSettings#PROPERTY_GBIDS}</li>
 *         <li>{@link io.joynr.messaging.mqtt.MqttModule#PROPERTY_MQTT_BROKER_URIS}</li>
 *         <li>{@link MessagingPropertyKeys#PROPERTY_SERVLET_CONTEXT_ROOT}</li>
 *         <li>{@link MessagingPropertyKeys#PROPERTY_SERVLET_HOST_PATH}</li>
 *     </ul>
 */
@Singleton
public class JoynrConfigurationProvider {

    private static final Logger logger = LoggerFactory.getLogger(JoynrConfigurationProvider.class);

    @Produces
    @JoynrProperties
    public Properties getJoynrProperties() {
        Properties joynrProperties = new Properties();
        readAndSetProperty(joynrProperties, MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        readAndSetProperty(joynrProperties,
                           MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT,
                           "/discovery-directory-jee/messaging");
        readAndSetProperty(joynrProperties, MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://localhost:8080");
        readAndSetProperty(joynrProperties, MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");
        readAndSetProperty(joynrProperties,
                           MessagingPropertyKeys.PERSISTENCE_FILE,
                           "discovery-directory-joynr.properties");
        readAndSetProperty(joynrProperties,
                           ParticipantIdKeyUtil.getProviderParticipantIdKey(getJoynrLocalDomain(),
                                                                            GlobalCapabilitiesDirectoryProvider.class),
                           readCapabilitiesDirectoryParticipantIdFromProperties());
        return joynrProperties;
    }

    private String readCapabilitiesDirectoryParticipantIdFromProperties() {
        Properties joynrDefaultProperties = PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);
        if (!joynrDefaultProperties.containsKey(PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID)) {
            logger.trace("Default properties loaded: {}", joynrDefaultProperties);
            throw new IllegalStateException("No capabilities directory participant ID found in properties.");
        }
        return joynrDefaultProperties.getProperty(PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID);
    }

    private void readAndSetProperty(Properties joynrProperties, String propertyKey, String defaultValue) {
        assert propertyKey != null
                && !propertyKey.trim().isEmpty() : "You must specify a non-null, non-empty property key";
        logger.trace("Called with joynrProperties {}, propertyKey {} and defaultValue {}",
                     joynrProperties,
                     propertyKey,
                     defaultValue);
        String value = System.getProperty(propertyKey, defaultValue);
        String envKey = propertyKey.replaceAll("\\.", "_");
        String envValue = null;
        for (Map.Entry<String, String> envEntry : System.getenv().entrySet()) {
            if (envEntry.getKey().equalsIgnoreCase(envKey)) {
                envValue = envEntry.getValue();
                break;
            }
        }
        if (envValue != null && !envValue.trim().isEmpty()) {
            value = envValue;
        }
        logger.debug("Setting property {} to value {}.", propertyKey, value);
        joynrProperties.setProperty(propertyKey, value);
    }

    @Produces
    @JoynrLocalDomain
    public String getJoynrLocalDomain() {
        return "io.joynr";
    }

    @Produces
    @Named(GCD_GBID)
    public String getGcdGbid() {
        Properties envPropertiesAll = new Properties();
        envPropertiesAll.putAll(System.getenv());
        String gcdGbid = PropertyLoader.getPropertiesWithPattern(envPropertiesAll, GCD_GBID).getProperty(GCD_GBID);
        if (gcdGbid == null || gcdGbid.isEmpty()) {
            gcdGbid = GcdUtilities.loadDefaultGbidsFromDefaultMessagingProperties()[0];
        }
        return gcdGbid;
    }

    @Produces
    @Named(VALID_GBIDS)
    public String getValidGbids() {
        Properties envPropertiesAll = new Properties();
        envPropertiesAll.putAll(System.getenv());
        String validGbidsString = PropertyLoader.getPropertiesWithPattern(envPropertiesAll, VALID_GBIDS)
                                                .getProperty(VALID_GBIDS);
        if (validGbidsString == null || validGbidsString.isEmpty()) {
            validGbidsString = getGcdGbid();
        }
        return validGbidsString;
    }

    @Produces
    @Named(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS)
    public long getDefaultExpiryTimeMs() {
        Properties joynrDefaultProperties = PropertyLoader.loadProperties(DEFAULT_MESSAGING_PROPERTIES_FILE);
        if (!joynrDefaultProperties.containsKey(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS)) {
            logger.error("PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS not found in default properties: {}",
                         joynrDefaultProperties);
            throw new JoynrIllegalStateException("PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS not found in default properties.");
        }
        return Long.parseLong(joynrDefaultProperties.getProperty(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS));
    }
}

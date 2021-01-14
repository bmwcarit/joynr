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
package io.joynr.accesscontrol.global.jee;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROL_LISTEDITOR_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ROLE_CONTROLLER_PARTICIPANT_ID;

import java.util.Map;
import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.PropertyLoader;
import joynr.infrastructure.GlobalDomainAccessControlListEditorProvider;
import joynr.infrastructure.GlobalDomainAccessControllerProvider;
import joynr.infrastructure.GlobalDomainRoleControllerProvider;

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
 *         <li>{@link ConfigurableMessagingSettings#PROPERTY_GBIDS}</li>
 *         <li>{@link MqttModule#PROPERTY_MQTT_BROKER_URIS}</li>
 *         <li>{@link MessagingPropertyKeys#PROPERTY_SERVLET_CONTEXT_ROOT}</li>
 *         <li>{@link MessagingPropertyKeys#PROPERTY_SERVLET_HOST_PATH}</li>
 *     </ul>
 * </p>
 */
@Singleton
public class TestJoynrConfigurationProvider {

    private static final Logger logger = LoggerFactory.getLogger(JoynrConfigurationProvider.class);

    @Produces
    @JoynrProperties
    public Properties getJoynrProperties() {
        Properties joynrProperties = new Properties();
        readAndSetProperty(joynrProperties, MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        readAndSetProperty(joynrProperties,
                           MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT,
                           "/domain-access-controller-jee/messaging");
        readAndSetProperty(joynrProperties, MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://localhost:8080");
        readAndSetProperty(joynrProperties, MessagingPropertyKeys.CHANNELID, "test_domainaccesscontroller_channelid");
        readAndSetProperty(joynrProperties,
                           MessagingPropertyKeys.PERSISTENCE_FILE,
                           "domain-access-controller-joynr.properties");

        Properties joynrDefaultProperties = PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);

        if (joynrDefaultProperties.containsKey(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID)) {
            readAndSetProperty(joynrProperties,
                               ParticipantIdKeyUtil.getProviderParticipantIdKey(getJoynrLocalDomain(),
                                                                                GlobalDomainAccessControllerProvider.class),
                               joynrDefaultProperties.getProperty(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID));
        }

        if (joynrDefaultProperties.containsKey(PROPERTY_DOMAIN_ACCESS_CONTROL_LISTEDITOR_PARTICIPANT_ID)) {
            readAndSetProperty(joynrProperties,
                               ParticipantIdKeyUtil.getProviderParticipantIdKey(getJoynrLocalDomain(),
                                                                                GlobalDomainAccessControlListEditorProvider.class),
                               joynrDefaultProperties.getProperty(PROPERTY_DOMAIN_ACCESS_CONTROL_LISTEDITOR_PARTICIPANT_ID));
        }

        if (joynrDefaultProperties.containsKey(PROPERTY_DOMAIN_ROLE_CONTROLLER_PARTICIPANT_ID)) {
            readAndSetProperty(joynrProperties,
                               ParticipantIdKeyUtil.getProviderParticipantIdKey(getJoynrLocalDomain(),
                                                                                GlobalDomainRoleControllerProvider.class),
                               joynrDefaultProperties.getProperty(PROPERTY_DOMAIN_ROLE_CONTROLLER_PARTICIPANT_ID));
        }

        return joynrProperties;
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
}

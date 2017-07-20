package io.joynr.systemintegrationtest.jee;

import java.util.Map;

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

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;

@Singleton
public class JoynrConfigurationProvider {

    private static final String SIT_JEE_LOCAL_DOMAIN_PREFIX_KEY = "SIT_JEE_LOCAL_DOMAIN_PREFIX";

    private static final Logger LOG = LoggerFactory.getLogger(JoynrConfigurationProvider.class);

    private static final String DEFAULT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT, "/sit-jee-app/messaging");
        joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://sit-jee-app:8080");
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID, "io.joynr.systemintegrationtest.jee");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://mqttbroker:1883");
        joynrProperties.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL, "tcp://mqttbroker:1883");
        joynrProperties.setProperty(MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL, "tcp://mqttbroker:1883");

        for (Map.Entry<String, String> envEntry : System.getenv().entrySet()) {
            if (envEntry.getKey().toLowerCase().startsWith("joynr") && envEntry.getValue() != null) {
                String joynrKey = envEntry.getKey().toLowerCase().replaceAll("_", ".");
                joynrProperties.put(joynrKey, envEntry.getValue());
            }
        }

        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        String localDomainPrefix = System.getenv(SIT_JEE_LOCAL_DOMAIN_PREFIX_KEY);
        String domainPrefix;
        if (localDomainPrefix == null) {
            domainPrefix = DEFAULT_DOMAIN_PREFIX;
        } else {
            domainPrefix = localDomainPrefix;
        }
        LOG.debug("Using domain prefix: " + domainPrefix);
        return domainPrefix + ".jee";
    }

}

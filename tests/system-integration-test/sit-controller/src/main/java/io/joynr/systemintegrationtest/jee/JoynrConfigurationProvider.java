/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.systemintegrationtest.jee;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GBIDS;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GLOBAL_CAPABILITIES_DIRECTORY_URL;

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.StaticCapabilitiesProvisioning;
import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;

@Singleton
public class JoynrConfigurationProvider {

    private static final Logger logger = LoggerFactory.getLogger(JoynrConfigurationProvider.class);
    private static final String SIT_JEE_LOCAL_DOMAIN_PREFIX_KEY = "SIT_JEE_LOCAL_DOMAIN_PREFIX";
    private static final String DEFAULT_SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String MQTT_BROKER_URIS = "tcp://mqttbroker-1:1883,tcp://mqttbroker-2:1883";
    private static final String GBIDS = "joynrdefaultgbid,othergbid";

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID, "io.joynr.systemintegrationtest.controller");

        joynrProperties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, MQTT_BROKER_URIS);
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "false");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,60");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "30,30");

        joynrProperties.setProperty(PROPERTY_GBIDS, GBIDS);

        joynrProperties.setProperty(PROPERTY_GLOBAL_CAPABILITIES_DIRECTORY_URL, "joynrdefaultgbid");

        joynrProperties.setProperty(StaticCapabilitiesProvisioning.PROPERTY_PROVISIONED_CAPABILITIES_FILE,
                                    "sit_provisioned_capabilities.json");

        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        String localDomainPrefix = System.getenv(SIT_JEE_LOCAL_DOMAIN_PREFIX_KEY);
        String domainPrefix;
        if (localDomainPrefix == null) {
            domainPrefix = DEFAULT_SIT_DOMAIN_PREFIX;
        } else {
            domainPrefix = localDomainPrefix;
        }
        logger.debug("Using domain prefix: " + domainPrefix);
        String domain = domainPrefix + ".jee";
        logger.debug("Using domain: " + domain);
        return domain;
    }

}

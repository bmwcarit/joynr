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
package io.joynr.systemintegrationtest.jee;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GBIDS;

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

    static final String SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String CHANNEL_ID = SIT_DOMAIN_PREFIX + ".jee";

    private static final Logger logger = LoggerFactory.getLogger(JoynrConfigurationProvider.class);

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        String domain = System.getenv("SIT_DOMAIN");
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID, domain + "_" + CHANNEL_ID);
        joynrProperties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, System.getenv("SIT_BROKERURIS"));
        joynrProperties.setProperty(PROPERTY_GBIDS, System.getenv("SIT_GBIDS"));
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC,
                                    System.getenv("SIT_CONNECTION_TIMEOUTS"));
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC,
                                    System.getenv("SIT_KEEP_ALIVE_TIMERS"));

        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        String domain = System.getenv("SIT_DOMAIN");
        String domainPrefix = SIT_DOMAIN_PREFIX;
        logger.debug("Using domain prefix: " + domainPrefix);
        return domain + "_" + domainPrefix + ".jee";
    }

}

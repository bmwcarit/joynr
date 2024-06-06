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
package io.joynr.systemintegrationtest.jeestatelessasync;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GBIDS;

import java.util.Properties;

import jakarta.ejb.Singleton;
import jakarta.enterprise.inject.Produces;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;
import joynr.test.SitControllerSync;

@Singleton
public class JoynrConfigurationProvider {

    private static final Logger logger = LoggerFactory.getLogger(JoynrConfigurationProvider.class);
    static final String SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String CHANNEL_ID = SIT_DOMAIN_PREFIX + ".jeestatelessconsumer";
    private static final String RECEIVER_ID = System.getenv("RECEIVER_ID");
    private static final String CONTROLLER_DOMAIN_PREFIX = SIT_DOMAIN_PREFIX + ".controller";
    static final String CONTROLLER_DOMAIN = CONTROLLER_DOMAIN_PREFIX + ".jee-stateless-consumer";
    private static final String CONTROLLER_PARTICIPANT_ID = "sit-controller." + System.getenv("RECEIVER_ID");
    private static final String MQTT_BROKER_URIS = "tcp://mqttbroker-1:1883,tcp://mqttbroker-2:1883";

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        logger.debug("Using CHANNEL_ID: " + CHANNEL_ID);
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID, CHANNEL_ID);
        logger.debug("Using RECEIVER_ID: " + RECEIVER_ID);
        joynrProperties.setProperty(MessagingPropertyKeys.RECEIVERID, RECEIVER_ID);
        joynrProperties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, MQTT_BROKER_URIS);
        joynrProperties.setProperty(PROPERTY_GBIDS, "joynrdefaultgbid,othergbid");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,60");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "30,30");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "true");
        joynrProperties.setProperty(ParticipantIdKeyUtil.getProviderParticipantIdKey(CONTROLLER_DOMAIN,
                                                                                     SitControllerSync.class),
                                    CONTROLLER_PARTICIPANT_ID);

        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        logger.debug("Using domain prefix: " + SIT_DOMAIN_PREFIX);
        String domain = SIT_DOMAIN_PREFIX + ".jee-stateless-consumer";
        logger.debug("Using domain: " + domain);
        return domain;
    }

}

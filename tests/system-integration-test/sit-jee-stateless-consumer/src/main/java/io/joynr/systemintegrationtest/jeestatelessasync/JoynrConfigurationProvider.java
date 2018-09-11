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

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;

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

    private static final Logger LOG = LoggerFactory.getLogger(JoynrConfigurationProvider.class);
    private static final String SIT_JEE_LOCAL_DOMAIN_PREFIX_KEY = "SIT_JEE_LOCAL_DOMAIN_PREFIX";
    private static final String DEFAULT_SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String MQTT_BROKER_URI = "tcp://mqttbroker:1883";

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID,
                                    "io.joynr.systemintegrationtest.jeestatelessconsumer");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, MQTT_BROKER_URI);
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "true");
        joynrProperties.setProperty(ParticipantIdKeyUtil.getProviderParticipantIdKey("io.joynr.systemintegrationtest.controller.jee-stateless-consumer",
                                                                                     SitControllerSync.class),
                                    "io.joynr.systemintegrationtest.controller.jee-stateless-consumer");
        joynrProperties.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL, MQTT_BROKER_URI);
        joynrProperties.setProperty(MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL, MQTT_BROKER_URI);

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
        LOG.debug("Using domain prefix: " + domainPrefix);
        String domain = domainPrefix + ".jee-stateless-consumer";
        LOG.debug("Using domain: " + domain);
        return domain;
    }

}

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
package io.joynr.sharedsubscription.test.clusteredapp;

import java.util.Properties;

import jakarta.ejb.Singleton;
import jakarta.enterprise.inject.Produces;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;

@Singleton
public class JoynrConfigurationProvider {

    private static final Logger logger = LoggerFactory.getLogger(JoynrConfigurationProvider.class);

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {

        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID, "io.joynr.sharedsubscriptions.test.clusteredapp");

        final String brokerUri = System.getenv("MQTT_BROKER_URL");
        if (brokerUri == null) {
            logger.error("Environment variable MQTT_BROKER_URL has not been set!");
        } else {
            logger.info("Environment variable MQTT_BROKER_URL has been set to: {}", brokerUri);
            joynrProperties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, brokerUri);

            // secure ssl connection
            if (brokerUri.startsWith("ssl:")) {
                setPropertiesForSecureMqttConnection(joynrProperties);
            }
        }

        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, Boolean.TRUE.toString());
        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        return "io.joynr.sharedsubscriptions.test.clusteredapp";
    }

    private void setPropertiesForSecureMqttConnection(Properties joynrProperties) {
        logger.debug("Using secure connection to the MQTT broker, setting keystore and truststore.");
        final String keyStorePath = System.getenv("CLIENT_KEYSTORE");
        final String keyStorePwd = System.getenv("KEYSTORE_PWD");
        final String trustStorePath = System.getenv("CA_TRUSTSTORE");
        final String trustStorePwd = System.getenv("TRUSTSTORE_PWD");

        if (keyStorePath == null) {
            logger.error("Environment variable CLIENT_KEYSTORE has not been set!");
        } else {
            joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, keyStorePath);
            logger.debug("Environment variable CLIENT_KEYSTORE has been set to: {}", keyStorePath);
            if (keyStorePath.endsWith(".p12")) {
                joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_TYPE, "PKCS12");
            }
        }
        if (keyStorePwd == null) {
            logger.error("Environment variable KEYSTORE_PWD has not been set!");
        } else {
            joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, keyStorePwd);
        }
        if (trustStorePath == null) {
            logger.error("Environment variable CA_TRUSTSTORE has not been set!");
        } else {
            joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, trustStorePath);
            logger.debug("Environment variable CA_TRUSTSTORE has been set to: {}", trustStorePath);
            if (trustStorePath.endsWith(".p12")) {
                joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE, "PKCS12");
            }
        }
        if (trustStorePwd == null) {
            logger.error("Environment variable TRUSTSTORE_PWD has not been set!");
        } else {
            joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, trustStorePwd);
        }
    }
}

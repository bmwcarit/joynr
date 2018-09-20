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
package io.joynr.backpressure.test.clusteredprovider;

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;

import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings;

@Singleton
public class JoynrConfigurationProvider {

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT,
                                    "/backpressure-provider/messaging");
        joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH,
                                    System.getenv("joynr_servlet_hostpath"));
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID, "io.joynr.backpressure.test.clusteredprovider");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, System.getenv("MQTT_BROKER_URL"));
        joynrProperties.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL, System.getenv("MQTT_BROKER_URL"));
        joynrProperties.setProperty(MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL, System.getenv("MQTT_BROKER_URL"));

        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, Boolean.TRUE.toString());

        // large queue
        joynrProperties.setProperty(LimitAndBackpressureSettings.PROPERTY_MAX_INCOMING_MQTT_REQUESTS, "1000");
        joynrProperties.setProperty(LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_ENABLED,
                                    Boolean.TRUE.toString());
        joynrProperties.setProperty(LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD,
                                    "70");
        joynrProperties.setProperty(LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD,
                                    "20");

        // limit parallel processing of requests
        joynrProperties.setProperty(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS, "1");

        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        return "io.joynr.backpressure.test.provider";
    }

}

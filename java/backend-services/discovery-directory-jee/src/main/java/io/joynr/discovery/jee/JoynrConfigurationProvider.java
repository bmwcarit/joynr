package io.joynr.discovery.jee;

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

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;

import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.runtime.GlobalAddressProvider;

@Singleton
public class JoynrConfigurationProvider {

    @JoynrProperties
    @Produces
    public Properties getJoynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://localhost:1883");
        joynrProperties.setProperty(GlobalAddressProvider.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String getJoynrLocalDomain() {
        return "io.joynr";
    }
}

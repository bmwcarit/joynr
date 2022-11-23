/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt.hivemq.client;

import com.google.inject.AbstractModule;

import io.joynr.messaging.mqtt.JoynrMqttClientCreator;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientSignalService;
import io.joynr.messaging.mqtt.MqttModule;

/**
 * Use this module if you want to use the HiveMQ MQTT Client backed implementation of MQTT communication for joynr.
 */
public class HivemqMqttClientModule extends AbstractModule {

    @Override
    protected void configure() {
        install(new MqttModule());
        bind(MqttClientFactory.class).to(HivemqMqttClientFactory.class);
        bind(IHivemqMqttClientTrustManagerFactory.class).to(HivemqMqttClientTrustManagerFactory.class);
        bind(JoynrMqttClientCreator.class).to(HivemqMqttClientCreator.class);
        bind(MqttClientSignalService.class).to(HivemqMqttClientFactory.class);
    }

}

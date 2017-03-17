package io.joynr.messaging.mqtt;

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

import com.google.inject.Inject;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.JoynrMessageSerializer;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttMessagingStubFactory extends AbstractMiddlewareMessagingStubFactory<MqttMessagingStub, MqttAddress> {

    private MqttMessageSerializerFactory mqttMessageSerializerFactory;
    private JoynrMqttClient mqttClient;

    @Inject
    public MqttMessagingStubFactory(MqttMessageSerializerFactory mqttMessageSerializerFactory,
                                    MqttClientFactory mqttClientFactory) {
        this.mqttMessageSerializerFactory = mqttMessageSerializerFactory;
        this.mqttClient = mqttClientFactory.create();
    }

    @Override
    protected MqttMessagingStub createInternal(MqttAddress address) {
        JoynrMessageSerializer messageSerializer = mqttMessageSerializerFactory.create(address);
        return new MqttMessagingStub(address, mqttClient, messageSerializer);
    }

    @Override
    public void shutdown() {
        // nothing to do. MqttClient is shutdown by the skeleton
    }
}

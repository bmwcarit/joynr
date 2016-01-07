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

import java.io.IOException;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.JoynrMessageSerializer;
import io.joynr.messaging.routing.MessageRouter;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Connects to the MQTT broker
 */
public class MqttMessagingSkeleton implements IMessagingSkeleton {

    private MessageRouter messageRouter;
    private JoynrMqttClient mqttClient;
    private JoynrMessageSerializer messageSerializer;
    private MqttClientFactory mqttClientFactory;

    @Inject
    public MqttMessagingSkeleton(@Named(MqttModule.PROPERTY_MQTT_ADDRESS) MqttAddress ownAddress,
                                 MessageRouter messageRouter,
                                 MqttClientFactory mqttClientFactory,
                                 MqttMessageSerializerFactory messageSerializerFactory) {
        this.messageRouter = messageRouter;
        this.mqttClientFactory = mqttClientFactory;
        messageSerializer = messageSerializerFactory.create(ownAddress);
    }

    @Override
    public void init() {
        mqttClient = mqttClientFactory.create();
        mqttClient.addMessageListener(this);
        mqttClient.start();
    }

    @Override
    public void shutdown() {
        mqttClient.shutdown();
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        try {
            messageRouter.route(message);
        } catch (JoynrSendBufferFullException | JoynrMessageNotSentException | IOException e) {
            failureAction.execute(e);
        }
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        JoynrMessage message = messageSerializer.deserialize(serializedMessage);
        transmit(message, failureAction);

    }
}

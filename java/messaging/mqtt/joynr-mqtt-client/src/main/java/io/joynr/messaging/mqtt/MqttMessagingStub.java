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

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.JoynrMessageSerializer;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Messaging stub used to send messages to a MQTT Broker
 */
public class MqttMessagingStub implements IMessaging {

    private static final String PRIORITY_LOW = "/low/";
    private static final String RAW = PRIORITY_LOW + "raw";
    private MqttAddress address;
    private JoynrMqttClient mqttClient;
    private JoynrMessageSerializer messageSerializer;
    private MqttMessageReplyToAddressCalculator mqttMessageReplyToAddressCalculator;

    public MqttMessagingStub(MqttAddress address,
                             JoynrMqttClient mqttClient,
                             JoynrMessageSerializer messageSerializer,
                             MqttMessageReplyToAddressCalculator mqttMessageReplyToAddressCalculator) {
        this.address = address;
        this.mqttClient = mqttClient;
        this.messageSerializer = messageSerializer;
        this.mqttMessageReplyToAddressCalculator = mqttMessageReplyToAddressCalculator;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        mqttMessageReplyToAddressCalculator.setReplyTo(message);
        String topic = address.getTopic() + PRIORITY_LOW + message.getTo();
        String serializedMessage = messageSerializer.serialize(message);
        try {
            mqttClient.publishMessage(topic, serializedMessage);
        } catch (Exception error) {
            failureAction.execute(error);
        }
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        // Unable to access participantId, so publishing to RAW topic
        String topic = address.getTopic() + RAW;
        try {
            mqttClient.publishMessage(topic, serializedMessage);
        } catch (Exception error) {
            failureAction.execute(error);
        }
    }

}

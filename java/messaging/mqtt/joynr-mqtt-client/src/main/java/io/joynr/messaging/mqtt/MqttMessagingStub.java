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
package io.joynr.messaging.mqtt;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Messaging stub used to send messages to a MQTT Broker
 */
public class MqttMessagingStub implements IMessagingStub {
    private static final Logger LOG = LoggerFactory.getLogger(MqttMessagingStub.class);

    public static final int DEFAULT_QOS_LEVEL = 1;
    public static final int BEST_EFFORT_QOS_LEVEL = 0;

    private static final String PRIORITY_LOW = "/low/";
    private MqttAddress address;
    private JoynrMqttClient mqttClient;

    public MqttMessagingStub(MqttAddress address, JoynrMqttClient mqttClient) {
        this.address = address;
        this.mqttClient = mqttClient;
    }

    @Override
    public void transmit(ImmutableMessage message, SuccessAction successAction, FailureAction failureAction) {
        String topic = address.getTopic();
        if (!Message.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
            topic += PRIORITY_LOW + message.getRecipient();
        }

        int qosLevel = DEFAULT_QOS_LEVEL;
        String effortHeaderValue = message.getEffort();
        if (effortHeaderValue != null && String.valueOf(MessagingQosEffort.BEST_EFFORT).equals(effortHeaderValue)) {
            qosLevel = BEST_EFFORT_QOS_LEVEL;
        }
        try {
            byte[] serializedMessage = message.getSerializedMessage();
            LOG.debug(">>> OUTGOING TO {} >>> {}bytes: {}", address.getBrokerUri(), serializedMessage.length, message);
            mqttClient.publishMessage(topic, serializedMessage, qosLevel, successAction, failureAction);
            successAction.execute();
        } catch (Exception error) {
            failureAction.execute(error);
        }
    }
}

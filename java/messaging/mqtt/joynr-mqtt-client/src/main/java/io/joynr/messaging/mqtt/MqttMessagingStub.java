package io.joynr.messaging.mqtt;

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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.JoynrMessageSerializer;
import io.joynr.messaging.MessagingQosEffort;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Messaging stub used to send messages to a MQTT Broker
 */
public class MqttMessagingStub implements IMessaging {
    private static final Logger LOG = LoggerFactory.getLogger(MqttMessagingStub.class);

    public static final int DEFAULT_QOS_LEVEL = 1;
    public static final int BEST_EFFORT_QOS_LEVEL = 0;

    private static final String PRIORITY_LOW = "/low/";
    private static final String RAW = PRIORITY_LOW + "raw";
    private MqttAddress address;
    private JoynrMqttClient mqttClient;
    private JoynrMessageSerializer messageSerializer;

    public MqttMessagingStub(MqttAddress address, JoynrMqttClient mqttClient, JoynrMessageSerializer messageSerializer) {
        this.address = address;
        this.mqttClient = mqttClient;
        this.messageSerializer = messageSerializer;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        LOG.debug(">>> OUTGOING >>> {}", message.toLogMessage());
        String topic = address.getTopic();
        if (!JoynrMessage.MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
            topic += PRIORITY_LOW + message.getTo();
        }
        String serializedMessage = messageSerializer.serialize(message);
        int qosLevel = DEFAULT_QOS_LEVEL;
        String effortHeaderValue = message.getHeaderValue(JoynrMessage.HEADER_NAME_EFFORT);
        if (effortHeaderValue != null && String.valueOf(MessagingQosEffort.BEST_EFFORT).equals(effortHeaderValue)) {
            qosLevel = BEST_EFFORT_QOS_LEVEL;
        }
        try {
            mqttClient.publishMessage(topic, serializedMessage, qosLevel);
        } catch (Exception error) {
            failureAction.execute(error);
        }
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        LOG.debug(">>> OUTGOING >>> {}", serializedMessage);
        // Unable to access participantId, so publishing to RAW topic
        String topic = address.getTopic() + RAW;
        try {
            mqttClient.publishMessage(topic, serializedMessage);
        } catch (Exception error) {
            failureAction.execute(error);
        }
    }

}

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

import io.joynr.common.ExpiryDate;
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
    private static final Logger logger = LoggerFactory.getLogger(MqttMessagingStub.class);

    public static final int DEFAULT_QOS_LEVEL = 1;
    public static final int BEST_EFFORT_QOS_LEVEL = 0;

    private static final String PRIORITY_LOW = "/low";
    private static final long MESSAGE_EXPIRY_MAX_INTERVAL = 4294967295L;
    private MqttAddress address;
    private JoynrMqttClient mqttClient;

    public MqttMessagingStub(MqttAddress address, JoynrMqttClient mqttClient) {
        this.address = (address != null) ? new MqttAddress(address) : null;
        this.mqttClient = mqttClient;
    }

    @Override
    public void transmit(ImmutableMessage message, SuccessAction successAction, FailureAction failureAction) {
        String topic = address.getTopic();
        if (!Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
            topic += PRIORITY_LOW;
        }

        int qosLevel = DEFAULT_QOS_LEVEL;
        String effortHeaderValue = message.getEffort();
        if (effortHeaderValue != null && String.valueOf(MessagingQosEffort.BEST_EFFORT).equals(effortHeaderValue)) {
            qosLevel = BEST_EFFORT_QOS_LEVEL;
        }
        ExpiryDate expiryDate = ExpiryDate.fromAbsolute(message.getTtlMs());
        long msgTtlSec = (long) Math.ceil(expiryDate.getRelativeTtl() / 1000.0);
        if (msgTtlSec > MESSAGE_EXPIRY_MAX_INTERVAL || msgTtlSec < 0) {
            msgTtlSec = MESSAGE_EXPIRY_MAX_INTERVAL;
        }
        byte[] serializedMessage = message.getSerializedMessage();
        if (logger.isTraceEnabled()) {
            logger.trace(">>> OUTGOING TO {} >>> {}", address.getBrokerUri(), message);
        } else {
            logger.debug(">>> OUTGOING TO {} >>> {}", address.getBrokerUri(), message.getTrackingInfo());
        }
        mqttClient.publishMessage(topic,
                                  serializedMessage,
                                  message.getPrefixedCustomHeaders(),
                                  qosLevel,
                                  msgTtlSec,
                                  successAction,
                                  failureAction);
    }
}

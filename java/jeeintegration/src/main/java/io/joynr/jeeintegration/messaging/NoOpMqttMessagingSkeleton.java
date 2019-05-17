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
package io.joynr.jeeintegration.messaging;

import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;

/**
 * Because the messaging stub will refuse to send a message via MQTT unless a messaging skeleton has been registered
 * for the MqttAddress type, we bind a dummy implementation in this module which simply does nothing (no operation -
 * NoOp).
 */
public class NoOpMqttMessagingSkeleton implements IMqttMessagingSkeleton {
    private final static Logger logger = LoggerFactory.getLogger(NoOpMqttMessagingSkeleton.class);

    private MqttClientFactory mqttClientFactory;
    private Set<JoynrMqttClient> mqttClients;
    private final String[] gbids;

    public NoOpMqttMessagingSkeleton(MqttClientFactory mqttClientFactory, String[] gbids) {
        this.mqttClientFactory = mqttClientFactory;
        this.gbids = gbids.clone();
        mqttClients = new HashSet<>();
    }

    @Override
    public void transmit(byte[] serializedMessage, FailureAction failureAction) {
        logger.trace("NoOp processing of mqtt message");
    }

    @Override
    public void init() {
        for (String gbid : gbids) {
            // NoOpMqttMessagingSkeleton initializes only the senders to enable Mqtt message publishing
            JoynrMqttClient mqttClient = mqttClientFactory.createSender(gbid);
            mqttClient.setMessageListener(this);
            mqttClient.start();
            mqttClients.add(mqttClient);
        }
    }

    @Override
    public void shutdown() {
        mqttClients.forEach(mqttClient -> mqttClient.shutdown());
    }

    @Override
    public void registerMulticastSubscription(String multicastId) {
    }

    @Override
    public void unregisterMulticastSubscription(String multicastId) {
    }

}

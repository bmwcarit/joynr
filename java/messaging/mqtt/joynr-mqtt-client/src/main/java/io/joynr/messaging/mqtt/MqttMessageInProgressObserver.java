/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_ENABLED;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_MAX_INCOMING_MQTT_REQUESTS;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class MqttMessageInProgressObserver {
    private static final Logger logger = LoggerFactory.getLogger(MqttMessageInProgressObserver.class);
    private static final String disablingBackpressureMessage = "Disabling backpressure mechanism because of invalid property settings";

    private int currentMessagesInProgress;

    private List<MqttMessagingSkeleton> mqttMessagingSkeletons;
    private HashSet<String> messagesInProgress;
    private final int maxIncomingMqttRequests;
    private final int reEnableMessageAcknowledgementThreshold;
    private final int backpressureEnablingThreshold;
    private boolean backpressureActive = false;
    private final boolean backpressureEnabled;

    @Inject
    public MqttMessageInProgressObserver(@Named(PROPERTY_BACKPRESSURE_ENABLED) boolean backpressureEnabled,
                                         @Named(PROPERTY_MAX_INCOMING_MQTT_REQUESTS) int maxIncomingMqttRequests,
                                         @Named(PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD) int reEnableMessageAcknowledgementThreshold,
                                         @Named(PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM) int receiveMaximum,
                                         @Named(GBID_ARRAY) String[] gbids,
                                         @Named(PROPERTY_KEY_SEPARATE_REPLY_RECEIVER) boolean separateReplyReceiver) {
        mqttMessagingSkeletons = new ArrayList<>();
        messagesInProgress = new HashSet<>();
        currentMessagesInProgress = 0;
        this.backpressureEnabled = backpressureEnabled;
        this.maxIncomingMqttRequests = maxIncomingMqttRequests;
        this.reEnableMessageAcknowledgementThreshold = reEnableMessageAcknowledgementThreshold;
        backpressureEnablingThreshold = maxIncomingMqttRequests - gbids.length * receiveMaximum;
        if (backpressureEnabled && !separateReplyReceiver) {
            logger.warn("Backpressure is enabled without a separate MQTT connection to receive reply messages. When backpressure is active on high load, reply messages might be held up as well.");
        }
        validateBackpressureValues();
    }

    private void validateBackpressureValues() {
        if (backpressureEnabled) {
            if (maxIncomingMqttRequests <= 0) {
                logger.error("Invalid value {} for {}, expecting a limit greater than 0 when backpressure is activated",
                             maxIncomingMqttRequests,
                             PROPERTY_MAX_INCOMING_MQTT_REQUESTS);
                throw new IllegalArgumentException(disablingBackpressureMessage);
            }

            if (backpressureEnablingThreshold <= 0) {
                logger.error("{} ({}) is less than {} ({} * number of configured backends/brokers)",
                             maxIncomingMqttRequests,
                             PROPERTY_MAX_INCOMING_MQTT_REQUESTS,
                             maxIncomingMqttRequests - backpressureEnablingThreshold,
                             PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM);
                throw new IllegalArgumentException(disablingBackpressureMessage);
            }

            if (reEnableMessageAcknowledgementThreshold < 0
                    || reEnableMessageAcknowledgementThreshold >= backpressureEnablingThreshold) {
                logger.error("Invalid value {} for {}, value has to be greater than 0 and less than {} ({} - ({} * number of configured backends/brokers)",
                             reEnableMessageAcknowledgementThreshold,
                             PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD,
                             backpressureEnablingThreshold,
                             PROPERTY_MAX_INCOMING_MQTT_REQUESTS,
                             PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM);
                throw new IllegalArgumentException(disablingBackpressureMessage);
            }
        }
    }

    public void registerMessagingSkeleton(MqttMessagingSkeleton skeleton) {
        mqttMessagingSkeletons.add(skeleton);
    }

    public boolean canMessageBeAcknowledged(String messageId) {
        if (!backpressureEnabled) {
            return true;
        }
        synchronized (messagesInProgress) {
            if (!messagesInProgress.add(messageId)) {
                logger.error("Could not add message with {} to messages in progress.", messageId);
                return true;
            }
            currentMessagesInProgress++;
            if (backpressureActive) {
                return false;
            }
            if (currentMessagesInProgress <= backpressureEnablingThreshold) {
                return true;
            }
            logger.warn("Backpressure mode entered. Incoming MQTT requests will no longer be acknowledged.");
            backpressureActive = true;
            return false;
        }
    }

    public void decrementMessagesInProgress(String messageId) {
        if (!backpressureEnabled) {
            return;
        }
        synchronized (messagesInProgress) {
            if (messagesInProgress.remove(messageId)) {
                currentMessagesInProgress--;
                if (backpressureActive && currentMessagesInProgress <= reEnableMessageAcknowledgementThreshold) {
                    backpressureActive = false;
                    logger.warn("Backpressure mode exited. Acknowledging all outstanding MQTT requests.");
                    for (MqttMessagingSkeleton skeleton : mqttMessagingSkeletons) {
                        skeleton.acknowledgeOutstandingPublishes();
                    }
                }
            }
        }
    }
}

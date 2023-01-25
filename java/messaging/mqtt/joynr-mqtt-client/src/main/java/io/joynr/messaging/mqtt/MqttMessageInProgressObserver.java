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

import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_ENABLED;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_MAX_INCOMING_MQTT_REQUESTS;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class MqttMessageInProgressObserver {
    private static final Logger logger = LoggerFactory.getLogger(MqttMessageInProgressObserver.class);
    private final String disablingBackpressureMessage = "Disabling backpressure mechanism because of invalid property settings";

    private AtomicInteger currentMessagesInProgress;

    private List<MqttMessagingSkeleton> mqttMessagingSkeletons;
    private HashSet<String> messagesInProgress;
    private final int maxIncomingMqttRequests;
    private final int reEnableMessageAcknowledgementTreshold;
    private final int receiveMaximum;
    private boolean backpressureMode = false;
    private boolean backpressureEnabled;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttMessageInProgressObserver(@Named(PROPERTY_BACKPRESSURE_ENABLED) boolean backpressureEnabled,
                                         @Named(PROPERTY_MAX_INCOMING_MQTT_REQUESTS) int maxIncomingMqttRequests,
                                         @Named(PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD) int reEnableMessageAcknowledgementTreshold,
                                         @Named(PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM) int receiveMaximum) {
        mqttMessagingSkeletons = new ArrayList<MqttMessagingSkeleton>();
        messagesInProgress = new HashSet<>();
        currentMessagesInProgress = new AtomicInteger(0);
        this.backpressureEnabled = backpressureEnabled;
        this.maxIncomingMqttRequests = maxIncomingMqttRequests;
        this.reEnableMessageAcknowledgementTreshold = reEnableMessageAcknowledgementTreshold;
        this.receiveMaximum = receiveMaximum;
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

            if (receiveMaximum >= maxIncomingMqttRequests) {
                backpressureEnabled = false;
                logger.error("Receive maximum {} ({}) is greater than maximum of enqueable incoming requests {} ({}). Disabling backpressure. Also, this setting does not make sense. Please reevaluate your configuration.",
                             receiveMaximum,
                             PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM,
                             maxIncomingMqttRequests,
                             PROPERTY_MAX_INCOMING_MQTT_REQUESTS);
                return;
            }

            if (reEnableMessageAcknowledgementTreshold < 0
                    || reEnableMessageAcknowledgementTreshold >= (maxIncomingMqttRequests - receiveMaximum)) {
                logger.error("Invalid value {} for {}, value has to be smaller than {} ({}) - {} ({})",
                             reEnableMessageAcknowledgementTreshold,
                             PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD,
                             maxIncomingMqttRequests,
                             PROPERTY_MAX_INCOMING_MQTT_REQUESTS,
                             receiveMaximum,
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
            messagesInProgress.add(messageId);
            if (currentMessagesInProgress.incrementAndGet() <= (maxIncomingMqttRequests - receiveMaximum)
                    && !backpressureMode) {
                return true;
            } else {
                backpressureMode = true;
                return false;
            }
        }
    }

    public void decrementMessagesInProgress(String messageId) {
        if (!backpressureEnabled) {
            return;
        }
        synchronized (messagesInProgress) {
            if (messagesInProgress.contains(messageId)) {
                messagesInProgress.remove(messageId);
                if (currentMessagesInProgress.decrementAndGet() <= reEnableMessageAcknowledgementTreshold
                        && backpressureMode) {
                    backpressureMode = false;
                    for (MqttMessagingSkeleton skeleton : mqttMessagingSkeletons) {
                        skeleton.acknowledgeOutstandingPublishes();
                    }
                }
            }
        }
    }
}

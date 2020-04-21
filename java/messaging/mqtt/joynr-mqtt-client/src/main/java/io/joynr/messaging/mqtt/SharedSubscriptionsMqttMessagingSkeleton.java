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

import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_MAX_INCOMING_MQTT_REQUESTS;
import static java.lang.String.format;

import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;

/**
 * Overrides the standard {@link MqttMessagingSkeleton} in order to customise the topic subscription strategy in the
 * case where HiveMQ shared subscriptions are available.
 *
 * @see io.joynr.messaging.mqtt.MqttModule#PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS
 */
public class SharedSubscriptionsMqttMessagingSkeleton extends MqttMessagingSkeleton {
    private static final Logger LOG = LoggerFactory.getLogger(SharedSubscriptionsMqttMessagingSkeleton.class);

    private static final String NON_ALPHA_REGEX_PATTERN = "[^a-zA-Z]";
    private final String channelId;
    private final String sharedSubscriptionsTopic;
    private final AtomicBoolean subscribedToSharedSubscriptionsTopic;
    private final String replyToTopic;
    private boolean backpressureEnabled;
    private final int backpressureIncomingMqttRequestsUpperThreshold;
    private final int backpressureIncomingMqttRequestsLowerThreshold;
    private final int unsubscribeThreshold;
    private final int resubscribeThreshold;

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public SharedSubscriptionsMqttMessagingSkeleton(String ownTopic,
                                                    int maxIncomingMqttRequests,
                                                    boolean backpressureEnabled,
                                                    int backpressureIncomingMqttRequestsUpperThreshold,
                                                    int backpressureIncomingMqttRequestsLowerThreshold,
                                                    String replyToTopic,
                                                    MessageRouter messageRouter,
                                                    MqttClientFactory mqttClientFactory,
                                                    String channelId,
                                                    MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                                    RawMessagingPreprocessor rawMessagingPreprocessor,
                                                    Set<JoynrMessageProcessor> messageProcessors,
                                                    JoynrStatusMetricsReceiver joynrStatusMetricsReceiver,
                                                    String ownGbid,
                                                    RoutingTable routingTable) {
        super(ownTopic,
              maxIncomingMqttRequests,
              messageRouter,
              mqttClientFactory,
              mqttTopicPrefixProvider,
              rawMessagingPreprocessor,
              messageProcessors,
              joynrStatusMetricsReceiver,
              ownGbid,
              routingTable);
        this.replyToTopic = replyToTopic;
        this.channelId = channelId;
        this.sharedSubscriptionsTopic = createSharedSubscriptionsTopic();
        this.subscribedToSharedSubscriptionsTopic = new AtomicBoolean(false);
        this.backpressureEnabled = backpressureEnabled;
        this.backpressureIncomingMqttRequestsUpperThreshold = backpressureIncomingMqttRequestsUpperThreshold;
        this.backpressureIncomingMqttRequestsLowerThreshold = backpressureIncomingMqttRequestsLowerThreshold;
        validateBackpressureValues();
        this.unsubscribeThreshold = (maxIncomingMqttRequests * backpressureIncomingMqttRequestsUpperThreshold) / 100;
        this.resubscribeThreshold = (maxIncomingMqttRequests * backpressureIncomingMqttRequestsLowerThreshold) / 100;
    }

    private void validateBackpressureValues() {
        if (backpressureEnabled) {
            boolean invalidPropertyValueDetected = false;

            if (maxIncomingMqttRequests <= 0) {
                invalidPropertyValueDetected = true;
                LOG.error("Invalid value {} for {}, expecting a limit greater than 0 when backpressure is activated",
                          maxIncomingMqttRequests,
                          PROPERTY_MAX_INCOMING_MQTT_REQUESTS);
            }

            if (backpressureIncomingMqttRequestsUpperThreshold <= 0
                    || backpressureIncomingMqttRequestsUpperThreshold > 100) {
                invalidPropertyValueDetected = true;
                LOG.error("Invalid value {} for {}, expecting percentage value in range (0,100]",
                          backpressureIncomingMqttRequestsUpperThreshold,
                          PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD);
            }

            if (backpressureIncomingMqttRequestsLowerThreshold < 0
                    || backpressureIncomingMqttRequestsLowerThreshold >= 100) {
                invalidPropertyValueDetected = true;
                LOG.error("Invalid value {} for {}, expecting percentage value in range [0,100)",
                          backpressureIncomingMqttRequestsLowerThreshold,
                          PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD);
            }

            if (backpressureIncomingMqttRequestsLowerThreshold >= backpressureIncomingMqttRequestsUpperThreshold) {
                invalidPropertyValueDetected = true;
                LOG.error("Lower threshold percentage {} must be stricly below the upper threshold percentage {}. Change the value of {} or {}",
                          backpressureIncomingMqttRequestsLowerThreshold,
                          backpressureIncomingMqttRequestsUpperThreshold,
                          PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD,
                          PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD);
            }

            // disable backpressure in case of detected problems and throw an exception
            if (invalidPropertyValueDetected) {
                backpressureEnabled = false;
                String disablingBackpressureMessage = "Disabling backpressure mechanism because of invalid property settings";
                LOG.error(disablingBackpressureMessage);
                throw new IllegalArgumentException(disablingBackpressureMessage);
            }
        }
    }

    @Override
    protected void subscribe() {
        LOG.info("Subscribing to shared topic: {}", sharedSubscriptionsTopic);
        getClient().subscribe(sharedSubscriptionsTopic);
        subscribedToSharedSubscriptionsTopic.set(true);
        String topic = replyToTopic + "/#";
        LOG.info("Subscribing to reply-to topic: {}", topic);
        getClient().subscribe(topic);
    }

    @Override
    protected void requestAccepted(String messageId) {
        super.requestAccepted(messageId);

        if (backpressureEnabled && getCurrentCountOfUnprocessedMqttRequests() >= unsubscribeThreshold) {
            // count of unprocessed requests bypasses upper threshold,
            // try to stop further incoming requests
            if (subscribedToSharedSubscriptionsTopic.compareAndSet(true, false)) {
                getClient().unsubscribe(sharedSubscriptionsTopic);
                LOG.info("Unsubscribed from topic {} due to enabled backpressure mechanism "
                        + "and passed upper threshold of unprocessed MQTT requests", sharedSubscriptionsTopic);
            }
        }
    }

    @Override
    protected void requestProcessed(String messageId) {
        super.requestProcessed(messageId);

        if (backpressureEnabled && getCurrentCountOfUnprocessedMqttRequests() < resubscribeThreshold) {
            // count of unprocessed requests drops below lower threshold,
            // try to get further incoming requests
            if (subscribedToSharedSubscriptionsTopic.compareAndSet(false, true)) {
                getClient().subscribe(sharedSubscriptionsTopic);
                LOG.info("Subscribed again to topic {} due to enabled backpressure mechanism "
                        + "and passed lower threshold of unprocessed MQTT requests", sharedSubscriptionsTopic);
            }
        }
    }

    private String createSharedSubscriptionsTopic() {
        StringBuilder sb = new StringBuilder("$share/");
        sb.append(sanitiseChannelIdForUseAsTopic());
        sb.append("/");
        sb.append(getOwnTopic());
        sb.append("/#");
        return sb.toString();
    }

    private String sanitiseChannelIdForUseAsTopic() {
        String result = channelId.replaceAll(NON_ALPHA_REGEX_PATTERN, "");
        if (result.isEmpty()) {
            throw new IllegalArgumentException(format("The channel ID %s cannot be converted to a valid MQTT topic fragment because it does not contain any alpha characters.",
                                                      channelId));
        }
        return result;
    }

}

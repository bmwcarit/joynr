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

import static java.lang.String.format;

import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;

/**
 * Overrides the standard {@link MqttMessagingSkeleton} in order to customise the topic subscription strategy in the
 * case where HiveMQ shared subscriptions are available.
 * <p>
 * It subscribes automatically to the replyTo topic and the shared topic when {@link #subscribe()} is called.
 *
 * @see io.joynr.messaging.mqtt.MqttModule#PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS
 */
public class SharedSubscriptionsMqttMessagingSkeleton extends MqttMessagingSkeleton {
    private static final Logger logger = LoggerFactory.getLogger(SharedSubscriptionsMqttMessagingSkeleton.class);

    private static final String NON_ALPHA_REGEX_PATTERN = "[^a-zA-Z]";
    private final String channelId;
    private final String sharedSubscriptionsTopic;
    private final AtomicBoolean subscribedToSharedSubscriptionsTopic;
    private final String replyToTopic;
    private JoynrMqttClient replyClient;
    boolean separateReplyMqttClient;

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public SharedSubscriptionsMqttMessagingSkeleton(String ownTopic,
                                                    String replyToTopic,
                                                    MessageRouter messageRouter,
                                                    MessageProcessedHandler messageProcessedHandler,
                                                    MqttClientFactory mqttClientFactory,
                                                    String channelId,
                                                    MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                                    RawMessagingPreprocessor rawMessagingPreprocessor,
                                                    Set<JoynrMessageProcessor> messageProcessors,
                                                    String ownGbid,
                                                    RoutingTable routingTable,
                                                    boolean separateReplyMqttClient,
                                                    String backendUid,
                                                    MqttMessageInProgressObserver mqttMessageInProgressObserver) {
        super(ownTopic,
              messageRouter,
              messageProcessedHandler,
              mqttClientFactory,
              mqttTopicPrefixProvider,
              rawMessagingPreprocessor,
              messageProcessors,
              ownGbid,
              routingTable,
              backendUid,
              mqttMessageInProgressObserver);
        this.replyToTopic = replyToTopic;
        this.channelId = channelId;
        this.sharedSubscriptionsTopic = createSharedSubscriptionsTopic();
        this.subscribedToSharedSubscriptionsTopic = new AtomicBoolean(false);
        this.separateReplyMqttClient = separateReplyMqttClient;
        replyClient = mqttClientFactory.createReplyReceiver(ownGbid);
    }

    @Override
    public void init() {
        logger.debug("Initializing shared subscriptions MQTT skeleton (ownGbid={}) ...", ownGbid);
        if (separateReplyMqttClient) {
            replyClient.setMessageListener(this);
            replyClient.start();
            mqttClientFactory.connect(replyClient);
        }
        super.init();
    }

    @Override
    protected void subscribe() {
        subscribeToReplyTopic();
        subscribeToSharedTopic();
    }

    protected void subscribeToReplyTopic() {
        String topic = replyToTopic + "/#";
        logger.info("Subscribing to reply-to topic: {}", topic);
        replyClient.subscribe(topic);
    }

    protected void subscribeToSharedTopic() {
        logger.info("Subscribing to shared topic: {}", sharedSubscriptionsTopic);
        client.subscribe(sharedSubscriptionsTopic);
        subscribedToSharedSubscriptionsTopic.set(true);
    }

    private String createSharedSubscriptionsTopic() {
        StringBuilder sb = new StringBuilder("$share/");
        sb.append(sanitiseChannelIdForUseAsTopic());
        sb.append("/");
        sb.append(ownTopic);
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

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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_MAX_INCOMING_MQTT_REQUESTS;

import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.Set;
import java.io.Serializable;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageProcessedListener;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Connects to the MQTT broker
 */
public class MqttMessagingSkeleton implements IMqttMessagingSkeleton, MessageProcessedListener {
    private static final Logger LOG = LoggerFactory.getLogger(MqttMessagingSkeleton.class);

    private final int maxIncomingMqttRequests;
    private MessageRouter messageRouter;
    private JoynrMqttClient mqttClient;
    private MqttClientFactory mqttClientFactory;
    private MqttAddress ownAddress;
    private ConcurrentMap<String, AtomicInteger> multicastSubscriptionCount = Maps.newConcurrentMap();
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;
    private RawMessagingPreprocessor rawMessagingPreprocessor;
    private Set<JoynrMessageProcessor> messageProcessors;
    private Set<String> incomingMqttRequests;
    private AtomicLong droppedMessagesCount;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 2 LINES
    public MqttMessagingSkeleton(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                 @Named(PROPERTY_MAX_INCOMING_MQTT_REQUESTS) int maxIncomingMqttRequests,
                                 MessageRouter messageRouter,
                                 MqttClientFactory mqttClientFactory,
                                 MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                 RawMessagingPreprocessor rawMessagingPreprocessor,
                                 Set<JoynrMessageProcessor> messageProcessors) {
        this.ownAddress = ownAddress;
        this.maxIncomingMqttRequests = maxIncomingMqttRequests;
        this.messageRouter = messageRouter;
        this.mqttClientFactory = mqttClientFactory;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
        this.rawMessagingPreprocessor = rawMessagingPreprocessor;
        this.messageProcessors = messageProcessors;
        this.incomingMqttRequests = Collections.synchronizedSet(new HashSet<String>());
        this.droppedMessagesCount = new AtomicLong();
    }

    @Override
    public void init() {
        LOG.debug("Initializing MQTT skeleton ...");

        messageRouter.registerMessageProcessedListener(this);

        mqttClient = mqttClientFactory.create();
        mqttClient.setMessageListener(this);
        mqttClient.start();
        subscribe();
    }

    /**
     * Performs standard subscription to the {@link #ownAddress own address'} topic; override this method to perform
     * custom subscriptions. One use-case could be to subscribe to one topic for incoming messages and another topic for
     * replies.
     */
    protected void subscribe() {
        mqttClient.subscribe(ownAddress.getTopic() + "/#");
    }

    @Override
    public void shutdown() {
        mqttClient.shutdown();
    }

    @Override
    public void registerMulticastSubscription(String multicastId) {
        multicastSubscriptionCount.putIfAbsent(multicastId, new AtomicInteger());
        int numberOfSubscriptions = multicastSubscriptionCount.get(multicastId).incrementAndGet();
        if (numberOfSubscriptions == 1) {
            mqttClient.subscribe(getSubscriptionTopic(multicastId));
        }
    }

    @Override
    public void unregisterMulticastSubscription(String multicastId) {
        AtomicInteger subscribersCount = multicastSubscriptionCount.get(multicastId);
        if (subscribersCount != null) {
            int remainingCount = subscribersCount.decrementAndGet();
            if (remainingCount == 0) {
                mqttClient.unsubscribe(getSubscriptionTopic(multicastId));
            }
        }
    }

    private String translateWildcard(String multicastId) {
        String topic = multicastId;
        if (topic.endsWith("/*")) {
            topic = topic.replaceFirst("/\\*$", "/#");
        }
        return topic;
    }

    @Override
    public void transmit(byte[] serializedMessage, FailureAction failureAction) {
        try {
            HashMap<String, Serializable> context = new HashMap<String, Serializable>();
            byte[] processedMessage = rawMessagingPreprocessor.process(serializedMessage, context);

            ImmutableMessage message = new ImmutableMessage(processedMessage);
            message.setContext(context);

            LOG.debug("<<< INCOMING <<< {}", message);

            if (messageProcessors != null) {
                for (JoynrMessageProcessor processor : messageProcessors) {
                    message = processor.processIncoming(message);
                }
            }

            if (dropMessage(message)) {
                droppedMessagesCount.incrementAndGet();
                return;
            }

            message.setReceivedFromGlobal(true);

            if (isRequestMessageTypeThatCanBeDropped(message.getType())) {
                incomingMqttRequests.add(message.getId());
            }

            try {
                messageRouter.route(message);
            } catch (Exception e) {
                LOG.error("Error processing incoming message. Message will be dropped: {} ", e.getMessage());
                messageProcessed(message.getId());
                failureAction.execute(e);
            }
        } catch (UnsuppportedVersionException | EncodingException | NullPointerException e) {
            LOG.error("Message: \"{}\", could not be deserialized, exception: {}", serializedMessage, e.getMessage());
            failureAction.execute(e);
        }
    }

    private boolean dropMessage(ImmutableMessage message) {
        // check if a limit for requests is set and
        // if there are already too many requests still not processed
        if (maxIncomingMqttRequests > 0 && incomingMqttRequests.size() >= maxIncomingMqttRequests) {
            if (isRequestMessageTypeThatCanBeDropped(message.getType())) {
                LOG.warn("Incoming MQTT message with id {} will be dropped as limit of {} requests is reached",
                         message.getId(),
                         maxIncomingMqttRequests);
                return true;
            }
        }

        return false;
    }

    // only certain types of messages can be dropped in order not to break
    // the communication, e.g. a reply message must not be dropped
    private boolean isRequestMessageTypeThatCanBeDropped(String messageType) {
        if (messageType.equals(Message.VALUE_MESSAGE_TYPE_REQUEST)
                || messageType.equals(Message.VALUE_MESSAGE_TYPE_ONE_WAY)) {
            return true;
        }

        return false;
    }

    protected JoynrMqttClient getClient() {
        return mqttClient;
    }

    protected MqttAddress getOwnAddress() {
        return ownAddress;
    }

    private String getSubscriptionTopic(String multicastId) {
        return mqttTopicPrefixProvider.getMulticastTopicPrefix() + translateWildcard(multicastId);
    }

    public long getDroppedMessagesCount() {
        return droppedMessagesCount.get();
    }

    @Override
    public void messageProcessed(String messageId) {
        if (incomingMqttRequests.remove(messageId)) {
            LOG.debug("Request {} was processed and is removed from the MQTT skeleton tracking list", messageId);
        }
    }

}

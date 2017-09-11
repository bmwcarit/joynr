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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS;

import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Set;
import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

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
import io.joynr.messaging.routing.TimedDelayed;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Connects to the MQTT broker
 */
public class MqttMessagingSkeleton implements IMqttMessagingSkeleton, MessageProcessedListener {

    private static final Logger LOG = LoggerFactory.getLogger(MqttMessagingSkeleton.class);
    private final int repeatedMqttMessageIgnorePeriodMs;
    private final int maxMqttMessagesInQueue;
    private MessageRouter messageRouter;
    private JoynrMqttClient mqttClient;
    private MqttClientFactory mqttClientFactory;
    private MqttAddress ownAddress;
    private ConcurrentMap<String, AtomicInteger> multicastSubscriptionCount = Maps.newConcurrentMap();
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;
    private RawMessagingPreprocessor rawMessagingPreprocessor;
    private Set<JoynrMessageProcessor> messageProcessors;
    private Map<String, MqttAckInformation> processingMessages;
    private DelayQueue<DelayedMessageId> processedMessagesQueue;

    private static class MqttAckInformation {
        private int mqttId;
        private int mqttQos;

        MqttAckInformation(int mqttId, int mqttQos) {
            this.mqttId = mqttId;
            this.mqttQos = mqttQos;
        }

        public int getMqttId() {
            return mqttId;
        }

        public int getMqttQos() {
            return mqttQos;
        }
    }

    private static class DelayedMessageId extends TimedDelayed {

        private String messageId;

        public DelayedMessageId(String messageId, long delayMs) {
            super(delayMs);
            this.messageId = messageId;
        }

        public String getMessageId() {
            return messageId;
        }

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = super.hashCode();
            result = prime * result + ((messageId == null) ? 0 : messageId.hashCode());
            return result;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj == null)
                return false;
            if (getClass() != obj.getClass()) {
                return false;
            }
            DelayedMessageId other = (DelayedMessageId) obj;
            if (messageId == null) {
                if (other.messageId != null) {
                    return false;
                }
            } else if (!messageId.equals(other.messageId)) {
                return false;
            }
            return true;
        }
    }

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttMessagingSkeleton(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                 @Named(PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS) int repeatedMqttMessageIgnorePeriodMs,
                                 @Named(PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE) int maxMqttMessagesInQueue,
                                 MessageRouter messageRouter,
                                 MqttClientFactory mqttClientFactory,
                                 MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                 RawMessagingPreprocessor rawMessagingPreprocessor,
                                 Set<JoynrMessageProcessor> messageProcessors) {
        this.ownAddress = ownAddress;
        this.repeatedMqttMessageIgnorePeriodMs = repeatedMqttMessageIgnorePeriodMs;
        this.maxMqttMessagesInQueue = maxMqttMessagesInQueue;
        this.messageRouter = messageRouter;
        this.mqttClientFactory = mqttClientFactory;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
        this.rawMessagingPreprocessor = rawMessagingPreprocessor;
        this.messageProcessors = messageProcessors;
        this.processingMessages = new HashMap<>();
        this.processedMessagesQueue = new DelayQueue<>();
    }

    @Override
    public void init() {
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

    private void forwardMessage(ImmutableMessage message, int mqttId, int mqttQos, FailureAction failureAction) {
        message.setReceivedFromGlobal(true);
        LOG.debug("<<< INCOMING <<< {}", message);
        synchronized (processingMessages) {
            // message.getId() == null (NullPointerException) already caught in transmit
            processingMessages.put(message.getId(), new MqttAckInformation(mqttId, mqttQos));
        }
        try {
            messageRouter.route(message);
        } catch (Exception e) {
            LOG.error("Error processing incoming message. Message will be dropped: {} ", e.getMessage());
            synchronized (processingMessages) {
                handleMessageProcessed(message.getId(), mqttId, mqttQos);
            }
            failureAction.execute(e);
        }
        synchronized (processingMessages) {
            removeProcessedMessageInformation();
        }
    }

    @Override
    public void transmit(byte[] serializedMessage, int mqttId, int mqttQos, FailureAction failureAction) {
        try {
            HashMap<String, Serializable> context = new HashMap<String, Serializable>();
            byte[] processedMessage = rawMessagingPreprocessor.process(serializedMessage, context);

            ImmutableMessage message = new ImmutableMessage(processedMessage);
            message.setContext(context);

            if (messageProcessors != null) {
                for (JoynrMessageProcessor processor : messageProcessors) {
                    message = processor.processIncoming(message);
                }
            }

            synchronized (processingMessages) {
                // The number of not yet processed (queued) Mqtt messages is the difference between
                // processingMessages.size() and the number of messages which are already processed but still
                // not removed from processingMessages.
                if (processingMessages.size() - processedMessagesQueue.size() >= maxMqttMessagesInQueue) {
                    LOG.warn("Maximum number of Mqtt messages in message queue reached. "
                            + "Incoming Mqtt message with id {} cannot be handled now.", message.getId());
                    return;
                }
                if (processingMessages.containsKey(message.getId())) {
                    LOG.debug("Dropping already received message with id {}", message.getId());
                    return;
                }
            }
            forwardMessage(message, mqttId, mqttQos, failureAction);
        } catch (UnsuppportedVersionException | EncodingException | NullPointerException e) {
            LOG.error("Message: \"{}\", could not be deserialized, exception: {}", serializedMessage, e.getMessage());
            mqttClient.sendMqttAck(mqttId, mqttQos);
            failureAction.execute(e);
        }
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

    private void removeProcessedMessageInformation() {
        DelayedMessageId delayedMessageId;
        while ((delayedMessageId = processedMessagesQueue.poll()) != null) {
            processingMessages.remove(delayedMessageId.getMessageId());
        }
    }

    private void handleMessageProcessed(String messageId, int mqttId, int mqttQos) {
        DelayedMessageId delayedMessageId = new DelayedMessageId(messageId, repeatedMqttMessageIgnorePeriodMs);
        if (!processedMessagesQueue.contains(delayedMessageId)) {
            mqttClient.sendMqttAck(mqttId, mqttQos);
            processedMessagesQueue.put(delayedMessageId);
        }
    }

    @Override
    public void messageProcessed(String messageId) {
        synchronized (processingMessages) {
            MqttAckInformation info = processingMessages.get(messageId);
            if (info == null) {
                return;
            }
            handleMessageProcessed(messageId, info.getMqttId(), info.getMqttQos());
            removeProcessedMessageInformation();
        }
    }

}

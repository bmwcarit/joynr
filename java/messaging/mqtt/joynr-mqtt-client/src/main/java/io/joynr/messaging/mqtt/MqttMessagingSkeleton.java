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

import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Set;
import java.io.Serializable;
import java.util.HashMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingMulticastSubscriber;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

/**
 * Connects to the MQTT broker
 */
public class MqttMessagingSkeleton implements IMessagingSkeleton, IMessagingMulticastSubscriber {

    private static final Logger LOG = LoggerFactory.getLogger(MqttMessagingSkeleton.class);
    private MessageRouter messageRouter;
    private JoynrMqttClient mqttClient;
    private MqttClientFactory mqttClientFactory;
    private MqttAddress ownAddress;
    private ConcurrentMap<String, AtomicInteger> multicastSubscriptionCount = Maps.newConcurrentMap();
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;
    private RawMessagingPreprocessor rawMessagingPreprocessor;
    private Set<JoynrMessageProcessor> messageProcessors;

    @Inject
    public MqttMessagingSkeleton(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                 MessageRouter messageRouter,
                                 MqttClientFactory mqttClientFactory,
                                 MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                 RawMessagingPreprocessor rawMessagingPreprocessor,
                                 Set<JoynrMessageProcessor> messageProcessors) {
        this.ownAddress = ownAddress;
        this.messageRouter = messageRouter;
        this.mqttClientFactory = mqttClientFactory;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
        this.rawMessagingPreprocessor = rawMessagingPreprocessor;
        this.messageProcessors = messageProcessors;
    }

    @Override
    public void init() {
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

    private void forwardMessage(ImmutableMessage message, FailureAction failureAction) {
        LOG.debug("<<< INCOMING <<< {}", message.toLogMessage());
        try {
            message.setReceivedFromGlobal(true);

            String replyToMqttAddress = message.getReplyTo();
            // because the message is received via global transport, isGloballyVisible must be true
            final boolean isGloballyVisible = true;
            if (replyToMqttAddress != null && !replyToMqttAddress.isEmpty()) {
                messageRouter.addNextHop(message.getSender(),
                                         RoutingTypesUtil.fromAddressString(replyToMqttAddress),
                                         isGloballyVisible);
            }
            messageRouter.route(message);
        } catch (Exception e) {
            LOG.error("Error processing incoming message. Message will be dropped: {} ", e);
            failureAction.execute(e);
        }
    }

    @Override
    public void transmit(byte[] serializedMessage, FailureAction failureAction) {
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

            forwardMessage(message, failureAction);
        } catch (UnsuppportedVersionException | EncodingException e) {
            LOG.error("Message: \"{}\", could not be serialized, exception: {}", serializedMessage, e.getMessage());
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

}

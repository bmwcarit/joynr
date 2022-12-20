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

import java.io.Serializable;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.AbstractGlobalMessagingSkeleton;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageProcessedListener;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessageRouterUtil;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import joynr.ImmutableMessage;
import joynr.Message;

/**
 * Connects to the MQTT broker
 */
public class MqttMessagingSkeleton extends AbstractGlobalMessagingSkeleton
        implements IMqttMessagingSkeleton, MessageProcessedListener {
    private static final Logger logger = LoggerFactory.getLogger(MqttMessagingSkeleton.class);

    protected final int maxIncomingMqttRequests;
    protected final String ownTopic;
    protected JoynrMqttClient client;
    protected final String ownGbid;
    private final MessageRouter messageRouter;
    private final MessageProcessedHandler messageProcessedHandler;
    private final MqttClientFactory mqttClientFactory;
    private final ConcurrentMap<String, AtomicInteger> multicastSubscriptionCount;
    private final MqttTopicPrefixProvider mqttTopicPrefixProvider;
    private final RawMessagingPreprocessor rawMessagingPreprocessor;
    private final Set<JoynrMessageProcessor> messageProcessors;
    private final Set<String> incomingMqttRequests;
    private final AtomicLong droppedMessagesCount;
    private final JoynrStatusMetricsReceiver joynrStatusMetricsReceiver;
    private final String backendUid;

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttMessagingSkeleton(String ownTopic,
                                 int maxIncomingMqttRequests,
                                 MessageRouter messageRouter,
                                 MessageProcessedHandler messageProcessedHandler,
                                 MqttClientFactory mqttClientFactory,
                                 MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                 RawMessagingPreprocessor rawMessagingPreprocessor,
                                 Set<JoynrMessageProcessor> messageProcessors,
                                 JoynrStatusMetricsReceiver joynrStatusMetricsReceiver,
                                 String ownGbid,
                                 RoutingTable routingTable,
                                 String backendUid) {
        super(routingTable);
        this.ownTopic = ownTopic;
        this.maxIncomingMqttRequests = maxIncomingMqttRequests;
        this.messageRouter = messageRouter;
        this.messageProcessedHandler = messageProcessedHandler;
        this.mqttClientFactory = mqttClientFactory;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
        this.rawMessagingPreprocessor = rawMessagingPreprocessor;
        this.messageProcessors = messageProcessors;
        this.incomingMqttRequests = Collections.synchronizedSet(new HashSet<String>());
        this.droppedMessagesCount = new AtomicLong();
        this.multicastSubscriptionCount = new ConcurrentHashMap<>();
        this.joynrStatusMetricsReceiver = joynrStatusMetricsReceiver;
        this.ownGbid = ownGbid;
        this.backendUid = backendUid;
        client = mqttClientFactory.createReceiver(ownGbid);
    }

    @Override
    public void init() {
        logger.debug("Initializing MQTT skeleton (ownGbid={}) ...", ownGbid);

        messageProcessedHandler.registerMessageProcessedListener(this);

        client.setMessageListener(this);
        client.start();
        mqttClientFactory.createSender(ownGbid).start();
        subscribe();
    }

    /**
     * Performs standard subscription to the own address topic; override this method to perform
     * custom subscriptions. One use-case could be to subscribe to one topic for incoming messages and another topic for
     * replies.
     */
    protected void subscribe() {
        client.subscribe(ownTopic + "/#");
    }

    @Override
    public void shutdown() {

    }

    @Override
    public void registerMulticastSubscription(String multicastId) {
        multicastSubscriptionCount.putIfAbsent(multicastId, new AtomicInteger());
        int numberOfSubscriptions = multicastSubscriptionCount.get(multicastId).incrementAndGet();
        if (numberOfSubscriptions == 1) {
            client.subscribe(getSubscriptionTopic(multicastId));
        }
    }

    @Override
    public void unregisterMulticastSubscription(String multicastId) {
        AtomicInteger subscribersCount = multicastSubscriptionCount.get(multicastId);
        if (subscribersCount != null) {
            int remainingCount = subscribersCount.decrementAndGet();
            if (remainingCount == 0) {
                client.unsubscribe(getSubscriptionTopic(multicastId));
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
    public void transmit(Mqtt5Publish mqtt5Publish,
                         Map<String, String> prefixedCustomHeaders,
                         FailureAction failureAction) {
            try {
                HashMap<String, Serializable> context = new HashMap<String, Serializable>();
                byte[] processedMessage = rawMessagingPreprocessor.process(mqtt5Publish.getPayloadAsBytes(),
                                                                           Optional.of(context));

            ImmutableMessage message = new ImmutableMessage(processedMessage);

            if (logger.isTraceEnabled()) {
                logger.trace("<<< INCOMING FROM {} <<< {}", ownGbid, message);
            } else {
                logger.debug("<<< INCOMING FROM {} <<< {} creatorUserId: {}",
                             ownGbid,
                             message.getTrackingInfo(),
                             backendUid);
            }

            // If this fails, we quit the processing due to a thrown exception
            MessageRouterUtil.checkExpiry(message);

            message.setContext(context);
            message.setPrefixedExtraCustomHeaders(prefixedCustomHeaders);
            message.setCreatorUserId(backendUid);

            if (messageProcessors != null) {
                for (JoynrMessageProcessor processor : messageProcessors) {
                    message = processor.processIncoming(message);
                }
            }

            if (dropMessage(message)) {
                droppedMessagesCount.incrementAndGet();
                joynrStatusMetricsReceiver.notifyMessageDropped();
                return;
            }

            message.setReceivedFromGlobal(true);

            if (isRequestMessageTypeThatCanBeDropped(message.getType())) {
                requestAccepted(message.getId());
            }

            boolean routingEntryRegistered = false;
            try {
                routingEntryRegistered = registerGlobalRoutingEntry(message, ownGbid);
                messageRouter.routeIn(message);
            } catch (Exception e) {
                removeGlobalRoutingEntry(message, routingEntryRegistered);
                messageProcessed(message.getId());
                failureAction.execute(e);
            }
        } catch (UnsuppportedVersionException | EncodingException | NullPointerException e) {
            logger.error("Message: \"{}\" could not be deserialized, exception: {}", serializedMessage, e.getMessage());
            failureAction.execute(e);
        } catch (Exception e) {
            logger.error("Message \"{}\" could not be transmitted:", serializedMessage, e);
            failureAction.execute(e);
        }
    }

    private boolean dropMessage(ImmutableMessage message) {
        // check if a limit for requests is set and
        // if there are already too many requests still not processed
        if (maxIncomingMqttRequests > 0 && incomingMqttRequests.size() >= maxIncomingMqttRequests) {
            if (isRequestMessageTypeThatCanBeDropped(message.getType())) {
                logger.warn("Incoming MQTT message {} will be dropped as limit of {} requests is reached",
                            message.getTrackingInfo(),
                            maxIncomingMqttRequests);
                return true;
            }
        }

        return false;
    }

    // only certain types of messages can be dropped in order not to break
    // the communication, e.g. a reply message must not be dropped
    private boolean isRequestMessageTypeThatCanBeDropped(Message.MessageType messageType) {
        if (messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST)
                || messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY)) {
            return true;
        }

        return false;
    }

    protected JoynrMqttClient getClient() {
        return client;
    }

    private String getSubscriptionTopic(String multicastId) {
        return mqttTopicPrefixProvider.getMulticastTopicPrefix() + translateWildcard(multicastId);
    }

    public long getDroppedMessagesCount() {
        return droppedMessagesCount.get();
    }

    protected int getCurrentCountOfUnprocessedMqttRequests() {
        return incomingMqttRequests.size();
    }

    @Override
    public void messageProcessed(String messageId) {
        if (incomingMqttRequests.remove(messageId)) {
            requestProcessed(messageId);
        }
    }

    protected void requestAccepted(String messageId) {
        incomingMqttRequests.add(messageId);
    }

    protected void requestProcessed(String messageId) {
        logger.debug("Request with messageId {} was processed and is removed from the MQTT skeleton tracking list",
                     messageId);
    }

}

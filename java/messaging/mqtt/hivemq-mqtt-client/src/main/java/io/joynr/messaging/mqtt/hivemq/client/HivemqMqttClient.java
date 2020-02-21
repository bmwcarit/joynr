/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt.hivemq.client;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Consumer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.exceptions.MqttClientStateException;
import com.hivemq.client.mqtt.mqtt5.Mqtt5RxClient;
import com.hivemq.client.mqtt.mqtt5.message.connect.Mqtt5Connect;
import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;
import com.hivemq.client.mqtt.mqtt5.message.subscribe.Mqtt5Subscribe;
import com.hivemq.client.mqtt.mqtt5.message.subscribe.Mqtt5Subscription;
import com.hivemq.client.mqtt.mqtt5.message.unsubscribe.Mqtt5Unsubscribe;

import edu.umd.cs.findbugs.annotations.SuppressFBWarnings;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.reactivex.BackpressureStrategy;
import io.reactivex.Flowable;

/**
 * This implements the {@link JoynrMqttClient} using the HiveMQ MQTT Client library.
 *
 * TODO
 * - HiveMQ MQTT Client offers better backpressure handling via rxJava, hence revisit this issue to see how we can make
 *   use of this for our own backpressure handling
 */
public class HivemqMqttClient implements JoynrMqttClient {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClient.class);

    private final Mqtt5RxClient client;
    private final boolean cleanSession;
    private Consumer<Mqtt5Publish> publishConsumer;
    private IMqttMessagingSkeleton messagingSkeleton;
    private int keepAliveTimeSeconds;
    private int connectionTimeoutSec;
    private int reconnectDelayMs;
    private volatile boolean shuttingDown;
    private AtomicInteger disconnectCount = new AtomicInteger(0);

    private Map<String, Mqtt5Subscription> subscriptions = new ConcurrentHashMap<>();

    public HivemqMqttClient(Mqtt5RxClient client,
                            int keepAliveTimeSeconds,
                            boolean cleanSession,
                            int connectionTimeoutSec,
                            int reconnectDelayMs) {
        this.client = client;
        this.keepAliveTimeSeconds = keepAliveTimeSeconds;
        this.cleanSession = cleanSession;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.reconnectDelayMs = reconnectDelayMs;
    }

    @Override
    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the connect via callbacks.")
    public synchronized void start() {
        logger.info("Initializing MQTT client {} -> {}", this, client);
        if (!client.getConfig().getState().isConnected()) {
            while (!client.getConfig().getState().isConnected()) {
                logger.info("Attempting to connect client {} (clean session {}) ...", client, cleanSession);
                Mqtt5Connect mqtt5Connect = Mqtt5Connect.builder()
                                                        .cleanStart(cleanSession)
                                                        .keepAlive(keepAliveTimeSeconds)
                                                        .build();
                try {
                    client.connect(mqtt5Connect)
                          .timeout(connectionTimeoutSec, TimeUnit.SECONDS)
                          .doOnSuccess(connAck -> logger.info("MQTT client {} connected: {}.", client, connAck))
                          .doOnError(throwable -> {
                              if (!(throwable instanceof MqttClientStateException)) {
                                  // ignore MqttClientStateException: MQTT client is already connected or connecting
                                  logger.error("Unable to connect MQTT client {}.", client, throwable);
                              }
                          })
                          .blockingGet();
                } catch (Exception e) {
                    if (!(e instanceof MqttClientStateException)) {
                        // ignore MqttClientStateException: MQTT client is already connected or connecting
                        logger.error("Exception while connecting MQTT client.", e);
                    }
                    try {
                        wait(reconnectDelayMs);
                    } catch (Exception exception) {
                        logger.error("Exception while waiting to reconnect to MQTT client.", exception);
                    }
                }
            }
            logger.info("MQTT client {} connected.", client);
        } else {
            logger.info("MQTT client {} already connected - skipping.", client);
        }
        if (publishConsumer == null) {
            logger.info("Setting up publishConsumer for {}", client);
            CountDownLatch publisherSetLatch = new CountDownLatch(1);
            Flowable<Mqtt5Publish> publishFlowable = Flowable.create(flowableEmitter -> {
                setPublishConsumer(flowableEmitter::onNext);
                publisherSetLatch.countDown();
            }, BackpressureStrategy.BUFFER);
            logger.info("Setting up publishing pipeline using {}", publishFlowable);
            client.publish(publishFlowable).subscribe(mqtt5PublishResult -> {
                logger.debug("Publish result: {}", mqtt5PublishResult);
                mqtt5PublishResult.getError().ifPresent(e -> {
                    logger.debug("Retrying {}", mqtt5PublishResult.getPublish());
                    publishConsumer.accept(mqtt5PublishResult.getPublish());
                });
            }, throwable -> logger.error("Publish encountered error.", throwable));
            try {
                publisherSetLatch.await(10L, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                logger.warn("Unable to set publisher in time. Please check logs for possible cause.");
                throw new JoynrRuntimeException("Unable to set publisher in time.", e);
            }
        } else {
            logger.info("publishConsumer already set up for {} - skipping.", client);
        }
    }

    @Override
    public void setMessageListener(IMqttMessagingSkeleton rawMessaging) {
        this.messagingSkeleton = rawMessaging;
    }

    @Override
    public synchronized void shutdown() {
        this.shuttingDown = true;
        client.disconnect().blockingAwait();
    }

    @Override
    public void publishMessage(String topic, byte[] serializedMessage) {
        publishMessage(topic, serializedMessage, MqttQos.AT_LEAST_ONCE.getCode());
    }

    @Override
    public void publishMessage(String topic, byte[] serializedMessage, int qosLevel) {
        if (publishConsumer == null) {
            logger.debug("Publishing to {} with qos {} failed: publishConsumer not set", topic, qosLevel);
            throw new JoynrDelayMessageException("MQTT Publish failed: publishConsumer has not been set yet");
        }
        logger.debug("Publishing to {} with qos {} using {}", topic, qosLevel, publishConsumer);
        Mqtt5Publish mqtt5Publish = Mqtt5Publish.builder()
                                                .topic(topic)
                                                .qos(safeParseQos(qosLevel))
                                                .payload(serializedMessage)
                                                .build();
        publishConsumer.accept(mqtt5Publish);
    }

    private MqttQos safeParseQos(int qosLevel) {
        MqttQos result = MqttQos.fromCode(qosLevel);
        if (result == null) {
            result = MqttQos.AT_LEAST_ONCE;
        }
        return result;
    }

    @Override
    public void subscribe(String topic) {
        logger.info("Subscribing to {}", topic);
        Mqtt5Subscription subscription = subscriptions.computeIfAbsent(topic,
                                                                       (t) -> Mqtt5Subscription.builder()
                                                                                               .topicFilter(t)
                                                                                               .qos(MqttQos.AT_LEAST_ONCE) // TODO make configurable
                                                                                               .build());
        doSubscribe(subscription);
    }

    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the subscribe via callbacks.")
    private void doSubscribe(Mqtt5Subscription subscription) {
        Mqtt5Subscribe subscribe = Mqtt5Subscribe.builder().addSubscription(subscription).build();
        client.subscribeStream(subscribe)
              .doOnSingle(mqtt5SubAck -> logger.debug("Subscribed to {} with result {}", subscription, mqtt5SubAck))
              .subscribe(this::handleIncomingMessage,
                         throwable -> logger.error("Error encountered for subscription {}.", subscription, throwable));
    }

    public void resubscribe() {
        logger.debug("Resubscribe triggered.");
        if (disconnectCount.get() > 0) {
            logger.trace("Disconnect count > 0, performing resubscribe.");
            disconnectCount.decrementAndGet();
            subscriptions.forEach((topic, subscription) -> {
                logger.info("Resubscribing to {}", topic);
                doSubscribe(subscription);
            });
        } else {
            logger.trace("Disconnect count 0, skipping resubscribe.");
        }
    }

    @Override
    public void unsubscribe(String topic) {
        logger.info("Unsubscribing from {}", topic);
        subscriptions.remove(topic);
        Mqtt5Unsubscribe unsubscribe = Mqtt5Unsubscribe.builder().addTopicFilter(topic).build();
        client.unsubscribe(unsubscribe)
              .doOnSuccess((unused) -> logger.debug("Unsubscribed from {}", topic))
              .doOnError(throwable -> logger.error("Unable to unsubscribe from {}", topic, throwable))
              .subscribe();
    }

    @Override
    public synchronized boolean isShutdown() {
        return shuttingDown;
    }

    private void setPublishConsumer(Consumer<Mqtt5Publish> publishConsumer) {
        logger.info("Setting publishConsumer to: {}", publishConsumer);
        this.publishConsumer = publishConsumer;
    }

    private void handleIncomingMessage(Mqtt5Publish mqtt5Publish) {
        logger.trace("Incoming message {} received by {}", mqtt5Publish, this);
        messagingSkeleton.transmit(mqtt5Publish.getPayloadAsBytes(),
                                   throwable -> logger.error("Unable to transmit {}", mqtt5Publish, throwable));
    }

    void incrementDisconnectCount() {
        disconnectCount.incrementAndGet();
    }
}

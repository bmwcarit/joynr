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
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

import edu.umd.cs.findbugs.annotations.SuppressFBWarnings;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.mqtt3.Mqtt3RxClient;
import com.hivemq.client.mqtt.mqtt3.message.connect.Mqtt3Connect;
import com.hivemq.client.mqtt.mqtt3.message.publish.Mqtt3Publish;
import com.hivemq.client.mqtt.mqtt3.message.subscribe.Mqtt3Subscribe;
import com.hivemq.client.mqtt.mqtt3.message.subscribe.Mqtt3Subscription;
import com.hivemq.client.mqtt.mqtt3.message.unsubscribe.Mqtt3Unsubscribe;

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

    private final Mqtt3RxClient client;
    private final boolean cleanSession;
    private Consumer<Mqtt3Publish> publishConsumer;
    private IMqttMessagingSkeleton messagingSkeleton;
    private int keepAliveTimeSeconds;
    private int connectionTimeoutSec;
    private volatile boolean shuttingDown;

    private Map<String, Mqtt3Subscription> subscriptions = new ConcurrentHashMap<>();

    public HivemqMqttClient(Mqtt3RxClient client,
                            int keepAliveTimeSeconds,
                            boolean cleanSession,
                            int connectionTimeoutSec) {
        this.client = client;
        this.keepAliveTimeSeconds = keepAliveTimeSeconds;
        this.cleanSession = cleanSession;
        this.connectionTimeoutSec = connectionTimeoutSec;
    }

    @Override
    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the connect via callbacks.")
    public synchronized void start() {
        logger.info("Initialising MQTT client {} -> {}", this, client);
        if (!client.getConfig().getState().isConnected()) {
            while (!client.getConfig().getState().isConnected()) {
                logger.info("Attempting to connect client {} (clean session {}) ...", client, cleanSession);
                Mqtt3Connect mqtt3Connect = Mqtt3Connect.builder()
                                                        .cleanSession(cleanSession)
                                                        .keepAlive(keepAliveTimeSeconds)
                                                        .build();
                try {
                    client.connect(mqtt3Connect)
                          .timeout(connectionTimeoutSec, TimeUnit.SECONDS)
                          .doOnSuccess(connAck -> logger.info("MQTT client {} connected: {}.", client, connAck))
                          .doOnError(throwable -> logger.error("Unable to connect MQTT client {}.", client, throwable))
                          .blockingGet();
                } catch (RuntimeException e) {
                    logger.error("Exception while connecting MQTT client.", e);
                }
            }
            logger.info("MQTT client {} connected.", client);
        } else {
            logger.info("MQTT client {} already connected - skipping.", client);
        }
        if (publishConsumer == null) {
            logger.info("Setting up publishConsumer for {}", client);
            Flowable<Mqtt3Publish> publishFlowable = Flowable.create(flowableEmitter -> {
                setPublishConsumer(flowableEmitter::onNext);
            }, BackpressureStrategy.BUFFER);
            logger.info("Setting up publishing pipeline using {}", publishFlowable);
            client.publish(publishFlowable).doOnNext(mqtt3PublishResult -> {
                logger.debug("Publish result: {}", mqtt3PublishResult);
                mqtt3PublishResult.getError().ifPresent(e -> {
                    logger.debug("Retrying {}", mqtt3PublishResult.getPublish());
                    publishConsumer.accept(mqtt3PublishResult.getPublish());
                });
            }).doOnError(throwable -> logger.error("Publish encountered error.", throwable)).subscribe();
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
        logger.debug("Publishing to {} with qos {} using {}", topic, qosLevel, publishConsumer);
        Mqtt3Publish mqtt3Publish = Mqtt3Publish.builder()
                                                .topic(topic)
                                                .qos(safeParseQos(qosLevel))
                                                .payload(serializedMessage)
                                                .build();
        publishConsumer.accept(mqtt3Publish);
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
        Mqtt3Subscription subscription = subscriptions.computeIfAbsent(topic,
                                                                       (t) -> Mqtt3Subscription.builder()
                                                                                               .topicFilter(t)
                                                                                               .qos(MqttQos.AT_LEAST_ONCE) // TODO make configurable
                                                                                               .build());
        doSubscribe(subscription);
    }

    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the subscribe via callbacks.")
    private void doSubscribe(Mqtt3Subscription subscription) {
        Mqtt3Subscribe subscribe = Mqtt3Subscribe.builder().addSubscription(subscription).build();
        client.subscribeStream(subscribe)
              .doOnSingle(mqtt3SubAck -> logger.debug("Subscribed to {} with result {}", subscription, mqtt3SubAck))
              .subscribe(mqtt3Publish -> messagingSkeleton.transmit(mqtt3Publish.getPayloadAsBytes(),
                                                                    throwable -> logger.error("Unable to transmit {}",
                                                                                              mqtt3Publish,
                                                                                              throwable)),
                         throwable -> logger.error("Error encountered for subscription {}.", subscription, throwable));
    }

    public void resubscribe() {
        logger.debug("Resubscribe triggered.");
        subscriptions.forEach((topic, subscription) -> {
            logger.info("Resubscribing to {}", topic);
            doSubscribe(subscription);
        });
    }

    @Override
    public void unsubscribe(String topic) {
        logger.info("Unsubscribing from {}", topic);
        subscriptions.remove(topic);
        Mqtt3Unsubscribe unsubscribe = Mqtt3Unsubscribe.builder().addTopicFilter(topic).build();
        client.unsubscribe(unsubscribe)
              .doOnComplete(() -> logger.debug("Unsubscribed from {}", topic))
              .doOnError(throwable -> logger.error("Unable to unsubscribe from {}", topic, throwable))
              .subscribe();
    }

    @Override
    public synchronized boolean isShutdown() {
        return shuttingDown;
    }

    private void setPublishConsumer(Consumer<Mqtt3Publish> publishConsumer) {
        logger.info("Setting publishConsumer to: {}", publishConsumer);
        this.publishConsumer = publishConsumer;
    }
}

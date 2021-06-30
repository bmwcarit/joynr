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

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.hivemq.client.mqtt.MqttClientState;
import com.hivemq.client.mqtt.MqttGlobalPublishFilter;
import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.exceptions.MqttClientStateException;
import com.hivemq.client.mqtt.exceptions.MqttSessionExpiredException;
import com.hivemq.client.mqtt.mqtt5.Mqtt5ClientConfig;
import com.hivemq.client.mqtt.mqtt5.Mqtt5RxClient;
import com.hivemq.client.mqtt.mqtt5.message.connect.Mqtt5Connect;
import com.hivemq.client.mqtt.mqtt5.message.connect.connack.Mqtt5ConnAckRestrictions;
import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;
import com.hivemq.client.mqtt.mqtt5.message.subscribe.Mqtt5Subscribe;
import com.hivemq.client.mqtt.mqtt5.message.subscribe.Mqtt5Subscription;
import com.hivemq.client.mqtt.mqtt5.message.unsubscribe.Mqtt5Unsubscribe;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.statusmetrics.ConnectionStatusMetricsImpl;
import io.reactivex.disposables.Disposable;

/**
 * This implements the {@link JoynrMqttClient} using the HiveMQ MQTT Client library.
 *
 * TODO
 * - HiveMQ MQTT Client offers better backpressure handling via rxJava, hence revisit this issue to see how we can make
 *   use of this for our own backpressure handling
 */
public class HivemqMqttClient implements JoynrMqttClient {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClient.class);
    private static final long NOT_CONNECTED_RETRY_INTERVAL_MS = 5000;

    private final Mqtt5RxClient client;
    private final Mqtt5ClientConfig clientConfig;
    private int maxMsgSizeBytes = Mqtt5ConnAckRestrictions.DEFAULT_MAXIMUM_PACKET_SIZE_NO_LIMIT;
    private final boolean cleanSession;
    private final int keepAliveTimeSeconds;
    private final int connectionTimeoutSec;
    private final int reconnectDelayMs;
    private final boolean isReceiver;
    private final boolean isSender;
    private final String clientInformation;
    private volatile boolean shuttingDown;
    private IMqttMessagingSkeleton messagingSkeleton;
    private ConnectionStatusMetricsImpl connectionStatusMetrics;

    private Disposable publishesDisposable;
    private HashMap<String, Disposable> subscriptionDisposables = new HashMap<>();
    private AtomicLong obsoleteUnsubscribeDisposableCount = new AtomicLong(0);
    private List<Disposable> unsubscribeDisposables = new ArrayList<>();

    private Map<String, Mqtt5Subscription> subscriptions = new HashMap<>();

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public HivemqMqttClient(Mqtt5RxClient client,
                            int keepAliveTimeSeconds,
                            boolean cleanSession,
                            int connectionTimeoutSec,
                            int reconnectDelayMs,
                            boolean isReceiver,
                            boolean isSender,
                            String gbid,
                            ConnectionStatusMetricsImpl connectionStatusMetrics) {
        this.client = client;
        clientConfig = client.getConfig();
        this.keepAliveTimeSeconds = keepAliveTimeSeconds;
        this.cleanSession = cleanSession;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.reconnectDelayMs = reconnectDelayMs;
        this.isReceiver = isReceiver;
        this.isSender = isSender;
        clientInformation = createClientInformationString(gbid);
        this.connectionStatusMetrics = connectionStatusMetrics;
        this.publishesDisposable = null;
        shuttingDown = false;
    }

    private void registerPublishCallback() {
        if (!isReceiver || publishesDisposable != null) {
            return;
        }
        publishesDisposable = client.publishes(MqttGlobalPublishFilter.ALL).subscribe(this::handleIncomingMessage,
                                                                                      throwable -> {
                                                                                          if (!cleanSession
                                                                                                  && throwable instanceof MqttSessionExpiredException) {
                                                                                              logger.warn("{}: MqttSessionExpiredException encountered in publish callback, trying to resubscribe.",
                                                                                                          clientInformation,
                                                                                                          throwable);
                                                                                          } else {
                                                                                              logger.error("{}: Error encountered in publish callback, trying to resubscribe.",
                                                                                                           clientInformation,
                                                                                                           throwable);
                                                                                          }
                                                                                          synchronized (this) {
                                                                                              if (publishesDisposable != null) {
                                                                                                  publishesDisposable.dispose();
                                                                                                  publishesDisposable = null;
                                                                                              }
                                                                                              registerPublishCallback();
                                                                                          }
                                                                                      });
    }

    String getClientInformationString() {
        return clientInformation;
    }

    private String createClientInformationString(final String gbid) {
        StringBuilder clientIdBuilder = new StringBuilder();
        clientIdBuilder.append("(clientHash=");
        clientIdBuilder.append(Integer.toHexString(client.hashCode()));
        clientIdBuilder.append(", GBID=");
        clientIdBuilder.append(gbid);
        clientIdBuilder.append(", ");
        if (isReceiver && isSender) {
            clientIdBuilder.append("bidirectional");
        } else if (isReceiver) {
            clientIdBuilder.append("receiver");
        } else {
            clientIdBuilder.append("sender");
        }
        clientIdBuilder.append(")");
        return clientIdBuilder.toString();
    }

    @Override
    public synchronized void start() {
        shuttingDown = false;
        logger.info("{}: Initializing HiveMQ MQTT client for address {}.",
                    clientInformation,
                    client.getConfig().getServerAddress());

        assert (!isReceiver || messagingSkeleton != null);
        registerPublishCallback();

        if (!client.getConfig().getState().isConnected()) {
            while (!client.getConfig().getState().isConnected()) {
                logger.info("{}: Attempting to connect client, clean session={} ...", clientInformation, cleanSession);
                Mqtt5Connect mqtt5Connect = Mqtt5Connect.builder()
                                                        .cleanStart(cleanSession)
                                                        .keepAlive(keepAliveTimeSeconds)
                                                        .noSessionExpiry()
                                                        .build();
                try {
                    connectionStatusMetrics.increaseConnectionAttempts();
                    client.connect(mqtt5Connect)
                          .timeout(connectionTimeoutSec, TimeUnit.SECONDS)
                          .doOnSuccess(connAck -> {
                              maxMsgSizeBytes = connAck.getRestrictions().getMaximumPacketSize();
                              logger.info("{}: MQTT client connected: {}, maxMsgSizeBytes = {}.",
                                          clientInformation,
                                          connAck,
                                          maxMsgSizeBytes);
                          })
                          .blockingGet();
                } catch (Exception e) {
                    logger.error("{}: Exception encountered while connecting MQTT client.", clientInformation, e);
                    do {
                        try {
                            logger.trace("{}: Waiting to reconnect, state: {}.",
                                         clientInformation,
                                         client.getConfig().getState());
                            wait(reconnectDelayMs);
                        } catch (Exception exception) {
                            logger.error("{}: Exception while waiting to reconnect.", clientInformation, exception);
                        }
                        // do while state != CONNECTED and state != DISCONNECTED
                    } while (client.getConfig().getState() == MqttClientState.CONNECTING
                            || client.getConfig().getState() == MqttClientState.CONNECTING_RECONNECT
                            || client.getConfig().getState() == MqttClientState.DISCONNECTED_RECONNECT);
                }
            }
        } else {
            logger.info("{}: MQTT client already connected - skipping.", clientInformation);
        }
    }

    @Override
    public void setMessageListener(IMqttMessagingSkeleton rawMessaging) {
        assert (isReceiver && messagingSkeleton == null);
        this.messagingSkeleton = rawMessaging;
    }

    @Override
    public synchronized void shutdown() {
        if (!shuttingDown) {
            try {
                logger.info("{}: Attempting to shutdown connection.", clientInformation);
                shuttingDown = true;
                client.disconnectWith().noSessionExpiry().applyDisconnect().doOnComplete(() -> {
                    logger.info("{}: Disconnected.", clientInformation);
                }).onErrorComplete(throwable -> {
                    logger.error("{}: Error encountered from disconnect.", clientInformation, throwable);
                    return true;
                }).blockingAwait(5000, TimeUnit.MILLISECONDS);
            } catch (Exception e) {
                logger.error("{}: Exception thrown on disconnect.", clientInformation, e.getMessage());
            }
            if (publishesDisposable != null) {
                publishesDisposable.dispose();
                publishesDisposable = null;
            }
            synchronized (subscriptions) {
                disposeSubscriptions();
                for (Disposable unsubscribeDisposable : unsubscribeDisposables) {
                    unsubscribeDisposable.dispose();
                }
                obsoleteUnsubscribeDisposableCount.set(0);
            }
        }
    }

    /**
     * Dispose reactivex subscriptions for MQTT subscriptions to release internal threads and unblock shutdown.
     */
    private void disposeSubscriptions() {
        for (Disposable subscriptionDisposable : subscriptionDisposables.values()) {
            subscriptionDisposable.dispose();
        }
        subscriptionDisposables.clear();
    }

    @Override
    public void publishMessage(String topic,
                               byte[] serializedMessage,
                               int qosLevel,
                               long messageExpiryIntervalSec,
                               SuccessAction successAction,
                               FailureAction failureAction) {
        assert (isSender);

        if (maxMsgSizeBytes != 0 && serializedMessage.length > maxMsgSizeBytes) {
            throw new JoynrMessageNotSentException("Publish failed: maximum allowed message size of " + maxMsgSizeBytes
                    + " bytes exceeded, actual size is " + serializedMessage.length + " bytes");
        }

        if (!clientConfig.getState().isConnected()) {
            failureAction.execute(new JoynrDelayMessageException(NOT_CONNECTED_RETRY_INTERVAL_MS,
                                                                 "Publish failed: Mqtt client not connected."));
            return;
        }

        Mqtt5Publish mqtt5Publish = Mqtt5Publish.builder()
                                                .topic(topic)
                                                .qos(safeParseQos(qosLevel))
                                                .payload(serializedMessage)
                                                .messageExpiryInterval(messageExpiryIntervalSec)
                                                .build();
        logger.debug("{}: Publishing to topic: {}, size: {}, qos: {}",
                     clientInformation,
                     topic,
                     serializedMessage.length,
                     qosLevel);
        client.toAsync().publish(mqtt5Publish).whenComplete((publishResult, throwable) -> {
            if (throwable != null) {
                logger.error("{}: Publishing to topic: {}, size: {}, qos: {} failed with exception.",
                             clientInformation,
                             topic,
                             serializedMessage.length,
                             qosLevel,
                             throwable);
                if (throwable instanceof MqttClientStateException) {
                    failureAction.execute(new JoynrDelayMessageException(NOT_CONNECTED_RETRY_INTERVAL_MS,
                                                                         "Publish failed: " + throwable.toString()));
                } else {
                    failureAction.execute(new JoynrDelayMessageException("Publish failed: " + throwable.toString()));
                }
            } else if (publishResult.getError().isPresent()) {
                logger.error("{}: Publishing to topic: {}, size: {}, qos: {} failed with error result: {}",
                             clientInformation,
                             topic,
                             serializedMessage.length,
                             qosLevel,
                             publishResult,
                             publishResult.getError().get());
                failureAction.execute(new JoynrDelayMessageException("Publish failed: "
                        + publishResult.getError().get().toString()));
            } else {
                connectionStatusMetrics.increaseSentMessages();
                if (logger.isTraceEnabled()) {
                    logger.trace("{}: Publishing to topic: {}, size: {}, qos: {} succeeded: {}",
                                 clientInformation,
                                 topic,
                                 serializedMessage.length,
                                 qosLevel,
                                 publishResult);
                } else {
                    logger.debug("{}: Publishing to topic: {}, size: {}, qos: {} succeeded.",
                                 clientInformation,
                                 topic,
                                 serializedMessage.length,
                                 qosLevel);

                }
                successAction.execute();
            }
        });
    }

    private MqttQos safeParseQos(int qosLevel) {
        MqttQos result = MqttQos.fromCode(qosLevel);
        if (result == null) {
            result = MqttQos.AT_LEAST_ONCE;
            logger.error("{}: Got invalid QoS level {} for publish, using default: {}",
                         clientInformation,
                         qosLevel,
                         result.getCode());
        }
        return result;
    }

    @Override
    public void subscribe(String topic) {
        assert (isReceiver);
        synchronized (subscriptions) {
            logger.info("{}: Subscribing to topic: {}", clientInformation, topic);
            Mqtt5Subscription subscription = subscriptions.computeIfAbsent(topic,
                                                                           (t) -> Mqtt5Subscription.builder()
                                                                                                   .topicFilter(t)
                                                                                                   .qos(MqttQos.AT_LEAST_ONCE) // TODO make configurable
                                                                                                   .build());

            if (isShutdown()) {
                return;
            }
            doSubscribe(subscription, topic);
        }
    }

    private void doSubscribe(Mqtt5Subscription subscription, String topic) {
        Mqtt5Subscribe subscribe = Mqtt5Subscribe.builder().addSubscription(subscription).build();
        Disposable disposable = client.subscribeStream(subscribe)
                                      .doOnSingle(mqtt5SubAck -> logger.debug("{}: Subscribed to topic: {}, result: {}",
                                                                              clientInformation,
                                                                              subscription,
                                                                              mqtt5SubAck))
                                      .subscribe(mqtt5Publish -> {
                                          /* handled in general publish callback */ },
                                                 throwable -> logger.error("{}: Error encountered for subscription {}.",
                                                                           clientInformation,
                                                                           subscription,
                                                                           throwable));
        subscriptionDisposables.put(topic, disposable);
    }

    public void resubscribe() {
        logger.debug("{}: Resubscribe triggered.", clientInformation);
        synchronized (subscriptions) {
            disposeSubscriptions();
            subscriptions.forEach((topic, subscription) -> {
                logger.info("{}: Resubscribing to topic: {}", clientInformation, topic);
                doSubscribe(subscription, topic);
            });
        }
    }

    @Override
    public void unsubscribe(String topic) {
        assert (isReceiver);
        Mqtt5Unsubscribe unsubscribe = Mqtt5Unsubscribe.builder().addTopicFilter(topic).build();
        synchronized (subscriptions) {
            logger.info("{}: Unsubscribing from topic: {}", clientInformation, topic);
            subscriptions.remove(topic);
            AtomicBoolean callbackCalled = new AtomicBoolean(false);
            Disposable unsubscribeDisposable = client.unsubscribe(unsubscribe).subscribe((unused) -> {
                logger.debug("{}: Unsubscribed from topic: {}", clientInformation, topic);
                if (callbackCalled.compareAndSet(false, true)) {
                    obsoleteUnsubscribeDisposableCount.getAndIncrement();
                }
            }, throwable -> {
                logger.error("{}: Unable to unsubscribe from topic: {}", clientInformation, topic, throwable);
                if (callbackCalled.compareAndSet(false, true)) {
                    obsoleteUnsubscribeDisposableCount.getAndIncrement();
                }
            });
            unsubscribeDisposables.add(unsubscribeDisposable);
            Disposable disposable = subscriptionDisposables.remove(topic);
            if (disposable != null) {
                disposable.dispose();
            }
            while (obsoleteUnsubscribeDisposableCount.get() > 0) {
                unsubscribeDisposable = unsubscribeDisposables.remove(0);
                if (unsubscribeDisposable != null) {
                    unsubscribeDisposable.dispose();
                }
                obsoleteUnsubscribeDisposableCount.decrementAndGet();
            }
        }
    }

    @Override
    public synchronized boolean isShutdown() {
        return shuttingDown;
    }

    private void handleIncomingMessage(Mqtt5Publish mqtt5Publish) {
        if (logger.isDebugEnabled()) {
            ByteBuffer payload = mqtt5Publish.getPayload().orElse(null);
            logger.debug("{}: Received publication: topic: {}, size: {}, qos: {}, retain: {}, expiryInterval: {}.",
                         clientInformation,
                         mqtt5Publish.getTopic(),
                         ((payload == null) ? 0 : payload.remaining()),
                         mqtt5Publish.getQos(),
                         mqtt5Publish.isRetain(),
                         mqtt5Publish.getMessageExpiryInterval().orElse(0));
        }
        connectionStatusMetrics.increaseReceivedMessages();
        messagingSkeleton.transmit(mqtt5Publish.getPayloadAsBytes(), (throwable) -> {
            if (throwable instanceof JoynrMessageExpiredException) {
                logger.warn("{}: Unable to handle incoming {}", clientInformation, mqtt5Publish, throwable);
            } else {
                logger.error("{}: Unable to handle incoming {}", clientInformation, mqtt5Publish, throwable);
            }
        });
    }

    // for testing
    Mqtt5RxClient getClient() {
        return client;
    }
}

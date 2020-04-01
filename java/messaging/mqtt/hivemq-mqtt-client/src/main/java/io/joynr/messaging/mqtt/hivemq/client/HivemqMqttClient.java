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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.hivemq.client.mqtt.MqttClientState;
import com.hivemq.client.mqtt.MqttGlobalPublishFilter;
import com.hivemq.client.mqtt.datatypes.MqttQos;
import com.hivemq.client.mqtt.exceptions.MqttClientStateException;
import com.hivemq.client.mqtt.exceptions.MqttSessionExpiredException;
import com.hivemq.client.mqtt.mqtt3.Mqtt3ClientConfig;
import com.hivemq.client.mqtt.mqtt3.Mqtt3RxClient;
import com.hivemq.client.mqtt.mqtt3.message.connect.Mqtt3Connect;
import com.hivemq.client.mqtt.mqtt3.message.publish.Mqtt3Publish;
import com.hivemq.client.mqtt.mqtt3.message.subscribe.Mqtt3Subscribe;
import com.hivemq.client.mqtt.mqtt3.message.subscribe.Mqtt3Subscription;
import com.hivemq.client.mqtt.mqtt3.message.unsubscribe.Mqtt3Unsubscribe;

import edu.umd.cs.findbugs.annotations.SuppressFBWarnings;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;

/**
 * This implements the {@link JoynrMqttClient} using the HiveMQ MQTT Client library.
 *
 * TODO
 * - HiveMQ MQTT Client offers better backpressure handling via rxJava, hence revisit this issue to see how we can make
 *   use of this for our own backpressure handling
 */
public class HivemqMqttClient implements JoynrMqttClient {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClient.class);
    private static final long NOT_CONNECTED_RETRY_INTERVAL_MS = 60000;

    private final Mqtt3RxClient client;
    private final Mqtt3ClientConfig clientConfig;
    private final boolean cleanSession;
    private final int keepAliveTimeSeconds;
    private final int connectionTimeoutSec;
    private final int reconnectDelayMs;
    private final boolean isReceiver;
    private final boolean isSender;
    private final String clientInformation;
    private volatile boolean shuttingDown;
    private IMqttMessagingSkeleton messagingSkeleton;

    private Map<String, Mqtt3Subscription> subscriptions = new ConcurrentHashMap<>();

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public HivemqMqttClient(Mqtt3RxClient client,
                            int keepAliveTimeSeconds,
                            boolean cleanSession,
                            int connectionTimeoutSec,
                            int reconnectDelayMs,
                            boolean isReceiver,
                            boolean isSender) {
        this.client = client;
        clientConfig = client.getConfig();
        this.keepAliveTimeSeconds = keepAliveTimeSeconds;
        this.cleanSession = cleanSession;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.reconnectDelayMs = reconnectDelayMs;
        this.isReceiver = isReceiver;
        this.isSender = isSender;
        clientInformation = createClientInformationString();
        shuttingDown = false;
        if (isReceiver) {
            registerPublishCallback();
        }
    }

    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the connect via callbacks.")
    private void registerPublishCallback() {
        client.publishes(MqttGlobalPublishFilter.ALL).subscribe(this::handleIncomingMessage, throwable -> {
            if (!cleanSession && throwable instanceof MqttSessionExpiredException) {
                logger.warn("{}: MqttSessionExpiredException encountered in publish callback, trying to resubscribe.",
                            clientInformation,
                            throwable);
                registerPublishCallback();
            } else {
                logger.error("{}: Error encountered in publish callback, trying to resubscribe.",
                             clientInformation,
                             throwable);
                registerPublishCallback();
            }
        });
    }

    String getClientInformationString() {
        return clientInformation;
    }

    private String createClientInformationString() {
        StringBuilder clientIdBuilder = new StringBuilder();
        clientIdBuilder.append("(clientHash=");
        clientIdBuilder.append(Integer.toHexString(client.hashCode()));
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
    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the connect via callbacks.")
    public synchronized void start() {
        logger.info("{}: Initializing HiveMQ MQTT client for address {}.",
                    clientInformation,
                    client.getConfig().getServerAddress());
        assert (!isReceiver || messagingSkeleton != null);
        if (!client.getConfig().getState().isConnected()) {
            while (!client.getConfig().getState().isConnected()) {
                logger.info("{}: Attempting to connect client, clean session={} ...", clientInformation, cleanSession);
                Mqtt3Connect mqtt3Connect = Mqtt3Connect.builder()
                                                        .cleanSession(cleanSession)
                                                        .keepAlive(keepAliveTimeSeconds)
                                                        .build();
                try {
                    client.connect(mqtt3Connect)
                          .timeout(connectionTimeoutSec, TimeUnit.SECONDS)
                          .doOnSuccess(connAck -> logger.info("{}: MQTT client connected: {}.",
                                                              clientInformation,
                                                              connAck))
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
        logger.info("{}: Attempting to shutdown connection.", clientInformation);
        this.shuttingDown = true;
        client.disconnect()
              .doOnComplete(() -> logger.debug("{}: Disconnected.", clientInformation))
              .onErrorComplete(throwable -> {
                  logger.error("{}: Error encountered from disconnect.", clientInformation, throwable);
                  return true;
              })
              .blockingAwait();
    }

    @Override
    public void publishMessage(String topic,
                               byte[] serializedMessage,
                               int qosLevel,
                               SuccessAction successAction,
                               FailureAction failureAction) {
        assert (isSender);
        if (!clientConfig.getState().isConnected()) {
            failureAction.execute(new JoynrDelayMessageException(NOT_CONNECTED_RETRY_INTERVAL_MS,
                                                                 "Publish failed: Mqtt client not connected."));
            return;
        }

        Mqtt3Publish mqtt3Publish = Mqtt3Publish.builder()
                                                .topic(topic)
                                                .qos(safeParseQos(qosLevel))
                                                .payload(serializedMessage)
                                                .build();
        logger.debug("{}: Publishing to {}: {}bytes with qos {}",
                     clientInformation,
                     topic,
                     serializedMessage.length,
                     qosLevel);
        client.toAsync().publish(mqtt3Publish).whenComplete((publishResult, throwable) -> {
            if (throwable == null) {
                logger.trace("{}: Publishing to {}: {}bytes with qos {} succeeded: {}",
                             clientInformation,
                             topic,
                             serializedMessage.length,
                             qosLevel,
                             publishResult);
                successAction.execute();
            } else {
                logger.error("{}: Publishing to {}: {}bytes with qos {} failed with exception.",
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
        logger.info("{}: Subscribing to {}", clientInformation, topic);
        Mqtt3Subscription subscription = subscriptions.computeIfAbsent(topic,
                                                                       (t) -> Mqtt3Subscription.builder()
                                                                                               .topicFilter(t)
                                                                                               .qos(MqttQos.AT_LEAST_ONCE) // TODO make configurable
                                                                                               .build());
        doSubscribe(subscription);
    }

    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the connect via callbacks.")
    private void doSubscribe(Mqtt3Subscription subscription) {
        Mqtt3Subscribe subscribe = Mqtt3Subscribe.builder().addSubscription(subscription).build();
        client.subscribeStream(subscribe)
              .doOnSingle(mqtt3SubAck -> logger.debug("{}: Subscribed to {} with result {}",
                                                      clientInformation,
                                                      subscription,
                                                      mqtt3SubAck))
              .subscribe(mqtt3Publish -> {
                  /* handled in general publish callback */ },
                         throwable -> logger.error("{}: Error encountered for subscription {}.",
                                                   clientInformation,
                                                   subscription,
                                                   throwable));
    }

    public void resubscribe() {
        logger.debug("{}: Resubscribe triggered.", clientInformation);
        subscriptions.forEach((topic, subscription) -> {
            logger.info("{}: Resubscribing to {}", clientInformation, topic);
            doSubscribe(subscription);
        });
    }

    @Override
    @SuppressFBWarnings(value = "RV_RETURN_VALUE_IGNORED", justification = "We handle the connect via callbacks.")
    public void unsubscribe(String topic) {
        assert (isReceiver);
        logger.info("{}: Unsubscribing from {}", clientInformation, topic);
        subscriptions.remove(topic);
        Mqtt3Unsubscribe unsubscribe = Mqtt3Unsubscribe.builder().addTopicFilter(topic).build();
        client.unsubscribe(unsubscribe)
              .doOnComplete(() -> logger.debug("{}: Unsubscribed from {}", clientInformation, topic))
              .onErrorComplete(throwable -> {
                  logger.error("{}: Unable to unsubscribe from {}", clientInformation, topic, throwable);
                  return true;
              });
    }

    @Override
    public synchronized boolean isShutdown() {
        return shuttingDown;
    }

    private void handleIncomingMessage(Mqtt3Publish mqtt3Publish) {
        logger.trace("{}: Incoming: {}.", clientInformation, mqtt3Publish);
        messagingSkeleton.transmit(mqtt3Publish.getPayloadAsBytes(),
                                   throwable -> logger.error("{}: Unable to transmit {}",
                                                             clientInformation,
                                                             mqtt3Publish,
                                                             throwable));
    }

    // for testing
    Mqtt3RxClient getClient() {
        return client;
    }
}

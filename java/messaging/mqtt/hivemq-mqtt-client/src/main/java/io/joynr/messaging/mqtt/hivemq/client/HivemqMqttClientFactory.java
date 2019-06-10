package io.joynr.messaging.mqtt.hivemq.client;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.concurrent.ScheduledExecutorService;

import com.google.inject.Singleton;
import com.hivemq.client.mqtt.MqttClient;
import com.hivemq.client.mqtt.MqttClientExecutorConfig;
import com.hivemq.client.mqtt.mqtt3.Mqtt3Client;
import com.hivemq.client.mqtt.mqtt3.Mqtt3ClientBuilder;
import com.hivemq.client.mqtt.mqtt3.Mqtt3RxClient;
import io.joynr.messaging.routing.MessageRouter;
import io.reactivex.schedulers.Schedulers;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import joynr.system.RoutingTypes.MqttAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * This factory class is responsible for producing joynr MQTT clients using the mqtt-bee library.
 * The mqtt-bee library is explicitly intended for use in backend applications, so using it on
 * constrained devices might not work well. Consider using the Paho client in those circumstances.
 *
 * Currently mqtt-bee is pre 1.0. Here are a list of things that need doing once the relevant missing
 * feature have been completed in mqtt-bee and we want to make this integration production ready:
 *
 * TODO
 * - Add ability to configure SSL setup (trust store / key store etc)
 * - When auto-reconnect feature exists, then enable it
 * - When persistent session configuration exists, then enable configuration thereof
 * - Document usage and configuration of mqtt-bee variant
 */
@Singleton
public class HivemqMqttClientFactory implements MqttClientFactory {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClientFactory.class);

    private final MqttAddress ownAddress;
    private final boolean separateConnections;
    private final MqttClientIdProvider mqttClientIdProvider;
    private final ScheduledExecutorService scheduledExecutorService;
    private final int keepAliveTimeSeconds;

    private JoynrMqttClient sender;
    private JoynrMqttClient receiver;

    @Inject
    public HivemqMqttClientFactory(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS) boolean separateConnections,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC) int keepAliveTimeSeconds,
                                   @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduledExecutorService,
                                   MqttClientIdProvider mqttClientIdProvider) {
        this.ownAddress = ownAddress;
        this.separateConnections = separateConnections;
        this.scheduledExecutorService = scheduledExecutorService;
        this.mqttClientIdProvider = mqttClientIdProvider;
        this.keepAliveTimeSeconds = keepAliveTimeSeconds;
    }

    @Override
    public JoynrMqttClient createSender() {
        if (sender == null) {
            synchronized (this) {
                if (sender == null) {
                    logger.info("Creating sender MQTT client");
                    sender = createClient(mqttClientIdProvider.getClientId() + (separateConnections ? "Pub" : ""));
                    logger.info("Sender MQTT client now: {}", sender);
                }
            }
        }
        return sender;
    }

    @Override
    public JoynrMqttClient createReceiver() {
        if (receiver == null) {
            synchronized (this) {
                if (receiver == null) {
                    logger.info("Creating receiver MQTT client");
                    if (separateConnections) {
                        receiver = createClient(mqttClientIdProvider.getClientId() + "Sub");
                    } else {
                        receiver = createSender();
                    }
                    logger.info("Receiver MQTT client now: {}", receiver);
                }
            }
        }
        return receiver;
    }

    private JoynrMqttClient createClient(String clientId) {
        try {
            URI serverUri = new URI(ownAddress.getBrokerUri());
            logger.info("Connecting to {}:{}", serverUri.getHost(), serverUri.getPort());
            MqttClientExecutorConfig executorConfig = MqttClientExecutorConfig.builder()
                                                                              .nettyExecutor(scheduledExecutorService)
                                                                              .applicationScheduler(Schedulers.from(scheduledExecutorService))
                                                                              .build();
            Mqtt3ClientBuilder clientBuilder = MqttClient.builder()
                                                         .useMqttVersion3()
                                                         .identifier(clientId)
                                                         .serverHost(serverUri.getHost())
                                                         .serverPort(serverUri.getPort())
                                                         .executorConfig(executorConfig);
            if (serverUri.getScheme().equals("ssl") || serverUri.getScheme().equals("tls")) {
                clientBuilder.useSslWithDefaultConfig();
            }
            Mqtt3RxClient client = clientBuilder.buildRx();
            return new HivemqMqttClient(client, keepAliveTimeSeconds);
        } catch (URISyntaxException e) {
            throw new JoynrIllegalStateException("Invalid MQTT broker URI: " + ownAddress.getBrokerUri(), e);
        }
    }
}

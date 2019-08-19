package io.joynr.android.consumer;

import android.content.Context;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Properties;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import joynr.vehicle.RadioProxy;

public class RadioConsumerApp {

    private static final String STATIC_PERSISTENCE_FILE = "consumer-joynr.properties";
    private static final String RADIO_LOCAL_DOMAIN = "radio.local.domain";
    private static final String CC_HOST = "localhost";
    private static final int CC_PORT = 4242;

    private RadioProxy radioProxy;

    private static final Logger LOG = LoggerFactory.getLogger(RadioConsumerApp.class);

    public void init(Context context) {
        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, CC_HOST);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + CC_PORT);
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                context.getCacheDir() + "/" + STATIC_PERSISTENCE_FILE);

        JoynrRuntime runtime = new JoynrInjectorFactory(joynrConfig, new LibjoynrWebSocketRuntimeModule())
                .createChildInjector().getInstance(JoynrRuntime.class);

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        radioProxy = runtime.getProxyBuilder(RADIO_LOCAL_DOMAIN, RadioProxy.class)
                .setDiscoveryQos(discoveryQos)
                .build();
    }

    String getCurrentStation() {
        return radioProxy.getCurrentStation().getName();
    }

    String shuffleStations() {
        radioProxy.shuffleStations();
        return getCurrentStation();
    }

}

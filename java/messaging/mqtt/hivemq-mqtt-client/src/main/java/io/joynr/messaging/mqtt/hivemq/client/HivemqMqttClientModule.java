package io.joynr.messaging.mqtt.hivemq.client;

import com.google.inject.AbstractModule;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttModule;

/**
 * Use this module if you want to use the HiveMQ MQTT Client backed implementation of MQTT communication for joynr.
 *
 * TODO
 * - Document how to choose which implementation your runtime uses, and also what the pros and cons of each
 *   are
 */
public class HivemqMqttClientModule extends AbstractModule {

    @Override
    protected void configure() {
        install(new MqttModule());
        bind(MqttClientFactory.class).to(HivemqMqttClientFactory.class);
    }

}

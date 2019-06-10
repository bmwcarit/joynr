package io.joynr.messaging.mqtt.hivemq.client;

import com.google.inject.AbstractModule;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttModule;

/**
 * Use this module if you want to use the mqtt-bee backed implementation of MQTT communication for joynr.
 *
 * Currently mqtt-bee is pre 1.0. Here are a list of things that need doing once the relevant missing
 * feature have been completed in mqtt-bee and we want to make this integration production ready:
 *
 * TODO
 * - As mqtt-bee brings in netty and rxjava as dependencies, we should probably move this integration out
 *   into its own artifact, so that constrained environments can still use the paho library implementation
 *   without pulling in netty and rxjava
 * - Equally, we should pull out the paho implementation into its own artifact, so that when you choose to
 *   use the mqtt-bee version, you don't end up pulling in paho and all of its dependencies
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

/**
 *
 */
package io.joynr.jeeintegration.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import com.google.inject.AbstractModule;
import com.google.inject.Inject;
import com.google.inject.Provides;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Named;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttGlobalAddressFactory;
import io.joynr.messaging.mqtt.MqttMessageSerializerFactory;
import io.joynr.messaging.mqtt.MqttMessagingStubFactory;
import io.joynr.messaging.mqtt.paho.client.MqttPahoClientFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.serialize.AbstractMiddlewareMessageSerializerFactory;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Like {@link io.joynr.messaging.mqtt.MqttModule}, but does not configure the messaging skeleton, so that messages are
 * only sent via MQTT, but not received via MQTT. This is so that we can receive messages via HTTP from the
 * {@link JeeServletMessageReceiver} so that a load balancer can distribute the load in a JEE cluster.
 */
public class JeeMqttMessageSendingModule extends AbstractModule {

    // property key
    public static final String PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS = "joynr.messaging.mqtt.reconnect.sleepms";
    public static final String PROPERTY_KEY_MQTT_BROKER_URI = "joynr.messaging.mqtt.brokeruri";
    public static final String PROPERTY_MQTT_ADDRESS = "property_mqtt_address";
    private MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>> messagingStubFactory;
    private MapBinder<Class<? extends Address>, AbstractMiddlewareMessageSerializerFactory<? extends Address>> messageSerializerFactory;
    private MapBinder<Class<? extends Address>, IMessagingSkeleton> messagingSkeletonFactory;

    public JeeMqttMessageSendingModule(MapBinder<Class<? extends Address>, IMessagingSkeleton> messagingSkeletonFactory,
                                       MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>> messagingStubFactory,
                                       MapBinder<Class<? extends Address>, AbstractMiddlewareMessageSerializerFactory<? extends Address>> messageSerializerFactory) {
        this.messagingSkeletonFactory = messagingSkeletonFactory;
        this.messagingStubFactory = messagingStubFactory;
        this.messageSerializerFactory = messageSerializerFactory;
    }

    @Provides
    @Named(PROPERTY_MQTT_ADDRESS)
    public MqttAddress provideMqttOwnAddress(MqttGlobalAddressFactory globalAddressFactory) {
        return (MqttAddress) globalAddressFactory.create();
    }

    @Override
    protected void configure() {
        messagingStubFactory.addBinding(MqttAddress.class).to(MqttMessagingStubFactory.class);
        messageSerializerFactory.addBinding(MqttAddress.class).to(MqttMessageSerializerFactory.class);
        messagingSkeletonFactory.addBinding(MqttAddress.class).to(NoOpMessagingSkeleton.class);

        Multibinder<GlobalAddressFactory<? extends Address>> globalAddresses;
        globalAddresses = Multibinder.newSetBinder(binder(),
                                                   new TypeLiteral<GlobalAddressFactory<? extends Address>>() {

                                                   });
        globalAddresses.addBinding().to(MqttGlobalAddressFactory.class);

        bind(MqttClientFactory.class).to(MqttPahoClientFactory.class);
    }

    /**
     * Because the messaging stub will refuse to send a message via MQTT unless a messaging skeleton has been registered
     * for the MqttAddress type, we bind a dummy implementation in this module which simply does nothing (no operation -
     * NoOp).
     */
    public static class NoOpMessagingSkeleton implements IMessagingSkeleton {

        private MqttClientFactory mqttClientFactory;
        private JoynrMqttClient mqttClient;

        @Inject
        public NoOpMessagingSkeleton(MqttClientFactory mqttClientFactory) {
            this.mqttClientFactory = mqttClientFactory;
        }

        @Override
        public void transmit(JoynrMessage message, FailureAction failureAction) {
        }

        @Override
        public void transmit(String serializedMessage, FailureAction failureAction) {
        }

        @Override
        public void init() {
            mqttClient = mqttClientFactory.create();
            mqttClient.setMessageListener(this);
            mqttClient.start();
        }

        @Override
        public void shutdown() {
            mqttClient.shutdown();
        }

    }

}

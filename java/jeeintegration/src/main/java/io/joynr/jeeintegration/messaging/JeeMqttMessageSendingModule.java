package io.joynr.jeeintegration.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
import com.google.inject.Provides;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Named;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.mqtt.DefaultMqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttGlobalAddressFactory;
import io.joynr.messaging.mqtt.MqttMessageReplyToAddressCalculator;
import io.joynr.messaging.mqtt.MqttMessageSerializerFactory;
import io.joynr.messaging.mqtt.MqttMessagingStubFactory;
import io.joynr.messaging.mqtt.MqttMulticastAddressCalculator;
import io.joynr.messaging.mqtt.paho.client.MqttPahoClientFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.routing.MulticastAddressCalculator;
import io.joynr.messaging.serialize.AbstractMiddlewareMessageSerializerFactory;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Like {@link io.joynr.messaging.mqtt.MqttModule}, but configures the {@link MqttMessagingSkeletonProvider} so that if
 * the {@link io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys#JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY} property
 * is set to <code>true</true>, messages are only sent via MQTT, but not received via MQTT and also that if the
 * {@link io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys#JEE_ENABLE_SHARED_SUBSCRIPTIONS} property is set to
 * <code>true</code>, then the shared subscription versions of the messaging skeleton and reply-to address calculator
 * are used.
 *
 * In the case of the HTTP bridge, this is so that we can receive messages via HTTP from the
 * {@link JeeServletMessageReceiver} in order for a load balancer to be able to distribute the load across a JEE
 * cluster.
 *
 * In the case of shared subscriptions we allow load balancing of incoming MQTT messages via the HiveMQ feature of
 * shared subscriptions.
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
        return globalAddressFactory.create();
    }

    @Override
    protected void configure() {
        messagingStubFactory.addBinding(MqttAddress.class).to(MqttMessagingStubFactory.class);
        messageSerializerFactory.addBinding(MqttAddress.class).to(MqttMessageSerializerFactory.class);
        messagingSkeletonFactory.addBinding(MqttAddress.class).toProvider(MqttMessagingSkeletonProvider.class);

        Multibinder<GlobalAddressFactory<? extends Address>> globalAddresses;
        globalAddresses = Multibinder.newSetBinder(binder(),
                                                   new TypeLiteral<GlobalAddressFactory<? extends Address>>() {

                                                   });
        globalAddresses.addBinding().to(MqttGlobalAddressFactory.class);

        bind(MqttClientFactory.class).to(MqttPahoClientFactory.class);
        bind(MqttMessageReplyToAddressCalculator.class).toProvider(SharedSubscriptionReplyToAddressCalculatorProvider.class);
        bind(MqttClientIdProvider.class).to(DefaultMqttClientIdProvider.class);

        Multibinder<MulticastAddressCalculator> multicastAddressCalculators = Multibinder.newSetBinder(binder(),
                                                                                                       new TypeLiteral<MulticastAddressCalculator>() {
                                                                                                       });
        multicastAddressCalculators.addBinding().to(MqttMulticastAddressCalculator.class);
    }

}

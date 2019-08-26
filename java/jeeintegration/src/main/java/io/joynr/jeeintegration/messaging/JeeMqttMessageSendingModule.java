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
package io.joynr.jeeintegration.messaging;

import static io.joynr.messaging.mqtt.MqttModule.MQTT_BROKER_URI_ARRAY;
import static io.joynr.messaging.mqtt.MqttModule.MQTT_GBID_TO_BROKERURI_MAP;
import static io.joynr.messaging.mqtt.MqttModule.MQTT_GBID_TO_CONNECTION_TIMEOUT_SEC_MAP;
import static io.joynr.messaging.mqtt.MqttModule.MQTT_TO_KEEP_ALIVE_TIMER_SEC_MAP;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_REPLY_TO_ADDRESS;
import static io.joynr.messaging.mqtt.MqttModule.MQTT_CIPHERSUITE_LIST;

import java.util.List;

import java.util.HashMap;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Named;
import com.google.inject.name.Names;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.mqtt.DefaultMqttClientIdProvider;
import io.joynr.messaging.mqtt.DefaultMqttTopicPrefixProvider;
import io.joynr.messaging.mqtt.MqttCiphersuiteListFactory;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttGlobalAddressFactory;
import io.joynr.messaging.mqtt.MqttMessagingStubFactory;
import io.joynr.messaging.mqtt.MqttMulticastAddressCalculator;
import io.joynr.messaging.mqtt.MqttMultipleBackendPropertyProvider;
import io.joynr.messaging.mqtt.MqttReplyToAddressFactory;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.routing.MulticastAddressCalculator;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Like {@link io.joynr.messaging.mqtt.MqttModule}, but configures the {@link JeeMqttMessagingSkeletonProvider} so that if
 * the {@link io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys#JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY} property
 * is set to <code>true</true>, messages are only sent via MQTT, but not received via MQTT.
 *
 * In the case of the HTTP bridge, this is so that we can receive messages via HTTP from the
 * {@link JeeServletMessageReceiver} in order for a load balancer to be able to distribute the load across a JEE
 * cluster.
 */
public class JeeMqttMessageSendingModule extends AbstractModule {
    private MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory;
    private MapBinder<Class<? extends Address>, IMessagingSkeletonFactory> messagingSkeletonFactory;

    public JeeMqttMessageSendingModule(MapBinder<Class<? extends Address>, IMessagingSkeletonFactory> messagingSkeletonFactory,
                                       MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory) {
        this.messagingSkeletonFactory = messagingSkeletonFactory;
        this.messagingStubFactory = messagingStubFactory;
    }

    @Provides
    @Named(PROPERTY_MQTT_GLOBAL_ADDRESS)
    public MqttAddress provideMqttOwnAddress(MqttGlobalAddressFactory globalAddressFactory) {
        return globalAddressFactory.create();
    }

    @Provides
    @Named(PROPERTY_MQTT_REPLY_TO_ADDRESS)
    public MqttAddress provideMqttOwnAddress(MqttReplyToAddressFactory replyToAddressFactory) {
        return replyToAddressFactory.create();
    }

    @Provides
    @Named(MQTT_GBID_TO_BROKERURI_MAP)
    public HashMap<String, String> provideGbidToBrokerUriMap(MqttMultipleBackendPropertyProvider mqttMultipleBackendPropertyProvider) {
        return mqttMultipleBackendPropertyProvider.provideGbidToBrokerUriMap();
    }

    @Provides
    @Named(MQTT_BROKER_URI_ARRAY)
    public String[] provideMqttBrokerUriArray(MqttMultipleBackendPropertyProvider mqttMultipleBackendPropertyProvider) {
        return mqttMultipleBackendPropertyProvider.provideBrokerUris();
    }

    @Provides
    @Named(MQTT_TO_KEEP_ALIVE_TIMER_SEC_MAP)
    public HashMap<String, Integer> provideGbidTpKeepAliveTimerSecMap(MqttMultipleBackendPropertyProvider mqttMultipleBackendPropertyProvider) {
        return mqttMultipleBackendPropertyProvider.provideGbidToKeepAliveTimerSecMap();
    }

    @Provides
    @Named(MQTT_GBID_TO_CONNECTION_TIMEOUT_SEC_MAP)
    public HashMap<String, Integer> provideGbidToConnectionTimeoutSecMap(MqttMultipleBackendPropertyProvider mqttMultipleBackendPropertyProvider) {
        return mqttMultipleBackendPropertyProvider.provideGbidToConnectionTimeoutSecMap();
    }

    @Provides
    @Named(MQTT_CIPHERSUITE_LIST)
    public List<String> provideMqttInternalCipherList(MqttCiphersuiteListFactory internalCipherListFactory) {
        return internalCipherListFactory.create();
    }

    @Override
    protected void configure() {
        messagingStubFactory.addBinding(MqttAddress.class).to(MqttMessagingStubFactory.class);
        messagingSkeletonFactory.addBinding(MqttAddress.class).toProvider(JeeMqttMessagingSkeletonProvider.class);

        Multibinder<GlobalAddressFactory<? extends Address>> globalAddresses;
        globalAddresses = Multibinder.newSetBinder(binder(),
                                                   new TypeLiteral<GlobalAddressFactory<? extends Address>>() {
                                                   },
                                                   Names.named(GlobalAddressProvider.GLOBAL_ADDRESS_FACTORIES));
        globalAddresses.addBinding().to(MqttGlobalAddressFactory.class);

        Multibinder<GlobalAddressFactory<? extends Address>> replyToAddresses;
        replyToAddresses = Multibinder.newSetBinder(binder(),
                                                    new TypeLiteral<GlobalAddressFactory<? extends Address>>() {
                                                    },
                                                    Names.named(ReplyToAddressProvider.REPLY_TO_ADDRESS_FACTORIES));
        replyToAddresses.addBinding().to(MqttReplyToAddressFactory.class);

        bind(MqttClientFactory.class).to(HivemqMqttClientFactory.class);
        bind(MqttTopicPrefixProvider.class).to(DefaultMqttTopicPrefixProvider.class);
        bind(MqttClientIdProvider.class).to(DefaultMqttClientIdProvider.class);

        Multibinder<MulticastAddressCalculator> multicastAddressCalculators = Multibinder.newSetBinder(binder(),
                                                                                                       new TypeLiteral<MulticastAddressCalculator>() {
                                                                                                       });
        multicastAddressCalculators.addBinding().to(MqttMulticastAddressCalculator.class);
    }

}

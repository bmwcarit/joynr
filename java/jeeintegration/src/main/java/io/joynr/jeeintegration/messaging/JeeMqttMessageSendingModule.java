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
import static io.joynr.messaging.mqtt.MqttModule.MQTT_CIPHERSUITE_LIST;
import static io.joynr.messaging.MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER;

import java.util.List;

import java.util.HashMap;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.OptionalBinder;
import com.google.inject.name.Named;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.DefaultMqttClientIdProvider;
import io.joynr.messaging.mqtt.DefaultMqttTopicPrefixProvider;
import io.joynr.messaging.mqtt.MqttCiphersuiteListFactory;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttMessagingStubFactory;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.MqttMulticastAddressCalculator;
import io.joynr.messaging.mqtt.MqttMultipleBackendPropertyProvider;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientTrustManagerFactory;
import io.joynr.messaging.mqtt.hivemq.client.IHivemqMqttClientTrustManagerFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientCreator;
import io.joynr.messaging.mqtt.JoynrMqttClientCreator;
import io.joynr.messaging.routing.MulticastAddressCalculator;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Like {@link io.joynr.messaging.mqtt.MqttModule}, but configures the {@link JeeMqttMessagingSkeletonProvider}
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
    @Named(MessagingPropertyKeys.GLOBAL_ADDRESS)
    public Address provideMqttOwnAddress(@Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbids,
                                         @Named(MessagingPropertyKeys.CHANNELID) String localChannelId,
                                         MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        return new MqttAddress(gbids[0], mqttTopicPrefixProvider.getUnicastTopicPrefix() + localChannelId);
    }

    @Provides
    @Named(MessagingPropertyKeys.REPLY_TO_ADDRESS)
    public Address provideMqttOwnReplyToAddress(@Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbids,
                                                @Named(MessagingPropertyKeys.CHANNELID) String localChannelId,
                                                @Named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) String enableSharedSubscriptions,
                                                @Named(MessagingPropertyKeys.RECEIVERID) String receiverId,
                                                MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        String replyToTopic;
        if (Boolean.valueOf(enableSharedSubscriptions)) {
            replyToTopic = mqttTopicPrefixProvider.getSharedSubscriptionsReplyToTopicPrefix() + localChannelId + "/"
                    + receiverId;
        } else {
            replyToTopic = mqttTopicPrefixProvider.getUnicastTopicPrefix() + localChannelId;
        }
        return new MqttAddress(gbids[0], replyToTopic);
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

    @Provides
    @Named(PROPERTY_KEY_SEPARATE_REPLY_RECEIVER)
    public boolean provideSeparateReplyReceiverSetting(@Named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) boolean enableSharedSubscriptions) {
        return enableSharedSubscriptions;
    }

    @Override
    protected void configure() {
        messagingStubFactory.addBinding(MqttAddress.class).to(MqttMessagingStubFactory.class);
        messagingSkeletonFactory.addBinding(MqttAddress.class).toProvider(JeeMqttMessagingSkeletonProvider.class);

        bind(MqttClientFactory.class).to(HivemqMqttClientFactory.class);
        bind(IHivemqMqttClientTrustManagerFactory.class).to(HivemqMqttClientTrustManagerFactory.class);
        bind(JoynrMqttClientCreator.class).to(HivemqMqttClientCreator.class);
        bind(MqttTopicPrefixProvider.class).to(DefaultMqttTopicPrefixProvider.class);
        bind(MqttClientIdProvider.class).to(DefaultMqttClientIdProvider.class);

        OptionalBinder.newOptionalBinder(binder(), MulticastAddressCalculator.class)
                      .setBinding()
                      .to(MqttMulticastAddressCalculator.class);
    }

}

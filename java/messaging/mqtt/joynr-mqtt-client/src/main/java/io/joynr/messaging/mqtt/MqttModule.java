/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import java.util.HashMap;
import java.util.List;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.OptionalBinder;
import com.google.inject.name.Named;
import com.google.inject.name.Names;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.MulticastAddressCalculator;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

import static io.joynr.messaging.MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER;

/**
 * If the {@link io.joynr.messaging.mqtt.MqttModule#PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS} property is set to
 * <code>true</code>, then the shared subscriptions version of the mqtt messaging skeleton is used instead of the
 * default mqtt messaging skeleton.
 * <p>
 * In the case of shared subscriptions we allow load balancing of incoming MQTT messages via the HiveMQ feature of
 * shared subscriptions.
 */
public class MqttModule extends AbstractModule {

    // property key
    public static final String PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS = "joynr.messaging.mqtt.reconnect.sleepms";
    public static final String PROPERTY_MQTT_BROKER_URIS = "joynr.messaging.mqtt.brokeruris";
    public static final String MQTT_BROKER_URI_ARRAY = "joynr.internal.messaging.mqtt.brokeruriarray";
    public static final String MQTT_GBID_TO_BROKERURI_MAP = "joynr.internal.messaging.mqtt.gbidtobrokerurimap";
    public static final String PROPERTY_KEY_MQTT_CLIENT_ID_PREFIX = "joynr.messaging.mqtt.clientidprefix";
    public static final String PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC = "joynr.messaging.mqtt.keepalivetimerssec";
    public static final String MQTT_TO_KEEP_ALIVE_TIMER_SEC_MAP = "joynr.internal.messaging.mqtt.gbidtokeepalivetimersecmap";
    public static final String PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC = "joynr.messaging.mqtt.connectiontimeoutssec";
    public static final String MQTT_GBID_TO_CONNECTION_TIMEOUT_SEC_MAP = "joynr.internal.messaging.mqtt.gbidtoconnectiontimeoutsecmap";
    public static final String PROPERTY_KEY_MQTT_KEYSTORE_PATH = "joynr.messaging.mqtt.ssl.keystore";
    public static final String PROPERTY_KEY_MQTT_TRUSTSTORE_PATH = "joynr.messaging.mqtt.ssl.truststore";
    public static final String PROPERTY_KEY_MQTT_KEYSTORE_TYPE = "joynr.messaging.mqtt.ssl.keystoretype";
    public static final String PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE = "joynr.messaging.mqtt.ssl.truststoretype";
    public static final String PROPERTY_KEY_MQTT_KEYSTORE_PWD = "joynr.messaging.mqtt.ssl.keystorepassword";
    public static final String PROPERTY_KEY_MQTT_TRUSTSTORE_PWD = "joynr.messaging.mqtt.ssl.truststorepassword";
    public static final String PROPERTY_KEY_MQTT_CIPHERSUITES = "joynr.messaging.mqtt.ssl.ciphersuites";
    public static final String MQTT_CIPHERSUITE_LIST = "joynr.internal.messaging.mqtt.ssl.ciphersuiteList";
    public static final String PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS = "joynr.messaging.mqtt.separateconnections";
    public static final String PROPERTY_KEY_MQTT_USERNAME = "joynr.messaging.mqtt.username";
    public static final String PROPERTY_KEY_MQTT_PASSWORD = "joynr.messaging.mqtt.password";
    public static final String PROPERTY_KEY_MQTT_DISABLE_HOSTNAME_VERIFICATION = "joynr.messaging.mqtt.disablehostnameverification";
    public static final String PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM = "joynr.messaging.mqtt.receivemaximum";
    public static final String PROPERTY_KEY_MQTT_CONNECT_ON_START = "joynr.messaging.mqtt.connectonstart";
    public static final String PROPERTY_KEY_MQTT_RETAIN = "joynr.messaging.mqtt.retain";

    /**
     * Use this key to activate shared subscription support by setting the property's value to <code>true</code>. Shared
     * subscriptions are a feature of HiveMQ which allow queue semantics to be used for subscribers to MQTT topics. That
     * is, only one subscriber receives a message, rather than all subscribers. This feature can be used to load balance
     * incoming messages on MQTT. This feature is useful if you want to run a cluster of JEE nodes while using only MQTT
     * for communication.
     */
    public static final String PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS = "joynr.messaging.mqtt.enable.sharedsubscriptions";
    public static final String PROPERTY_MQTT_CLEAN_SESSION = "joynr.messaging.mqtt.cleansession";

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
                                                @Named(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) String enableSharedSubscriptions,
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
        MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory;
        messagingStubFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
        }, new TypeLiteral<AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>>() {
        }, Names.named(MessagingStubFactory.MIDDLEWARE_MESSAGING_STUB_FACTORIES));
        messagingStubFactory.addBinding(MqttAddress.class).to(MqttMessagingStubFactory.class);

        MapBinder<Class<? extends Address>, IMessagingSkeletonFactory> messagingSkeletonFactory;
        messagingSkeletonFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
        }, new TypeLiteral<IMessagingSkeletonFactory>() {
        }, Names.named(MessagingSkeletonFactory.MIDDLEWARE_MESSAGING_SKELETON_FACTORIES));
        messagingSkeletonFactory.addBinding(MqttAddress.class).toProvider(MqttMessagingSkeletonProvider.class);

        OptionalBinder.newOptionalBinder(binder(), MulticastAddressCalculator.class)
                      .setBinding()
                      .to(MqttMulticastAddressCalculator.class);

        bind(MqttClientIdProvider.class).to(DefaultMqttClientIdProvider.class);
        bind(MqttTopicPrefixProvider.class).to(DefaultMqttTopicPrefixProvider.class);
    }
}

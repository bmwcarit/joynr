package io.joynr.messaging.mqtt;

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

import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_REPLY_TO_ADDRESS;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * A provider for {@link IMessagingSkeleton} instances which checks with the property configured under
 * {@link io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS} If shared subscriptions are
 * enabled, it returns an instance of {@link SharedSubscriptionsMqttMessagingSkeleton}. Otherwise (default behaviour),
 * it returns an instance of the normal {@link MqttMessagingSkeleton}.
 */
public class MqttMessagingSkeletonProvider implements Provider<IMessagingSkeleton> {

    private final static Logger logger = LoggerFactory.getLogger(MqttMessagingSkeletonProvider.class);

    protected MqttClientFactory mqttClientFactory;
    private boolean sharedSubscriptionsEnabled;
    private MqttAddress ownAddress;
    private MqttAddress replyToAddress;
    private MessageRouter messageRouter;
    private MqttMessageSerializerFactory messageSerializerFactory;
    private String channelId;
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    @Inject
    public MqttMessagingSkeletonProvider(@Named(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) String enableSharedSubscriptions,
                                         @Named(PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                         @Named(PROPERTY_MQTT_REPLY_TO_ADDRESS) MqttAddress replyToAddress,
                                         MessageRouter messageRouter,
                                         MqttClientFactory mqttClientFactory,
                                         MqttMessageSerializerFactory messageSerializerFactory,
                                         @Named(CHANNELID) String channelId,
                                         MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        sharedSubscriptionsEnabled = Boolean.valueOf(enableSharedSubscriptions);
        this.ownAddress = ownAddress;
        this.replyToAddress = replyToAddress;
        this.messageRouter = messageRouter;
        this.mqttClientFactory = mqttClientFactory;
        this.messageSerializerFactory = messageSerializerFactory;
        this.channelId = channelId;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
        logger.debug("Created with sharedSubscriptionsEnabled: {} ownAddress: {} channelId: {}", new Object[]{
                sharedSubscriptionsEnabled, this.ownAddress, this.channelId });
    }

    @Override
    public IMessagingSkeleton get() {
        if (sharedSubscriptionsEnabled) {
            return new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                                replyToAddress,
                                                                messageRouter,
                                                                mqttClientFactory,
                                                                messageSerializerFactory,
                                                                channelId,
                                                                mqttTopicPrefixProvider);
        }
        return new MqttMessagingSkeleton(ownAddress,
                                         messageRouter,
                                         mqttClientFactory,
                                         messageSerializerFactory,
                                         mqttTopicPrefixProvider);
    }

}

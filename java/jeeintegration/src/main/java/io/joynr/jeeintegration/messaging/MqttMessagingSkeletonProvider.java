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

import static io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys.JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY;
import static io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys.JEE_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.RECEIVERID;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageSerializerFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeleton;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * A provider for {@link IMessagingSkeleton} instances which checks with the property configured under
 * {@link io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys#JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY} to see if
 * messages should be received via MQTT or not. If they should (default behaviour), then it returns an instance of
 * {@link MqttMessagingSkeleton}, otherwise an instance of {@link NoOpMessagingSkeleton}.
 */
public class MqttMessagingSkeletonProvider implements Provider<IMessagingSkeleton> {

    private final static Logger logger = LoggerFactory.getLogger(MqttMessagingSkeletonProvider.class);

    private boolean httpBridgeEnabled;
    private boolean sharedSubscriptionsEnabled;
    private MqttAddress ownAddress;
    private MessageRouter messageRouter;
    private MqttClientFactory mqttClientFactory;
    private MqttMessageSerializerFactory messageSerializerFactory;
    private String channelId;
    private String receiverId;
    private RawMessagingPreprocessor rawMessagingPreprocessor;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttMessagingSkeletonProvider(@Named(JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY) String enableHttpBridge,
                                         @Named(JEE_ENABLE_SHARED_SUBSCRIPTIONS) String enableSharedSubscriptions,
                                         @Named(MqttModule.PROPERTY_MQTT_ADDRESS) MqttAddress ownAddress,
                                         MessageRouter messageRouter,
                                         MqttClientFactory mqttClientFactory,
                                         MqttMessageSerializerFactory messageSerializerFactory,
                                         @Named(CHANNELID) String channelId,
                                         @Named(RECEIVERID) String receiverId,
                                         RawMessagingPreprocessor rawMessagingPreprocessor) {
        // CHECKSTYLE:ON
        httpBridgeEnabled = Boolean.valueOf(enableHttpBridge);
        sharedSubscriptionsEnabled = Boolean.valueOf(enableSharedSubscriptions);
        this.rawMessagingPreprocessor = rawMessagingPreprocessor;
        this.ownAddress = ownAddress;
        this.messageRouter = messageRouter;
        this.mqttClientFactory = mqttClientFactory;
        this.messageSerializerFactory = messageSerializerFactory;
        this.channelId = channelId;
        this.receiverId = receiverId;
        logger.debug("Created with httpBridgeEnabled: {}\n\tsharedSubscriptionsEnabled: {}\n\t"
                + "ownAddress: {}\n\tmessageRouter: {}\n\tmqttClientFactory: {}"
                + "\n\tmessageSerializer: {}\n\tchannelId: {}",
                     new Object[]{ httpBridgeEnabled, sharedSubscriptionsEnabled, this.ownAddress, this.messageRouter,
                             this.mqttClientFactory, this.messageSerializerFactory, this.channelId });
    }

    @Override
    public IMessagingSkeleton get() {
        if (httpBridgeEnabled) {
            return new NoOpMessagingSkeleton(mqttClientFactory);
        } else if (sharedSubscriptionsEnabled) {
            return new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                                messageRouter,
                                                                mqttClientFactory,
                                                                messageSerializerFactory,
                                                                channelId,
                                                                receiverId,
                                                                rawMessagingPreprocessor);
        }
        return new MqttMessagingSkeleton(ownAddress,
                                         messageRouter,
                                         mqttClientFactory,
                                         messageSerializerFactory,
                                         rawMessagingPreprocessor);
    }

}

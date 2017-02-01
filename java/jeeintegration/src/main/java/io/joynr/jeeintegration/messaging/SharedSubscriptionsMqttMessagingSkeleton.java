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

import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_ADDRESS;
import static io.joynr.jeeintegration.messaging.SharedSubscriptionReplyToAddressCalculatorProvider.REPLYTO_PREFIX;

import static java.lang.String.format;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageSerializerFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Overrides the standard {@link MqttMessagingSkeleton} in order to customise the topic subscription strategy in the
 * case where HiveMQ shared subscriptions are available.
 *
 * @see io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys#JEE_ENABLE_SHARED_SUBSCRIPTIONS
 */
public class SharedSubscriptionsMqttMessagingSkeleton extends MqttMessagingSkeleton {

    private static final String NON_ALPHA_REGEX_PATTERN = "[^a-zA-Z]";
    private String channelId;
    private String receiverId;

    @Inject
    public SharedSubscriptionsMqttMessagingSkeleton(@Named(PROPERTY_MQTT_ADDRESS) MqttAddress ownAddress,
                                                    MessageRouter messageRouter,
                                                    MqttClientFactory mqttClientFactory,
                                                    MqttMessageSerializerFactory messageSerializerFactory,
                                                    @Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                                    @Named(MessagingPropertyKeys.RECEIVERID) String receiverId,
                                                    RawMessagingPreprocessor rawMessagingPreprocessor) {
        super(ownAddress, messageRouter, mqttClientFactory, messageSerializerFactory, rawMessagingPreprocessor);
        this.channelId = channelId;
        this.receiverId = receiverId;
    }

    @Override
    protected void subscribe() {
        getClient().subscribe("$share:" + sanitiseChannelIdForUseAsTopic() + ":" + getOwnAddress().getTopic() + "/#");
        getClient().subscribe(REPLYTO_PREFIX + getOwnAddress().getTopic() + "/" + receiverId + "/#");
    }

    private String sanitiseChannelIdForUseAsTopic() {
        String result = channelId.replaceAll(NON_ALPHA_REGEX_PATTERN, "");
        if (result.isEmpty()) {
            throw new IllegalArgumentException(format("The channel ID %s cannot be converted to a valid MQTT topic fragment because it does not contain any alpha characters.",
                                                      channelId));
        }
        return result;
    }

}

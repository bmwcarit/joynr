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

import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_REPLY_TO_ADDRESS;

import static java.lang.String.format;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Overrides the standard {@link MqttMessagingSkeleton} in order to customise the topic subscription strategy in the
 * case where HiveMQ shared subscriptions are available.
 *
 * @see io.joynr.messaging.mqtt.MqttModule#PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS
 */
public class SharedSubscriptionsMqttMessagingSkeleton extends MqttMessagingSkeleton {

    private static final String NON_ALPHA_REGEX_PATTERN = "[^a-zA-Z]";
    private String channelId;
    private MqttAddress replyToAddress;

    @Inject
    public SharedSubscriptionsMqttMessagingSkeleton(@Named(PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                                    @Named(PROPERTY_MQTT_REPLY_TO_ADDRESS) MqttAddress replyToAddress,
                                                    MessageRouter messageRouter,
                                                    MqttClientFactory mqttClientFactory,
                                                    MqttMessageSerializerFactory messageSerializerFactory,
                                                    @Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                                    MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        super(ownAddress, messageRouter, mqttClientFactory, messageSerializerFactory, mqttTopicPrefixProvider);
        this.replyToAddress = replyToAddress;
        this.channelId = channelId;
    }

    @Override
    protected void subscribe() {
        getClient().subscribe("$share:" + sanitiseChannelIdForUseAsTopic() + ":" + getOwnAddress().getTopic() + "/#");
        getClient().subscribe(replyToAddress.getTopic() + "/#");
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

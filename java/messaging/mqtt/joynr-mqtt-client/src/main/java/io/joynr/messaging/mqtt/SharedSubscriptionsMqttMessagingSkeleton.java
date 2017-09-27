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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_REPLY_TO_ADDRESS;

import static java.lang.String.format;

import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.RawMessagingPreprocessor;
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
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 8 LINES
    public SharedSubscriptionsMqttMessagingSkeleton(@Named(PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                                    @Named(PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS) int repeatedMqttMessageIgnorePeriodMs,
                                                    @Named(PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE) int maxIncomingMqttMessagesInQueue,
                                                    @Named(PROPERTY_MQTT_REPLY_TO_ADDRESS) MqttAddress replyToAddress,
                                                    MessageRouter messageRouter,
                                                    MqttClientFactory mqttClientFactory,
                                                    @Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                                    MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                                    RawMessagingPreprocessor rawMessagingPreprocessor,
                                                    Set<JoynrMessageProcessor> messageProcessors) {
        super(ownAddress,
              repeatedMqttMessageIgnorePeriodMs,
              maxIncomingMqttMessagesInQueue,
              messageRouter,
              mqttClientFactory,
              mqttTopicPrefixProvider,
              rawMessagingPreprocessor,
              messageProcessors);
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

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
package io.joynr.messaging.mqtt;

import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.RECEIVERID;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS;

import java.util.Optional;

import javax.inject.Named;

import com.google.inject.Inject;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.GlobalAddressFactory;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttReplyToAddressFactory extends GlobalAddressFactory<MqttAddress> {
    private static final String SUPPORTED_TRANSPORT_MQTT = "mqtt";
    private String replyToTopic;
    private String brokerUri;

    @Inject
    public MqttReplyToAddressFactory(@Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbids,
                                     @Named(PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress globalAddress,
                                     @Named(CHANNELID) String localChannelId,
                                     @Named(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) String enableSharedSubscriptions,
                                     @Named(RECEIVERID) String receiverId,
                                     MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        this.brokerUri = gbids[0];
        if (Boolean.valueOf(enableSharedSubscriptions)) {
            replyToTopic = mqttTopicPrefixProvider.getSharedSubscriptionsReplyToTopicPrefix() + localChannelId + "/"
                    + receiverId;
        } else {
            replyToTopic = globalAddress.getTopic();
        }
    }

    @Override
    public MqttAddress create() {
        return new MqttAddress(brokerUri, replyToTopic);
    }

    @Override
    public boolean supportsTransport(Optional<String> transport) {
        return SUPPORTED_TRANSPORT_MQTT.equals(transport.isPresent() ? transport.get() : null);
    }

}

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

import javax.inject.Named;

import com.google.inject.Inject;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.GlobalAddressFactory;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttGlobalAddressFactory extends GlobalAddressFactory<MqttAddress> {
    private static final String SUPPORTED_TRANSPORT_MQTT = "mqtt";
    private String localChannelId;
    private String brokerUri;
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    @Inject
    public MqttGlobalAddressFactory(@Named(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI) String brokerUri,
                                    @Named(MessagingPropertyKeys.CHANNELID) String localChannelId,
                                    MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        this.brokerUri = brokerUri;
        this.localChannelId = localChannelId;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
    }

    @Override
    public MqttAddress create() {
        return new MqttAddress(brokerUri, mqttTopicPrefixProvider.getUnicastTopicPrefix() + localChannelId);
    }

    @Override
    public boolean supportsTransport(String transport) {
        return SUPPORTED_TRANSPORT_MQTT.equals(transport);
    }

}

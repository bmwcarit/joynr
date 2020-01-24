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

import java.util.Optional;

import javax.inject.Named;

import com.google.inject.Inject;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.GlobalAddressFactory;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttGlobalAddressFactory extends GlobalAddressFactory<MqttAddress> {
    private static final String SUPPORTED_TRANSPORT_MQTT = "mqtt";
    private final String defaultGbid;
    private final String localChannelId;
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    @Inject
    public MqttGlobalAddressFactory(@Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbids,
                                    @Named(MessagingPropertyKeys.CHANNELID) String localChannelId,
                                    MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        defaultGbid = gbids[0];
        this.localChannelId = localChannelId;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
    }

    @Override
    public MqttAddress create() {
        return new MqttAddress(defaultGbid, mqttTopicPrefixProvider.getUnicastTopicPrefix() + localChannelId);
    }

    @Override
    public boolean supportsTransport(Optional<String> transport) {
        return SUPPORTED_TRANSPORT_MQTT.equals(transport.isPresent() ? transport.get() : null);
    }

}

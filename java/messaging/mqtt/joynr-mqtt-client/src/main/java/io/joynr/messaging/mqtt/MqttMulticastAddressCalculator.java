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

import com.google.inject.Inject;
import com.google.inject.name.Named;
import io.joynr.messaging.routing.MulticastAddressCalculator;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttMulticastAddressCalculator implements MulticastAddressCalculator {

    private MqttAddress globalAddress;
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    @Inject
    public MqttMulticastAddressCalculator(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress globalAddress,
                                          MqttTopicPrefixProvider mqttTopicPrefixProvider) {
        this.globalAddress = (MqttAddress) globalAddress;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
    }

    @Override
    public Address calculate(ImmutableMessage message) {
        Address result = null;
        if (globalAddress != null) {
            String topic = mqttTopicPrefixProvider.getMulticastTopicPrefix() + message.getRecipient();
            result = new MqttAddress(globalAddress.getBrokerUri(), topic);
        }
        return result;
    }

    @Override
    public boolean supports(String transport) {
        return "mqtt".equalsIgnoreCase(transport);
    }
}

package io.joynr.messaging.mqtt;

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

import static joynr.JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import joynr.JoynrMessage;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

public class DefaultMqttMessageReplyToAddressCalculator implements MqttMessageReplyToAddressCalculator {

    private MqttAddress replyToMqttAddress;

    @Inject
    public DefaultMqttMessageReplyToAddressCalculator(@Named(MqttModule.PROPERTY_MQTT_ADDRESS) MqttAddress replyToMqttAddress) {
        this.replyToMqttAddress = replyToMqttAddress;
    }

    @Override
    public void setReplyTo(JoynrMessage message) {
        String type = message.getType();
        if (type != null
                && message.getReplyTo() == null
                && (type.equals(MESSAGE_TYPE_REQUEST) || type.equals(MESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                        || type.equals(MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) || type.equals(MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST))) {
            message.setReplyTo(RoutingTypesUtil.toAddressString(replyToMqttAddress));
        }
    }

}

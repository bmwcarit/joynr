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

import static io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys.JEE_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_ADDRESS;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.messaging.mqtt.DefaultMqttMessageReplyToAddressCalculator;
import io.joynr.messaging.mqtt.MqttMessageReplyToAddressCalculator;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Calculates the reply-to address to be used in outgoing joynr messages if
 * {@link io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys#JEE_ENABLE_SHARED_SUBSCRIPTIONS shared subscriptions}
 * are enabled, and sets provides instances of {@link MqttMessageReplyToAddressCalculator} with the relevant reply-to
 * address set.
 */
public class SharedSubscriptionReplyToAddressCalculatorProvider implements
        Provider<MqttMessageReplyToAddressCalculator> {

    private MqttAddress replyToMqttAddress;

    @Inject
    public SharedSubscriptionReplyToAddressCalculatorProvider(@Named(PROPERTY_MQTT_ADDRESS) MqttAddress replyToMqttAddress,
                                                              @Named(JEE_ENABLE_SHARED_SUBSCRIPTIONS) String enableSharedSubscriptions) {
        MqttAddress replyToAddressToUse = replyToMqttAddress;
        if (Boolean.valueOf(enableSharedSubscriptions)) {
            replyToAddressToUse = new MqttAddress(replyToMqttAddress.getBrokerUri(), "replyto/"
                    + replyToMqttAddress.getTopic());
        }
        this.replyToMqttAddress = replyToAddressToUse;
    }

    @Override
    public MqttMessageReplyToAddressCalculator get() {
        return new DefaultMqttMessageReplyToAddressCalculator(replyToMqttAddress);
    }

}

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

import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;

import java.util.HashMap;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttMessagingStubFactory extends AbstractMiddlewareMessagingStubFactory<MqttMessagingStub, MqttAddress> {

    private static final Logger logger = LoggerFactory.getLogger(MqttMessagingStubFactory.class);

    private Map<String, JoynrMqttClient> gbidToMqttClientMap;

    @Inject
    public MqttMessagingStubFactory(MqttClientFactory mqttClientFactory, @Named(GBID_ARRAY) String[] gbid_array) {
        gbidToMqttClientMap = new HashMap<String, JoynrMqttClient>();
        for (String gbid : gbid_array) {
            gbidToMqttClientMap.put(gbid, mqttClientFactory.createSender(gbid));
        }
    }

    @Override
    protected MqttMessagingStub createInternal(MqttAddress address) {
        String gbid = address.getBrokerUri();
        if (!gbidToMqttClientMap.containsKey(gbid)) {
            logger.error("Gbid {} is not known in MqttMessagingStubFactory.", gbid);
            return null;
        }
        return new MqttMessagingStub(address, gbidToMqttClientMap.get(gbid));
    }
}

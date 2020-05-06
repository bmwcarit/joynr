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

import static io.joynr.messaging.MessagingPropertyKeys.RECEIVERID;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class DefaultMqttClientIdProvider implements MqttClientIdProvider {
    private static final Logger logger = LoggerFactory.getLogger(DefaultMqttClientIdProvider.class);
    private static final String DEFAULT_CLIENT_ID_PREFIX = "";

    static class ClientIdPrefixHolder {
        @Inject(optional = true)
        @Named(MqttModule.PROPERTY_KEY_MQTT_CLIENT_ID_PREFIX)
        String clientIdPrefix = DEFAULT_CLIENT_ID_PREFIX;
    }

    private String clientId;

    @Inject
    public DefaultMqttClientIdProvider(@Named(RECEIVERID) String receiverId,
                                       ClientIdPrefixHolder clientIdPrefixHolder) {
        String clientIdPrefix = clientIdPrefixHolder.clientIdPrefix;
        if (receiverId.length() != 16) {
            logger.warn("ReceiverId " + receiverId + " is not a UUID of expected length 16");
        }
        this.clientId = clientIdPrefix + "joynr:" + receiverId.substring(0, Math.min(16, receiverId.length()));
    }

    @Override
    public String getClientId() {
        return clientId;
    }

}

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

import static io.joynr.messaging.MessagingPropertyKeys.MQTT_TOPIC_PREFIX_REPLYTO;
import static io.joynr.messaging.MessagingPropertyKeys.MQTT_TOPIC_PREFIX_UNICAST;
import static io.joynr.messaging.MessagingPropertyKeys.MQTT_TOPIC_PREFIX_MULTICAST;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class DefaultMqttTopicPrefixProvider implements MqttTopicPrefixProvider {

    private String replyToPrefix;
    private String unicastPrefix;
    private String multicastPrefix;

    @Inject
    public DefaultMqttTopicPrefixProvider(@Named(MQTT_TOPIC_PREFIX_REPLYTO) String replyToPrefix,
                                          @Named(MQTT_TOPIC_PREFIX_UNICAST) String unicastPrefix,
                                          @Named(MQTT_TOPIC_PREFIX_MULTICAST) String multicastPrefix) {
        this.replyToPrefix = replyToPrefix;
        this.unicastPrefix = unicastPrefix;
        this.multicastPrefix = multicastPrefix;
    }

    @Override
    public String getMulticastTopicPrefix() {
        return multicastPrefix;
    }

    @Override
    public String getSharedSubscriptionsReplyToTopicPrefix() {
        return replyToPrefix;
    }

    @Override
    public String getUnicastTopicPrefix() {
        return unicastPrefix;
    }

}

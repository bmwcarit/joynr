package io.joynr.messaging.mqtt;

/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

public interface MqttTopicPrefixProvider {

    /**
     * Retrieve MQTT topic prefix for multicast.
     *
     * @return return the specified prefix as String
     */
    String getMulticastTopicPrefix();

    /**
     * Retrieve MQTT topic prefix for replyTo when using shared subscriptions.
     * If shared subscriptions are disabled, the unicast prefix is used.
     *
     * @return return the specified prefix as String
     */
    String getSharedSubscriptionsReplyToTopicPrefix();

    /**
     * Retrieve MQTT topic prefix for unicast.
     * Can be used to set the cluster topic (shared by all nodes in the cluster).
     *
     * @return return the specified prefix as String
     */
    String getUnicastTopicPrefix();
}

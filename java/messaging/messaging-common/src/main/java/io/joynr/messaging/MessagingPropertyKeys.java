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
package io.joynr.messaging;

public final class MessagingPropertyKeys {
    //NOTE: all property identifiers must be lower-case only.
    public static final String CHANNELID = "joynr.messaging.channelid"; //NOT USUALLY SET BY THE APPLICATION!
    public static final String BOUNCE_PROXY_URL = "joynr.messaging.bounceproxyurl";
    public static final String RECEIVERID = "joynr.messaging.receiverid"; //NEVER SET BY THE APPLICATION!
    public static final String PERSISTENCE_FILE = "joynr.messaging.persistence_file";
    public static final String DEFAULT_PERSISTENCE_FILE = "joynr.properties";
    public static final String DEFAULT_MESSAGING_PROPERTIES_FILE = "defaultMessaging.properties";

    public static final String CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY = "joynr.messaging.capabilitiesdirectory.discoveryentry";

    public static final String JOYNR_PROPERTIES = "joynr.properties";

    public static final String PROPERTY_SERVLET_CONTEXT_ROOT = "joynr.servlet.context.root";
    public static final String PROPERTY_SERVLET_SHUTDOWN_TIMEOUT = "joynr.servlet.shutdown.timeout";
    public static final String PROPERTY_SERVLET_HOST_PATH = "joynr.servlet.hostpath";
    public static final String PROPERTY_SERVLET_SKIP_LONGPOLL_DEREGISTRATION = "joynr.servlet.skiplongpollderegistration";
    public static final String PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT = "joynr.messaging.primaryglobaltransport";
    public static final String PROPERTY_MESSAGING_COMPRESS_REPLIES = "joynr.messaging.compressreplies";

    public static final String MQTT_TOPIC_PREFIX_REPLYTO = "joynr.messaging.mqtt.topicprefix.sharedsubscriptionsreplyto";
    public static final String MQTT_TOPIC_PREFIX_UNICAST = "joynr.messaging.mqtt.topicprefix.unicast";
    public static final String MQTT_TOPIC_PREFIX_MULTICAST = "joynr.messaging.mqtt.topicprefix.multicast";
    public static final String GBID_ARRAY = "joynr.internal.messaging.gbidArray";

    private MessagingPropertyKeys() {
        throw new AssertionError();
    }
}

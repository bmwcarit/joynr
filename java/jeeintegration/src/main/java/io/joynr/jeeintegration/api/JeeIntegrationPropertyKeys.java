package io.joynr.jeeintegration.api;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

/**
 * The keys for the properties which can be set for the joynr JEE integration.
 */
public class JeeIntegrationPropertyKeys {

    /**
     * Use this property key to set the value for the URI of the HTTP Bridge
     * endpoint registry used by the JEE integration to register itself
     * as a recipient of joynr messages for a given topic.
     * Its value is 'joynr.jeeintegration.endpointregistry.uri'.
     * You only need to specify a value for this property if you have the
     * {@link #JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY HTTP Bridge enabled}.
     *
     * @see io.joynr.jeeintegration.httpbridge.HttpBridgeEndpointRegistryClient
     */
    public static final String JEE_INTEGRATION_ENDPOINTREGISTRY_URI = "joynr.jeeintegration.endpointregistry.uri";

    /**
     * This property holds the key used to lookup the managed scheduled executor service from the container.
     */
    public static final String JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE = "concurrent/joynrMessagingScheduledExecutor";

    /**
     * This key is used to check whether the JEE integration should activate
     * the HTTP bridge, which has two consequences - one, the integration
     * will attempt to register itself with an endpoint registry
     * (See {@link io.joynr.integration.httpbridge.HttpBridgeRegistryClient}),
     * and two, the {@link io.joynr.jeeintegration.messaging.JeeMqttMessageSendingModule}
     * will register the {@link io.joynr.jeeintegration.messaging.NoOpMessagingSkeleton}
     * to prevent the MQTT messaging module to subscribe to the channel's
     * topic on the broker, because the messages arriving at the broker will
     * be forwarded via HTTP.
     *
     * See {@link io.joynr.jeeintegration.messaging.JeeMqttMessageSendingModule}.
     */
    public static final String JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY = "joynr.jeeintegration.enable.httpbridge";

    /**
     * Use this key to activate shared subscription support by setting the property's value to <code>true</code>. Shared
     * subscriptions are a feature of HiveMQ which allow queue semantics to be used for subscribers to MQTT topics. That
     * is, only one subscriber receives a message, rather than all subscribers. This feature can be used to load balance
     * incoming messages on MQTT. This feature is useful if you want to run a cluster of JEE nodes while using only MQTT
     * for communication (an alternative is to use the {@link #JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY HTTP bridge}
     * configuration).
     */
    public static final String JEE_ENABLE_SHARED_SUBSCRIPTIONS = "joynr.jeeintegration.enable.sharedsubscriptions";

}

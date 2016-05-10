/**
 * 
 */
package io.joynr.jeeintegration.api;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
     * 
     * @see io.joynr.jeeintegration.httpbridge.HttpBridgeEndpointRegistryClient
     */
    public static final String JEE_INTEGRATION_ENDPOINTREGISTRY_URI = "joynr.jeeintegration.endpointregistry.uri";

    /**
     * This property holds the key used to lookup the managed scheduled executor service from the container.
     */
    public static final String JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE = "concurrent/joynrMessagingScheduledExecutor";

}

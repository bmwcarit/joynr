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
package io.joynr.jeeintegration.api;

/**
 * The keys for the properties which can be set for the joynr JEE integration.
 */
public class JeeIntegrationPropertyKeys {

    /**
     * This key identifies the managed scheduled executor service that is required by joynr. It has to be provided with
     * this name by the container, e.g. Payara.
     */
    public static final String JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE = "concurrent/joynrMessagingScheduledExecutor";

    /**
     * This key is used to configure the number of provider registration retry attempts for a single provider.
     * <br>
     * Joynr JEE integration automatically registers providers during startup (deployment). If the registration fails,
     * it will retry the registration until it succeeds or the maximum number of retry attempts is reached.
     * <br>
     * See also {@link JeeIntegrationPropertyKeys#PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS}.
     */
    public static final String PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES = "joynr.jeeintegration.registration.retries";
    /**
     * This key is used to configure the wait time between provider registration retry attempts for a single provider.
     * <br>
     * Joynr JEE integration automatically registers providers during startup (deployment). If the registration fails,
     * it will retry the registration until it succeeds or the maximum number of retry attempts is reached.
     * <br>
     * See also {@link JeeIntegrationPropertyKeys#PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES}.
     */
    public static final String PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS = "joynr.jeeintegration.registration.retryintervalms";
    /**
     * This key is used to enable or disable awaitRegistration and registration retries for the automatic provider
     * registration in joynr JEE.
     * <br>
     * See also {@link JeeIntegrationPropertyKeys#PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS} and
     * {@link JeeIntegrationPropertyKeys#PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES}.
     */
    public static final String PROPERTY_JEE_AWAIT_REGISTRATION = "joynr.jeeintegration.awaitregistration";

}

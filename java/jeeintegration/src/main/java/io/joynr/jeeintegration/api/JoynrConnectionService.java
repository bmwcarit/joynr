/*-
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
 * Inject this service into your application in order to programmatically trigger the mqtt subscription to the shared subscriptions
 * topic. This is allowed only if the automatic subscription on startup was disabled via
 * {@link JeeIntegrationPropertyKeys#PROPERTY_KEY_JEE_SUBSCRIBE_ON_STARTUP}.
 */
public interface JoynrConnectionService {

    /**
     * Call this method when your providers are ready to handle requests. It will trigger the MQTT subscription to the shared subscription topic.
     *
     * Use only if the automatic subscription on startup was disabled via {@link JeeIntegrationPropertyKeys#PROPERTY_KEY_JEE_SUBSCRIBE_ON_STARTUP}.
     */
    void notifyReadyForRequestProcessing();

}

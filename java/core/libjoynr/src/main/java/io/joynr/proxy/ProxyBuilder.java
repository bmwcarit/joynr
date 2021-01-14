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
package io.joynr.proxy;

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;

/**
 * Builds a proxy instance for the given interface {@literal <T>}. Default proxy properties can be overwritten by the
 * set...Qos methods. After calling build the proxy can be used like a local instance of the provider. All invocations
 * will be queued until either the message TTL expires or the arbitration finishes successfully. Synchronous calls will
 * block until the arbitration is done.
 *
 * @param <T>
 *            Provided interface
 */
public interface ProxyBuilder<T> {

    /**
     * Callback for async proxy creation
     *
     * @param <T> Provided interface
     */
    public interface ProxyCreatedCallback<T> {

        /**
         * Called when the proxy is created and ready to use.
         * Discovery has completed successfully
         *
         * @param result result of proxy creation
         */
        void onProxyCreationFinished(T result);

        /**
         * Called when an error occurred during proxy creation.
         *
         * @param error A JoynrRuntimeException that prevented proxy creation
         */
        void onProxyCreationError(JoynrRuntimeException error);
    }

    /**
     * @return The proxies participantId
     */
    // TODO should this really be public ?
    String getParticipantId();

    void setParticipantId(String participantId);

    /**
     * Sets arbitration strategy, timeout and strategy specific parameters.
     *
     * @param discoveryQos discovery quality of service
     * @return Returns the ProxyBuilder
     * @throws DiscoveryException in case arbitration fails
     */
    ProxyBuilder<T> setDiscoveryQos(DiscoveryQos discoveryQos) throws DiscoveryException;

    /**
     * Sets the MessagingQos (e.g. request timeouts) which will be used by the created proxy.
     *
     * @param messagingQos messaging quality of service
     * @return Returns the ProxyBuilder
     */
    ProxyBuilder<T> setMessagingQos(MessagingQos messagingQos);

    /**
     * If you want to use any {@link io.joynr.StatelessAsync} calls, then you must provide a use case name, which will
     * be used in constructing the relevant stateless async callback IDs, so that when Reply payloads arrive at a
     * runtime the latter is able to discern which callback handler instance to route the data to.
     *
     * @param useCase the use case for which the proxy is being used, in order to construct stateless async callback IDs
     *        mapping to the correct callback handler.
     * @return the proxy builder instance.
     */
    ProxyBuilder<T> setStatelessAsyncCallbackUseCase(String useCase);

    /**
     * Sets the GBIDs (Global Backend Identifiers) to select the backends in which the provider will be discovered.<br>
     * Global discovery (if enabled in DiscoveryQos) will be done via the GlobalCapabilitiesDirectory in the backend of
     * the first provided GBID.<br>
     * By default, providers will be discovered in all backends known to the cluster controller via the
     * GlobalCapabilitiesDirectory in the default backend.
     *
     * @param gbids an array of GBIDs
     * @return Returns the ProxyBuilder
     * @throws IllegalArgumentException if provided gbids array is null or empty
     */
    ProxyBuilder<T> setGbids(String[] gbids);

    /**
     * Final step to create a proxy object. Make sure all QoS parameters have been set before this method is called. Non
     * blocking.
     *
     * @return Returns a dynamic proxy object, implementing all methods of the interfaces passed in when the
     *         proxyBuilder was created.
     */
    T build();

    /**
     * Passes a callback to {@link build} that is called when the proxy is finished
     *
     * @param callback callback for asynchronous handling
     * @return A dynamic proxy object, implementing all methods of the interfaces passed in when the proxyBuilder was
     *         created.<br>
     *         Returns <code>null</code> if an exception was thrown. The exception will be reported via the callback.
     */
    T build(ProxyCreatedCallback<T> callback);

    /**
     * For internal use only!
     * @param arbitrationResult ArbitrationResult
     * @return proxy object
     */
    T build(ArbitrationResult arbitrationResult);

}

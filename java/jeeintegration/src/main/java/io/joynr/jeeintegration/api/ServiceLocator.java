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

import java.util.Set;
import java.util.concurrent.CompletableFuture;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.ProxyBuilder;

/**
 * The container runtime will provide an instance of this which can be injected and then used in order to obtain client
 * proxies for services.
 */
public interface ServiceLocator {

    /**
     * Obtains a client proxy for the given service interface in the given domain. Calls
     * {@link #get(Class, String, MessagingQos, DiscoveryQos)} with default values for messaging and discovery quality
     * of service.
     *
     * @param <I>
     *            the service type
     * @param serviceInterface
     *            the service interface for which to get a client proxy.
     * @param domain
     *            the domain in which the service implementation must be available.
     *
     * @return the client proxy, which can be used to call methods on the service service.
     *
     * @deprecated prefer using the builder.
     */
    @Deprecated
    <I> I get(Class<I> serviceInterface, String domain);

    /**
     * Like {@link #get(Class, String)}, but allows you to specify the maximum time-to-live for messages sent to the
     * service. Calls {@link #get(Class, String, MessagingQos, DiscoveryQos)}, setting the TTL passed in on the
     * messaging quality of service and providing default values for the discovery quality of service.
     *
     * @param <I>
     *            the service type
     * @param serviceInterface
     *            the service interface for which to get a client proxy.
     * @param domain
     *            the domain in which the service implementation must be available.
     * @param ttl
     *            the maximum time to live for messages sent via the service. If the time to live expires (the message
     *            takes longer to deliver), then an exception is thrown.
     *
     * @return the client proxy, which can be used to call methods on the service interface.
     *
     * @deprecated prefer using the builder.
     */
    @Deprecated
    <I> I get(Class<I> serviceInterface, String domain, long ttl);

    /**
     * Like {@link #get(Class, String)}, but allows you to specify the messaging and discovery quality of service
     * information to use.
     *
     * @param <I>
     *            the service type
     * @param serviceInterface
     *            the service interface for which to get a client proxy.
     * @param domain
     *            the domain in which the service implementation must be available.
     * @param messagingQos
     *            the messaging quality of service information (including the time to live value) to use when
     *            communicating to the service.
     * @param discoveryQos
     *            the discovery quality of service information to use when communicating to the service.
     *
     * @return the client proxy, which can be used to call methods on the service service.
     *
     * @deprecated prefer using the builder.
     */
    @Deprecated
    <I> I get(Class<I> serviceInterface, String domain, MessagingQos messagingQos, DiscoveryQos discoveryQos);

    /**
     * Obtains a client proxy for multiple services of the given service interface in the given domains. Calls
     * {@link #get(Class, Set, MessagingQos, DiscoveryQos)} with default values for messaging and discovery quality
     * of service.
     *
     * @param <I>
     *            the service type
     * @param serviceInterface
     *            the BCI interface for which to get a client proxy.
     * @param domains
     *            the set of domains in which the service implementations must be available.
     *
     * @return the client proxy, which can be used to call methods on the BCI service.
     *
     * @deprecated prefer using the builder.
     */
    @Deprecated
    <I> I get(Class<I> serviceInterface, Set<String> domains);

    /**
     * Like {@link #get(Class, Set)}, but allows you to specify the maximum time-to-live for messages sent to the
     * services. Calls {@link #get(Class, Set, MessagingQos, DiscoveryQos)}, setting the TTL passed in on the
     * messaging quality of service and providing default values for the discovery quality of service.
     *
     * @param <I>
     *            the service type
     * @param serviceInterface
     *            the BCI interface for which to get a client proxy.
     * @param domains
     *            the set of domains in which the service implementations must be available.
     * @param ttl
     *            the maximum time to live for messages sent via the service. If the time to live expires (the message
     *            takes longer to deliver), then an exception is thrown.
     *
     * @return the client proxy, which can be used to call methods on the BCI service.
     *
     * @deprecated prefer using the builder.
     */
    @Deprecated
    <I> I get(Class<I> serviceInterface, Set<String> domains, long ttl);

    /**
     * Like {@link #get(Class, Set)}, but allows you to specify the messaging and discovery quality of service
     * information to use.
     *
     * @param <I>
     *            the service type
     * @param serviceInterface
     *            the BCI interface for which to get a client proxy.
     * @param domains
     *            the set of domains in which the service implementations must be available.
     * @param messagingQos
     *            the messaging quality of service information (including the time to live value) to use when
     *            communicating to the service.
     * @param discoveryQos
     *            the discovery quality of service information to use when communicating to the service.
     *
     * @return the client proxy, which can be used to call methods on the BCI service.
     *
     * @deprecated prefer using the builder.
     */
    @Deprecated
    <I> I get(Class<I> serviceInterface, Set<String> domains, MessagingQos messagingQos, DiscoveryQos discoveryQos);

    /**
     * Like {@link #get(Class, Set)}, but allows you to specify the messaging and discovery quality of service
     * information to use, as well as the stateless async use case in order to identify the {@link CallbackHandler}
     * bean to use for processing stateless async replies received for requests made from this proxy.
     *
     * @param <I>
     *            the service type
     * @param serviceInterface
     *            the BCI interface for which to get a client proxy.
     * @param domains
     *            the set of domains in which the service implementations must be available.
     * @param messagingQos
     *            the messaging quality of service information (including the time to live value) to use when
     *            communicating to the service.
     * @param discoveryQos
     *            the discovery quality of service information to use when communicating to the service.
     * @param useCase the name of the use case for identifying the relevant {@link CallbackHandler} bean to use for
     *                processing stateless async replies received for requests made from this proxy.
     *
     * @return the client proxy, which can be used to call methods on the BCI service.
     *
     * @deprecated prefer using the builder.
     */
    @Deprecated
    <I> I get(Class<I> serviceInterface,
              Set<String> domains,
              MessagingQos messagingQos,
              DiscoveryQos discoveryQos,
              String useCase);

    /**
     * Builder allowing you to set the various properties necessary for constructing a service proxy.
     *
     * The proxy returned might not be immediately connected to the provider, in which case any calls will be queued
     * until such a time as the provider has been discovered and connected.
     *
     * If you want to ensure that you don't make any calls to the proxy until arbitration was successful (i.e. the proxy
     * is connected to an actual provider), then prefer using the {@link #useFuture()} method to obtain a version of
     * the proxy builder which returns a <code>CompletableFuture</code> which isn't completed until the proxy is
     * successfully connected, or which will complete exceptionally if no provider can be found in time.
     *
     * @param <T> the type of the service for which to build a proxy.
     */
    interface ServiceProxyBuilder<T> {

        /**
         * Set the time to live for messages being sent from the resulting proxy.
         * Subsequent calls to {@link #withMessagingQos(MessagingQos)} will override any value you set via this
         * method. If you want to set other messaging QoS values, then you should simply set the TTL on a
         * messaging QoS instance and call {@link #withMessagingQos(MessagingQos)}.
         *
         * @param ttl the time to live to use when sending messages from the proxy.
         * @return the builder.
         */
        ServiceProxyBuilder<T> withTtl(long ttl);

        /**
         * Set the messaging quality-of-service to use when sending messages from the resulting proxy.
         *
         * @param messagingQos the messaging quality-of-service to use.
         * @return the builder.
         */
        ServiceProxyBuilder<T> withMessagingQos(MessagingQos messagingQos);

        /**
         * Set the discovery quality-of-service to use when looking up the service which the proxy will talk to.
         *
         * @param discoveryQos the discovery quality-of-service to use.
         * @return the builder.
         */
        ServiceProxyBuilder<T> withDiscoveryQos(DiscoveryQos discoveryQos);

        /**
         * Set the stateless async use case name use to find the relevant {@link CallbackHandler} to use for processing
         * stateless async replies resulting from requests sent from this proxy.
         *
         * @param useCase stateless async use case name
         * @return the builder.
         */
        ServiceProxyBuilder<T> withUseCase(String useCase);

        /**
         * Set the GBIDs to use when looking up the service which the proxy will talk to.
         *
         * @param gbids an array of GBIDs
         * @return the builder.
         */
        ServiceProxyBuilder<T> withGbids(String[] gbids);

        /**
         * Set the callback to use when creating the proxy. The callback can be used to be informed of when the proxy
         * is successfully attached to the provider or in the case that this fails.
         *
         * @param callback the callback to use when building the proxy.
         * @return the builder.
         */
        ServiceProxyBuilder<T> withCallback(ProxyBuilder.ProxyCreatedCallback<T> callback);

        /**
         * If you want to have a <code>CompletableFuture</code> returned which will complete when the proxy has been
         * fully created and initialised successfully, then call this method before calling {@link #build()}.
         *
         * This also allows you to react to any error encountered while attempting to initialise the proxy, such as
         * encountering discovery timeouts if the requested provider isn't available in time.
         *
         * If you want to additionally use a {@link #withCallback(ProxyBuilder.ProxyCreatedCallback) callback}
         * make sure to set the callback on the builder before calling this method, as it will not be possible any
         * more after the call to <code>useFuture()</code>.
         *
         * @return returns a version of the proxy builder which returns a <code>CompletableFuture</code> for the proxy
         * rather than the proxy itself.
         */
        ServiceProxyBuilder<CompletableFuture<T>> useFuture();

        /**
         * Create the service proxy using all of the settings previously setup using the 'with*' methods.
         *
         * @return the service proxy. Alternatively a <code>CompletableFuture</code> which will yield the service
         * proxy upon successful completion if you previously called {@link #useFuture()}.
         */
        T build();
    }

    /**
     * Create a {@link ServiceProxyBuilder} for the given service interface and domains.
     *
     * @param serviceInterface the service interface for which to create the service proxy builder.
     * @param domains the list of domains which the proxy will use for looking up the services to communicate with. At least one domain must be provided.
     * @param <I> the type of the service interface for which a service proxy builder should be created.
     * @return the {@link ServiceProxyBuilder} which can be used to setup the configuration with which to build the service proxy.
     */
    <I> ServiceProxyBuilder<I> builder(Class<I> serviceInterface, String... domains);

    GuidedProxyBuilder getGuidedProxyBuilder(Class<?> interfaceClass, Set<String> domains);
}

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

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.GuidedProxyBuilder;

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
     */
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
     */
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
     */
    <I> I get(Class<I> serviceInterface, String domain, MessagingQos messagingQos, DiscoveryQos discoveryQos);

    /**
     * Obtains a client proxy for multiple services of the given service interface in the given domains. Calls
     * {@link #get(Class, Set<String>, MessagingQos, DiscoveryQos)} with default values for messaging and discovery quality
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
     */
    <I> I get(Class<I> serviceInterface, Set<String> domains);

    /**
     * Like {@link #get(Class, Set<String>)}, but allows you to specify the maximum time-to-live for messages sent to the
     * services. Calls {@link #get(Class, Set<String>, MessagingQos, DiscoveryQos)}, setting the TTL passed in on the
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
     */
    <I> I get(Class<I> serviceInterface, Set<String> domains, long ttl);

    /**
     * Like {@link #get(Class, Set<String>)}, but allows you to specify the messaging and discovery quality of service
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
     */
    <I> I get(Class<I> serviceInterface, Set<String> domains, MessagingQos messagingQos, DiscoveryQos discoveryQos);

    /**
     * Like {@link #get(Class, Set<String>)}, but allows you to specify the messaging and discovery quality of service
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
     */
    <I> I get(Class<I> serviceInterface,
              Set<String> domains,
              MessagingQos messagingQos,
              DiscoveryQos discoveryQos,
              String useCase);

    /**
     * Builder allowing you to set the various properties necessary for constructing a service proxy.
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
         * @param useCase
         * @return the builder.
         */
        ServiceProxyBuilder<T> withUseCase(String useCase);

        /**
         * Create the service proxy using all of the settings previously setup using the 'with*' methods.
         *
         * @return the service proxy.
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

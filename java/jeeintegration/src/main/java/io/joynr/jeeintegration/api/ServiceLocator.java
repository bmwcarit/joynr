/**
 *
 */
package io.joynr.jeeintegration.api;

import java.util.Set;

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

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;

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

}

package io.joynr.runtime;

/*
 * #%L
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

import io.joynr.capabilities.RegistrationFuture;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.ProxyBuilder;

/**
 * Central Joyn Api object, used to register / unregister providers and create proxy builders
 * 
 */
public interface JoynrRuntime {

    /**
     * Registers a provider in the joynr framework
     * 
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @param authenticationToken
     *            Token to authenticate the provider. Should be persistent between application startups.
     * @return Returns a RegistrationFuture which can be used to check the local and global registration status.
     */
    RegistrationFuture registerCapability(String domain, JoynrProvider provider, String authenticationToken);

    /**
     * Unregisters the provider from the joynr framework. It can no longer be used or discovered.
     * 
     * @param domain
     *            The domain the provider was registered for.
     * @param provider
     *            The provider instance.
     */
    void unregisterCapability(String domain, JoynrProvider provider, String authenticationToken);

    /**
     * Returns a proxy builder instance to build a proxy object.
     * 
     * @param participantId
     *            ParticipantId for the created proxy.
     * @param proxyInterface
     *            Interface the proxy should implement (extending async, sync & subscription interfaces)
     * @param syncInterface
     *            Synchronous interface of the service.
     * @param asyncInterface
     *            Asynchronous interface of the service.
     * @param domain
     *            Domain of the provider.
     * @param interfaceClass
     *            Interface the provider offers.
     * @return After setting arbitration, proxy and messaging QoS parameters the returned ProxyBuilder can be used to
     *         build the proxy instance.
     */
    <T extends JoynrInterface> ProxyBuilder<T> getProxyBuilder(final String domain, final Class<T> interfaceClass);

    /**
     * Shutdown the joynr instance:
     * <ul>
     * <li>Discards pending outgoing messages
     * <li>Does not wait for incoming messages
     * </ul>
     * 
     * @param clear
     *            If true, the instance removes all artifacts it created:
     *            <ul>
     *            <li>all global capabilities are deregistered from the Global Capabilities Directory
     *            <li>The channel is deregistered from the Channel Url Directory
     *            <li>The channel is removed from the bounce proxy
     *            </ul>
     */
    void shutdown(boolean clear);
}

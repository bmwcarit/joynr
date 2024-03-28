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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.runtime.ShutdownNotifier;
import joynr.system.DiscoveryAsync;
import io.joynr.messaging.MessagingPropertyKeys;

public class ProxyBuilderFactoryImpl implements ProxyBuilderFactory {

    private final DiscoveryAsync localDiscoveryAggregator;
    private final ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;
    private final ShutdownNotifier shutdownNotifier;
    private final StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory;
    private final long maxMessagingTtl;
    private final long defaultDiscoveryTimeoutMs;
    private final long defaultDiscoveryRetryIntervalMs;
    private final long minimumArbitrationRetryDelay;
    private final boolean separateReplyReceiver;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public ProxyBuilderFactoryImpl(DiscoveryAsync localDiscoveryAggregator,
                                   ProxyInvocationHandlerFactory proxyInvocationHandlerFactory,
                                   ShutdownNotifier shutdownNotifier,
                                   StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_TTL_MS) long maxMessagingTtl,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS) long defaultDiscoveryTimeoutMs,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS) long defaultDiscoveryRetryIntervalMs,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS) long minimumArbitrationRetryDelay,
                                   @Named(MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER) boolean separateReplyReceiver) {

        // CHECKSTYLE:ON
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
        this.shutdownNotifier = shutdownNotifier;
        this.statelessAsyncCallbackDirectory = statelessAsyncCallbackDirectory;
        this.maxMessagingTtl = maxMessagingTtl;
        this.defaultDiscoveryTimeoutMs = defaultDiscoveryTimeoutMs;
        this.defaultDiscoveryRetryIntervalMs = defaultDiscoveryRetryIntervalMs;
        this.minimumArbitrationRetryDelay = minimumArbitrationRetryDelay;
        this.separateReplyReceiver = separateReplyReceiver;
    }

    @Override
    public <T> ProxyBuilder<T> get(String domain, Class<T> interfaceClass) {
        return get(new HashSet<String>(Arrays.asList(domain)), interfaceClass);
    }

    @Override
    public <T> ProxyBuilder<T> get(Set<String> domains, Class<T> interfaceClass) {
        return new ProxyBuilderDefaultImpl<>(localDiscoveryAggregator,
                                             domains,
                                             interfaceClass,
                                             proxyInvocationHandlerFactory,
                                             shutdownNotifier,
                                             statelessAsyncCallbackDirectory,
                                             maxMessagingTtl,
                                             defaultDiscoveryTimeoutMs,
                                             defaultDiscoveryRetryIntervalMs,
                                             minimumArbitrationRetryDelay,
                                             separateReplyReceiver);
    }
}

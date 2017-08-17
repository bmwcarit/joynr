package io.joynr.proxy;

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

import io.joynr.messaging.ConfigurableMessagingSettings;
import java.util.Set;

import javax.inject.Inject;

import com.google.common.collect.Sets;
import com.google.inject.name.Named;

import joynr.system.DiscoveryAsync;

public class ProxyBuilderFactoryImpl implements ProxyBuilderFactory {

    private final DiscoveryAsync localDiscoveryAggregator;
    private final ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;
    private final long maxMessagingTtl;
    private final long defaultDiscoveryTimeoutMs;
    private final long defaultDiscoveryRetryIntervalMs;

    @Inject
    public ProxyBuilderFactoryImpl(DiscoveryAsync localDiscoveryAggregator,
                                   ProxyInvocationHandlerFactory proxyInvocationHandlerFactory,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_TTL_MS) long maxMessagingTtl,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS) long defaultDiscoveryTimeoutMs,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_RETRY_INTERVAL_MS) long defaultDiscoveryRetryIntervalMs) {
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
        this.maxMessagingTtl = maxMessagingTtl;
        this.defaultDiscoveryTimeoutMs = defaultDiscoveryTimeoutMs;
        this.defaultDiscoveryRetryIntervalMs = defaultDiscoveryRetryIntervalMs;
    }

    @Override
    public <T> ProxyBuilder<T> get(String domain, Class<T> interfaceClass) {
        return get(Sets.newHashSet(domain), interfaceClass);
    }

    @Override
    public <T> ProxyBuilder<T> get(Set<String> domains, Class<T> interfaceClass) {
        return new ProxyBuilderDefaultImpl<>(localDiscoveryAggregator,
                                             domains,
                                             interfaceClass,
                                             proxyInvocationHandlerFactory,
                                             maxMessagingTtl,
                                             defaultDiscoveryTimeoutMs,
                                             defaultDiscoveryRetryIntervalMs);
    }
}

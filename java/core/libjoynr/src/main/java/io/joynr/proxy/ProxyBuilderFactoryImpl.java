package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import io.joynr.messaging.routing.MessageRouter;
import java.util.Set;

import javax.inject.Inject;

import com.google.common.collect.Sets;
import com.google.inject.name.Named;

import io.joynr.runtime.SystemServicesSettings;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;

public class ProxyBuilderFactoryImpl implements ProxyBuilderFactory {

    private final DiscoveryAsync localDiscoveryAggregator;
    private final ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;
    private final MessageRouter messageRouter;
    private final Address libjoynrMessagingAddress;
    private final long maxMessagingTtl;

    @Inject
    public ProxyBuilderFactoryImpl(DiscoveryAsync localDiscoveryAggregator,
                                   ProxyInvocationHandlerFactory proxyInvocationHandlerFactory,
                                   MessageRouter messageRouter,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_TTL_MS) long maxMessagingTtl,
                                   @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address libjoynrMessagingAddress) {
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
        this.messageRouter = messageRouter;
        this.maxMessagingTtl = maxMessagingTtl;
        this.libjoynrMessagingAddress = libjoynrMessagingAddress;
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
                                             messageRouter,
                                             maxMessagingTtl,
                                             libjoynrMessagingAddress);
    }
}

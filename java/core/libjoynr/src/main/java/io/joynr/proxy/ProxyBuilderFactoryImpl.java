package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;

import javax.inject.Inject;

import com.google.inject.name.Named;

import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;

public class ProxyBuilderFactoryImpl implements ProxyBuilderFactory {

    private final DiscoveryAsync localDiscoveryAggregator;
    private final ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;
    private final MessageRouter messageRouter;
    private final Address libjoynrMessagingAddress;

    @Inject
    public ProxyBuilderFactoryImpl(DiscoveryAsync localDiscoveryAggregator,
                                   ProxyInvocationHandlerFactory proxyInvocationHandlerFactory,
                                   MessageRouter messageRouter,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress) {
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
        this.messageRouter = messageRouter;
        this.libjoynrMessagingAddress = libjoynrMessagingAddress;
    }

    public <T extends JoynrInterface> ProxyBuilder<T> get(String domain, Class<T> interfaceClass) {
        return new ProxyBuilderDefaultImpl<>(localDiscoveryAggregator,
                domain,
                interfaceClass,
                proxyInvocationHandlerFactory,
                messageRouter,
                libjoynrMessagingAddress);
    }
}

package io.joynr.runtime;

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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

import org.reflections.Reflections;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.AddressOperation;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.subtypes.JoynrType;
import joynr.BroadcastSubscriptionRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.system.RoutingTypes.Address;
import joynr.types.ProviderQos;

abstract public class JoynrRuntimeImpl implements JoynrRuntime {

    private static final Logger logger = LoggerFactory.getLogger(JoynrRuntimeImpl.class);

    @Inject
    private CapabilitiesRegistrar capabilitiesRegistrar;

    private Dispatcher dispatcher;

    @Inject
    public ObjectMapper objectMapper;

    @Inject
    ShutdownNotifier shutdownNotifier;

    @Inject
    @Named(JOYNR_SCHEDULER_CLEANUP)
    ScheduledExecutorService cleanupScheduler;

    private final ProxyBuilderFactory proxyBuilderFactory;

    // CHECKSTYLE:OFF
    @Inject
    public JoynrRuntimeImpl(ObjectMapper objectMapper,
                            ProxyBuilderFactory proxyBuilderFactory,
                            Dispatcher dispatcher,
                            MessagingSkeletonFactory messagingSkeletonFactory,
                            LocalDiscoveryAggregator localDiscoveryAggregator,
                            RoutingTable routingTable,
                            @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                            @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                            @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress) {
        // CHECKSTYLE:ON
        this.dispatcher = dispatcher;
        this.objectMapper = objectMapper;

        Reflections reflections = new Reflections("joynr");
        Set<Class<? extends JoynrType>> subClasses = reflections.getSubTypesOf(JoynrType.class);
        objectMapper.registerSubtypes(subClasses.toArray(new Class<?>[subClasses.size()]));

        Class<?>[] messageTypes = new Class[]{ Request.class, Reply.class, SubscriptionRequest.class,
                SubscriptionStop.class, SubscriptionPublication.class, BroadcastSubscriptionRequest.class };
        objectMapper.registerSubtypes(messageTypes);
        this.proxyBuilderFactory = proxyBuilderFactory;
        if (dispatcherAddress instanceof InProcessAddress) {
            ((InProcessAddress) dispatcherAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        routingTable.apply(new AddressOperation() {
            @Override
            public void perform(Address address) {
                if (address instanceof InProcessAddress && ((InProcessAddress) address).getSkeleton() == null) {
                    ((InProcessAddress) address).setSkeleton(new InProcessLibjoynrMessagingSkeleton(JoynrRuntimeImpl.this.dispatcher));
                }
            }
        });

        if (discoveryProviderAddress instanceof InProcessAddress) {
            ((InProcessAddress) discoveryProviderAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }

        localDiscoveryAggregator.forceQueryOfDiscoveryProxy();

        messagingSkeletonFactory.start();
    }

    @Override
    public <T> ProxyBuilder<T> getProxyBuilder(final String domain, final Class<T> interfaceClass) {
        Set<String> domains = new HashSet<>();
        domains.add(domain);
        return getProxyBuilder(domains, interfaceClass);
    }

    @Override
    public <T> ProxyBuilder<T> getProxyBuilder(final Set<String> domains, final Class<T> interfaceClass) {
        if (domains == null || domains.isEmpty() || domains.contains(null)) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: domain was not set");
        }
        if (interfaceClass == null) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: interfaceClass may not be NULL");
        }
        return proxyBuilderFactory.get(domains, interfaceClass);
    }

    /**
     * Registers a provider in the joynr framework
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @param providerQos
     *            the provider's quality of service settings
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    public Future<Void> registerProvider(String domain, Object provider, ProviderQos providerQos) {
        return capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
    }

    @Override
    public void unregisterProvider(String domain, Object provider) {
        capabilitiesRegistrar.unregisterProvider(domain, provider);

    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN runtime");
        shutdownNotifier.shutdown();
    }
}

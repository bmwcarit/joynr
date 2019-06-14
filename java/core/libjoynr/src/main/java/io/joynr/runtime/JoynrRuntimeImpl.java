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
package io.joynr.runtime;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashSet;
import java.util.Queue;
import java.util.Set;
import java.util.concurrent.ConcurrentLinkedQueue;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.ProvidesJoynrTypesInfo;
import io.joynr.UsedBy;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.AddressOperation;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.JoynrInterface;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.util.AnnotationUtil;
import joynr.BroadcastSubscriptionRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.exceptions.ApplicationException;
import joynr.system.RoutingTypes.Address;
import joynr.types.ProviderQos;

abstract public class JoynrRuntimeImpl implements JoynrRuntime {

    private static final Logger logger = LoggerFactory.getLogger(JoynrRuntimeImpl.class);

    private final StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory;

    @Inject
    private CapabilitiesRegistrar capabilitiesRegistrar;

    private Dispatcher dispatcher;

    @Inject
    public ObjectMapper objectMapper;

    @Inject
    ShutdownNotifier shutdownNotifier;

    private final ProxyBuilderFactory proxyBuilderFactory;

    private Queue<Future<Void>> unregisterProviderQueue = new ConcurrentLinkedQueue<Future<Void>>();

    // CHECKSTYLE:OFF
    @Inject
    public JoynrRuntimeImpl(ObjectMapper objectMapper,
                            ProxyBuilderFactory proxyBuilderFactory,
                            Dispatcher dispatcher,
                            MessagingSkeletonFactory messagingSkeletonFactory,
                            LocalDiscoveryAggregator localDiscoveryAggregator,
                            RoutingTable routingTable,
                            StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory,
                            @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                            @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                            @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress) {
        // CHECKSTYLE:ON
        this.dispatcher = dispatcher;
        this.objectMapper = objectMapper;
        this.statelessAsyncCallbackDirectory = statelessAsyncCallbackDirectory;

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
        ArbitratorFactory.start();

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

        registerInterfaceClassTypes(interfaceClass, "Cannot create ProxyBuilder");
        return proxyBuilderFactory.get(domains, interfaceClass);
    }

    /**
     * Registers a provider in the joynr framework for the default GBID
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
        final boolean awaitGlobalRegistration = false;
        return registerProvider(domain, provider, providerQos, awaitGlobalRegistration);
    }

    /**
     * Registers a provider in the joynr framework for the provided GBIDs
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @param providerQos
     *            the provider's quality of service settings
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout, if required.
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    public Future<Void> registerProvider(String domain,
                                         Object provider,
                                         ProviderQos providerQos,
                                         boolean awaitGlobalRegistration) {
        JoynrInterface joynrInterfaceAnnotatation = AnnotationUtil.getAnnotation(provider.getClass(),
                                                                                 JoynrInterface.class);
        if (joynrInterfaceAnnotatation == null) {
            throw new IllegalArgumentException("The provider object must have a JoynrInterface annotation");
        }
        Class interfaceClass = joynrInterfaceAnnotatation.provides();

        return registerProvider(domain, provider, providerQos, awaitGlobalRegistration, interfaceClass);
    }

    /**
     * Registers a provider in the joynr framework for the provided GBIDs
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @param providerQos
     *            the provider's quality of service settings
     * @param gbids
     *            The GBIDs in which the provider shall be registered. If no GBID is provided then the provider is
     *            registered in the default backend. Provided GBIDs must not be empty.
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout, if required.
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    public Future<Void> registerProvider(String domain,
                                         Object provider,
                                         ProviderQos providerQos,
                                         String[] gbids,
                                         boolean awaitGlobalRegistration) {
        for (String gbid : gbids) {
            if (gbid == null || gbid.equals("")) {
                throw new IllegalArgumentException("Provided gbids must not be empty!");
            }
        }
        JoynrInterface joynrInterfaceAnnotatation = AnnotationUtil.getAnnotation(provider.getClass(),
                                                                                 JoynrInterface.class);
        if (joynrInterfaceAnnotatation == null) {
            throw new IllegalArgumentException("The provider object must have a JoynrInterface annotation");
        }
        Class interfaceClass = joynrInterfaceAnnotatation.provides();
        registerInterfaceClassTypes(interfaceClass, "Cannot registerProvider");
        return capabilitiesRegistrar.registerProvider(domain, provider, providerQos, gbids, awaitGlobalRegistration);
    }

    /**
     * Registers a provider in the joynr framework by JEE
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @param providerQos
     *            the provider's quality of service settings
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout, if required.
     * @param interfaceClass
     *            The interface class of the provider.
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    public Future<Void> registerProvider(String domain,
                                         Object provider,
                                         ProviderQos providerQos,
                                         boolean awaitGlobalRegistration,
                                         final Class<?> interfaceClass) {
        if (interfaceClass == null) {
            throw new IllegalArgumentException("Cannot registerProvider: interfaceClass may not be NULL");
        }

        registerInterfaceClassTypes(interfaceClass, "Cannot registerProvider");
        return capabilitiesRegistrar.registerProvider(domain, provider, providerQos, awaitGlobalRegistration);
    }

    /**
     * Registers a provider in the joynr framework for all known GBIDs.
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @param providerQos
     *            the provider's quality of service settings
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout, if required.
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    public Future<Void> registerInAllKnownBackends(String domain,
                                                   Object provider,
                                                   ProviderQos providerQos,
                                                   boolean awaitGlobalRegistration) {
        JoynrInterface joynrInterfaceAnnotatation = AnnotationUtil.getAnnotation(provider.getClass(),
                                                                                 JoynrInterface.class);
        if (joynrInterfaceAnnotatation == null) {
            throw new IllegalArgumentException("The provider object must have a JoynrInterface annotation");
        }
        Class interfaceClass = joynrInterfaceAnnotatation.provides();
        registerInterfaceClassTypes(interfaceClass, "Cannot registerProvider");
        return capabilitiesRegistrar.registerInAllKnownBackends(domain, provider, providerQos, awaitGlobalRegistration);
    }

    @Override
    public void unregisterProvider(String domain, Object provider) {

        synchronized (unregisterProviderQueue) {
            unregisterProviderQueue.add(capabilitiesRegistrar.unregisterProvider(domain, provider));
        }
    }

    @Override
    public void registerStatelessAsyncCallback(StatelessAsyncCallback statelessAsyncCallback) {
        final UsedBy usedByAnnotation = AnnotationUtil.getAnnotation(statelessAsyncCallback.getClass(), UsedBy.class);
        if (usedByAnnotation == null) {
            throw new IllegalArgumentException("Cannot registerStatelessAsyncCallback: callback has no UsedBy annotation");
        }
        final Class<?> stateProxyClass = usedByAnnotation.value();
        if (stateProxyClass == null) {
            throw new IllegalArgumentException("Cannot registerStatelessAsyncCallback: stateProxyClass may not be NULL");
        }
        registerInterfaceClassTypes(stateProxyClass, "Cannot registerStatelessAsyncCallback");
        statelessAsyncCallbackDirectory.register(statelessAsyncCallback);
    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN runtime");

        synchronized (unregisterProviderQueue) {
            // wait till all unregister provider calls are finished
            while (unregisterProviderQueue.peek() != null) {
                try {
                    logger.trace("unregister Provider Requests pending: " + unregisterProviderQueue.size());
                    // while this waits till a provider is unregistered in the local discovery it won't wait for global
                    Future<Void> unregisterFinished = unregisterProviderQueue.poll();
                    unregisterFinished.get(5000);
                } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                    logger.error("unregister Provider failed", e);
                    break; // break if unregistering a provider takes more than 5s
                }
            }
            shutdownNotifier.shutdown();
        }
    }

    @Override
    public void prepareForShutdown() {
        logger.info("Preparing for shutdown of runtime");
        shutdownNotifier.prepareForShutdown();
    }

    private synchronized void registerInterfaceClassTypes(final Class<?> interfaceClass, String errorPrefix) {
        try {
            ProvidesJoynrTypesInfo providesJoynrTypesInfoAnnotation = AnnotationUtil.getAnnotation(interfaceClass,
                                                                                                   ProvidesJoynrTypesInfo.class);
            Method m = providesJoynrTypesInfoAnnotation.interfaceClass().getDeclaredMethod("getDataTypes");
            Set<Class<?>> subClasses = (Set<Class<?>>) m.invoke(null);
            objectMapper.registerSubtypes(subClasses.toArray(new Class<?>[subClasses.size()]));
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            throw new IllegalArgumentException(errorPrefix + ": failed to register interface data types", e);
        }
    }
}

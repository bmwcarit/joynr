/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.UsedBy;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.ParticipantIdStorage;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.DiscoverySettingsStorage;
import io.joynr.proxy.Future;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.util.AnnotationUtil;
import io.joynr.util.ObjectMapper;
import io.joynr.util.ReflectionUtils;
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

    @Inject
    public ObjectMapper objectMapper;

    private final MessageRouter messageRouter;

    @Inject
    ShutdownNotifier shutdownNotifier;

    private DiscoverySettingsStorage discoverySettingsStorage;

    private final ProxyBuilderFactory proxyBuilderFactory;

    private Queue<Future<Void>> unregisterProviderQueue = new ConcurrentLinkedQueue<Future<Void>>();

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public JoynrRuntimeImpl(ObjectMapper objectMapper,
                            ProxyBuilderFactory proxyBuilderFactory,
                            MessagingSkeletonFactory messagingSkeletonFactory,
                            LocalDiscoveryAggregator localDiscoveryAggregator,
                            RoutingTable routingTable,
                            MessageRouter messageRouter,
                            StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory,
                            DiscoverySettingsStorage discoverySettingsStorage,
                            ParticipantIdStorage participantIdStorage,
                            @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                            @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                            @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address ccMessagingAddress) {
        // CHECKSTYLE:ON
        this.messageRouter = messageRouter;
        this.objectMapper = objectMapper;
        this.statelessAsyncCallbackDirectory = statelessAsyncCallbackDirectory;
        this.discoverySettingsStorage = discoverySettingsStorage;

        Class<?>[] messageTypes = new Class[]{ Request.class, Reply.class, SubscriptionRequest.class,
                SubscriptionStop.class, SubscriptionPublication.class, BroadcastSubscriptionRequest.class };
        objectMapper.registerSubtypes(messageTypes);
        this.proxyBuilderFactory = proxyBuilderFactory;

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
            throw new IllegalArgumentException("Cannot create ProxyBuilder: interfaceClass must not be null");
        }

        registerInterfaceClassTypes(interfaceClass, "Cannot create ProxyBuilder");
        return proxyBuilderFactory.get(domains, interfaceClass);
    }

    /**
     * Registers a provider with the joynr communication framework asynchronously.
     *
     * If registration to local and global scope is requested by 'providerQos' parameter,
     * the provider is registered to all GBIDs configured in the cluster controller.
     *
     * The global capabilities directory identified by the first selected GBID performs
     * the registration.
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     *            It is assumed that the provided implementation offers the following annotations in its
     *            (inherited) class definition: {@link io.joynr.provider.JoynrInterface} and {@link io.joynr.JoynrVersion}.
     * @param providerQos
     *            The provider's quality of service settings.
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    @Deprecated
    public Future<Void> registerProvider(String domain, Object provider, ProviderQos providerQos) {
        final boolean awaitGlobalRegistration = false;
        return registerProvider(domain, provider, providerQos, awaitGlobalRegistration);
    }

    /**
     * Registers a provider with the joynr communication framework asynchronously.
     *
     * If registration to local and global scope is requested by 'providerQos' parameter,
     * the provider is registered to all GBIDs configured in the cluster controller.
     *
     * The global capabilities directory identified by the first selected GBID performs
     * the registration.
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     *            It is assumed that the provided implementation offers the following annotations in its
     *            (inherited) class definition: {@link io.joynr.provider.JoynrInterface} and {@link io.joynr.JoynrVersion}.
     * @param providerQos
     *            The provider's quality of service settings.
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout in case of problems.
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    @Deprecated
    public Future<Void> registerProvider(String domain,
                                         Object provider,
                                         ProviderQos providerQos,
                                         boolean awaitGlobalRegistration) {
        final String[] emptyGbids = new String[0];
        return registerProvider(domain, provider, providerQos, emptyGbids, awaitGlobalRegistration);
    }

    /**
     * Registers a provider with the joynr communication framework asynchronously.
     *
     * If registration to local and global scope is requested by 'providerQos' parameter,
     * the provider is registered to the GBIDs provided with the 'gbids' parameter.
     * If the 'gbids' parameter is empty, the GBIDs configured in the cluster controller are used.
     *
     * The global capabilities directory identified by the first selected GBID performs
     * the registration.
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     *            It is assumed that the provided implementation offers the following annotations in its
     *            (inherited) class definition: {@link io.joynr.provider.JoynrInterface} and {@link io.joynr.JoynrVersion}.
     * @param providerQos
     *            The provider's quality of service settings.
     * @param gbids
     *            Subset of GBIDs configured in the cluster controller for custom global
     *            registration.
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout in case of problems.
     * @return Returns a Future which can be used to check the registration status.
     */
    @Override
    @Deprecated
    public Future<Void> registerProvider(String domain,
                                         Object provider,
                                         ProviderQos providerQos,
                                         String[] gbids,
                                         boolean awaitGlobalRegistration) {
        if (gbids == null) {
            throw new IllegalArgumentException("Cannot registerProvider: gbids must not be null");
        }
        for (String gbid : gbids) {
            if (gbid == null || gbid.equals("")) {
                throw new IllegalArgumentException("Cannot registerProvider: gbid value(s) must not be null or empty");
            }
        }
        JoynrInterface joynrInterfaceAnnotatation = AnnotationUtil.getAnnotation(provider.getClass(),
                                                                                 JoynrInterface.class);
        if (joynrInterfaceAnnotatation == null) {
            throw new IllegalArgumentException("The provider object must have a JoynrInterface annotation");
        }
        Class<?> interfaceClass = joynrInterfaceAnnotatation.provides();
        registerInterfaceClassTypes(interfaceClass, "Cannot registerProvider");
        return capabilitiesRegistrar.registerProvider(domain, provider, providerQos, gbids, awaitGlobalRegistration);
    }

    /**
     * Returns a provider registrar instance to register a provider in the given domain.
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     *            It is assumed that the provided implementation offers the following annotations in its
     *            (inherited) class definition: {@link io.joynr.provider.JoynrInterface} and {@link io.joynr.JoynrVersion}.
     * @return After setting additional parameters, e.g. ProviderQos, Gbids, the returned ProviderRegistrar can be used
     *            to register the provider instance.
     */
    public ProviderRegistrar getProviderRegistrar(String domain, JoynrProvider provider) {
        JoynrInterface joynrInterfaceAnnotatation = AnnotationUtil.getAnnotation(provider.getClass(),
                                                                                 JoynrInterface.class);
        if (joynrInterfaceAnnotatation == null) {
            throw new IllegalArgumentException("The provider object must have a JoynrInterface annotation");
        }
        Class<?> interfaceClass = joynrInterfaceAnnotatation.provides();
        registerInterfaceClassTypes(interfaceClass, "Cannot registerProvider");
        return new ProviderRegistrar(capabilitiesRegistrar, domain, provider);
    }

    public GuidedProxyBuilder getGuidedProxyBuilder(final Set<String> domains, final Class<?> interfaceClass) {
        GuidedProxyBuilder guidedProxyBuilder = new GuidedProxyBuilder(discoverySettingsStorage,
                                                                       domains,
                                                                       interfaceClass,
                                                                       messageRouter);
        return guidedProxyBuilder;
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
            throw new IllegalArgumentException("Cannot registerStatelessAsyncCallback: stateProxyClass must not be null");
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
                    logger.trace("Unregister Provider Requests pending: {}", unregisterProviderQueue.size());
                    // while this waits till a provider is unregistered in the local discovery it won't wait for global
                    Future<Void> unregisterFinished = unregisterProviderQueue.poll();
                    unregisterFinished.get(5000);
                } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
                    logger.error("Unregister Provider failed", e);
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

    protected synchronized void registerInterfaceClassTypes(final Class<?> interfaceClass, String errorPrefix) {
        try {
            Method m = ReflectionUtils.getStaticMethodFromSuperInterfaces(interfaceClass, "getDataTypes");
            @SuppressWarnings("unchecked")
            Set<Class<?>> subClasses = (Set<Class<?>>) m.invoke(null);
            objectMapper.registerSubtypes(subClasses.toArray(new Class<?>[subClasses.size()]));
        } catch (NoSuchMethodException | SecurityException | IllegalAccessException | InvocationTargetException e) {
            throw new IllegalArgumentException(errorPrefix + ": failed to register interface data types", e);
        }
    }
}

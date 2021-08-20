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
package io.joynr.proxy;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.text.MessageFormat;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.ArbitrationCallback;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.Arbitrator;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.util.ObjectMapper;
import io.joynr.util.ReflectionUtils;
import io.joynr.util.VersionUtil;
import joynr.system.DiscoveryAsync;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Version;

/**
 * The GuidedProxyBuilder provides methods to discover providers for a given interface and build a proxy for selected
 * providers. Compared to {@link io.joynr.proxy.ProxyBuilder}, it separates the provider discovery from the actual proxy
 * creation and allows extended (manual) control over the provider selection. The user has full control over the provider
 * selection. Filtering by version and arbitration strategy is skipped in the GuidedProxyBuilder and has to be done manually.
 * The discovered providers are only filtered by support for on change subscriptions if
 * {@link DiscoveryQos#setProviderMustSupportOnChange(boolean)} has been set to <code>true</code>.
 * In contrast to the ProxyBuilder, the provider version is not required to be known in advance. The appropriate Proxy
 * interface class for the selected provider has to be passed to the GuidedProxyBuilder in the second step.
 * <p>
 * The proxy creation with the GuidedProxyBuilder consists of two steps:
 * <ol>
 * <li> Select domains and interface of the provider, set optional additional parameters
 * ({@link #setDiscoveryQos(DiscoveryQos)}, {@link #setGbids(String[])}, {@link #setMessagingQos(MessagingQos)},
 * {@link #setStatelessAsyncCallbackUseCase(String)}) and call {@link #discover()} or {@link #discoverAsync()} to receive
 * either the {@link DiscoveryResult} or a CompletableFuture thereof.<br>
 * NOTE: make sure to call either {@link #buildNone()} or {@link #buildProxy(Class, String)} after a
 * successful discovery.
 * <li> From the discoveryResult, select the entry with the provider that you are looking for and pass its participantId
 * together with the matching Proxy interface class (same interface and same or compatible version as the selected
 * provider) to the {@link #buildProxy(Class, String)} method. In contrast to the ProxyBuilder, this proxy interface class
 * does not necessarily have to be the same as the one used to create the GuidedProxyBuilder and configure the interface
 * for the discovery.<br>
 * Call {@link #buildNone()} instead of {@link #buildProxy(Class, String)} if you do not want to build a proxy
 * for any of the discovered providers.
 * Note that only providers that have been discovered with the most recent discovery will be accepted.
 * </ol>
 *
 * @see io.joynr.proxy.ProxyBuilder
 */
public class GuidedProxyBuilder {

    private static final Logger logger = LoggerFactory.getLogger(GuidedProxyBuilder.class);

    private boolean discoveryCompletedOnce;
    private boolean proxyBuiltOnce;

    private String[] gbids;
    private String statelessAsyncCallbackUseCase;
    private String interfaceName;
    private Set<String> domains;

    private ProxyBuilderFactory proxyBuilderFactory;
    private ArbitrationResult savedArbitrationResult;
    private DiscoveryAsync localDiscoveryAggregator;
    private DiscoveryQos discoveryQos;
    private MessagingQos messagingQos;
    private Arbitrator arbitrator;
    private final long maxMessagingTtl;
    private final long defaultDiscoveryTimeoutMs;
    private final long defaultDiscoveryRetryIntervalMs;
    private ObjectMapper objectMapper;
    private boolean discoveryInProgress;

    private final MessageRouter messageRouter;

    /**
     * Constructor for internal use only.
     * <p>
     * Do not use the constructor of the GuidedProxyBuilder to create instances of this class.
     * Use the {@link io.joynr.runtime.JoynrRuntime#getGuidedProxyBuilder(Set, Class)} method (Java) or the
     * <code>getGuidedProxyBuilder</code> method of <code>ServiceLocator</code> (JEE) instead.
     *
     * @param discoverySettingsStorage the discoverySettingsStorage
     * @param domains the set of domains to be considered when searching the providers
     * @param interfaceClass Interface the provider offers
     */
    public GuidedProxyBuilder(DiscoverySettingsStorage discoverySettingsStorage,
                              Set<String> domains,
                              Class<?> interfaceClass,
                              MessageRouter messageRouter) {
        this.proxyBuilderFactory = discoverySettingsStorage.getProxyBuilderFactory();
        this.objectMapper = discoverySettingsStorage.getObjectMapper();
        this.localDiscoveryAggregator = discoverySettingsStorage.getLocalDiscoveryAggregator();
        this.maxMessagingTtl = discoverySettingsStorage.getMaxMessagingTtl();
        this.defaultDiscoveryTimeoutMs = discoverySettingsStorage.getDefaultDiscoveryTimeoutMs();
        this.defaultDiscoveryRetryIntervalMs = discoverySettingsStorage.getDefaultDiscoveryRetryIntervalMs();
        this.domains = domains;
        this.messageRouter = messageRouter;
        try {
            interfaceName = (String) interfaceClass.getField("INTERFACE_NAME").get(String.class);
        } catch (Exception e) {
            String errorMessage = MessageFormat.format("INTERFACE_NAME needs to be set in the interface class {0}",
                                                       interfaceClass);
            logger.error(errorMessage);
            throw new JoynrIllegalStateException(errorMessage, e);
        }
    }

    /**
     * Sets discovery scope, timeout and other discovery specific parameters.
     *
     * @param discoveryQos discovery quality of service
     * @return Returns the GuidedProxyBuilder instance.
     * @throws JoynrIllegalStateException in case the setter gets called after a discovery has been started
     * @see DiscoveryQos
     */
    public synchronized GuidedProxyBuilder setDiscoveryQos(final DiscoveryQos discoveryQos) throws DiscoveryException {
        if (discoveryInProgress) {
            throw new JoynrIllegalStateException("setDiscoveryQos called while discovery in progress");
        }
        if (discoveryCompletedOnce) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder already performed a successful discovery! setDiscoveryQos cannot be called anymore.");
        }
        this.discoveryQos = new DiscoveryQos(discoveryQos);
        applyDefaultValues(this.discoveryQos);
        return this;
    }

    /**
     * Sets the MessagingQos (e.g. request timeouts) which will be used by the created proxy.
     *
     * @param messagingQos messaging quality of service
     * @return Returns the GuidedProxyBuilder instance.
     * @throws JoynrIllegalStateException in case the setter gets called after a discovery has been started
     * @see MessagingQos
     */
    public synchronized GuidedProxyBuilder setMessagingQos(final MessagingQos messagingQos) throws DiscoveryException {
        if (discoveryInProgress) {
            throw new JoynrIllegalStateException("setMessagingQos called while discovery in progress");
        }
        if (discoveryCompletedOnce) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder already performed a successful discovery! setMessagingQos cannot be called anymore.");
        }
        if (messagingQos.getRoundTripTtl_ms() > maxMessagingTtl) {
            logger.warn("Error in MessageQos. domains: {} interface: {} Max allowed ttl: {}. Passed ttl: {}",
                        domains,
                        interfaceName,
                        maxMessagingTtl,
                        messagingQos.getRoundTripTtl_ms());
            messagingQos.setTtl_ms(maxMessagingTtl);
        }

        this.messagingQos = messagingQos;
        return this;
    }

    /**
     * If you want to use any {@link io.joynr.StatelessAsync} calls, then you must provide a use case name, which will
     * be used in constructing the relevant stateless async callback IDs, so that when Reply payloads arrive at a
     * runtime the latter is able to discern which callback handler instance to route the data to.
     *
     * @param statelessAsyncCallbackUseCase the use case for which the proxy is being used, in order to construct
     * stateless async callback IDs mapping to the correct callback handler.
     * @return Returns the GuidedProxyBuilder instance.
     * @throws JoynrIllegalStateException in case the setter gets called after a discovery has been started
     */
    public synchronized GuidedProxyBuilder setStatelessAsyncCallbackUseCase(String statelessAsyncCallbackUseCase) {
        if (discoveryInProgress) {
            throw new JoynrIllegalStateException("setStatelessAsyncCallbackUseCase called while discovery in progress");
        }
        if (discoveryCompletedOnce) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder already performed a successful discovery! setStatelessAsyncCallbackUseCase cannot be called anymore.");
        }
        this.statelessAsyncCallbackUseCase = statelessAsyncCallbackUseCase;
        return this;
    }

    /**
     * Sets the GBIDs (Global Backend Identifiers) to select the backends in which the provider will be discovered.<br>
     * Global discovery (if enabled in DiscoveryQos) will be done via the GlobalCapabilitiesDirectory in the backend of
     * the first provided GBID.<br>
     * By default, providers will be discovered in all backends known to the cluster controller via the
     * GlobalCapabilitiesDirectory in the default backend.
     *
     * @param gbids an array of GBIDs
     * @return Returns the GuidedProxyBuilder instance.
     * @throws IllegalArgumentException if provided gbids array is null or empty
     * @throws JoynrIllegalStateException in case the setter gets called after a discovery has been started
     */
    public synchronized GuidedProxyBuilder setGbids(final String[] gbids) {
        if (discoveryInProgress) {
            throw new JoynrIllegalStateException("setGbids called while discovery in progress");
        }
        if (discoveryCompletedOnce) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder already performed a successful discovery! setGbids cannot be called anymore.");
        }
        if (gbids == null || gbids.length == 0) {
            throw new IllegalArgumentException("GBIDs array must not be null or empty.");
        }
        this.gbids = gbids.clone();
        return this;
    }

    /**
     * Triggers a synchronous discovery with the previously set parameters
     * and returns as soon as the discovery is finished. This is a blocking call.
     * After one of discover() or discoverAsync() was called on an instance of the
     * GuidedProxyBuilder, no further discovery is possible with that instance.
     * A second discovery attempt will cause an JoynrIllegalStateException.
     *
     * @return Returns a DiscoveryResult containing the discovered DiscoveryEntries.
     * @throws DiscoveryException if the discovery fails.
     * @throws JoynrIllegalStateException if a discovery has already been completed by this instance of the GuidedProxyBuilder.
     */
    public synchronized DiscoveryResult discover() {
        try {
            return discoverAsync().get();
        } catch (InterruptedException e) {
            throw new DiscoveryException(e.toString());
        } catch (ExecutionException e) {
            if (e.getCause() == null) {
                String domainString = String.join(",", domains);
                throw new DiscoveryException("Lookup failed for domains: " + domainString + ", interface: "
                        + interfaceName + " gbids: " + Arrays.toString(gbids) + " with: " + e.toString());
            } else {
                throw new DiscoveryException(e.getCause().getMessage());
            }
        }
    }

    /**
     * Triggers an asynchronous discovery with the previously set parameters
     * and returns immediately. The returned CompletableFuture can either be completed normally,
     * in which case it will contain the accumulated DiscoveryResult, or it can be completed
     * exceptionally. In the latter case, the ComplatableFuture contains an exception with
     * the cause of the error.
     * After one of discover() or discoverAsync() was called on an instance of the
     * GuidedProxyBuilder, no further discovery is possible with that instance.
     * A second discovery attempt will cause an JoynrIllegalStateException.
     *
     * @return Returns a CompletableFuture that will contain the result of the discovery as soon as it is completed.
     * @throws JoynrIllegalStateException if a discovery has already been completed by this instance of the GuidedProxyBuilder.
     */
    public synchronized CompletableFuture<DiscoveryResult> discoverAsync() {
        return discoverAsyncInternal().thenCompose(this::createDiscoveryResultFromArbitrationResult);
    }

    private CompletableFuture<DiscoveryResult> createDiscoveryResultFromArbitrationResult(ArbitrationResult result) {
        return CompletableFuture.completedFuture(new DiscoveryResult(result.getDiscoveryEntries()));
    }

    private void applyDefaultValues(DiscoveryQos discoveryQos) {
        if (discoveryQos.getDiscoveryTimeoutMs() == DiscoveryQos.NO_VALUE) {
            discoveryQos.setDiscoveryTimeoutMs(defaultDiscoveryTimeoutMs);
        }

        if (discoveryQos.getRetryIntervalMs() == DiscoveryQos.NO_VALUE) {
            discoveryQos.setRetryIntervalMs(defaultDiscoveryRetryIntervalMs);
        }
    }

    private CompletableFuture<ArbitrationResult> discoverAsyncInternal() {
        if (discoveryCompletedOnce) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder already performed a successful discovery!");
        }
        if (discoveryInProgress) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder can not perform another discovery while a discovery is in progress!");
        }
        if (arbitrator == null) {
            if (discoveryQos == null) {
                discoveryQos = new DiscoveryQos();
                applyDefaultValues(discoveryQos);
            }
            arbitrator = ArbitratorFactory.create(domains,
                                                  interfaceName,
                                                  new Version(),
                                                  discoveryQos,
                                                  localDiscoveryAggregator,
                                                  gbids);
        }
        CompletableFuture<ArbitrationResult> resultFuture = new CompletableFuture<>();
        arbitrator.setArbitrationListener(new ArbitrationCallback() {
            @Override
            public void onSuccess(ArbitrationResult arbitrationResult) {
                savedArbitrationResult = arbitrationResult;
                discoveryCompletedOnce = true;
                discoveryInProgress = false;
                resultFuture.complete(arbitrationResult);
            }

            @Override
            public void onError(Throwable throwable) {
                JoynrRuntimeException reason;
                if (throwable instanceof JoynrRuntimeException) {
                    reason = (JoynrRuntimeException) throwable;
                } else {
                    reason = new JoynrRuntimeException(throwable);
                }
                discoveryInProgress = false;
                resultFuture.completeExceptionally(reason);
            }
        });
        savedArbitrationResult = new ArbitrationResult();
        discoveryInProgress = true;
        arbitrator.scheduleArbitration(false);
        return resultFuture;
    }

    /**
     * Do the necessary cleanup after successful discovery if no proxy shall be built for any
     * of the discovered providers.
     * <p>
     * This method indicates that the customer does not intend to build a proxy to any of the
     *  discovered providers. It releases previously allocated resources to avoid unnecessary
     *  memory usage: it decrements the reference counts of the routing entries of all the
     *  discovered providers because none of them is required anymore for this GuidedProxyBuilder
     *  instance.
     * @throws JoynrIllegalStateException if no discovery was completed yet, or if a proxy has
     *  been already built with this instance of the GuidedProxyBuilder.
    */
    public synchronized void buildNone() {
        if (proxyBuiltOnce) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder already has been used by calling buildProxy or buildNone");
        }
        if (!discoveryCompletedOnce) {
            throw new JoynrIllegalStateException("Discovery has to be completed before calling buildNone");
        }
        proxyBuiltOnce = true;
        // decrease all the reference counts for the associated routing entries.
        for (DiscoveryEntryWithMetaInfo selectedDiscoveryEntry : savedArbitrationResult.getDiscoveryEntries()) {
            messageRouter.removeNextHop(selectedDiscoveryEntry.getParticipantId());
        }
    }

    /**
     * Builds a proxy for the given interface class that connects to the provider with the given participantId.
     * The version of the interface class must fit the version of the discovered provider (the DiscoverEntry
     * containing the participantId also contains a version). Also, the provider must have been discovered by the
     * same GuidedProxyBuilder that the proxy is built with.
     * A proxy can be built via the GuidedProxyBuilder only once. An attempt to build further proxies
     * will cause an JoynrIllegalStateException.
     *
     * @param <T> Class of the required proxy.
     * @param interfaceClass Class of the required proxy.
     * @param participantId ParticipantId of the target provider.
     * @return A proxy implementing all methods of the interfaceClass that is connected to the provider with the given
     * participantId.
     * @throws JoynrIllegalStateException if no discovery was completed yet, or if a proxy has been already built
     * with this instance of the GuidedProxyBuilder.
     */
    public synchronized <T> T buildProxy(Class<T> interfaceClass, String participantId) {
        if (proxyBuiltOnce) {
            throw new JoynrIllegalStateException("This GuidedProxyBuilder already has been used by calling buildProxy or buildNone!");
        }
        if (!discoveryCompletedOnce) {
            throw new JoynrIllegalStateException("Discovery has to be completed before building a proxy!");
        }

        DiscoveryEntryWithMetaInfo discoveryEntryForProxy = null;
        for (DiscoveryEntryWithMetaInfo entry : savedArbitrationResult.getDiscoveryEntries()) {
            if (entry.getParticipantId().equals(participantId)) {
                discoveryEntryForProxy = entry;
            }
        }
        if (discoveryEntryForProxy == null) {
            throw new IllegalArgumentException("No provider with participant ID " + participantId + " was discovered!");
        }
        Version interfaceVersion = VersionUtil.getVersionFromAnnotation(interfaceClass);
        Version providerVersion = discoveryEntryForProxy.getProviderVersion();
        if (!interfaceVersion.equals(providerVersion)) {
            throw new IllegalArgumentException("Provider Version " + providerVersion
                    + " does not match interface version " + interfaceVersion + " !");
        }
        Set<DiscoveryEntryWithMetaInfo> discoveryEntriesForProxy = new HashSet<>(Arrays.asList(discoveryEntryForProxy));

        Set<DiscoveryEntryWithMetaInfo> otherDiscoveryEntries = new HashSet<>(savedArbitrationResult.getOtherDiscoveryEntries());
        otherDiscoveryEntries.addAll(savedArbitrationResult.getDiscoveryEntries());
        otherDiscoveryEntries.remove(discoveryEntryForProxy);
        ArbitrationResult arbitrationResultForProxy = new ArbitrationResult(discoveryEntriesForProxy,
                                                                            otherDiscoveryEntries);
        registerInterfaceClassTypes(interfaceClass, "Cannot create ProxyBuilder");
        ProxyBuilder<T> proxyBuilder = proxyBuilderFactory.get(domains, interfaceClass);
        if (discoveryQos == null) {
            // discoveryQos should be already set, since this is done latest in
            // discoverAsyncInternal() which must have been called already since
            // discoveryCompletedOnce is true
            throw new JoynrIllegalStateException("DiscoveryQos not set, internal error!");
        }
        proxyBuilder.setDiscoveryQos(discoveryQos);

        if (statelessAsyncCallbackUseCase != null) {
            proxyBuilder.setStatelessAsyncCallbackUseCase(statelessAsyncCallbackUseCase);
        }

        if (messagingQos == null) {
            proxyBuilder.setMessagingQos(new MessagingQos());
        } else {
            proxyBuilder.setMessagingQos(messagingQos);
        }
        if (!(gbids == null) && gbids.length > 0) {
            proxyBuilder.setGbids(gbids);
        }
        proxyBuiltOnce = true;
        return proxyBuilder.build(arbitrationResultForProxy);
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

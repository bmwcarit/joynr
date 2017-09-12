package io.joynr.arbitration;

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

import java.util.HashMap;
import java.util.Map;

import com.google.common.collect.Maps;

/**
 * Storage class to pass all settings to an arbitrator defining the strategy and conditions for provider arbitration.
 *
 */

public class DiscoveryQos {
    public static final DiscoveryQos NO_FILTER;
    public static final long NO_VALUE = -1L;

    private long discoveryTimeoutMs;

    private ArbitrationStrategy arbitrationStrategy;
    private ArbitrationStrategyFunction arbitrationStrategyFunction;
    private static final ArbitrationStrategy DEFAULT_ARBITRATIONSTRATEGY = ArbitrationStrategy.LastSeen;

    long cacheMaxAgeMs;
    private static final long DEFAULT_CACHEMAXAGE = 0L;
    public static final long NO_MAX_AGE = Long.MAX_VALUE;

    private boolean providerMustSupportOnChange;
    private static final boolean DEFAULT_PROVIDERMUSTSUPPORTONCHANGE = false;

    private long retryIntervalMs;

    private DiscoveryScope discoveryScope;
    private static final DiscoveryScope DEFAULT_DISCOVERYSCOPE = DiscoveryScope.LOCAL_AND_GLOBAL;

    private HashMap<String, String> customParameters = Maps.newHashMap();

    /**
     * DiscoveryQos object with default values.
     */

    static {
        NO_FILTER = new DiscoveryQos(Long.MAX_VALUE, ArbitrationStrategy.NotSet, Long.MAX_VALUE);
    }

    public DiscoveryQos() {
        this.discoveryTimeoutMs = NO_VALUE;
        this.arbitrationStrategy = DEFAULT_ARBITRATIONSTRATEGY;
        this.cacheMaxAgeMs = DEFAULT_CACHEMAXAGE;
        this.providerMustSupportOnChange = DEFAULT_PROVIDERMUSTSUPPORTONCHANGE;
        this.retryIntervalMs = NO_VALUE;
        this.discoveryScope = DEFAULT_DISCOVERYSCOPE;
    }

    /**
     * @param discoveryTimeout
     *            Timeout for rpc calls to wait for arbitration to finish.
     * @param arbitrationStrategy
     *            Strategy for choosing the appropriate provider from the list returned by the capabilities directory
     * @param cacheMaxAge
     *            Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the
     *            local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     */
    public DiscoveryQos(long discoveryTimeout, ArbitrationStrategy arbitrationStrategy, long cacheMaxAge) {
        this(discoveryTimeout, arbitrationStrategy, cacheMaxAge, DEFAULT_DISCOVERYSCOPE);
    }

    /**
     * @param discoveryTimeout
     *            Timeout for rpc calls to wait for arbitration to finish.
     * @param arbitrationStrategyFunction
     *            function that chooses the appropriate provider from the list returned by the capabilities directory
     * @param cacheMaxAge
     *            Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the
     *            local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     */
    public DiscoveryQos(long discoveryTimeout, ArbitrationStrategyFunction arbitrationStrategyFunction, long cacheMaxAge) {
        this(discoveryTimeout, arbitrationStrategyFunction, cacheMaxAge, DEFAULT_DISCOVERYSCOPE);
    }

    /**
     * @param discoveryTimeout
     *            Timeout for rpc calls to wait for arbitration to finish.
     * @param arbitrationStrategy
     *            Strategy for choosing the appropriate provider from the list returned by the capabilities directory
     * @param cacheMaxAge
     *            Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the
     *            local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     * @param discoveryScope
     *            determines where the discovery process will look for matching providers<br>
     *            <ul>
     *            <li>LOCAL_ONLY: only locally registered providers will be considered.
     *            <li>LOCAL_THEN_GLOBAL locally registered providers are preferred. When none is found, the global
     *            providers are included in search results.
     *            <li>LOCAL_AND_GLOBAL: all providers registered locally, and query results from the gobal directory are
     *            combined and returned.
     *            <li>GLOBAL_ONLY only returns providers that are found in the global directory.
     *            </ul>
     */
    public DiscoveryQos(long discoveryTimeout,
                        ArbitrationStrategy arbitrationStrategy,
                        long cacheMaxAge,
                        DiscoveryScope discoveryScope) {
        this(discoveryTimeout, NO_VALUE, arbitrationStrategy, cacheMaxAge, discoveryScope);
    }

    /**
     * @param discoveryTimeout
     *            Timeout for rpc calls to wait for arbitration to finish.
     * @param retryIntervalMs
     *            Lookups for the arbitration will be repeated after this time interval if they were not successful
     * @param arbitrationStrategy
     *            Strategy for choosing the appropriate provider from the list returned by the capabilities directory
     * @param cacheMaxAge
     *            Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the
     *            local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     * @param discoveryScope
     *            determines where the discovery process will look for matching providers<br>
     *            <ul>
     *            <li>LOCAL_ONLY: only locally registered providers will be considered.
     *            <li>LOCAL_THEN_GLOBAL locally registered providers are preferred. When none is found, the global
     *            providers are included in search results.
     *            <li>LOCAL_AND_GLOBAL: all providers registered locally, and query results from the gobal directory are
     *            combined and returned.
     *            <li>GLOBAL_ONLY only returns providers that are found in the global directory.
     *            </ul>
     */
    public DiscoveryQos(long discoveryTimeout,
                        long retryIntervalMs,
                        ArbitrationStrategy arbitrationStrategy,
                        long cacheMaxAge,
                        DiscoveryScope discoveryScope) {
        if (arbitrationStrategy.equals(ArbitrationStrategy.Custom)) {
            throw new IllegalStateException("A Custom strategy can only be set by passing an arbitration strategy function to the DisocveryQos constructor");
        }

        this.cacheMaxAgeMs = cacheMaxAge;
        this.discoveryScope = discoveryScope;
        this.discoveryTimeoutMs = discoveryTimeout;
        this.retryIntervalMs = retryIntervalMs;
        this.arbitrationStrategy = arbitrationStrategy;
        this.retryIntervalMs = NO_VALUE;
        this.providerMustSupportOnChange = DEFAULT_PROVIDERMUSTSUPPORTONCHANGE;
    }

    public DiscoveryQos(long discoveryTimeout,
                        ArbitrationStrategyFunction arbitrationStrategyFunction,
                        long cacheMaxAge,
                        DiscoveryScope discoveryScope) {
        this(discoveryTimeout, NO_VALUE, arbitrationStrategyFunction, cacheMaxAge, discoveryScope);
    }

    public DiscoveryQos(long discoveryTimeout,
                        long retryIntervalMs,
                        ArbitrationStrategyFunction arbitrationStrategyFunction,
                        long cacheMaxAge,
                        DiscoveryScope discoveryScope) {
        this.arbitrationStrategy = ArbitrationStrategy.Custom;
        this.discoveryTimeoutMs = discoveryTimeout;
        this.arbitrationStrategyFunction = arbitrationStrategyFunction;
        this.cacheMaxAgeMs = cacheMaxAge;
        this.discoveryScope = discoveryScope;
        this.retryIntervalMs = NO_VALUE;
        this.providerMustSupportOnChange = DEFAULT_PROVIDERMUSTSUPPORTONCHANGE;
    }

    /**
     * The discovery process outputs a list of matching providers. The arbitration strategy then chooses one or more of
     * them to be used by the proxy.
     *
     * @param arbitrationStrategy
     *            Defines the strategy used to choose the "best" provider.
     */
    public void setArbitrationStrategy(ArbitrationStrategy arbitrationStrategy) {
        if (arbitrationStrategy.equals(ArbitrationStrategy.Custom)) {
            throw new IllegalStateException("A Custom strategy can only be set by passing an arbitration strategy function to the DisocveryQos constructor");
        }
        this.arbitrationStrategy = arbitrationStrategy;
    }

    /**
     * The discovery process outputs a list of matching providers. The arbitration strategy then chooses one or more of
     * them to be used by the proxy.
     *
     * @return the arbitration strategy used to pick the "best" provider of the list of matching providers
     */
    public ArbitrationStrategy getArbitrationStrategy() {
        return arbitrationStrategy;
    }

    /**
     * As soon as the arbitration QoS is set on the proxy builder, discovery of suitable providers is triggered. If the
     * discovery process does not find matching providers within the arbitration timeout duration it will be terminated
     * and you will get an arbitration exception.
     *
     * @param discoveryTimeoutMs
     *            Sets the amount of time the arbitrator keeps trying to find a suitable provider. The arbitration
     *            lookup might happen multiple times during this time span.
     */
    public void setDiscoveryTimeoutMs(long discoveryTimeoutMs) {
        this.discoveryTimeoutMs = discoveryTimeoutMs;
    }

    /**
     * As soon as the arbitration QoS is set on the proxy builder, discovery of suitable providers is triggered. If the
     * discovery process does not find matching providers within the arbitration timeout duration it will be terminated
     * and you will get an arbitration exception.
     *
     * @return the duration used to discover matching providers
     */
    public long getDiscoveryTimeoutMs() {
        return discoveryTimeoutMs;
    }

    /**
     * addCustomParameter allows to add special parameters to the DiscoveryQos which will be used only by some
     * strategies.
     *
     * @param key
     *            String to identify the arbitration parameter
     * @param value
     *            Any object used by the arbitrator to choose a provider.
     */
    public void addCustomParameter(String key, String value) {
        customParameters.put(key, value);
    }

    /**
     * getCustomParameter returns the parameters previously specified by addParameter
     *
     * @param key key to identify the custom parameter
     * @return Returns the value to which the specified key is mapped, or null if the map of additional parameters
     *         contains no mapping for the key
     */
    public Object getCustomParameter(String key) {
        return customParameters.get(key);
    }

    /**
     * Provider entries in the global capabilities directory are cached locally. Discovery will consider entries in this
     * cache valid if they are younger than the max age of cached providers as defined in the QoS. All valid entries
     * will be processed by the arbitrator when searching for and arbitrating the "best" matching provider.
     * <p>
     * NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities directory. Therefore,
     * providers registered with the global capabilities after the last lookup and before the cacheMaxAge expires will
     * not be discovered.
     *
     * @return the maximum age of locally cached provider entries to be used during discovery and arbitration before
     *         refreshing from the global directory
     */
    public long getCacheMaxAgeMs() {
        return cacheMaxAgeMs;
    }

    /**
     * Provider entries in the global capabilities directory are cached locally. Discovery will consider entries in this
     * cache valid if they are younger than the max age of cached providers as defined in the QoS. All valid entries
     * will be processed by the arbitrator when searching for and arbitrating the "best" matching provider.
     * <p>
     * NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities directory. Therefore,
     * providers registered with the global capabilities after the last lookup and before the cacheMaxAge expires will
     * not be discovered.
     *
     * @param cacheMaxAgeMs
     *            Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the
     *            local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     */
    public void setCacheMaxAgeMs(long cacheMaxAgeMs) {
        this.cacheMaxAgeMs = cacheMaxAgeMs < 0L ? 0L : cacheMaxAgeMs;
    }

    public boolean isLocalOnly() {
        return discoveryScope == DiscoveryScope.LOCAL_ONLY;
    }

    /**
     *
     * @return the interval used for retrying discovery if the previous attempt was unsuccessful
     */
    public long getRetryIntervalMs() {
        return retryIntervalMs;
    }

    /**
     * @param retryIntervalMs
     *            The time to wait between discovery retries after encountering a discovery error. The actual delay may
     *            be longer, as there is a system-wide minimum delay.
     */
    public void setRetryIntervalMs(long retryIntervalMs) {
        this.retryIntervalMs = retryIntervalMs;
    }

    /**
     * Indicates if arbitration should only consider providers that support onChange subscriptions
     *
     * @return true if only providers that support onChange subscriptions are considered
     */
    public boolean getProviderMustSupportOnChange() {
        return providerMustSupportOnChange;
    }

    /**
     * Indicates if arbitration should only consider providers that support onChange subscriptions
     *
     * @param providerMustSupportOnChange
     *            true if only providers that support onChange subscriptions should be considered
     */
    public void setProviderMustSupportOnChange(boolean providerMustSupportOnChange) {
        this.providerMustSupportOnChange = providerMustSupportOnChange;
    }

    /**
     * @param discoveryScope selects capability registries to choose from for provider discovery
     */
    public void setDiscoveryScope(DiscoveryScope discoveryScope) {
        this.discoveryScope = discoveryScope;
    }

    /**
     * @return scope criteria to select from capability registries for provider discovery
     */
    public DiscoveryScope getDiscoveryScope() {
        return discoveryScope;
    }

    ArbitrationStrategyFunction getArbitrationStrategyFunction() {
        return arbitrationStrategyFunction;
    }

    public Map<String, String> getCustomParameters() {
        return customParameters;
    }
}

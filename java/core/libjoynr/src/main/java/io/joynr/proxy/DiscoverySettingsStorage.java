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

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.util.ObjectMapper;
import joynr.system.DiscoveryAsync;

public class DiscoverySettingsStorage {

    private final ProxyBuilderFactory proxyBuilderFactory;
    private final ObjectMapper objectMapper;
    private final DiscoveryAsync localDiscoveryAggregator;
    private final long maxMessagingTtl;
    private final long defaultDiscoveryTimeoutMs;
    private final long defaultDiscoveryRetryIntervalMs;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public DiscoverySettingsStorage(ProxyBuilderFactory proxyBuilderFactory,
                                    ObjectMapper objectMapper,
                                    DiscoveryAsync localDiscoveryAggregator,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_TTL_MS) long maxMessagingTtl,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS) long defaultDiscoveryTimeoutMs,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS) long defaultDiscoveryRetryIntervalMs) {
        this.proxyBuilderFactory = proxyBuilderFactory;
        this.objectMapper = new ObjectMapper(objectMapper);
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.maxMessagingTtl = maxMessagingTtl;
        this.defaultDiscoveryTimeoutMs = defaultDiscoveryTimeoutMs;
        this.defaultDiscoveryRetryIntervalMs = defaultDiscoveryRetryIntervalMs;
    }

    public ProxyBuilderFactory getProxyBuilderFactory() {
        return proxyBuilderFactory;
    }

    public ObjectMapper getObjectMapper() {
        return new ObjectMapper(objectMapper);
    }

    public DiscoveryAsync getLocalDiscoveryAggregator() {
        return localDiscoveryAggregator;
    }

    public long getMaxMessagingTtl() {
        return maxMessagingTtl;
    }

    public long getDefaultDiscoveryTimeoutMs() {
        return defaultDiscoveryTimeoutMs;
    }

    public long getDefaultDiscoveryRetryIntervalMs() {
        return defaultDiscoveryRetryIntervalMs;
    }
}

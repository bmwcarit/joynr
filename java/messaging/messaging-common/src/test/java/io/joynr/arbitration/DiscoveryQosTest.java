/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.arbitration;

import static org.junit.Assert.assertEquals;

import org.junit.Before;
import org.junit.Test;

/**
 * Unit tests for the {@link DiscoveryQos}.
 */
public class DiscoveryQosTest {
    private DiscoveryQos discoveryQos;

    @Before
    public void setup() {
        discoveryQos = new DiscoveryQos();
    }

    @Test
    public void testDefaultDiscoveryTimeoutMs() {
        assertEquals(io.joynr.arbitration.DiscoveryQos.NO_VALUE, discoveryQos.getDiscoveryTimeoutMs());
    }

    @Test
    public void testNonDefaultDiscoveryTimeoutMs() {
        long expectedDiscoveryTimeoutMs = 1000L;
        discoveryQos.setDiscoveryTimeoutMs(expectedDiscoveryTimeoutMs);
        assertEquals(expectedDiscoveryTimeoutMs, discoveryQos.getDiscoveryTimeoutMs());
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidDiscoveryTimeoutMs() {
        long expectedDiscoveryTimeoutMs = -128;
        discoveryQos.setDiscoveryTimeoutMs(expectedDiscoveryTimeoutMs);
    }

    @Test
    public void testDefaultArbitrationStrategy() {
        assertEquals(ArbitrationStrategy.LastSeen, discoveryQos.getArbitrationStrategy());
    }

    @Test
    public void testNonDefaultArbitrationStrategy() {
        ArbitrationStrategy expectedArbitrationStrategy = ArbitrationStrategy.HighestPriority;
        discoveryQos.setArbitrationStrategy(expectedArbitrationStrategy);
        assertEquals(expectedArbitrationStrategy, discoveryQos.getArbitrationStrategy());
    }

    @Test
    public void testDefaultCacheMaxAgeMs() {
        assertEquals(0L, discoveryQos.getCacheMaxAgeMs());
    }

    @Test
    public void testNonDefaultCacheMaxAgeMs() {
        long expectedCacheMaxAge = 1000L;
        discoveryQos.setCacheMaxAgeMs(expectedCacheMaxAge);
        assertEquals(expectedCacheMaxAge, discoveryQos.getCacheMaxAgeMs());
    }

    @Test
    public void testDefaultDiscoveryScope() {
        assertEquals(DiscoveryScope.LOCAL_THEN_GLOBAL, discoveryQos.getDiscoveryScope());
    }

    @Test
    public void testNonDefaultDiscoveryScope() {
        DiscoveryScope expectedDiscoveryScope = DiscoveryScope.LOCAL_ONLY;
        discoveryQos.setDiscoveryScope(expectedDiscoveryScope);
        assertEquals(expectedDiscoveryScope, discoveryQos.getDiscoveryScope());
    }

    @Test
    public void testDefaultProviderMustSupportOnChange() {
        assertEquals(false, discoveryQos.getProviderMustSupportOnChange());
    }

    @Test
    public void testNonDefaultProviderMustSupportOnChange() {
        boolean expectedProviderMustSupportOnChange = true;
        discoveryQos.setProviderMustSupportOnChange(expectedProviderMustSupportOnChange);
        assertEquals(expectedProviderMustSupportOnChange, discoveryQos.getProviderMustSupportOnChange());
    }

    @Test
    public void testDefaultRetryIntervalMs() {
        assertEquals(io.joynr.arbitration.DiscoveryQos.NO_VALUE, discoveryQos.getRetryIntervalMs());
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidRetryIntervalMs() {
        long expectedRetryIntervalMs = -64;
        discoveryQos.setRetryIntervalMs(expectedRetryIntervalMs);
    }

    @Test
    public void testNonDefaultRetryIntervalMsViaSetter() {
        long expectedRetryIntervalMs = 1000L;
        discoveryQos.setRetryIntervalMs(expectedRetryIntervalMs);
        assertEquals(expectedRetryIntervalMs, discoveryQos.getRetryIntervalMs());
    }

    @Test
    public void testNonDefaultRetryIntervalMsViaConstructor() {
        long expectedRetryIntervalMs = 1000L;
        discoveryQos = new DiscoveryQos(DiscoveryQos.NO_VALUE,
                                        expectedRetryIntervalMs,
                                        ArbitrationStrategy.LastSeen,
                                        0,
                                        DiscoveryScope.LOCAL_AND_GLOBAL);
        assertEquals(expectedRetryIntervalMs, discoveryQos.getRetryIntervalMs());
    }

    @Test
    public void testToString() {
        String expectedToStringResult = "DiscoveryQos [arbitrationStrategy=LastSeen, cacheMaxAgeMs=0, "
                + "customParameters=" + discoveryQos.getCustomParameters().toString() + ", "
                + "discoveryScope=LOCAL_THEN_GLOBAL, discoveryTimeoutMs=-1, providerMustSupportOnChange=false, "
                + "retryIntervalMs=-1]";
        assertEquals(expectedToStringResult, discoveryQos.toString());
    }
}

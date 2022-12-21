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
package joynr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

import io.joynr.pubsub.SubscriptionQos;

public class SubscriptionQosTest {

    @Before
    public void setUp() {
    }

    @Test
    public void createOnChangeSubscriptionQos() throws Exception {
        long expiryDateMs = System.currentTimeMillis() + 100000;
        long publicationTtlMs = 2000;
        long minIntervalMs = 100;
        OnChangeSubscriptionQos onChangeSubscriptionQos = new OnChangeSubscriptionQos().setExpiryDateMs(expiryDateMs)
                                                                                       .setPublicationTtlMs(publicationTtlMs)
                                                                                       .setMinIntervalMs(minIntervalMs);
        assertEquals(expiryDateMs, onChangeSubscriptionQos.getExpiryDateMs());
        assertEquals(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtlMs());
        assertEquals(minIntervalMs, onChangeSubscriptionQos.getMinIntervalMs());
    }

    @Test
    public void createOnChangeSubscriptionQosWithValidity() throws Exception {
        long validityMs = 100000;

        long lowerBound = System.currentTimeMillis() + validityMs;
        SubscriptionQos onChangeSubscriptionQos = new OnChangeSubscriptionQos().setValidityMs(validityMs);
        long upperBound = System.currentTimeMillis() + validityMs;

        assertTrue(onChangeSubscriptionQos.getExpiryDateMs() >= lowerBound);
        assertTrue(onChangeSubscriptionQos.getExpiryDateMs() <= upperBound);
    }

    @Test
    public void createPeriodicSubscriptionQos() throws Exception {
        long expiryDateMs = System.currentTimeMillis() + 100000;
        long publicationTtlMs = 2000;
        long periodMs = 800;
        long alertAfterIntervalMs = 3000;
        PeriodicSubscriptionQos periodicSubscriptionQos = new PeriodicSubscriptionQos().setExpiryDateMs(expiryDateMs)
                                                                                       .setPublicationTtlMs(publicationTtlMs)
                                                                                       .setPeriodMs(periodMs)
                                                                                       .setAlertAfterIntervalMs(alertAfterIntervalMs);
        assertEquals(expiryDateMs, periodicSubscriptionQos.getExpiryDateMs());
        assertEquals(publicationTtlMs, periodicSubscriptionQos.getPublicationTtlMs());
        assertEquals(periodMs, periodicSubscriptionQos.getPeriodMs());
        assertEquals(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterIntervalMs());
    }

    @Test
    public void createPeriodicSubscriptionQosWithValidity() throws Exception {
        long validityMs = 1000;

        long lowerBound = System.currentTimeMillis() + validityMs;
        SubscriptionQos periodicSubscriptionQos = new PeriodicSubscriptionQos().setValidityMs(validityMs);
        long upperBound = System.currentTimeMillis() + validityMs;

        assertTrue(periodicSubscriptionQos.getExpiryDateMs() >= lowerBound);
        assertTrue(periodicSubscriptionQos.getExpiryDateMs() <= upperBound);
    }

    @Test
    public void createOnChangeWithKeepAliveSubscriptionQos() throws Exception {
        long expiryDateMs = System.currentTimeMillis() + 100000;
        long publicationTtlMs = 2000;
        long alertAfterIntervalMs = 4000;
        long maxIntervalMs = 3000;
        long minIntervalMs = 100;
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setExpiryDateMs(expiryDateMs)
                                                                                                                              .setPublicationTtlMs(publicationTtlMs)
                                                                                                                              .setMaxIntervalMs(maxIntervalMs)
                                                                                                                              .setMinIntervalMs(minIntervalMs)
                                                                                                                              .setAlertAfterIntervalMs(alertAfterIntervalMs);
        assertEquals(expiryDateMs, onChangeWithKeepAliveSubscriptionQos.getExpiryDateMs());
        assertEquals(publicationTtlMs, onChangeWithKeepAliveSubscriptionQos.getPublicationTtlMs());
        assertEquals(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
        assertEquals(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinIntervalMs());
        assertEquals(alertAfterIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs());

    }

    @Test
    public void alertAfterIntervalAdjustedIfSmallerThanPeriod() throws Exception {
        long periodMs = 5000;
        long alertAfterIntervalMs = 1000;
        PeriodicSubscriptionQos periodicSubscriptionQos = new PeriodicSubscriptionQos().setPeriodMs(periodMs)
                                                                                       .setAlertAfterIntervalMs(alertAfterIntervalMs);
        assertEquals(periodMs, periodicSubscriptionQos.getPeriodMs());
        assertEquals(periodMs, periodicSubscriptionQos.getAlertAfterIntervalMs());
    }

    @Test
    public void alertAfterIntervalAdjustedIfSmallerThanMaxInterval() throws Exception {
        long alertAfterIntervalMs = 4000;
        long maxIntervalMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMaxIntervalMs(maxIntervalMs)
                                                                                                                              .setAlertAfterIntervalMs(alertAfterIntervalMs);
        assertEquals(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
        assertEquals(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs());
    }

    @Test
    public void maxIntervalAdjustedIfSmallerThanMinInterval() throws Exception {
        long minIntervalMs = 2000;
        long maxIntervalMs = 1000;
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMaxIntervalMs(maxIntervalMs)
                                                                                                                              .setMinIntervalMs(minIntervalMs);
        assertEquals(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinIntervalMs());
        assertEquals(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
    }
}

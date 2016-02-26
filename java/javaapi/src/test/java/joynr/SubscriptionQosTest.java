package joynr;

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
import static org.junit.Assert.*;

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
        OnChangeSubscriptionQos onChangeSubscriptionQos = new OnChangeSubscriptionQos().setExpiryDate(expiryDateMs)
                                                                                       .setPublicationTtl(publicationTtlMs)
                                                                                       .setMinInterval(minIntervalMs);
        assertEquals(expiryDateMs, onChangeSubscriptionQos.getExpiryDate());
        assertEquals(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtl());
        assertEquals(minIntervalMs, onChangeSubscriptionQos.getMinInterval());
    }

    @Test
    public void createOnChangeSubscriptionQosWithValidity() throws Exception {
        long validityMs = 100000;

        long lowerBound = System.currentTimeMillis() + validityMs;
        SubscriptionQos onChangeSubscriptionQos = new OnChangeSubscriptionQos().setValidityMs(validityMs);
        long upperBound = System.currentTimeMillis() + validityMs;

        assertTrue(onChangeSubscriptionQos.getExpiryDate() >= lowerBound);
        assertTrue(onChangeSubscriptionQos.getExpiryDate() <= upperBound);
    }

    @Test
    public void createPeriodicSubscriptionQos() throws Exception {
        long expiryDateMs = System.currentTimeMillis() + 100000;
        long publicationTtlMs = 2000;
        long periodMs = 800;
        long alertAfterIntervalMs = 3000;
        PeriodicSubscriptionQos periodicSubscriptionQos = new PeriodicSubscriptionQos().setExpiryDate(expiryDateMs)
                                                                                       .setPublicationTtl(publicationTtlMs)
                                                                                       .setPeriod(periodMs)
                                                                                       .setAlertAfterInterval(alertAfterIntervalMs);
        assertEquals(expiryDateMs, periodicSubscriptionQos.getExpiryDate());
        assertEquals(publicationTtlMs, periodicSubscriptionQos.getPublicationTtl());
        assertEquals(periodMs, periodicSubscriptionQos.getPeriod());
        assertEquals(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterInterval());
    }

    @Test
    public void createPeriodicSubscriptionQosWithValidity() throws Exception {
        long validityMs = 1000;

        long lowerBound = System.currentTimeMillis() + validityMs;
        SubscriptionQos periodicSubscriptionQos = new PeriodicSubscriptionQos().setValidityMs(validityMs);
        long upperBound = System.currentTimeMillis() + validityMs;

        assertTrue(periodicSubscriptionQos.getExpiryDate() >= lowerBound);
        assertTrue(periodicSubscriptionQos.getExpiryDate() <= upperBound);
    }

    @Test
    public void createOnChangeWithKeepAliveSubscriptionQos() throws Exception {
        long expiryDateMs = System.currentTimeMillis() + 100000;
        long publicationTtlMs = 2000;
        long alertAfterIntervalMs = 4000;
        long maxIntervalMs = 3000;
        long minIntervalMs = 100;
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setExpiryDate(expiryDateMs)
                                                                                                                              .setPublicationTtl(publicationTtlMs)
                                                                                                                              .setMaxInterval(maxIntervalMs)
                                                                                                                              .setMinInterval(minIntervalMs)
                                                                                                                              .setAlertAfterInterval(alertAfterIntervalMs);
        assertEquals(expiryDateMs, onChangeWithKeepAliveSubscriptionQos.getExpiryDate());
        assertEquals(publicationTtlMs, onChangeWithKeepAliveSubscriptionQos.getPublicationTtl());
        assertEquals(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxInterval());
        assertEquals(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinInterval());
        assertEquals(alertAfterIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterInterval());

    }

    @Test
    public void alertAfterIntervalAdjustedIfSmallerThanPeriod() throws Exception {
        long periodMs = 5000;
        long alertAfterIntervalMs = 1000;
        PeriodicSubscriptionQos periodicSubscriptionQos = new PeriodicSubscriptionQos().setPeriod(periodMs)
                                                                                       .setAlertAfterInterval(alertAfterIntervalMs);
        assertEquals(periodMs, periodicSubscriptionQos.getPeriod());
        assertEquals(periodMs, periodicSubscriptionQos.getAlertAfterInterval());
    }

    @Test
    public void alertAfterIntervalAdjustedIfSmallerThanMaxInterval() throws Exception {
        long alertAfterIntervalMs = 4000;
        long maxIntervalMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMaxInterval(maxIntervalMs)
                                                                                                                              .setAlertAfterInterval(alertAfterIntervalMs);
        assertEquals(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxInterval());
        assertEquals(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterInterval());
    }

    @Test
    public void maxIntervalAdjustedIfSmallerThanMinInterval() throws Exception {
        long minIntervalMs = 2000;
        long maxIntervalMs = 1000;
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMaxInterval(maxIntervalMs)
                                                                                                                              .setMinInterval(minIntervalMs);
        assertEquals(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinInterval());
        assertEquals(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxInterval());
    }
}

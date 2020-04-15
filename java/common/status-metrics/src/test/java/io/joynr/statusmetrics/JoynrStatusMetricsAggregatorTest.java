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
package io.joynr.statusmetrics;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class JoynrStatusMetricsAggregatorTest {

    private JoynrStatusMetricsAggregator subject = new JoynrStatusMetricsAggregator();

    @Test
    public void testNotifyMessageDropped() {
        assertEquals(0, subject.getNumDroppedMessages());
        subject.notifyMessageDropped();
        assertEquals(1, subject.getNumDroppedMessages());
    }

    @Test
    public void addSingleConnectionStatusMetricsWithGbid() {
        String gbid1 = "gbid1";
        assertEquals(0, subject.getAllConnectionStatusMetrics().size());
        assertEquals(0, subject.getConnectionStatusMetrics(gbid1).size());

        ConnectionStatusMetricsImpl testMetrics = new ConnectionStatusMetricsImpl();
        testMetrics.setGbid(gbid1);
        subject.addConnectionStatusMetrics(testMetrics);

        assertEquals(1, subject.getAllConnectionStatusMetrics().size());
        assertEquals(1, subject.getConnectionStatusMetrics(gbid1).size());
        assertEquals(testMetrics, subject.getAllConnectionStatusMetrics().iterator().next());
    }

    @Test
    public void addSingleConnectionStatusMetricsWithoutGbid() {
        assertEquals(0, subject.getAllConnectionStatusMetrics().size());
        assertEquals(0, subject.getConnectionStatusMetrics("").size());

        ConnectionStatusMetricsImpl testMetrics = new ConnectionStatusMetricsImpl();
        subject.addConnectionStatusMetrics(testMetrics);

        assertEquals(1, subject.getAllConnectionStatusMetrics().size());
        assertEquals(0, subject.getConnectionStatusMetrics("").size());
        assertEquals(testMetrics, subject.getAllConnectionStatusMetrics().iterator().next());
    }

    @Test
    public void testAddMultipleConnectionStatusMetricsSameGbid() {
        String gbid1 = "gbid1";
        assertEquals(0, subject.getAllConnectionStatusMetrics().size());
        assertEquals(0, subject.getConnectionStatusMetrics(gbid1).size());

        ConnectionStatusMetricsImpl testMetrics1 = new ConnectionStatusMetricsImpl();
        testMetrics1.setGbid(gbid1);
        ConnectionStatusMetricsImpl testMetrics2 = new ConnectionStatusMetricsImpl();
        testMetrics2.setGbid(gbid1);
        subject.addConnectionStatusMetrics(testMetrics1);
        subject.addConnectionStatusMetrics(testMetrics2);

        assertEquals(2, subject.getAllConnectionStatusMetrics().size());
        assertEquals(2, subject.getConnectionStatusMetrics(gbid1).size());
    }

    @Test
    public void testAddMultipleConnectionStatusMetricsDifferentGbids() {
        String gbid1 = "gbid1";
        String gbid2 = "gbid2";
        assertEquals(0, subject.getAllConnectionStatusMetrics().size());
        assertEquals(0, subject.getConnectionStatusMetrics(gbid1).size());

        ConnectionStatusMetricsImpl testMetrics1 = new ConnectionStatusMetricsImpl();
        testMetrics1.setGbid(gbid1);
        ConnectionStatusMetricsImpl testMetrics2 = new ConnectionStatusMetricsImpl();
        testMetrics2.setGbid(gbid2);
        subject.addConnectionStatusMetrics(testMetrics1);
        subject.addConnectionStatusMetrics(testMetrics2);

        assertEquals(2, subject.getAllConnectionStatusMetrics().size());
        assertEquals(1, subject.getConnectionStatusMetrics(gbid1).size());
        assertEquals(1, subject.getConnectionStatusMetrics(gbid2).size());
        assertEquals(testMetrics1, subject.getConnectionStatusMetrics(gbid1).iterator().next());
        assertEquals(testMetrics2, subject.getConnectionStatusMetrics(gbid2).iterator().next());
    }
}

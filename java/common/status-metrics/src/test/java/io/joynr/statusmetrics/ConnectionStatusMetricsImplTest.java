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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import java.time.Instant;
import java.util.Optional;

import org.junit.Test;

/**
 * Unit tests for {@link ConnectionStatusMetricsImpl}.
 */
public class ConnectionStatusMetricsImplTest {
    private ConnectionStatusMetricsImpl subject = new ConnectionStatusMetricsImpl();

    @Test
    public void testInitialization() {
        assertEquals(Optional.empty(), subject.getGbid());
        String testurl = "testurl";
        String testgbid = "testgbid";
        subject.setGbid(testgbid);
        subject.setUrl(testurl);
        subject.setSender(true);
        subject.setReceiver(true);
        assertEquals(testgbid, subject.getGbid().get());
        assertEquals(testurl, subject.getUrl());
        assertTrue(subject.isSender());
        assertTrue(subject.isReceiver());
    }

    @Test
    public void testSetConnected() {
        assertFalse(subject.isConnected());
        subject.setConnected(true);
        assertTrue(subject.isConnected());
    }

    @Test
    public void testSetLastConnectionStateChangesOnlyOnActualChange() throws InterruptedException {
        Instant instant0 = subject.getLastConnectionStateChangeDate();

        Thread.sleep(10);
        subject.setConnected(true);
        Instant instant1 = subject.getLastConnectionStateChangeDate();
        assertTrue(instant0.compareTo(instant1) < 0);

        Thread.sleep(10);
        subject.setConnected(true);
        Instant instant2 = subject.getLastConnectionStateChangeDate();
        assertEquals(instant1, instant2);

        Thread.sleep(10);
        subject.setConnected(false);
        Instant instant3 = subject.getLastConnectionStateChangeDate();
        assertNotEquals(instant1, instant3);
        assertTrue(instant2.compareTo(instant3) < 0);
    }

    @Test
    public void testIncrementReceivedMessages() {
        assertEquals(0, subject.getReceivedMessages());
        subject.increaseReceivedMessages();
        assertEquals(1, subject.getReceivedMessages());
    }

    @Test
    public void testIncrementSentMessages() {
        assertEquals(0, subject.getSentMessages());
        subject.increaseSentMessages();
        assertEquals(1, subject.getSentMessages());
    }

    @Test
    public void testIncrementConnectionDrops() {
        assertEquals(0, subject.getConnectionDrops());
        subject.increaseConnectionDrops();
        assertEquals(1, subject.getConnectionDrops());
    }

    @Test
    public void testIncrementConnectionAttempts() {
        assertEquals(0, subject.getConnectionAttempts());
        subject.increaseConnectionAttempts();
        assertEquals(1, subject.getConnectionAttempts());
    }

}

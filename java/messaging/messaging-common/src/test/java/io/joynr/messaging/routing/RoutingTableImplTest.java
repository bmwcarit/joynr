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
package io.joynr.messaging.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Before;
import org.junit.Test;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.system.RoutingTypes.Address;

/**
 * Unit tests for the {@link RoutingTableImpl}.
 * 
 * @author clive.jevons
 */
public class RoutingTableImplTest {

    private RoutingTableImpl subject;
    private final long routingTableGracePeriod = 42;

    @Before
    public void setup() {
        subject = new RoutingTableImpl(routingTableGracePeriod);
    }

    @Test
    public void testPutAndGet() {
        Address address = new Address();
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        final boolean allowUpdate = false;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);

        Address result = subject.get(participantId);

        assertNotNull(result);
        assertEquals(address, result);
        assertEquals(isGloballyVisible, subject.getIsGloballyVisible(participantId));
    }

    @Test
    public void testNonExistingEntry() {
        assertFalse(subject.containsKey("participantId"));
    }

    @Test
    public void testContainsKeyForExistingEntry() {
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        final boolean allowUpdate = false;
        subject.put(participantId, new Address(), isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
        assertTrue(subject.containsKey(participantId));
    }

    @Test
    public void testPutAddsRoutingTableGracePeriod() {
        Address address = new Address();
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = 1024;
        final boolean isSticky = false;
        final boolean allowUpdate = false;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);

        long result = subject.getExpiryDateMs(participantId);

        assertEquals(expiryDateMs + routingTableGracePeriod, result);
    }

    @Test
    public void testPutAddsRoutingTableGracePeriodWithOverflow() {
        Address address = new Address();
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE - (routingTableGracePeriod / 2);
        final boolean isSticky = false;
        final boolean allowUpdate = false;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);

        long result = subject.getExpiryDateMs(participantId);

        assertEquals(Long.MAX_VALUE, result);
    }

    @Test
    public void testOnlyPutOnce() {
        String participantId = "participantId";
        assertFalse(subject.containsKey(participantId));

        Address address1 = new Address();
        final boolean isGloballyVisible1 = false;
        final long expiryDateMs1 = Long.MAX_VALUE;
        final boolean isSticky1 = false;
        final boolean allowUpdate = false;
        subject.put(participantId, address1, isGloballyVisible1, expiryDateMs1, isSticky1, allowUpdate);
        assertEquals(isGloballyVisible1, subject.getIsGloballyVisible(participantId));

        assertEquals(address1, subject.get(participantId));

        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        final long expiryDateMs2 = Long.MAX_VALUE;
        final boolean isSticky2 = false;
        // insertion shouldn't take place. participantId is already exists in the table and has address1
        subject.put(participantId, address2, isGloballyVisible2, expiryDateMs2, isSticky2, allowUpdate);
        assertEquals(address1, subject.get(participantId));
        assertEquals(isGloballyVisible1, subject.getIsGloballyVisible(participantId));

    }

    @Test
    public void testGetIsGloballyVisible() {
        String participantId = "participantId";
        final boolean allowUpdate = false;
        try {
            subject.getIsGloballyVisible(participantId); // empty routing table
            fail("Expected exception was not thrown");
        } catch (JoynrRuntimeException e) {
        }

        Address address = new Address();
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
        assertEquals(false, subject.getIsGloballyVisible(participantId));

        String participantId1 = "participantId1";
        Address address1 = new Address();
        final boolean isGloballyVisible1 = false;
        final long expiryDateMs1 = Long.MAX_VALUE;
        final boolean isSticky1 = false;
        subject.put(participantId1, address1, isGloballyVisible1, expiryDateMs1, isSticky1, allowUpdate);
        assertEquals(false, subject.getIsGloballyVisible(participantId1));

        String participantId2 = "participantId2";
        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        final long expiryDateMs2 = Long.MAX_VALUE;
        final boolean isSticky2 = false;
        subject.put(participantId2, address2, isGloballyVisible2, expiryDateMs2, isSticky2, allowUpdate);
        assertEquals(true, subject.getIsGloballyVisible(participantId2));
    }

    @Test
    public void testPurge() throws Exception {
        Address address = new Address();
        String participantId = "participantId";
        String participantId2 = "participantId2";
        final boolean isGloballyVisible = false;
        final boolean allowUpdate = false;
        long expiryDateMs = System.currentTimeMillis() + 1000;
        boolean isSticky = false;
        boolean isSticky2 = true;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
        subject.put(participantId2, address, isGloballyVisible, expiryDateMs, isSticky2, allowUpdate);

        Address result = subject.get(participantId);

        assertNotNull(result);
        assertEquals(address, result);
        assertEquals(isGloballyVisible, subject.getIsGloballyVisible(participantId));

        // entry should be still available after immediate purging
        subject.purge();
        result = subject.get(participantId);
        assertNotNull(result);
        assertEquals(address, result);
        assertEquals(isGloballyVisible, subject.getIsGloballyVisible(participantId));

        // entry should be purgable after 1000 msec
        Thread.sleep(1001 + routingTableGracePeriod);
        subject.purge();
        result = subject.get(participantId);
        assertNull(result);
        result = subject.get(participantId2);
        assertNotNull(result);
        assertEquals(address, result);
        assertEquals(isGloballyVisible, subject.getIsGloballyVisible(participantId2));
    }

    @Test
    public void testUpdateExpiryDateOfExistingRoutingEntry() throws Exception {
        Address address = new Address();
        boolean isGloballyVisible = false;
        boolean isSticky = false;
        long expiryDateMs1 = 1;
        long expiryDateMs2 = 2;
        String participantId = "participantId";
        final boolean allowUpdate = false;

        subject.put(participantId, address, isGloballyVisible, expiryDateMs1, isSticky, allowUpdate);
        assertEquals(subject.getExpiryDateMs(participantId), expiryDateMs1 + routingTableGracePeriod);

        subject.put(participantId, address, isGloballyVisible, expiryDateMs2, isSticky, allowUpdate);
        assertEquals(subject.getExpiryDateMs(participantId), expiryDateMs2 + routingTableGracePeriod);

        // Lower expiry dates shall be ignored
        subject.put(participantId, address, isGloballyVisible, expiryDateMs1, isSticky, allowUpdate);
        assertEquals(subject.getExpiryDateMs(participantId), expiryDateMs2 + routingTableGracePeriod);
    }

    @Test
    public void testUpdateStickFlagOfExistingRoutingEntry() throws Exception {
        Address address = new Address();
        boolean isGloballyVisible = false;
        boolean isNotSticky = false;
        boolean isSticky = true;
        long expiryDateMs = 0;
        String participantId = "participantId";
        final boolean allowUpdate = false;

        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isNotSticky, allowUpdate);
        assertEquals(subject.getIsSticky(participantId), isNotSticky);

        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);
        assertEquals(subject.getIsSticky(participantId), isSticky);

        // Stick flag shall not be removed by an update.
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isNotSticky, allowUpdate);
        assertEquals(subject.getIsSticky(participantId), isSticky);
    }

    public void testAllowUpdate() throws Exception {
        final boolean NO_UPDATE = false;
        final boolean DO_UPDATE = true;

        Address address1 = new Address();
        Address address2 = new Address();
        boolean isGloballyVisible = true;
        boolean isSticky = true;
        long expiryDateMs = System.currentTimeMillis();
        String participantId = "participantId";

        subject.put(participantId, address1, isGloballyVisible, expiryDateMs, isSticky, NO_UPDATE);
        assertNotNull(subject.containsKey(participantId));

        subject.put(participantId, address2, isGloballyVisible, expiryDateMs, isSticky, NO_UPDATE);
        assertEquals(address1, subject.get(participantId));

        subject.put(participantId, address2, isGloballyVisible, expiryDateMs, isSticky, DO_UPDATE);
        assertEquals(address2, subject.get(participantId));
    }
}

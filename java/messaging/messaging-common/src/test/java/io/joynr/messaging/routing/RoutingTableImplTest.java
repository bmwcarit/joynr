package io.joynr.messaging.routing;

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

    @Before
    public void setup() {
        subject = new RoutingTableImpl();
    }

    @Test
    public void testPutAndGet() {
        Address address = new Address();
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky);

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
        subject.put(participantId, new Address(), isGloballyVisible, expiryDateMs, isSticky);
        assertTrue(subject.containsKey(participantId));
    }

    @Test
    public void testOnlyPutOnce() {
        String participantId = "participantId";
        assertFalse(subject.containsKey(participantId));

        Address address1 = new Address();
        final boolean isGloballyVisible1 = false;
        final long expiryDateMs1 = Long.MAX_VALUE;
        final boolean isSticky1 = false;
        subject.put(participantId, address1, isGloballyVisible1, expiryDateMs1, isSticky1);
        assertEquals(isGloballyVisible1, subject.getIsGloballyVisible(participantId));

        assertEquals(address1, subject.get(participantId));

        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        final long expiryDateMs2 = Long.MAX_VALUE;
        final boolean isSticky2 = false;
        // insertion shouldn't take place. participantId is already exists in the table and has address1
        subject.put(participantId, address2, isGloballyVisible2, expiryDateMs2, isSticky2);
        assertEquals(address1, subject.get(participantId));
        assertEquals(isGloballyVisible1, subject.getIsGloballyVisible(participantId));

    }

    @Test
    public void testGetIsGloballyVisible() {
        String participantId = "participantId";
        try {
            subject.getIsGloballyVisible(participantId); // empty routing table
            fail("Expected exception was not thrown");
        } catch (JoynrRuntimeException e) {
        }

        Address address = new Address();
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky);
        assertEquals(false, subject.getIsGloballyVisible(participantId));

        String participantId1 = "participantId1";
        Address address1 = new Address();
        final boolean isGloballyVisible1 = false;
        final long expiryDateMs1 = Long.MAX_VALUE;
        final boolean isSticky1 = false;
        subject.put(participantId1, address1, isGloballyVisible1, expiryDateMs1, isSticky1);
        assertEquals(false, subject.getIsGloballyVisible(participantId1));

        String participantId2 = "participantId2";
        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        final long expiryDateMs2 = Long.MAX_VALUE;
        final boolean isSticky2 = false;
        subject.put(participantId2, address2, isGloballyVisible2, expiryDateMs2, isSticky2);
        assertEquals(true, subject.getIsGloballyVisible(participantId2));
    }

    @Test
    public void testPurge() throws Exception {
        Address address = new Address();
        String participantId = "participantId";
        String participantId2 = "participantId2";
        final boolean isGloballyVisible = false;
        long expiryDateMs = System.currentTimeMillis() + 3000;
        boolean isSticky = false;
        boolean isSticky2 = true;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky);
        subject.put(participantId2, address, isGloballyVisible, expiryDateMs, isSticky2);

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

        // entry should be purgable after 3000 msec
        Thread.sleep(3001);
        subject.purge();
        result = subject.get(participantId);
        assertNull(result);
        result = subject.get(participantId2);
        assertNotNull(result);
        assertEquals(address, result);
        assertEquals(isGloballyVisible, subject.getIsGloballyVisible(participantId2));
    }
}

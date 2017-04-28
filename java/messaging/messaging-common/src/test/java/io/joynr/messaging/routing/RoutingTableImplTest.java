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
        subject.put(participantId, address, isGloballyVisible);

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
        subject.put(participantId, new Address(), isGloballyVisible);
        assertTrue(subject.containsKey(participantId));
    }

    @Test
    public void testOnlyPutOnce() {
        String participantId = "participantId";
        assertFalse(subject.containsKey(participantId));

        Address address1 = new Address();
        final boolean isGloballyVisible1 = false;
        subject.put(participantId, address1, isGloballyVisible1);
        assertEquals(isGloballyVisible1, subject.getIsGloballyVisible(participantId));

        assertEquals(address1, subject.get(participantId));

        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        // insertion shouldn't take place. participantId is already exists in the table and has address1
        subject.put(participantId, address2, isGloballyVisible2);
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
        subject.put(participantId, address, isGloballyVisible);
        assertEquals(false, subject.getIsGloballyVisible(participantId));

        String participantId1 = "participantId1";
        Address address1 = new Address();
        final boolean isGloballyVisible1 = false;
        subject.put(participantId1, address1, isGloballyVisible1);
        assertEquals(false, subject.getIsGloballyVisible(participantId1));

        String participantId2 = "participantId2";
        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        subject.put(participantId2, address2, isGloballyVisible2);
        assertEquals(true, subject.getIsGloballyVisible(participantId2));
    }
}

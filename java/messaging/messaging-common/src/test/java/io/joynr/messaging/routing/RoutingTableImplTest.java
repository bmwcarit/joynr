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

import org.junit.Before;
import org.junit.Test;

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
        subject.put(participantId, address);

        Address result = subject.get(participantId);

        assertNotNull(result);
        assertEquals(address, result);
    }

    @Test
    public void testNonExistingEntry() {
        assertFalse(subject.containsKey("participantId"));
    }

    @Test
    public void testContainsKeyForExistingEntry() {
        String participantId = "participantId";
        subject.put(participantId, new Address());
        assertTrue(subject.containsKey(participantId));
    }

    @Test
    public void testOnlyPutOnce() {
        String participantId = "participantId";
        assertFalse(subject.containsKey(participantId));

        Address address1 = new Address();
        subject.put(participantId, address1);

        assertEquals(address1, subject.get(participantId));

        Address address2 = new Address();
        subject.put(participantId, address2);
        assertEquals(address1, subject.get(participantId));
    }

}

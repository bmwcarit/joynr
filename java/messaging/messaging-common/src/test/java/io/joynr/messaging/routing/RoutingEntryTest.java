/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import org.junit.Before;
import org.junit.Test;

import joynr.system.RoutingTypes.Address;

public class RoutingEntryTest {

    private final Address address = new Address();
    private final boolean isGloballyVisible = false;
    private final long expiryDateMs = 10000L;
    private final boolean isSticky = false;

    private RoutingEntry routingEntry;

    @Before
    public void setUp() {
        routingEntry = new RoutingEntry(address, isGloballyVisible, expiryDateMs, isSticky);
    }

    @Test
    public void initWithCorrectRefCountValue() {
        long expectedRefCount = 1L;
        assertEquals(expectedRefCount, routingEntry.getRefCount());
    }

    @Test
    public void setAndGetRefCount() {
        long expectedRefCount = 3L;
        routingEntry.setRefCount(expectedRefCount);
        assertEquals(expectedRefCount, routingEntry.getRefCount());
    }

    @Test
    public void incrementRefCount() {
        long expectedRefCount = 2L;
        long actualRefCount = routingEntry.incRefCount();
        assertEquals(expectedRefCount, actualRefCount);
    }

    @Test
    public void decrementRefCountSuccess() {
        long expectedRefCount = 1L;
        routingEntry.setRefCount(expectedRefCount + 1);
        long actualRefCount = routingEntry.decRefCount();
        assertEquals(expectedRefCount, actualRefCount);
    }

    @Test(expected = IllegalStateException.class)
    public void decrementRefCountThrowsException() throws Exception {
        routingEntry.setRefCount(0L);
        routingEntry.decRefCount();
    }
}

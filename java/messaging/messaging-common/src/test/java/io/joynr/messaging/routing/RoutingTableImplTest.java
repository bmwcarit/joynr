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
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.inprocess.InProcessAddress;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

/**
 * Unit tests for the {@link RoutingTableImpl}.
 * 
 * @author clive.jevons
 */
@RunWith(MockitoJUnitRunner.class)
public class RoutingTableImplTest {

    private RoutingTableImpl subject;
    private final long routingTableGracePeriod = 42;

    @Mock
    private RoutingTableAddressValidator addressValidatorMock;

    @Before
    public void setup() {
        doReturn(true).when(addressValidatorMock).isValidForRoutingTable(any(Address.class));
        subject = new RoutingTableImpl(routingTableGracePeriod, addressValidatorMock);
    }

    @Test
    public void testPutAndGet() {
        Address address = new Address();
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);

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
        subject.put(participantId, new Address(), isGloballyVisible, expiryDateMs);
        assertTrue(subject.containsKey(participantId));
    }

    @Test
    public void testPutAddsRoutingTableGracePeriod() {
        Address address = new Address();
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = 1024;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);

        long result = subject.getExpiryDateMs(participantId);

        assertEquals(expiryDateMs + routingTableGracePeriod, result);
    }

    @Test
    public void testPutAddsRoutingTableGracePeriodWithOverflow() {
        Address address = new Address();
        String participantId = "participantId";
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE - (routingTableGracePeriod / 2);
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);

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
        subject.put(participantId, address1, isGloballyVisible1, expiryDateMs1);
        assertEquals(isGloballyVisible1, subject.getIsGloballyVisible(participantId));

        assertEquals(address1, subject.get(participantId));

        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        final long expiryDateMs2 = Long.MAX_VALUE;
        // insertion shouldn't take place. participantId is already exists in the table and has address1
        subject.put(participantId, address2, isGloballyVisible2, expiryDateMs2);
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
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);
        assertEquals(false, subject.getIsGloballyVisible(participantId));

        String participantId1 = "participantId1";
        Address address1 = new Address();
        final boolean isGloballyVisible1 = false;
        final long expiryDateMs1 = Long.MAX_VALUE;
        subject.put(participantId1, address1, isGloballyVisible1, expiryDateMs1);
        assertEquals(false, subject.getIsGloballyVisible(participantId1));

        String participantId2 = "participantId2";
        Address address2 = new Address();
        final boolean isGloballyVisible2 = true;
        final long expiryDateMs2 = Long.MAX_VALUE;
        subject.put(participantId2, address2, isGloballyVisible2, expiryDateMs2);
        assertEquals(true, subject.getIsGloballyVisible(participantId2));
    }

    @Test
    public void testPurge() throws Exception {
        Address address = new Address();
        String participantId = "participantId";
        String participantId2 = "participantId2";
        final boolean isGloballyVisible = false;
        long expiryDateMs = System.currentTimeMillis() + 1000;
        boolean isStickyFalse = false;
        boolean isStickyTrue = true;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isStickyFalse);
        subject.put(participantId2, address, isGloballyVisible, expiryDateMs, isStickyTrue);

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
        long expiryDateMs1 = 1;
        long expiryDateMs2 = 2;
        String participantId = "participantId";

        subject.put(participantId, address, isGloballyVisible, expiryDateMs1);
        assertEquals(subject.getExpiryDateMs(participantId), expiryDateMs1 + routingTableGracePeriod);

        subject.put(participantId, address, isGloballyVisible, expiryDateMs2);
        assertEquals(subject.getExpiryDateMs(participantId), expiryDateMs2 + routingTableGracePeriod);

        // Lower expiry dates shall be ignored
        subject.put(participantId, address, isGloballyVisible, expiryDateMs1);
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

        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isNotSticky);
        assertEquals(subject.getIsSticky(participantId), isNotSticky);

        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky);
        assertEquals(subject.getIsSticky(participantId), isSticky);

        // Stick flag shall not be removed by an update.
        subject.put(participantId, address, isGloballyVisible, expiryDateMs, isNotSticky);
        assertEquals(subject.getIsSticky(participantId), isSticky);
    }

    @Test
    public void stickyEntriesAreNotReplaced() {
        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean sticky = true;
        final MqttAddress oldAddress = new MqttAddress("testBrokerUri", "testTopic");

        subject.put(participantId, oldAddress, isGloballyVisible, expiryDateMs, sticky);
        assertEquals(oldAddress, subject.get(participantId));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        subject.put(participantId, inProcessAddress, isGloballyVisible, expiryDateMs, sticky);
        assertEquals(oldAddress, subject.get(participantId));
    }

    @Test
    public void stickyEntriesCannotBeRemoved() {
        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean sticky = true;
        final MqttAddress oldAddress = new MqttAddress("testBrokerUri", "testTopic");

        subject.put(participantId, oldAddress, isGloballyVisible, expiryDateMs, sticky);
        assertEquals(oldAddress, subject.get(participantId));

        subject.remove(participantId);
        assertEquals(oldAddress, subject.get(participantId));
    }

    @Test
    public void allAddressTypesAreValidatedBeforePut() {
        final MqttAddress mqttAddress = new MqttAddress();
        final ChannelAddress channelAddress = new ChannelAddress();
        final WebSocketAddress webSocketAddress = new WebSocketAddress();
        final WebSocketClientAddress webSocketClientAddress = new WebSocketClientAddress();
        final InProcessAddress inProcessAddress = new InProcessAddress();

        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final long expiryDateMs = 42l;

        subject.put(participantId, mqttAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, channelAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, webSocketAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, webSocketClientAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, inProcessAddress, isGloballyVisible, expiryDateMs);

        verify(addressValidatorMock).isValidForRoutingTable(mqttAddress);
        verify(addressValidatorMock).isValidForRoutingTable(channelAddress);
        verify(addressValidatorMock).isValidForRoutingTable(webSocketAddress);
        verify(addressValidatorMock).isValidForRoutingTable(webSocketClientAddress);
        verify(addressValidatorMock).isValidForRoutingTable(inProcessAddress);
    }

    @Test
    public void addressIsNotInsertedWhenValidationFails() {
        doReturn(false).when(addressValidatorMock).isValidForRoutingTable(any(Address.class));

        final MqttAddress mqttAddress = new MqttAddress();
        final String participantId = "testParticipantId";
        subject.put(participantId, mqttAddress, true, Long.MAX_VALUE);

        verify(addressValidatorMock, times(0)).allowUpdate(any(RoutingEntry.class), any(RoutingEntry.class));
        assertFalse(subject.containsKey(participantId));
        assertNull(subject.get(participantId));
    }

}

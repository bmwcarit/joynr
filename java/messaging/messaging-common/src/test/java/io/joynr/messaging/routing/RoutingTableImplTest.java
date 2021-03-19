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
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingSkeleton;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.UdsAddress;
import joynr.system.RoutingTypes.UdsClientAddress;
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
    private final String[] gbidsArray = { "joynrtestgbid1", "joynrtestgbid2" };
    private final String participantId = "participantId";
    private final String gcdParticipantId = "gcdParticipantId";

    @Mock
    private RoutingTableAddressValidator addressValidatorMock;

    @Before
    public void setup() {
        doReturn(true).when(addressValidatorMock).isValidForRoutingTable(any(Address.class));
        doReturn(false).when(addressValidatorMock).allowUpdate(any(RoutingEntry.class), any(RoutingEntry.class));
        subject = getRoutingTable(gbidsArray);
        subject.setGcdParticipantId(gcdParticipantId);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testSetGcdParticipantIdWithNull() {
        subject.setGcdParticipantId(null);
    }

    @Test
    public void testSetGcdParticipantIdMethod() throws NoSuchFieldException, SecurityException,
                                                IllegalArgumentException, IllegalAccessException {
        RoutingTableImpl routingTable1 = getRoutingTable(gbidsArray);

        Class<?> reflectClass = routingTable1.getClass();
        Field field = reflectClass.getDeclaredField("gcdParticipantId");
        field.setAccessible(true);
        assertEquals("", field.get(routingTable1));

        routingTable1.setGcdParticipantId(gcdParticipantId);
        assertEquals(gcdParticipantId, field.get(routingTable1));
    }

    private RoutingTableImpl getRoutingTable(String[] gbidsArray) {
        return new RoutingTableImpl(routingTableGracePeriod, gbidsArray, addressValidatorMock);
    }

    @Test
    public void testPutAndGet() {
        Address address = new Address();
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);

        Address result = subject.get(participantId);

        assertNotNull(result);
        assertEquals(address, result);
        assertEquals(isGloballyVisible, subject.getIsGloballyVisible(participantId));
    }

    @Test
    public void testPutAndGetForGcdParticipantId_WebSocketAddress() {
        WebSocketAddress address = new WebSocketAddress();
        WebSocketAddress expectedAddress = new WebSocketAddress(address);
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        subject.put(gcdParticipantId, address, isGloballyVisible, expiryDateMs);

        Address result = subject.get(gcdParticipantId);
        assertNotNull(result);
        assertEquals(expectedAddress, result);

    }

    @Test
    public void testPutAndGetForGcdParticipantId_UdsAddress() {
        UdsAddress address = new UdsAddress();
        UdsAddress expectedAddress = new UdsAddress(address);
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        subject.put(gcdParticipantId, address, isGloballyVisible, expiryDateMs);

        Address result = subject.get(gcdParticipantId);
        assertNotNull(result);
        assertEquals(expectedAddress, result);

    }

    @Test
    public void testPutAndGetForGcdParticipantId_MqttAddress() {
        MqttAddress address = new MqttAddress();
        MqttAddress expectedAddress = new MqttAddress(address);
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        subject.put(gcdParticipantId, address, isGloballyVisible, expiryDateMs);

        Address result = subject.get(gcdParticipantId);
        assertNotNull(result);
        assertEquals(expectedAddress, result);

    }

    private void testPutAndGetWithGbid(String participantId, String gbid, Address address, Address expectedAddress) {
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);

        Address result = subject.get(participantId, gbid);
        assertEquals(expectedAddress, result);
        if (!address.equals(expectedAddress)) {
            assertNotEquals(address, result);
        }
        if (gcdParticipantId.equals(participantId) && expectedAddress instanceof MqttAddress) {
            assertEquals(gbid, ((MqttAddress) result).getBrokerUri());
        } else if (expectedAddress != null) {
            assertEquals(expectedAddress, address);
        }
    }

    @Test
    public void putAndGetWithGBID_emptyGbidNotKnown_gcdParticipantId_mqttAddress() {
        MqttAddress address = new MqttAddress();
        MqttAddress unmodifiedAddress = new MqttAddress(address);
        MqttAddress expectedAddress = new MqttAddress(address);

        testPutAndGetWithGbid(gcdParticipantId, "", address, null);

        testPutAndGetWithGbid(gcdParticipantId, "unknownGbid", address, null);

        expectedAddress.setBrokerUri(gbidsArray[0]);
        testPutAndGetWithGbid(gcdParticipantId, gbidsArray[0], address, expectedAddress);

        expectedAddress.setBrokerUri(gbidsArray[1]);
        testPutAndGetWithGbid(gcdParticipantId, gbidsArray[1], address, expectedAddress);

        // calling the old get API should return the unmodified address
        Address result = subject.get(gcdParticipantId);
        assertEquals(unmodifiedAddress, result);
        assertNotEquals(expectedAddress, unmodifiedAddress);
        assertEquals(unmodifiedAddress, address);
    }

    @Test
    public void putAndGetWithGBID_emptyGbidKnown_gcdParticipantId_mqttAddress() {
        String[] gbidsArray = new String[]{ "" };
        subject = getRoutingTable(gbidsArray);
        subject.setGcdParticipantId(gcdParticipantId);
        MqttAddress address = new MqttAddress();
        MqttAddress unmodifiedAddress = new MqttAddress(address);
        MqttAddress expectedAddress = new MqttAddress(address);

        testPutAndGetWithGbid(gcdParticipantId, "unknownGbid", address, null);

        expectedAddress.setBrokerUri(gbidsArray[0]);
        testPutAndGetWithGbid(gcdParticipantId, "", address, expectedAddress);

        // calling the old get API should return the unmodified address
        Address result = subject.get(gcdParticipantId);
        assertEquals(unmodifiedAddress, result);
        assertEquals(unmodifiedAddress, address);
        assertEquals(unmodifiedAddress, expectedAddress);
    }

    private void testPutAndGetWithGbid_noGbidReplacement(String participantId,
                                                         Address address,
                                                         Address expectedAddress) {
        testPutAndGetWithGbid(participantId, "", address, expectedAddress);

        testPutAndGetWithGbid(participantId, "unknownGbid", address, expectedAddress);

        testPutAndGetWithGbid(participantId, gbidsArray[0], address, expectedAddress);

        testPutAndGetWithGbid(participantId, gbidsArray[1], address, expectedAddress);

        // calling the old get API should return the unmodified address
        Address result = subject.get(participantId);
        assertEquals(expectedAddress, result);
        assertEquals(expectedAddress, address);

        // cleanup
        subject.remove(participantId);
        subject.remove(participantId);
        subject.remove(participantId);
        subject.remove(participantId);
    }

    @Test
    public void putAndGetWithGBID_emptyGbidNotKnown_otherParticipantId_mqttAddress() {
        MqttAddress address = new MqttAddress();
        MqttAddress expectedAddress = new MqttAddress(address);

        testPutAndGetWithGbid_noGbidReplacement(participantId, address, expectedAddress);
    }

    @Test
    public void putAndGetWithGBID_emptyGbidKnown_otherParticipantId_mqttAddress() {
        String[] gbidsArray = new String[]{ "" };
        subject = getRoutingTable(gbidsArray);
        subject.setGcdParticipantId(gcdParticipantId);
        MqttAddress address = new MqttAddress();
        MqttAddress expectedAddress = new MqttAddress(address);

        testPutAndGetWithGbid_noGbidReplacement(participantId, address, expectedAddress);
    }

    private void testPutAndGetWithGbid_nonMqttAddress_noGbidReplacement(String participantId) {
        WebSocketAddress webSocketAddress = new WebSocketAddress();
        WebSocketAddress expectedWebSocketAddress = new WebSocketAddress(webSocketAddress);

        testPutAndGetWithGbid_noGbidReplacement(participantId, webSocketAddress, expectedWebSocketAddress);

        UdsAddress udsAddress = new UdsAddress();
        UdsAddress expectedUdsAddress = new UdsAddress(udsAddress);

        testPutAndGetWithGbid_noGbidReplacement(participantId, udsAddress, expectedUdsAddress);

        UdsClientAddress udsClientAddress = new UdsClientAddress();
        UdsClientAddress expectedUdsClientAddress = new UdsClientAddress(udsClientAddress);

        testPutAndGetWithGbid_noGbidReplacement(participantId, udsClientAddress, expectedUdsClientAddress);

        WebSocketClientAddress webSocketClientAddress = new WebSocketClientAddress();
        WebSocketClientAddress expectedWebSocketClientAddress = new WebSocketClientAddress(webSocketClientAddress);

        testPutAndGetWithGbid_noGbidReplacement(participantId, webSocketClientAddress, expectedWebSocketClientAddress);

        InProcessAddress inProcessAddress = new InProcessAddress(mock(InProcessMessagingSkeleton.class));
        InProcessAddress expectedInProcessAddress = new InProcessAddress(inProcessAddress.getSkeleton());

        testPutAndGetWithGbid_noGbidReplacement(gcdParticipantId, inProcessAddress, expectedInProcessAddress);
    }

    @Test
    public void putAndGetWithGBID_emptyGbidNotKnown_gcdParticipantId() {
        testPutAndGetWithGbid_nonMqttAddress_noGbidReplacement(gcdParticipantId);
    }

    @Test
    public void putAndGetWithGBID_emptyGbidKnown_gcdParticipantId() {
        String[] gbidsArray = new String[]{ "" };
        subject = getRoutingTable(gbidsArray);
        subject.setGcdParticipantId(gcdParticipantId);

        testPutAndGetWithGbid_nonMqttAddress_noGbidReplacement(gcdParticipantId);
    }

    @Test
    public void putAndGetWithGBID_emptyGbidNotKnown_otherParticipantId() {
        testPutAndGetWithGbid_nonMqttAddress_noGbidReplacement(participantId);
    }

    @Test
    public void putAndGetWithGBID_emptyGbidKnown_otherParticipantId() {
        String[] gbidsArray = new String[]{ "" };
        subject = getRoutingTable(gbidsArray);
        subject.setGcdParticipantId(gcdParticipantId);
        testPutAndGetWithGbid_nonMqttAddress_noGbidReplacement(participantId);
    }

    @Test
    public void testNonExistingEntry() {
        assertFalse(subject.containsKey("participantId"));
    }

    @Test
    public void testContainsKeyForExistingEntry() {
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        subject.put(participantId, new Address(), isGloballyVisible, expiryDateMs);
        assertTrue(subject.containsKey(participantId));
    }

    @Test
    public void testPutAddsRoutingTableGracePeriod() {
        Address address = new Address();
        final boolean isGloballyVisible = false;
        final long expiryDateMs = 1024;
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);

        long result = subject.getExpiryDateMs(participantId);

        assertEquals(expiryDateMs + routingTableGracePeriod, result);
    }

    @Test
    public void testPutAddsRoutingTableGracePeriodWithOverflow() {
        Address address = new Address();
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE - (routingTableGracePeriod / 2);
        subject.put(participantId, address, isGloballyVisible, expiryDateMs);

        long result = subject.getExpiryDateMs(participantId);

        assertEquals(Long.MAX_VALUE, result);
    }

    @Test
    public void testOnlyPutOnce() {
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
        doReturn(true).when(addressValidatorMock).allowUpdate(any(RoutingEntry.class), any(RoutingEntry.class));
        final MqttAddress mqttAddress = new MqttAddress();
        final WebSocketAddress webSocketAddress = new WebSocketAddress();
        final WebSocketClientAddress webSocketClientAddress = new WebSocketClientAddress();
        final UdsAddress udsAddress = new UdsAddress();
        final UdsClientAddress udsClientAddress = new UdsClientAddress();
        final InProcessAddress inProcessAddress = new InProcessAddress();

        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final long expiryDateMs = 42l;

        subject.put(participantId, mqttAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, webSocketAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, webSocketClientAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, udsAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, udsClientAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, inProcessAddress, isGloballyVisible, expiryDateMs);

        verify(addressValidatorMock).isValidForRoutingTable(mqttAddress);
        verify(addressValidatorMock).isValidForRoutingTable(webSocketAddress);
        verify(addressValidatorMock).isValidForRoutingTable(webSocketClientAddress);
        verify(addressValidatorMock).isValidForRoutingTable(udsAddress);
        verify(addressValidatorMock).isValidForRoutingTable(udsClientAddress);
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

    @Test
    public void entriesRemovedOnZeroRefCount() {
        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final MqttAddress oldAddress = new MqttAddress("testBrokerUri", "testTopic");

        subject.put(participantId, oldAddress, isGloballyVisible, expiryDateMs);
        subject.put(participantId, oldAddress, isGloballyVisible, expiryDateMs);
        assertEquals(oldAddress, subject.get(participantId));

        subject.remove(participantId);
        assertEquals(oldAddress, subject.get(participantId));
        subject.remove(participantId);
        assertNull(subject.get(participantId));
    }

    @Test
    public void incrementRefCountWithoutPut() {
        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final MqttAddress oldAddress = new MqttAddress("testBrokerUri", "testTopic");

        subject.put(participantId, oldAddress, isGloballyVisible, expiryDateMs);
        subject.incrementReferenceCount(participantId);
        assertEquals(oldAddress, subject.get(participantId));

        subject.remove(participantId);
        assertEquals(oldAddress, subject.get(participantId));
        subject.remove(participantId);
        assertNull(subject.get(participantId));
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void incrementRefCountThrowsOnNoEntry() {
        final String participantId = "testParticipantId";
        subject.incrementReferenceCount(participantId);
    }

}

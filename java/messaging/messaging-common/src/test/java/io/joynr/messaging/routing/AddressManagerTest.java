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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.util.Set;

import com.google.common.collect.Sets;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class AddressManagerTest {

    @Mock
    private RoutingTable routingTable;

    @Mock
    private MulticastAddressCalculator multicastAddressCalculator;

    @Mock
    private MulticastReceiverRegistry multicastReceiverRegistry;

    @Mock
    private ImmutableMessage joynrMessage;

    @Mock
    private Address address;

    private final String participantId = "participantId";

    private AddressManager subject;

    @Before
    public void setup() {
        subject = null;
    }

    @Test(expected = JoynrMessageNotSentException.class)
    public void testNoAddressAvailableForMulticast() {
        createAddressManager(null);
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        subject.getAddresses(joynrMessage);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testNoAddressFoundForNonMulticastMessage() {
        createAddressManager(null);
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_REQUEST);
        subject.getAddresses(joynrMessage);
    }

    @Test
    public void testGetAddressFromRoutingTable() {
        createAddressManager(null);
        when(routingTable.containsKey(participantId)).thenReturn(true);
        when(routingTable.get(participantId)).thenReturn(address);
        when(joynrMessage.getRecipient()).thenReturn(participantId);

        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertNotNull(result.iterator().next());
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyVisibleProvider() {
        createAddressManager(null, multicastAddressCalculator);

        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);

        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(address);

        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertNotNull(result.iterator().next());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testMultipleCalculatorsNoPrimaryGlobalTransport() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        createAddressManager(null, multicastAddressCalculator, anotherMulticastAddressCalculator);
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);

        subject.getAddresses(joynrMessage);
    }

    @Test
    public void testGetAddressFromMultipleCalculators() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        when(anotherMulticastAddressCalculator.supports("mqtt")).thenReturn(true);
        when(anotherMulticastAddressCalculator.calculate(joynrMessage)).thenReturn(address);
        createAddressManager("mqtt", multicastAddressCalculator, anotherMulticastAddressCalculator);
        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);

        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(address, result.iterator().next());
    }

    @Test
    public void testGetLocalMulticastParticipantAddresses() {
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        when(joynrMessage.isReceivedFromGlobal()).thenReturn(true);
        when(joynrMessage.getSender()).thenReturn("from");
        String multicastId = "from/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        Set<String> participantIds = Sets.newHashSet("one", "two");
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(participantIds);
        Address addressOne = mock(Address.class);
        when(routingTable.get("one")).thenReturn(addressOne);
        Address addressTwo = mock(Address.class);
        when(routingTable.get("two")).thenReturn(addressTwo);

        createAddressManager(null, multicastAddressCalculator);
        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(2, result.size());
        assertTrue(result.contains(addressOne));
        assertTrue(result.contains(addressTwo));
    }

    @Test
    public void testGetLocalMulticastParticipantWithoutGlobalTransports() {
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        when(joynrMessage.getSender()).thenReturn("from");
        String multicastId = "from/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        Set<String> particpantIds = Sets.newHashSet("one");
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(particpantIds);
        Address one = mock(Address.class);
        when(routingTable.get("one")).thenReturn(one);

        createAddressManager(null);
        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.contains(one));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyVisibleProvider() {
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        String multicastId = participantId + "/to";
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        Set<String> localParticipantIds = Sets.newHashSet("one");
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(localParticipantIds);
        Address localAddress = mock(Address.class);
        when(routingTable.get("one")).thenReturn(localAddress);

        Address globalAddress = mock(Address.class);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(globalAddress);

        createAddressManager("mqtt", multicastAddressCalculator);

        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(2, result.size());
        assertTrue(result.contains(localAddress));
        assertTrue(result.contains(globalAddress));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyInvisibleProvider() {
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        when(joynrMessage.getSender()).thenReturn("participantId");
        String multicastId = participantId + "/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(false);
        Set<String> localParticipantIds = Sets.newHashSet("one");
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(localParticipantIds);
        Address localAddress = mock(Address.class);
        when(routingTable.get("one")).thenReturn(localAddress);

        Address globalAddress = mock(Address.class);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(globalAddress);

        createAddressManager("mqtt", multicastAddressCalculator);

        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.contains(localAddress));
        assertFalse(result.contains(globalAddress));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForNonExistingProvider() {
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        when(joynrMessage.getSender()).thenReturn("participantId");
        String multicastId = participantId + "/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        when(routingTable.getIsGloballyVisible(participantId)).thenThrow(new JoynrRuntimeException());
        Set<String> localParticipantIds = Sets.newHashSet("one");
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(localParticipantIds);
        Address localAddress = mock(Address.class);
        when(routingTable.get("one")).thenReturn(localAddress);

        Address globalAddress = mock(Address.class);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(globalAddress);

        createAddressManager("mqtt", multicastAddressCalculator);

        Set<Address> result = subject.getAddresses(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.contains(localAddress));
        assertFalse(result.contains(globalAddress));
    }

    @Test(expected = JoynrMessageNotSentException.class)
    public void testGetMulticastAddressFromSingleCalculatorForGloballyInvisibleProvider() {
        createAddressManager(null, multicastAddressCalculator);
        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(false);
        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(address);

        subject.getAddresses(joynrMessage);
    }

    @Test(expected = JoynrMessageNotSentException.class)
    public void testGetMulticastAddressFromSingleCalculatorForNonExistingProvider() {
        createAddressManager(null, multicastAddressCalculator);
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(routingTable.getIsGloballyVisible(participantId)).thenThrow(new JoynrRuntimeException());

        subject.getAddresses(joynrMessage);
    }

    private void createAddressManager(String primaryGlobalTransport,
                                      MulticastAddressCalculator... multicastAddressCalculators) {
        subject = new AddressManager(routingTable,
                                     new AddressManager.PrimaryGlobalTransportHolder(primaryGlobalTransport),
                                     Sets.newHashSet(multicastAddressCalculators),
                                     multicastReceiverRegistry);
    }
}

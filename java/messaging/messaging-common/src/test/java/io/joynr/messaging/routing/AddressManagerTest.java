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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.util.Set;

import com.google.common.collect.Sets;
import io.joynr.exceptions.JoynrIllegalStateException;
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
    private static final String NO_PRIMARY_GLOBAL_TRANSPORT = null;
    private static final String PRIMARY_GLOBAL_TRANSPORT_MQTT = "mqtt";

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
        when(joynrMessage.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_MULTICAST);
    }

    @Test
    public void testNoAddressAvailableForMulticast() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT);

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertEquals(0, result.size());
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyVisibleProvider() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT, multicastAddressCalculator);

        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(address);

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(address, result.iterator().next());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testMultipleCalculatorsNoPrimaryGlobalTransport() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT, multicastAddressCalculator, anotherMulticastAddressCalculator);

        subject.getAddresses(joynrMessage);
    }

    @Test
    public void testGetAddressFromMultipleCalculators() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        when(anotherMulticastAddressCalculator.supports(PRIMARY_GLOBAL_TRANSPORT_MQTT)).thenReturn(true);
        when(anotherMulticastAddressCalculator.calculate(joynrMessage)).thenReturn(address);
        createAddressManager(PRIMARY_GLOBAL_TRANSPORT_MQTT,
                             multicastAddressCalculator,
                             anotherMulticastAddressCalculator);

        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(address, result.iterator().next());
    }

    @Test
    public void testGetLocalMulticastParticipantAddresses() {
        createAddressManager(PRIMARY_GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

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

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertNotNull(result);
        assertEquals(2, result.size());
        assertTrue(result.contains(addressOne));
        assertTrue(result.contains(addressTwo));
    }

    @Test
    public void testGetLocalMulticastParticipantWithoutGlobalTransports() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT);

        when(joynrMessage.getSender()).thenReturn("from");
        String multicastId = "from/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        Set<String> particpantIds = Sets.newHashSet("one");
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(particpantIds);
        Address one = mock(Address.class);
        when(routingTable.get("one")).thenReturn(one);

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.contains(one));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyVisibleProvider() {
        createAddressManager(PRIMARY_GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

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

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertNotNull(result);
        assertEquals(2, result.size());
        assertTrue(result.contains(localAddress));
        assertTrue(result.contains(globalAddress));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyInvisibleProvider() {
        createAddressManager(PRIMARY_GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

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

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.contains(localAddress));
        assertFalse(result.contains(globalAddress));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForNonExistingProvider() {
        createAddressManager(PRIMARY_GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

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

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.contains(localAddress));
        assertFalse(result.contains(globalAddress));
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyInvisibleProvider() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT, multicastAddressCalculator);

        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(false);
        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(address);

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertEquals(0, result.size());
    }

    @Test
    public void testGetMulticastAddressGlobalVisibilityCheckThrowsException() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT, multicastAddressCalculator);

        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(routingTable.getIsGloballyVisible(participantId)).thenThrow(new JoynrRuntimeException());

        Set<Address> result = subject.getAddresses(joynrMessage);
        assertEquals(0, result.size());
    }

    private void createAddressManager(String primaryGlobalTransport,
                                      MulticastAddressCalculator... multicastAddressCalculators) {
        subject = new AddressManager(routingTable,
                                     new AddressManager.PrimaryGlobalTransportHolder(primaryGlobalTransport),
                                     Sets.newHashSet(multicastAddressCalculators),
                                     multicastReceiverRegistry);
    }
}

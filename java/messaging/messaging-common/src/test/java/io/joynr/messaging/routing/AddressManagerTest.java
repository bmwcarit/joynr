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
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class AddressManagerTest {
    private static final String NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED = null;
    private static final String GLOBAL_TRANSPORT_MQTT = "mqtt";

    @Mock
    private RoutingTable routingTable;

    @Mock
    private MulticastAddressCalculator multicastAddressCalculator;

    @Mock
    private MulticastReceiverRegistry multicastReceiverRegistry;

    @Mock
    private ImmutableMessage joynrMessage;

    private Address multicastAddress;

    private Set<Address> multicastAddresses;

    private final String participantId = "participantId";

    private AddressManager subject;

    @Before
    public void setup() {
        String globalTestGbid = "globaltestgbid";
        multicastAddress = new MqttAddress(globalTestGbid, "globaltesttopic");
        multicastAddresses = new HashSet<>();
        multicastAddresses.add(multicastAddress);
        subject = null;
        when(joynrMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST);
    }

    @Test
    public void testNoAddressAvailableForMulticast() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertEquals(0, result.size());
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyVisibleProvider() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED, multicastAddressCalculator);

        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.iterator().next());
        assertEquals(Optional.of(multicastAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testMultipleCalculatorsNoPrimaryGlobalTransport() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED,
                             multicastAddressCalculator,
                             anotherMulticastAddressCalculator);

        subject.getParticipantIdsForImmutableMessage(joynrMessage);
    }

    @Test
    public void testGetAddressFromMultipleCalculators() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        when(anotherMulticastAddressCalculator.supports(GLOBAL_TRANSPORT_MQTT)).thenReturn(true);
        when(anotherMulticastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);
        createAddressManager(GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator, anotherMulticastAddressCalculator);

        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.iterator().next());
        assertEquals(Optional.of(multicastAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
    }

    @Test
    public void testGetLocalMulticastParticipantAddresses() {
        createAddressManager(GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

        when(joynrMessage.isReceivedFromGlobal()).thenReturn(true);
        String multicastId = "from/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        Set<String> participantIds = new HashSet<>(Arrays.asList("one", "two"));
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(participantIds);
        Address addressOne = mock(Address.class);
        when(routingTable.get("one")).thenReturn(addressOne);
        Address addressTwo = mock(Address.class);
        when(routingTable.get("two")).thenReturn(addressTwo);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertNotNull(result);
        assertEquals(2, result.size());

        boolean messageOneFound = false;
        boolean messageTwoFound = false;

        for (String participantId : result) {
            DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(joynrMessage,
                                                                                       1000,
                                                                                       participantId);
            Optional<Address> resultAddress = subject.getAddressForDelayableImmutableMessage(delayableMessage);
            if (resultAddress.equals(Optional.of(addressOne))) {
                messageOneFound = true;
            } else if (resultAddress.equals(Optional.of(addressTwo))) {
                messageTwoFound = true;
            }
        }

        assertTrue(messageOneFound && messageTwoFound);
    }

    @Test
    public void testGetLocalMulticastParticipantWithoutGlobalTransports() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED);

        String multicastId = "from/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        Set<String> participantIds = new HashSet<>(Arrays.asList("one"));
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(participantIds);
        Address one = mock(Address.class);
        when(routingTable.get("one")).thenReturn(one);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.iterator().next());
        assertEquals(Optional.of(one), subject.getAddressForDelayableImmutableMessage(delayablemessage));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyVisibleProvider() {
        createAddressManager(GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

        String multicastId = participantId + "/to";
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        Set<String> localParticipantIds = new HashSet<>(Arrays.asList("one"));
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(localParticipantIds);
        Address localAddress = mock(Address.class);
        when(routingTable.get("one")).thenReturn(localAddress);
        Address globalAddress = mock(MqttAddress.class);
        Set<Address> globalAddressSet = new HashSet<>();
        globalAddressSet.add(globalAddress);
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(globalAddressSet);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertNotNull(result);
        assertEquals(2, result.size());

        boolean messageOneFound = false;
        boolean messageTwoFound = false;

        for (String participantId : result) {
            DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(joynrMessage,
                                                                                       1000,
                                                                                       participantId);
            Optional<Address> resultAddress = subject.getAddressForDelayableImmutableMessage(delayableMessage);
            if (resultAddress.equals(Optional.of(localAddress))) {
                messageOneFound = true;
            } else if (resultAddress.equals(Optional.of(globalAddress))) {
                messageTwoFound = true;
            }
        }

        assertTrue(messageOneFound && messageTwoFound);
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyInvisibleProvider() {
        createAddressManager(GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

        when(joynrMessage.getSender()).thenReturn("participantId");
        String multicastId = participantId + "/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(false);
        Set<String> localParticipantIds = new HashSet<>(Arrays.asList("one"));
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(localParticipantIds);
        Address localAddress = mock(Address.class);
        when(routingTable.get("one")).thenReturn(localAddress);
        Address globalAddress = mock(Address.class);
        Set<Address> globalAddressSet = new HashSet<>();
        globalAddressSet.add(globalAddress);
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.iterator().next());
        assertEquals(Optional.of(localAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
        verify(multicastAddressCalculator, times(0)).calculate(any(ImmutableMessage.class));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForNonExistingProvider() {
        createAddressManager(GLOBAL_TRANSPORT_MQTT, multicastAddressCalculator);

        when(joynrMessage.getSender()).thenReturn("participantId");
        String multicastId = participantId + "/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        when(routingTable.getIsGloballyVisible(participantId)).thenThrow(new JoynrRuntimeException());
        Set<String> localParticipantIds = new HashSet<>(Arrays.asList("one"));
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(localParticipantIds);
        Address localAddress = mock(Address.class);
        when(routingTable.get("one")).thenReturn(localAddress);
        Address globalAddress = mock(Address.class);
        Set<Address> globalAddressSet = new HashSet<>();
        globalAddressSet.add(globalAddress);
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.iterator().next());
        assertEquals(Optional.of(localAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
        verify(multicastAddressCalculator, times(0)).calculate(any(ImmutableMessage.class));
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyInvisibleProvider() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED, multicastAddressCalculator);

        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertEquals(0, result.size());
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyInvisibleProviderNoGlobalTransport() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED, multicastAddressCalculator);

        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(false);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertEquals(1, result.size());
    }

    @Test
    public void testGetSeveralMulticastAddressesFromSingleCalculatorForGloballyVisibleProvider() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED, multicastAddressCalculator);

        MqttAddress secondMulticastAddress = new MqttAddress("testgbid", "testtopic");
        multicastAddresses.add(secondMulticastAddress);
        String sender = participantId + "/multicastname";
        when(joynrMessage.getSender()).thenReturn(sender);
        when(routingTable.getIsGloballyVisible(sender)).thenReturn(true);
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertEquals(2, result.size());

        boolean messageOneFound = false;
        boolean messageTwoFound = false;

        for (String participantId : result) {
            DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(joynrMessage,
                                                                                       1000,
                                                                                       participantId);
            Optional<Address> resultAddress = subject.getAddressForDelayableImmutableMessage(delayableMessage);
            if (resultAddress.equals(Optional.of(multicastAddress))) {
                messageOneFound = true;
            } else if (resultAddress.equals(Optional.of(secondMulticastAddress))) {
                messageTwoFound = true;
            }
        }

        assertTrue(messageOneFound && messageTwoFound);
    }

    @Test
    public void testGetMulticastAddressGlobalVisibilityCheckThrowsException() {
        createAddressManager(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED, multicastAddressCalculator);

        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertEquals(0, result.size());
    }

    private void createAddressManager(String primaryGlobalTransport,
                                      MulticastAddressCalculator... multicastAddressCalculators) {
        subject = new AddressManager(routingTable,
                                     new AddressManager.PrimaryGlobalTransportHolder(primaryGlobalTransport),
                                     new HashSet<MulticastAddressCalculator>(Arrays.asList(multicastAddressCalculators)),
                                     multicastReceiverRegistry);
    }
}

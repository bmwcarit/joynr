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
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class AddressManagerTest {
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
        createAddressManager();

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertEquals(0, result.size());
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyVisibleProvider() {
        createAddressManager(multicastAddressCalculator);

        when(routingTable.getIsGloballyVisible(participantId)).thenReturn(true);
        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");
        when(joynrMessage.getSender()).thenReturn(participantId);
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(1, result.values().iterator().next().size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.values().iterator().next());
        assertEquals(Optional.of(multicastAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
    }

    @Test
    public void testMultipleCalculators() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        createAddressManager(multicastAddressCalculator, anotherMulticastAddressCalculator);
    }

    @Test
    public void testGetAddressFromMultipleCalculators() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        when(anotherMulticastAddressCalculator.supports(GLOBAL_TRANSPORT_MQTT)).thenReturn(true);
        when(anotherMulticastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);
        createAddressManager(multicastAddressCalculator, anotherMulticastAddressCalculator);

        when(joynrMessage.getRecipient()).thenReturn(participantId + "/multicastname");

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(1, result.values().iterator().next().size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.values().iterator().next());
        assertEquals(Optional.of(multicastAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
    }

    @Test
    public void testGetLocalMulticastParticipantAddresses() {
        createAddressManager(multicastAddressCalculator);

        when(joynrMessage.isReceivedFromGlobal()).thenReturn(true);
        String multicastId = "from/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        Set<String> participantIds = new HashSet<>(Arrays.asList("one", "two"));
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(participantIds);
        Address addressOne = mock(Address.class);
        when(routingTable.get("one")).thenReturn(addressOne);
        Address addressTwo = mock(Address.class);
        when(routingTable.get("two")).thenReturn(addressTwo);

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertNotNull(result);
        assertEquals(2, result.size());
        assertNotNull(result.get(addressOne));
        assertEquals(1, result.get(addressOne).size());
        assertNotNull(result.get(addressTwo));
        assertEquals(1, result.get(addressTwo).size());

        boolean messageOneFound = false;
        boolean messageTwoFound = false;

        for (Set<String> participantId : result.values()) {
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
        createAddressManager();

        String multicastId = "from/to";
        when(joynrMessage.getRecipient()).thenReturn(multicastId);
        Set<String> participantIds = new HashSet<>(Arrays.asList("one"));
        when(multicastReceiverRegistry.getReceivers(multicastId)).thenReturn(participantIds);
        Address one = mock(Address.class);
        when(routingTable.get("one")).thenReturn(one);

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(1, result.values().iterator().next().size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.values().iterator().next());
        assertEquals(Optional.of(one), subject.getAddressForDelayableImmutableMessage(delayablemessage));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyVisibleProvider() {
        createAddressManager(multicastAddressCalculator);

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

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertNotNull(result);
        assertEquals(2, result.size());
        assertNotNull(result.get(localAddress));
        assertEquals(1, result.get(localAddress).size());
        assertNotNull(result.get(globalAddress));
        assertEquals(1, result.get(globalAddress).size());

        String participantIdLocal = result.get(localAddress).iterator().next();
        DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   Set.of(participantIdLocal));
        Optional<Address> resultAddress = subject.getAddressForDelayableImmutableMessage(delayableMessage);
        assertEquals(Optional.of(localAddress), resultAddress);

        String participantIdGlobal = result.get(globalAddress).iterator().next();
        delayableMessage = new DelayableImmutableMessage(joynrMessage, 1000, Set.of(participantIdGlobal));
        resultAddress = subject.getAddressForDelayableImmutableMessage(delayableMessage);
        assertEquals(Optional.of(globalAddress), resultAddress);
    }

    @Test
    public void testGetLocalAndGlobalAddressesForGloballyInvisibleProvider() {
        createAddressManager(multicastAddressCalculator);

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

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(1, result.values().iterator().next().size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.values().iterator().next());
        assertEquals(Optional.of(localAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
        verify(multicastAddressCalculator, times(0)).calculate(any(ImmutableMessage.class));
    }

    @Test
    public void testGetLocalAndGlobalAddressesForNonExistingProvider() {
        createAddressManager(multicastAddressCalculator);

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

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(1, result.values().iterator().next().size());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.values().iterator().next());
        assertEquals(Optional.of(localAddress), subject.getAddressForDelayableImmutableMessage(delayablemessage));
        verify(multicastAddressCalculator, times(0)).calculate(any(ImmutableMessage.class));
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyInvisibleProvider() {
        createAddressManager(multicastAddressCalculator);

        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertEquals(0, result.size());
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculatorForGloballyInvisibleProviderNoGlobalTransport() {
        createAddressManager(multicastAddressCalculator);

        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(false);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertEquals(1, result.size());
        assertEquals(1, result.values().iterator().next().size());
    }

    @Test
    public void testGetSeveralMulticastAddressesFromSingleCalculatorForGloballyVisibleProvider() {
        createAddressManager(multicastAddressCalculator);

        MqttAddress secondMulticastAddress = new MqttAddress("testgbid", "testtopic");
        multicastAddresses.add(secondMulticastAddress);
        String sender = participantId + "/multicastname";
        when(joynrMessage.getSender()).thenReturn(sender);
        when(routingTable.getIsGloballyVisible(sender)).thenReturn(true);
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(multicastAddresses);

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertEquals(2, result.size());
        assertNotNull(result.get(secondMulticastAddress));
        assertEquals(1, result.get(secondMulticastAddress).size());
        assertNotNull(result.get(multicastAddress));
        assertEquals(1, result.get(multicastAddress).size());

        String participantId1 = result.get(multicastAddress).iterator().next();
        DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   Set.of(participantId1));
        Optional<Address> resultAddress = subject.getAddressForDelayableImmutableMessage(delayableMessage);
        assertEquals(Optional.of(multicastAddress), resultAddress);

        String participantId2 = result.get(secondMulticastAddress).iterator().next();
        delayableMessage = new DelayableImmutableMessage(joynrMessage, 1000, Set.of(participantId2));
        resultAddress = subject.getAddressForDelayableImmutableMessage(delayableMessage);
        assertEquals(Optional.of(secondMulticastAddress), resultAddress);
    }

    @Test
    public void testGetMulticastAddressGlobalVisibilityCheckThrowsException() {
        createAddressManager(multicastAddressCalculator);

        when(joynrMessage.getSender()).thenReturn(participantId + "/multicastname");
        when(multicastAddressCalculator.createsGlobalTransportAddresses()).thenReturn(true);

        Map<Address, Set<String>> result = subject.getParticipantIdMap(joynrMessage);
        assertEquals(0, result.size());
    }

    private void createAddressManager(MulticastAddressCalculator... multicastAddressCalculators) {
        subject = new AddressManager(routingTable,
                                     new HashSet<MulticastAddressCalculator>(Arrays.asList(multicastAddressCalculators)),
                                     multicastReceiverRegistry);
    }
}

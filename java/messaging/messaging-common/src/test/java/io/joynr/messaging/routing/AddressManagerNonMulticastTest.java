/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameter;
import org.junit.runners.Parameterized.Parameters;
import org.mockito.Mock;

import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(Parameterized.class)
public class AddressManagerNonMulticastTest {
    private static final String NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED = null;
    private static final String PARTICIPANT_ID = "participantId";

    @Parameters(name = "{index}: MessageType={0}")
    public static Iterable<? extends Object> getNonMulticastMessageTypes() {
        return Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                             Message.MessageType.VALUE_MESSAGE_TYPE_REPLY,
                             Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION,
                             Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                             Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY);
    }

    @Parameter
    public Message.MessageType messageTypeParameter;

    @Mock
    private RoutingTable routingTable;

    @Mock
    private MulticastReceiverRegistry multicastReceiverRegistry;

    @Mock
    private ImmutableMessage joynrMessage;

    @Mock
    private Address address;

    private AddressManager subject;

    @Before
    public void setup() {
        initMocks(this);
        subject = new AddressManager(routingTable,
                                     new AddressManager.PrimaryGlobalTransportHolder(NO_PRIMARY_GLOBAL_TRANSPORT_SELECTED),
                                     new HashSet<MulticastAddressCalculator>(),
                                     multicastReceiverRegistry);
        when(joynrMessage.getType()).thenReturn(messageTypeParameter);
    }

    @Test
    public void testNoAddressFoundForNonMulticastMessage() {
        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);
        assertEquals(0, result.size());
    }

    @Test
    public void testGetAddressFromRoutingTableForMessageWithoutGbidHeader() {
        when(routingTable.containsKey(PARTICIPANT_ID)).thenReturn(true);
        when(routingTable.get(PARTICIPANT_ID)).thenReturn(address);
        when(joynrMessage.getRecipient()).thenReturn(PARTICIPANT_ID);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(PARTICIPANT_ID, result.iterator().next());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.iterator().next());
        assertEquals(Optional.of(address), subject.getAddressForDelayableImmutableMessage(delayablemessage));

        verify(routingTable).get(PARTICIPANT_ID);
        verify(routingTable, times(0)).get(eq(PARTICIPANT_ID), anyString());
    }

    @Test
    public void testGetAddressFromRoutingTableForMessageWithGbidHeader() {
        final String gbidVal = "gbidVal";
        MqttAddress address = new MqttAddress();
        when(routingTable.containsKey(PARTICIPANT_ID)).thenReturn(true);
        when(routingTable.get(PARTICIPANT_ID, gbidVal)).thenReturn(address);
        when(joynrMessage.getRecipient()).thenReturn(PARTICIPANT_ID);

        Map<String, String> customHeader = new HashMap<>();
        customHeader.put(Message.CUSTOM_HEADER_GBID_KEY, gbidVal);
        when(joynrMessage.getCustomHeaders()).thenReturn(customHeader);

        Set<String> result = subject.getParticipantIdsForImmutableMessage(joynrMessage);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(PARTICIPANT_ID, result.iterator().next());

        DelayableImmutableMessage delayablemessage = new DelayableImmutableMessage(joynrMessage,
                                                                                   1000,
                                                                                   result.iterator().next());
        assertEquals(Optional.of(address), subject.getAddressForDelayableImmutableMessage(delayablemessage));

        verify(routingTable, times(0)).get(PARTICIPANT_ID);
        verify(routingTable, times(1)).get(PARTICIPANT_ID, gbidVal);
    }
}

package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import com.google.common.collect.Sets;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import joynr.JoynrMessage;
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
    private JoynrMessage joynrMessage;

    @Mock
    private Address address;

    private final String participantId = "participantId";

    private AddressManager subject;

    @Before
    public void setup() {
        subject = null;
    }

    @Test(expected = JoynrMessageNotSentException.class)
    public void testNoAddressAvailable() {
        createAddressManager(null);
        subject.getAddress(joynrMessage);
    }

    @Test
    public void testGetAddressFromRoutingTable() {
        createAddressManager(null);
        when(routingTable.containsKey(participantId)).thenReturn(true);
        when(routingTable.get(participantId)).thenReturn(address);
        when(joynrMessage.getTo()).thenReturn(participantId);

        Address result = subject.getAddress(joynrMessage);

        assertNotNull(result);
    }

    @Test
    public void testGetMulticastAddressFromSingleCalculator() {
        createAddressManager(null, multicastAddressCalculator);
        when(joynrMessage.getType()).thenReturn(JoynrMessage.MESSAGE_TYPE_MULTICAST);
        when(multicastAddressCalculator.calculate(joynrMessage)).thenReturn(address);

        Address result = subject.getAddress(joynrMessage);

        assertNotNull(result);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testMultipleCalculatorsNoPrimaryGlobalTransport() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        createAddressManager(null, multicastAddressCalculator, anotherMulticastAddressCalculator);
        when(joynrMessage.getType()).thenReturn(JoynrMessage.MESSAGE_TYPE_MULTICAST);

        subject.getAddress(joynrMessage);
    }

    @Test
    public void testGetAddressFromMultipleCalculators() {
        MulticastAddressCalculator anotherMulticastAddressCalculator = mock(MulticastAddressCalculator.class);
        when(anotherMulticastAddressCalculator.supports("mqtt")).thenReturn(true);
        when(anotherMulticastAddressCalculator.calculate(joynrMessage)).thenReturn(address);
        createAddressManager("mqtt", multicastAddressCalculator, anotherMulticastAddressCalculator);
        when(joynrMessage.getType()).thenReturn(JoynrMessage.MESSAGE_TYPE_MULTICAST);

        Address result = subject.getAddress(joynrMessage);

        assertNotNull(result);
        assertEquals(address, result);
    }

    private void createAddressManager(String primaryGlobalTransport,
                                      MulticastAddressCalculator... multicastAddressCalculators) {
        subject = new AddressManager(routingTable,
                                     new AddressManager.PrimaryGlobalTransportHolder(primaryGlobalTransport),
                                     Sets.newHashSet(multicastAddressCalculators));
    }
}

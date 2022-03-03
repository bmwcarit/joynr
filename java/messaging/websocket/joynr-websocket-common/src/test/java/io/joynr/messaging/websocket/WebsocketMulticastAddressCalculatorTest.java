/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.websocket;

import static org.junit.Assert.assertTrue;
import java.util.Set;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;

@RunWith(MockitoJUnitRunner.class)
public class WebsocketMulticastAddressCalculatorTest {

    @Mock
    private WebSocketAddress webSocketAddress;

    @Mock
    private MqttAddress mqttAddress;

    @Mock
    private ImmutableMessage immutableMessage;

    @Test
    public void testAcceptWebSocketAddress() {
        WebSocketMulticastAddressCalculator subject = new WebSocketMulticastAddressCalculator(webSocketAddress);
        Set<Address> resultSet = subject.calculate(immutableMessage);
        assertTrue(resultSet.size() == 1);
        assertTrue(resultSet.contains(webSocketAddress));
    }

    @Test
    public void testDenyNonWebSocketAddress() {
        WebSocketMulticastAddressCalculator subject = new WebSocketMulticastAddressCalculator(mqttAddress);
        Set<Address> resultSet = subject.calculate(immutableMessage);
        assertTrue(resultSet.size() == 0);
    }

    @Test
    public void testSupportsWebsocketTransport() {
        WebSocketMulticastAddressCalculator subject = new WebSocketMulticastAddressCalculator(webSocketAddress);
        assertTrue(subject.supports("websocket"));
    }

}

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
package io.joynr.messaging.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import org.junit.Before;
import org.junit.Test;

import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingSkeleton;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

public class LibjoynrRoutingTableAddressValidatorTest {

    private LibjoynrRoutingTableAddressValidator validator;

    @Before
    public void setup() {
        validator = new LibjoynrRoutingTableAddressValidator();
    }

    @Test
    public void inProcessAndWebSocketAddressTypesAreValid() {
        assertTrue(validator.isValidForRoutingTable(new InProcessAddress()));
        assertTrue(validator.isValidForRoutingTable(new WebSocketAddress()));
    }

    @Test
    public void otherAddressTypesAreNotValid() {
        assertFalse(validator.isValidForRoutingTable(new MqttAddress()));
        assertFalse(validator.isValidForRoutingTable(new WebSocketClientAddress()));
    }

    @Test
    public void allowUpdateOfInProcessAddress() {
        // inProcessAddress can only be replaced with InProcessAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final InProcessAddress oldAddress = new InProcessAddress(mock(InProcessMessagingSkeleton.class));
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

    @Test
    public void allowUpdateOfWebSocketClientAddress() {
        // precedence: InProcessAddress > WebSocketAddress > WebSocketClientAddress > MqttAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final WebSocketClientAddress oldAddress = new WebSocketClientAddress("testWebSocketId");
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

    @Test
    public void allowUpdateOfWebSocketAddress() {
        // precedence: InProcessAddress > WebSocketAddress > WebSocketClientAddress > MqttAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final WebSocketAddress oldAddress = new WebSocketAddress(WebSocketProtocol.WSS, "testHost", 23, "testPath");
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

    @Test
    public void allowUpdateOfMqttAddress() {
        // precedence: InProcessAddress > WebSocketAddress > WebSocketClientAddress > MqttAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final MqttAddress oldAddress = new MqttAddress("testBrokerUri", "testTopic");
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

}

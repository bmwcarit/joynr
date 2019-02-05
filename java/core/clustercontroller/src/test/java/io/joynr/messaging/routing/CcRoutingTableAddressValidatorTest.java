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
import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BrowserAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

@RunWith(MockitoJUnitRunner.class)
public class CcRoutingTableAddressValidatorTest {

    private CcRoutingTableAddressValidator validator;

    private final String brokerUri = "ownBrokerUri";
    private final String globalTopic = "ownTopic";
    private final String replyToTopic = "ownTopic";
    private final MqttAddress globalAddress = new MqttAddress(brokerUri, globalTopic);
    private final MqttAddress replyToAddress = new MqttAddress(brokerUri, replyToTopic);

    @Mock
    private GlobalAddressProvider globalAddressProviderMock;
    @Mock
    private ReplyToAddressProvider replyToAddressProviderMock;

    ArgumentCaptor<TransportReadyListener> globalAddressReadyListener;
    ArgumentCaptor<TransportReadyListener> replyToAddressReadyListener;

    @Before
    public void setup() {
        globalAddressReadyListener = ArgumentCaptor.forClass(TransportReadyListener.class);
        replyToAddressReadyListener = ArgumentCaptor.forClass(TransportReadyListener.class);
        validator = new CcRoutingTableAddressValidator(globalAddressProviderMock, replyToAddressProviderMock);
        verify(globalAddressProviderMock).registerGlobalAddressesReadyListener(globalAddressReadyListener.capture());
        verify(replyToAddressProviderMock).registerGlobalAddressesReadyListener(replyToAddressReadyListener.capture());
    }

    @Test
    public void globalAddressIsNotValidAsSoonAsGlobalAddressIsReady() {
        final MqttAddress testOwnAddress = new MqttAddress(brokerUri, globalTopic);
        assertTrue(validator.isValidForRoutingTable(testOwnAddress));

        globalAddressReadyListener.getValue().transportReady(globalAddress);
        assertFalse(validator.isValidForRoutingTable(testOwnAddress));
    }

    @Test
    public void replyToAddressIsNotValidAsSoonAsReplyToIsReady() {
        final MqttAddress testOwnAddress = new MqttAddress(brokerUri, replyToTopic);
        assertTrue(validator.isValidForRoutingTable(testOwnAddress));

        replyToAddressReadyListener.getValue().transportReady(replyToAddress);
        assertFalse(validator.isValidForRoutingTable(testOwnAddress));
    }

    @Test
    public void multipleOwnAddresses() {
        globalAddressReadyListener.getValue().transportReady(globalAddress);
        replyToAddressReadyListener.getValue().transportReady(replyToAddress);

        final MqttAddress testGlobalAddress = new MqttAddress(brokerUri, globalTopic);
        final MqttAddress testReplyToAddress = new MqttAddress(brokerUri, replyToTopic);
        // TODO
        assertFalse(validator.isValidForRoutingTable(testGlobalAddress));
        assertFalse(validator.isValidForRoutingTable(testReplyToAddress));
    }

    @Test
    public void otherAddressesOfOwnAddressTypeAreValid() {
        globalAddressReadyListener.getValue().transportReady(globalAddress);
        assertFalse(validator.isValidForRoutingTable(globalAddress));

        Address otherMqttAddress = new MqttAddress(brokerUri, "otherTopic");
        assertTrue(validator.isValidForRoutingTable(otherMqttAddress));

        otherMqttAddress = new MqttAddress("otherBroker", globalTopic);
        assertTrue(validator.isValidForRoutingTable(otherMqttAddress));

        otherMqttAddress = new MqttAddress("otherBroker", "otherTopic");
        assertTrue(validator.isValidForRoutingTable(otherMqttAddress));
    }

    @Test
    public void otherAddressTypesAreValid() {
        globalAddressReadyListener.getValue().transportReady(globalAddress);
        assertFalse(validator.isValidForRoutingTable(globalAddress));

        Address otherAddress = new ChannelAddress();
        assertTrue(validator.isValidForRoutingTable(otherAddress));

        otherAddress = new WebSocketAddress();
        assertTrue(validator.isValidForRoutingTable(otherAddress));

        otherAddress = new WebSocketClientAddress();
        assertTrue(validator.isValidForRoutingTable(otherAddress));

        otherAddress = new InProcessAddress();
        assertTrue(validator.isValidForRoutingTable(otherAddress));

        otherAddress = new BrowserAddress();
        assertTrue(validator.isValidForRoutingTable(otherAddress));

    }
}

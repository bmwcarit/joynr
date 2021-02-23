/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static org.mockito.Matchers.anyBoolean;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyObject;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.FailureAction;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

@RunWith(MockitoJUnitRunner.class)
public class AbstractGlobalMessagingSkeletonTest {

    private class DummyGlobalMessagingSkeleton extends AbstractGlobalMessagingSkeleton {
        public DummyGlobalMessagingSkeleton(RoutingTable routingTable) {
            super(routingTable);
        }

        public void dummyRegisterGlobalRoutingEntry(ImmutableMessage message, String gbid) {
            registerGlobalRoutingEntry(message, gbid);
        }

        @Override
        public void init() {
            throw new UnsupportedOperationException();

        }

        @Override
        public void shutdown() {
            throw new UnsupportedOperationException();

        }

        @Override
        public void registerMulticastSubscription(String multicastId) {
            throw new UnsupportedOperationException();

        }

        @Override
        public void unregisterMulticastSubscription(String multicastId) {
            throw new UnsupportedOperationException();

        }
    }

    private DummyGlobalMessagingSkeleton subject;

    private ObjectMapper objectMapper;

    private final String[] gbidsArray = { "joynrtestgbid1", "joynrtestgbid2" };

    @Mock
    private ImmutableMessage immutableMessage;

    @Mock
    private RoutingTable routingTable;

    @Before
    public void setUp() throws Exception {
        objectMapper = new ObjectMapper();
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, objectMapper);

        // create test subject
        subject = new DummyGlobalMessagingSkeleton(routingTable);
    }

    @Test
    public void testRegisteringGlobalRoutingEntryForRequestMqttAddress() throws Exception {
        final String brokerUri = "testBrokerUri";
        final String topic = "testTopic";
        final MqttAddress address = new MqttAddress(brokerUri, topic);

        final String expectedBrokerUri = gbidsArray[0];
        final MqttAddress expectedAddress = new MqttAddress(expectedBrokerUri, topic);

        testRegisteringGlobalRoutingEntryForRequestTypes(address, expectedAddress);
    }

    private void testRegisteringGlobalRoutingEntryForRequestTypes(final Address address,
                                                                  final Address expectedAddress) throws Exception {
        testRegisteringGlobalRoutingEntry(address, expectedAddress, Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);
        testRegisteringGlobalRoutingEntry(address,
                                          expectedAddress,
                                          Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
        testRegisteringGlobalRoutingEntry(address,
                                          expectedAddress,
                                          Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST);
        testRegisteringGlobalRoutingEntry(address,
                                          expectedAddress,
                                          Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST);
    }

    private void testRegisteringGlobalRoutingEntry(final Address address,
                                                   final Address expectedAddress,
                                                   Message.MessageType messageType) throws Exception {
        final long ttlMs = 1000L;
        final boolean isGloballyVisible = true;

        when(immutableMessage.getType()).thenReturn(messageType);
        when(immutableMessage.getReplyTo()).thenReturn(objectMapper.writeValueAsString(address));
        when(immutableMessage.getSender()).thenReturn("fromParticipantId");
        when(immutableMessage.getTtlMs()).thenReturn(ttlMs);

        subject.dummyRegisterGlobalRoutingEntry(immutableMessage, gbidsArray[0]);

        verify(routingTable).put(immutableMessage.getSender(), expectedAddress, isGloballyVisible, ttlMs);
        reset(routingTable);
    }

    @Test
    public void testRegisteringGlobalRoutingEntryForNonRequestTypes() throws Exception {
        testRegisteringGlobalRoutingEntryForNonRequestType(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST);
        testRegisteringGlobalRoutingEntryForNonRequestType(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY);
        testRegisteringGlobalRoutingEntryForNonRequestType(Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION);
        testRegisteringGlobalRoutingEntryForNonRequestType(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY);
        testRegisteringGlobalRoutingEntryForNonRequestType(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);
        testRegisteringGlobalRoutingEntryForNonRequestType(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP);
    }

    private void testRegisteringGlobalRoutingEntryForNonRequestType(Message.MessageType nonRequestType) {
        when(immutableMessage.getType()).thenReturn(nonRequestType);

        subject.dummyRegisterGlobalRoutingEntry(immutableMessage, gbidsArray[0]);

        verify(immutableMessage, times(0)).getReplyTo();
        verify(immutableMessage, times(0)).getSender();
        verify(immutableMessage, times(0)).getTtlMs();
        verify(routingTable, times(0)).put(anyString(), anyObject(), anyBoolean(), anyLong());
    }

}

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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Message.MessageType;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

@RunWith(MockitoJUnitRunner.class)
public class AbstractGlobalMessagingSkeletonTest {
    public static final Set<MessageType> MESSAGE_TYPE_REQUESTS = new HashSet<>(Arrays.asList(MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                                                             MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                                                                                             MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                                                                                             MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST));
    public static final Set<MessageType> MESSAGE_TYPE_OTHER = new HashSet<>(Arrays.asList(MessageType.VALUE_MESSAGE_TYPE_MULTICAST,
                                                                                          MessageType.VALUE_MESSAGE_TYPE_ONE_WAY,
                                                                                          MessageType.VALUE_MESSAGE_TYPE_PUBLICATION,
                                                                                          MessageType.VALUE_MESSAGE_TYPE_REPLY,
                                                                                          MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY,
                                                                                          MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP));

    private class DummyGlobalMessagingSkeleton extends AbstractGlobalMessagingSkeleton {
        public DummyGlobalMessagingSkeleton(RoutingTable routingTable) {
            super(routingTable);
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

        subject.registerGlobalRoutingEntry(immutableMessage, gbidsArray[0]);

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

        subject.registerGlobalRoutingEntry(immutableMessage, gbidsArray[0]);

        verify(immutableMessage, times(0)).getReplyTo();
        verify(immutableMessage, times(0)).getSender();
        verify(immutableMessage, times(0)).getTtlMs();
        verify(routingTable, times(0)).put(anyString(), any(), anyBoolean(), anyLong());
    }

    private void testUnregisterGlobalRoutingEntry(MessageType msgType, boolean alreadyProcessed) {
        when(immutableMessage.getType()).thenReturn(msgType);
        when(immutableMessage.isMessageProcessed()).thenReturn(alreadyProcessed);

        subject.removeGlobalRoutingEntry(immutableMessage);

        verify(immutableMessage).isMessageProcessed();

        if (alreadyProcessed) {
            verify(immutableMessage, times(0)).messageProcessed();
            verify(routingTable, times(0)).remove(any(String.class));
        } else if (MESSAGE_TYPE_REQUESTS.contains(msgType)) {
            verify(immutableMessage, times(1)).messageProcessed();
            verify(routingTable, times(1)).remove(eq(immutableMessage.getSender()));
        } else {
            verify(immutableMessage, times(1)).messageProcessed();
            verify(routingTable, times(0)).remove(any(String.class));
        }

        reset(routingTable, immutableMessage);
    }

    @Test
    public void unregisterGlobalRoutingEntryForRequests() {
        for (MessageType msgType : MESSAGE_TYPE_REQUESTS) {
            testUnregisterGlobalRoutingEntry(msgType, false);
        }
    }

    @Test
    public void unregisterGlobalRoutingEntryForRequests_alreadyProcessed() {
        for (MessageType msgType : MESSAGE_TYPE_REQUESTS) {
            testUnregisterGlobalRoutingEntry(msgType, true);
        }
    }

    @Test
    public void unregisterGlobalRoutingEntryForNonRequests() {
        for (MessageType msgType : MESSAGE_TYPE_OTHER) {
            testUnregisterGlobalRoutingEntry(msgType, false);
        }
    }

    @Test
    public void unregisterGlobalRoutingEntryForNonRequests_alreadyProcessed() {
        for (MessageType msgType : MESSAGE_TYPE_OTHER) {
            testUnregisterGlobalRoutingEntry(msgType, true);
        }
    }
}

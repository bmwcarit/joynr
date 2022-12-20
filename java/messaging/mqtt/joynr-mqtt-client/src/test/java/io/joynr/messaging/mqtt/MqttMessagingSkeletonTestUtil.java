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
package io.joynr.messaging.mqtt;

import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.Semaphore;

import com.hivemq.client.internal.checkpoint.Confirmable;
import com.hivemq.client.internal.mqtt.message.publish.MqttPublish;
import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;

import io.joynr.messaging.FailureAction;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

final class MqttMessagingSkeletonTestUtil {
    static final FailureAction failIfCalledAction = new FailureAction() {
        @Override
        public void execute(Throwable error) {
            fail("failure action was erroneously called");
        }
    };

    static FailureAction getExpectToBeCalledAction(final Semaphore semaphore) {
        return new FailureAction() {
            @Override
            public void execute(Throwable error) {
                semaphore.release();
            }
        };
    }

    static List<String> feedMqttSkeletonWithRequests(MqttMessagingSkeleton skeleton, int numRequests) throws Exception {
        return feedMqttSkeletonWithMessages(skeleton, Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, numRequests);
    }

    static List<String> feedMqttSkeletonWithMessages(MqttMessagingSkeleton skeleton,
                                                     Message.MessageType messageType,
                                                     int numMessages) throws Exception {
        List<String> messageIds = new LinkedList<>();

        for (int i = 0; i < numMessages; i++) {
            Mqtt5Publish message = createTestMessage(messageType);
            skeleton.transmit(message,
                              getImmutableMessageFromPublish(message).getPrefixedCustomHeaders(),
                              failIfCalledAction);
            messageIds.add(getImmutableMessageFromPublish(message).getId());
        }
        return messageIds;
    }

    static ImmutableMessage getImmutableMessageFromPublish(Mqtt5Publish message) throws Exception {
        return new ImmutableMessage(message.getPayloadAsBytes());
    }

    static Mqtt5Publish createTestRequestMessage() throws Exception {
        return createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);
    }

    static Mqtt5Publish createTestMessage(Message.MessageType messageType) throws Exception {
        return createTestMessage(messageType, 100000, true);
    }

    static Mqtt5Publish createTestMessage(Message.MessageType messageType,
                                          long ttlMs,
                                          boolean ttlAbsolute) throws Exception {
        MutableMessage message = new MutableMessage();

        MqttAddress address = new MqttAddress("testBrokerUri", "testTopic");

        message.setSender("someSender");
        message.setRecipient("someRecipient");
        message.setTtlAbsolute(ttlAbsolute);
        message.setTtlMs(System.currentTimeMillis() + ttlMs);
        message.setPayload(new byte[]{ 0, 1, 2 });
        message.setType(messageType);
        message.setReplyTo(RoutingTypesUtil.toAddressString(address));

        Confirmable confirmableMock = mock(Confirmable.class);
        when(confirmableMock.confirm()).thenReturn(true, false);

        Mqtt5Publish publish = Mqtt5Publish.builder()
                                           .topic("testTopic")
                                           .payload(message.getImmutableMessage().getSerializedMessage())
                                           .build();
        MqttPublish publish1 = (MqttPublish) publish;
        return publish1.withConfirmable(confirmableMock);
    }
}

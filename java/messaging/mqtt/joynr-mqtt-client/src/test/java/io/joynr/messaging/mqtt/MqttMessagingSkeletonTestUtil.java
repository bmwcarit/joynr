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

import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.Semaphore;

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
            ImmutableMessage message = createTestMessage(messageType);
            skeleton.transmit(message.getSerializedMessage(), failIfCalledAction);
            messageIds.add(message.getId());
        }
        return messageIds;
    }

    static ImmutableMessage createTestRequestMessage() throws Exception {
        return createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);
    }

    static ImmutableMessage createTestMessage(Message.MessageType messageType) throws Exception {
        MutableMessage message = new MutableMessage();

        MqttAddress address = new MqttAddress("testBrokerUri", "testTopic");

        message.setSender("someSender");
        message.setRecipient("someRecipient");
        message.setTtlAbsolute(true);
        message.setTtlMs(100000);
        message.setPayload(new byte[]{ 0, 1, 2 });
        message.setType(messageType);
        message.setReplyTo(RoutingTypesUtil.toAddressString(address));

        return message.getImmutableMessage();
    }
}

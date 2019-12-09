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
package io.joynr.messaging.sender;

import org.mockito.Mock;

import io.joynr.messaging.routing.MessageRouter;
import joynr.Message;
import joynr.MutableMessage;

public class MessageSenderTestBase {
    @Mock
    MessageRouter messageRouterMock;

    protected MutableMessage createTestRequestMessage() {
        MutableMessage message = new MutableMessage();
        message.setType(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);
        message.setLocalMessage(false);
        message.setSender("");
        message.setRecipient("");
        message.setPayload(new byte[]{ 0, 1, 2 });

        return message;
    }
}

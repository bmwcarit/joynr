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
package io.joynr.messaging.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.List;

import org.apache.commons.lang.ArrayUtils;
import org.junit.Test;

import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;

public class SplitSMRFTest {

    @Test
    public void deserializeMessages() throws Exception {
        String expectedSender1 = "expectedSender1";
        String expectedRecipient1 = "expectedRecipient1";

        String expectedSender2 = "expectedSender2";
        String expectedRecipient2 = "expectedRecipient2";

        ImmutableMessage message1 = createImmutableMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                           expectedSender1,
                                                           expectedRecipient1);
        ImmutableMessage message2 = createImmutableMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                           expectedSender2,
                                                           expectedRecipient2);

        byte[] message1Serialized = message1.getSerializedMessage();
        byte[] message2Serialized = message2.getSerializedMessage();

        byte[] concatenatedMessages = ArrayUtils.addAll(message2Serialized, message1Serialized);

        List<ImmutableMessage> splitMessages = Utilities.splitSMRF(concatenatedMessages);
        assertEquals(2, splitMessages.size());

        ImmutableMessage retrievedMessage1 = splitMessages.get(1);
        ImmutableMessage retrievedMessage2 = splitMessages.get(0);

        assertEquals(message1.getSender(), retrievedMessage1.getSender());
        assertEquals(message1.getRecipient(), retrievedMessage1.getRecipient());
        assertTrue(Arrays.equals(message1.getUnencryptedBody(), retrievedMessage1.getUnencryptedBody()));

        assertEquals(message2.getSender(), retrievedMessage2.getSender());
        assertEquals(message2.getRecipient(), retrievedMessage2.getRecipient());
        assertTrue(Arrays.equals(message2.getUnencryptedBody(), retrievedMessage2.getUnencryptedBody()));
    }

    private ImmutableMessage createImmutableMessage(Message.MessageType type,
                                                    String sender,
                                                    String recipient) throws SecurityException, EncodingException,
                                                                      UnsuppportedVersionException {
        MutableMessage message = new MutableMessage();

        message.setType(type);
        message.setSender(sender);
        message.setRecipient(recipient);
        message.setPayload(new byte[]{ 0, 1, 2 });

        return message.getImmutableMessage();
    }
}

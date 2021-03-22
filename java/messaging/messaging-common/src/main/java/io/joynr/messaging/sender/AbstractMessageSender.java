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

import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;

public abstract class AbstractMessageSender implements MessageSender {
    private static final Logger logger = LoggerFactory.getLogger(AbstractMessageSender.class);

    private MessageRouter messageRouter;
    private String replyToAddress = null;
    private List<MutableMessage> noReplyToAddressQueue = new ArrayList<MutableMessage>();
    private String globalAddress;

    protected AbstractMessageSender(MessageRouter messageRouter) {
        this.messageRouter = messageRouter;
    }

    @Override
    public void sendMessage(MutableMessage message) {
        boolean needsReplyToAddress = needsReplyToAddress(message);

        if (needsReplyToAddress && replyToAddress == null) {
            synchronized (this) {
                if (replyToAddress == null) {
                    noReplyToAddressQueue.add(message);
                    return;
                }
            }
        }
        if (needsReplyToAddress) {
            message.setReplyTo(message.isStatelessAsync() ? globalAddress : replyToAddress);
        }
        routeMutableMessage(message);
    }

    private boolean needsReplyToAddress(MutableMessage message) {
        final Message.MessageType msgType = message.getType();
        boolean noReplyTo = message.getReplyTo() == null;
        boolean msgTypeNeedsReplyTo = msgType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST)
                || msgType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                || msgType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST)
                || msgType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST);

        return noReplyTo && msgTypeNeedsReplyTo && !message.isLocalMessage();
    }

    protected synchronized void setReplyToAddress(String replyToAddress, String globalAddress) {
        this.replyToAddress = replyToAddress;
        this.globalAddress = globalAddress;

        for (MutableMessage queuedMessage : noReplyToAddressQueue) {
            queuedMessage.setReplyTo(queuedMessage.isStatelessAsync() ? globalAddress : replyToAddress);
            try {
                routeMutableMessage(queuedMessage);
            } catch (Exception e) {
                logger.error("Discarding unroutable message (queued earlier because of temporary unavailability of replyToAddress): ",
                             e);
            }
        }
        noReplyToAddressQueue.clear();
    }

    private void routeMutableMessage(MutableMessage mutableMessage) {
        ImmutableMessage immutableMessage;
        try {
            immutableMessage = mutableMessage.getImmutableMessage();
        } catch (SecurityException | EncodingException | UnsuppportedVersionException exception) {
            throw new JoynrRuntimeException(exception.getMessage());
        }

        messageRouter.route(immutableMessage);
    }
}

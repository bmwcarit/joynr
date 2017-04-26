package io.joynr.messaging.sender;

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

import java.util.ArrayList;
import java.util.List;

import io.joynr.messaging.routing.MessageRouter;
import joynr.JoynrMessage;

public abstract class AbstractMessageSender implements MessageSender {
    private MessageRouter messageRouter;
    private String replyToAddress = null;
    private List<JoynrMessage> noReplyToAddressQueue = new ArrayList<JoynrMessage>();

    protected AbstractMessageSender(MessageRouter messageRouter) {
        this.messageRouter = messageRouter;
    }

    @Override
    public synchronized void sendMessage(JoynrMessage message) {
        boolean needsReplyToAddress = needsReplyToAddress(message);

        if (replyToAddress == null && needsReplyToAddress) {
            noReplyToAddressQueue.add(message);
        } else {
            if (needsReplyToAddress) {
                message.setReplyTo(replyToAddress);
            }
            messageRouter.route(message);
        }
    }

    private boolean needsReplyToAddress(JoynrMessage message) {
        final String msgType = message.getType();
        boolean noReplyTo = message.getReplyTo() == null;
        boolean msgTypeNeedsReplyTo = msgType.equals(JoynrMessage.MESSAGE_TYPE_REQUEST)
                || msgType.equals(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                || msgType.equals(JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST)
                || msgType.equals(JoynrMessage.MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST);

        return noReplyTo && msgTypeNeedsReplyTo && !message.isLocalMessage();
    }

    protected synchronized void setReplyToAddress(String replyToAddress) {
        this.replyToAddress = replyToAddress;

        for (JoynrMessage queuedMessage : noReplyToAddressQueue) {
            queuedMessage.setReplyTo(replyToAddress);
            messageRouter.route(queuedMessage);
        }
        noReplyToAddressQueue.clear();
    }
}

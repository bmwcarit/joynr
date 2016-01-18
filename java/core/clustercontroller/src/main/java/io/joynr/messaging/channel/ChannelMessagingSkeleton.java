package io.joynr.messaging.channel;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.dispatching.DispatcherImpl;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;

import java.io.IOException;

import joynr.JoynrMessage;
import joynr.system.RoutingTypes.ChannelAddress;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

public class ChannelMessagingSkeleton implements IMessagingSkeleton {
    private final MessageRouter messageRouter;

    private static final Logger logger = LoggerFactory.getLogger(DispatcherImpl.class);

    @Inject
    public ChannelMessagingSkeleton(MessageRouter messageRouter) {
        this.messageRouter = messageRouter;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        final String replyToChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
        addRequestorToMessageRouter(message.getFrom(), replyToChannelId);
        try {
            messageRouter.route(message);
        } catch (JoynrSendBufferFullException | JoynrMessageNotSentException | IOException exception) {
            logger.error("Error processing incoming message. Message will be dropped: {} ", message.getHeader());
            failureAction.execute(exception);
        }
    }

    private void addRequestorToMessageRouter(String requestorParticipantId, String replyToChannelId) {
        if (replyToChannelId != null && !replyToChannelId.isEmpty()) {
            messageRouter.addNextHop(requestorParticipantId, new ChannelAddress(replyToChannelId));
        } else {
            /*
             * TODO make sure that all requests (ie not one-way) also have replyTo
             * set, otherwise log an error.
             * Caution: the replyToChannelId is not set in case of local communication
             */
        }
    }

    @Override
    public void init() {
        //do nothing
    }

    @Override
    public void shutdown() {
        //do nothing
    }
}

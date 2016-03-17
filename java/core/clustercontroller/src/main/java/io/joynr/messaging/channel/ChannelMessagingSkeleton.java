package io.joynr.messaging.channel;

import java.io.IOException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

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

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.routing.MessageRouter;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;

public class ChannelMessagingSkeleton implements IMessagingSkeleton {
    private final MessageRouter messageRouter;

    private static final Logger logger = LoggerFactory.getLogger(ChannelMessagingSkeleton.class);

    private MessageReceiver messageReceiver;

    private ObjectMapper objectMapper;

    @Inject
    public ChannelMessagingSkeleton(MessageRouter messageRouter,
                                    MessageReceiver messageReceiver,
                                    ObjectMapper objectMapper) {
        this.messageRouter = messageRouter;
        this.messageReceiver = messageReceiver;
        this.objectMapper = objectMapper;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        final String replyToChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
        addRequestorToMessageRouter(message.getFrom(), replyToChannelId);
        try {
            messageRouter.route(message);
        } catch (JoynrSendBufferFullException | JoynrMessageNotSentException | IOException exception) {
            logger.error("Error processing incoming message. Message will be dropped: {} ", message.getHeader(), exception);
            failureAction.execute(exception);
        }
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        // TODO Auto-generated method stub

    }

    private void addRequestorToMessageRouter(String requestorParticipantId, String replyToChannelId) {
        if (replyToChannelId != null && !replyToChannelId.isEmpty()) {
            Address address = null;
            if (replyToChannelId.contains("MqttAddress")) {
                try {
                    address = objectMapper.readValue(replyToChannelId, MqttAddress.class);
                } catch (IOException e) {
                    logger.warn("Unable to parse MqttAddress from " + replyToChannelId, e);
                }
            }
            if (address == null) {
                address = new ChannelAddress(replyToChannelId);
            }
            messageRouter.addNextHop(requestorParticipantId, address);
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
        messageReceiver.start(new MessageArrivedListener() {

            @Override
            public void messageArrived(final JoynrMessage message) {
                transmit(message, new FailureAction() {
                    @Override
                    public void execute(Throwable error) {
                        logger.error("error processing incoming message: {} error: {}",
                                     message.getId(),
                                     error.getMessage());
                    }
                });
            }

            @Override
            public void error(JoynrMessage message, Throwable error) {
                logger.error("error receiving incoming message: {} error: {}", message.getId(), error.getMessage());
            }
        }, new ReceiverStatusListener() {

            @Override
            public void receiverStarted() {

            }

            @Override
            public void receiverException(Throwable e) {
                logger.error("error in long polling message receiver error: {}", e.getMessage());
                shutdown();
            }
        });
    }

    @Override
    public void shutdown() {
        messageReceiver.shutdown(false);
    }

}

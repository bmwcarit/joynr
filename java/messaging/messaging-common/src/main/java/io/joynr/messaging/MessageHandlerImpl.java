package io.joynr.messaging;

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

import java.io.IOException;

import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.Inject;
import com.google.inject.name.Named;

/**
 * Repeatedly tries to send a message until successful sending or the time to live has expired.
 */

public class MessageHandlerImpl implements MessageHandler {
    private static final Logger logger = LoggerFactory.getLogger(MessageHandlerImpl.class);
    public static final int THREADPOOLSIZE = 4;

    private MessageScheduler sendRequestScheduler;

    @Inject
    public MessageHandlerImpl(MessageScheduler sendRequestScheduler,
                              @Named(MessagingPropertyKeys.CHANNELID) String ownChannelId) {
        this.sendRequestScheduler = sendRequestScheduler;
    }

    /* (non-Javadoc)
     * @see io.joynr.messaging.MessageHandler#sendMessage(java.lang.String, joynr.JoynrMessage)
     */
    @Override
    public void sendMessage(final Address address, final JoynrMessage message) throws JoynrSendBufferFullException,
                                                                              JoynrMessageNotSentException,
                                                                              JsonGenerationException,
                                                                              JsonMappingException, IOException {

        sendRequestScheduler.scheduleMessage(address, message);
    }

    /* (non-Javadoc)
     * @see io.joynr.messaging.MessageHandler#shutdown()
     */
    @Override
    public void shutdown() {
        try {
            sendRequestScheduler.shutdown();
        } catch (Throwable e) {
            logger.error("Exception caught while shutting down");
        }
    }
}

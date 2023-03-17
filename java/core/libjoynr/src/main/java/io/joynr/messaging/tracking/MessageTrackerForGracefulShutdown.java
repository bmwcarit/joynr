/*-
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.messaging.tracking;

import static io.joynr.proxy.StatelessAsyncIdCalculator.REQUEST_REPLY_ID_SEPARATOR;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Request;

@Singleton
public class MessageTrackerForGracefulShutdown implements ShutdownListener {

    private static final Logger logger = LoggerFactory.getLogger(MessageTrackerForGracefulShutdown.class);

    private static final Set<Message.MessageType> MESSAGE_TYPE_REQUESTS = new HashSet<>(Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                                                                      Message.MessageType.VALUE_MESSAGE_TYPE_REPLY));

    private static final Set<Message.MessageType> MESSAGE_TYPE_SUPPORTED = new HashSet<>(Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY));

    private static final String PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT = "joynr.runtime.prepareforshutdowntimeout";

    private Set<String> registeredMessages = ConcurrentHashMap.newKeySet();

    private ObjectMapper objectMapper;

    @Inject(optional = true)
    @Named(PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT)
    private int prepareForShutdownTimeoutSec = 5;

    @Inject
    public MessageTrackerForGracefulShutdown(ShutdownNotifier shutdownNotifier, ObjectMapper objectMapper) {
        shutdownNotifier.registerForShutdown(this);
        this.objectMapper = objectMapper;
    }

    /**
     * Method for registering a message (single message or message pair in case of RPC request)
     * @param immutableMessage message to be registered
     */
    public void register(final ImmutableMessage immutableMessage) {
        if (immutableMessage != null) {
            Message.MessageType type = immutableMessage.getType();
            if (!isMessageTypeSupported(type)) {
                return;
            }

            String messageId = getId(immutableMessage);

            logger.info("Trying to register message with the following ID: {}", messageId);
            boolean registrationResult = registeredMessages.add(messageId);
            if (registrationResult) {
                logger.info("The message with the following ID: {} has been successfully registered", messageId);
            } else {
                logger.error("The message with the following ID is already registered: {}", messageId);
            }
        } else {
            throw new JoynrIllegalStateException("The ImmutableMessage object passed for registering is null.");
        }
    }

    /**
     * Method to remove a message (or message pair in case of RPC request/reply) from the list of tracked messages
     * @param immutableMessage message to be unregistered
     */
    public void unregister(final ImmutableMessage immutableMessage) {
        if (immutableMessage != null) {
            Message.MessageType type = immutableMessage.getType();
            if (!isMessageTypeSupported(type)) {
                return;
            }

            String messageId = getId(immutableMessage);

            logger.info("Trying to unregister message with the following ID: {}", messageId);
            boolean unregistrationResult = registeredMessages.remove(messageId);
            if (unregistrationResult) {
                logger.info("The message with the following ID: {} has been successfully un-registered", messageId);
            } else {
                logger.error("The message with the following ID has not been registered: {}", messageId);
            }
        } else {
            throw new JoynrIllegalStateException("The ImmutableMessage object passed for registering is null.");
        }
    }

    /**
     * Method returning number of registered messages
     * @return - number of registered messages
     */
    public int getNumberOfRegisteredMessages() {
        return registeredMessages.size();
    }

    /**
     * Method checking if the message type is request or reply that needs special handling
     * @param type - MessageType
     * @return true if there is a request otherwise false
     */
    private boolean isRequestOrReply(final Message.MessageType type) {
        return MESSAGE_TYPE_REQUESTS.contains(type);
    }

    /**
     * Method for obtaining message identifier messageId or requestReplyId
     * @param immutableMessage
     * @return message identifier
     */
    private String getId(final ImmutableMessage immutableMessage) {
        String messageId;

        if (isRequestOrReply(immutableMessage.getType())) {
            messageId = getRequestReplyId(immutableMessage);
        } else {
            messageId = immutableMessage.getId();
        }

        if (messageId == null) {
            throw new JoynrIllegalStateException("The id of message or request is null.");
        }
        return messageId;
    }

    /**
     * Method for obtaining requestReplyId
     * @param immutableMessage
     * @return requestReplyId as String value
     */
    private String getRequestReplyId(final ImmutableMessage immutableMessage) {
        String requestReplyId = immutableMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
        if (requestReplyId == null || requestReplyId.isEmpty()) {
            try {
                String deserializedPayload = new String(immutableMessage.getUnencryptedBody(), StandardCharsets.UTF_8);
                final Request request = objectMapper.readValue(deserializedPayload, Request.class);
                requestReplyId = request.getRequestReplyId();
            } catch (Exception e) {
                logger.error("Error while trying to get requestReplyId from the message. msgId: {}. from: {} to: {}. Error:",
                             immutableMessage.getId(),
                             immutableMessage.getSender(),
                             immutableMessage.getRecipient(),
                             e);
            }
            if (requestReplyId == null || requestReplyId.isEmpty()) {
                return immutableMessage.getId();
            }
        }
        if (requestReplyId.contains(REQUEST_REPLY_ID_SEPARATOR)) {
            // stateless async
            return immutableMessage.getId();
        }
        return requestReplyId;
    }

    private boolean isMessageTypeSupported(Message.MessageType type) {
        return MESSAGE_TYPE_SUPPORTED.contains(type);
    }

    @Override
    public void prepareForShutdown() {
        int remainingMessages = getNumberOfRegisteredMessages();
        logger.info("PrepareForShutdown called. Number of messages: {}", remainingMessages);

        if (remainingMessages > 0) {
            long shutdownStart = System.currentTimeMillis();
            while (System.currentTimeMillis() - shutdownStart < prepareForShutdownTimeoutSec) {
                if (getNumberOfRegisteredMessages() == 0) {
                    break;
                }
                try {
                    Thread.sleep(5);
                } catch (InterruptedException e) {
                    logger.error("Interrupted while waiting for joynr message tracker to drain.");
                    Thread.currentThread().interrupt();
                }
            }
        }

        remainingMessages = getNumberOfRegisteredMessages();
        if (remainingMessages == 0) {
            logger.info("Joynr message tracker ready for shutdown. There are no more messages for further processing.");
        } else {
            logger.info("Joynr message tracker still reports the following number of messages after prepareForShutdown: {}.",
                        remainingMessages);
        }
    }

    @Override
    public void shutdown() {
        //The method will remain empty
    }

}

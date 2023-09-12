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
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.runtime.PrepareForShutdownListener;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Request;

@Singleton
public class MessageTrackerForGracefulShutdown implements ShutdownListener, PrepareForShutdownListener {

    private static final Logger logger = LoggerFactory.getLogger(MessageTrackerForGracefulShutdown.class);

    private static final int PREPARE_FOR_SHUTDOWN_INNER_WAIT = 500;

    private static final Set<Message.MessageType> MESSAGE_TYPE_REQUESTS = Set.of(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                                                 Message.MessageType.VALUE_MESSAGE_TYPE_REPLY);

    private static final Set<Message.MessageType> MESSAGE_TYPE_UNSUPPORTED = Set.of(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);

    private static final String PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT = "joynr.runtime.prepareforshutdowntimeout";

    private final Set<String> registeredMessages = ConcurrentHashMap.newKeySet();

    private final ObjectMapper objectMapper;

    @Inject(optional = true)
    @Named(PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT)
    private int prepareForShutdownTimeoutSec = 5;

    @Inject
    public MessageTrackerForGracefulShutdown(final ShutdownNotifier shutdownNotifier, final ObjectMapper objectMapper) {
        shutdownNotifier.registerMessageTrackerShutdownListener(this);
        shutdownNotifier.registerMessageTrackerPrepareForShutdownListener(this);
        this.objectMapper = objectMapper;
    }

    /**
     * Method for registering a message (single message or message pair in case of RPC request)
     * @param immutableMessage message to be registered
     */
    public void register(final ImmutableMessage immutableMessage) {
        if (immutableMessage == null) {
            throw new JoynrIllegalStateException("ImmutableMessage cannot be null");
        }

        Message.MessageType type = immutableMessage.getType();
        if (isMessageTypeUnsupported(type)) {
            return;
        }

        String messageId = getId(immutableMessage);

        logger.info("Trying to register message with following ID: {}", messageId);
        boolean isRegistered = registeredMessages.add(messageId);
        if (isRegistered) {
            logger.info("Message with following ID: {} has been successfully registered", messageId);
        } else {
            logger.warn("Message with following ID is already registered: {}", messageId);
        }
    }

    /**
     * Method to remove a message (or message pair in case of RPC request/reply) from the list of tracked messages
     * @param immutableMessage message to be unregistered
     */
    public void unregister(final ImmutableMessage immutableMessage) {
        if (immutableMessage == null) {
            throw new JoynrIllegalStateException("ImmutableMessage cannot be null");
        }

        final Message.MessageType type = immutableMessage.getType();
        if (isMessageTypeUnsupported(type)) {
            return;
        }

        final String messageId = getId(immutableMessage);

        logger.info("Trying to unregister message with following ID: {}", messageId);
        boolean isUnregistered = registeredMessages.remove(messageId);
        if (isUnregistered) {
            synchronized (registeredMessages) {
                if (registeredMessages.isEmpty()) {
                    registeredMessages.notify();
                }
            }
            logger.info("Message with following ID: {} has been successfully unregistered", messageId);
        } else {
            logger.warn("Message with following ID has not been registered: {}", messageId);
        }
    }

    /**
     * Method for unregistering request with requestReplyId after expiry ReplyCaller
     * @param requestReplyId request reply id
     */
    public void unregisterAfterReplyCallerExpired(final String requestReplyId) {
        if (requestReplyId == null || requestReplyId.isEmpty()) {
            throw new JoynrIllegalStateException("The requestReplyId passed for unregistering is null or empty.");
        }

        logger.info("Trying to unregister request with requestReplyId: {} after expiry ReplyCaller", requestReplyId);
        final boolean isUnregistered = registeredMessages.remove(requestReplyId);
        if (isUnregistered) {
            synchronized (registeredMessages) {
                if (registeredMessages.isEmpty()) {
                    registeredMessages.notify();
                }
            }
            logger.info("The request with the following requestReplyId: {} has been successfully unregistered",
                        requestReplyId);
        } else {
            logger.warn("The request with the following requestReplyId has not been registered: {}", requestReplyId);
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
     * @param immutableMessage immutable message
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
     * @param immutableMessage immutable message
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

    private boolean isMessageTypeUnsupported(Message.MessageType type) {
        return MESSAGE_TYPE_UNSUPPORTED.contains(type);
    }

    @Override
    public void prepareForShutdown() {
        logger.info("PrepareForShutdown called. Number of messages: {}", getNumberOfRegisteredMessages());

        synchronized (registeredMessages) {
            final long timeout = System.currentTimeMillis() + TimeUnit.SECONDS.toMillis(prepareForShutdownTimeoutSec);
            while (!registeredMessages.isEmpty() && System.currentTimeMillis() < timeout) {
                try {
                    registeredMessages.wait(PREPARE_FOR_SHUTDOWN_INNER_WAIT);
                } catch (final InterruptedException exception) {
                    logger.error("Interrupted exception inside wait thread: ", exception);
                }
            }
        }

        final int remainingMessages = getNumberOfRegisteredMessages();
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

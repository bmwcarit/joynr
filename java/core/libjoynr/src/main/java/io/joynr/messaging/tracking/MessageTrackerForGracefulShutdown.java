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
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

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
    private static final String PROPERTY_ENABLE_LOGGING = "joynr.message.tracker.logging.enabled";

    private final Set<MessageToTrack> registeredMessages = ConcurrentHashMap.newKeySet();

    private final ObjectMapper objectMapper;

    @SuppressWarnings("FieldCanBeLocal")
    @Inject(optional = true)
    @Named(PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT)
    private int prepareForShutdownTimeoutSec = 5;
    @Inject(optional = true)
    @Named(PROPERTY_ENABLE_LOGGING)
    private boolean loggingEnabled = false;

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
        process(immutableMessage, registerAction);
    }

    private final Consumer<MessageToTrack> registerAction = messageToTrack -> {
        logIfPossible("Trying to register message with following ID: {} and requestReplyId: {}",
                      messageToTrack.getMessageId(),
                      messageToTrack.getRequestReplyId());
        if (registeredMessages.add(messageToTrack)) {
            logIfPossible("Message with following ID: {} and requestReplyId: {} has been successfully registered",
                          messageToTrack.getMessageId(),
                          messageToTrack.getRequestReplyId());
        } else {
            logIfPossible("Message with following ID: {} and requestReplyId: {} is already registered.",
                          messageToTrack.getMessageId(),
                          messageToTrack.getRequestReplyId());
        }
    };

    private final Consumer<MessageToTrack> unregisterAction = messageToTrack -> {
        logIfPossible("Trying to unregister message with following ID: {} and requestReplyId: {}",
                      messageToTrack.getMessageId(),
                      messageToTrack.getRequestReplyId());
        if (registeredMessages.remove(messageToTrack)) {
            synchronized (registeredMessages) {
                if (registeredMessages.isEmpty()) {
                    registeredMessages.notify();
                }
            }
            logIfPossible("Message with following ID: {} and requestReplyId: {} has been successfully unregistered",
                          messageToTrack.getMessageId(),
                          messageToTrack.getRequestReplyId());
        } else {
            logIfPossible("Message with following ID {} and requestReplyId: {} has not been unregistered.",
                          messageToTrack.getMessageId(),
                          messageToTrack.getRequestReplyId());
        }
    };

    /**
     * Processes an action with immutable message
     * @param immutableMessage immutable message instance
     * @param action           action to invoke (register or unregister)
     */
    private void process(final ImmutableMessage immutableMessage, final Consumer<MessageToTrack> action) {
        checkIfNull(immutableMessage);
        if (isMessageTypeUnsupported(immutableMessage)) {
            return;
        }
        final String messageId = getId(immutableMessage);
        final String requestReplyId = getRequestReplyId(immutableMessage);
        final MessageToTrack messageToTrack = new MessageToTrack(messageId, requestReplyId, immutableMessage.getType());
        action.accept(messageToTrack);
    }

    /**
     * Logs message as a debug one if logging is enabled
     * @param message   message to log
     * @param parameter parameter of message to log
     */
    private void logIfPossible(final String message, final String parameter) {
        if (loggingEnabled) {
            logger.debug(message, parameter);
        }
    }

    /**
     * Logs message as a debug one if logging is enabled
     * @param message   message to log
     * @param parameter1 first parameter of message to log
     * @param parameter2 second parameter of message to log
     */
    private void logIfPossible(final String message, final String parameter1, final String parameter2) {
        if (loggingEnabled) {
            logger.debug(message, parameter1, parameter2);
        }
    }

    private void checkIfNull(final ImmutableMessage immutableMessage) {
        if (immutableMessage == null) {
            throw new JoynrIllegalStateException("ImmutableMessage cannot be null");
        }
    }

    /**
     * Method to remove a message (or message pair in case of RPC request/reply) from the list of tracked messages
     * @param immutableMessage message to be unregistered
     */
    public void unregister(final ImmutableMessage immutableMessage) {
        process(immutableMessage, unregisterAction);
    }

    /**
     * Method for unregistering request with requestReplyId after expiry ReplyCaller
     * @param requestReplyId request reply id
     */
    public void unregisterAfterReplyCallerExpired(final String requestReplyId) {
        if (requestReplyId == null || requestReplyId.isEmpty()) {
            throw new JoynrIllegalStateException("The requestReplyId passed for unregistering is null or empty.");
        }

        logIfPossible("Trying to unregister request with requestReplyId: {} after expiry ReplyCaller", requestReplyId);
        final Optional<MessageToTrack> optional = findRequestByRequestReplyId(requestReplyId);
        optional.ifPresentOrElse((messageToTrack) -> {
            registeredMessages.remove(messageToTrack);
            synchronized (registeredMessages) {
                if (registeredMessages.isEmpty()) {
                    registeredMessages.notify();
                }
            }
            logIfPossible("The request with the following requestReplyId: {} has been successfully unregistered",
                          requestReplyId);
        },
                                 () -> logIfPossible("The request with the following requestReplyId has not been unregistered: {}",
                                                     requestReplyId));
    }

    private Optional<MessageToTrack> findRequestByRequestReplyId(final String requestReplyId) {
        return registeredMessages.stream()
                                 .filter(messageToTrack -> messageToTrack.isRequestWithRequestReplyId(requestReplyId))
                                 .findAny();
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
        final String messageId = immutableMessage.getId();
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
        if (isRequestOrReply(immutableMessage.getType())) {
            String requestReplyId = immutableMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
            if (requestReplyId == null || requestReplyId.isEmpty()) {
                try {
                    String deserializedPayload = new String(immutableMessage.getUnencryptedBody(),
                                                            StandardCharsets.UTF_8);
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
        } else {
            return immutableMessage.getId();
        }
    }

    private boolean isMessageTypeUnsupported(final ImmutableMessage immutableMessage) {
        return isMessageTypeUnsupported(immutableMessage.getType());
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

    public static class MessageToTrack {
        private final String messageId;
        private final String requestReplyId;
        private final Message.MessageType messageType;

        public MessageToTrack(final String messageId,
                              final String requestReplyId,
                              final Message.MessageType messageType) {
            this.messageId = messageId;
            this.requestReplyId = requestReplyId;
            this.messageType = messageType;
        }

        public String getMessageId() {
            return messageId;
        }

        public String getRequestReplyId() {
            return requestReplyId;
        }

        public boolean isRequestWithRequestReplyId(final String requestReplyId) {
            return messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST)
                    && this.requestReplyId.equals(requestReplyId);
        }

        @Override
        public boolean equals(final Object o) {
            if (this == o)
                return true;
            if (o == null || getClass() != o.getClass())
                return false;
            final MessageToTrack that = (MessageToTrack) o;
            return Objects.equals(messageId, that.messageId) && Objects.equals(requestReplyId, that.requestReplyId)
                    && messageType == that.messageType;
        }

        @Override
        public int hashCode() {
            return Objects.hash(messageId, requestReplyId, messageType);
        }
    }
}

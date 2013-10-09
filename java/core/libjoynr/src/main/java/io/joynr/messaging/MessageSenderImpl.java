package io.joynr.messaging;

/*
 * #%L
 * joynr::java::core::libjoynr
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static joynr.JoynrMessage.MESSAGE_TYPE_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.httpoperation.FailureAction;
import io.joynr.messaging.httpoperation.HttpConstants;
import io.joynr.messaging.httpoperation.LongPollingMessageReceiver;

import java.io.IOException;
import java.text.DateFormat;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.List;

import joynr.JoynrMessage;
import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

/**
 * Repeatedly tries to send a message until successful sending or the time to live has expired.
 */

public class MessageSenderImpl implements MessageSender {
    private static final Logger logger = LoggerFactory.getLogger(MessageSenderImpl.class);
    private static final DateFormat DateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
    public static final int THREADPOOLSIZE = 4;

    private static final long SECONDS = 1000;

    private final MessagingSettings settings;
    private SendRequestScheduler sendRequestScheduler;

    private final String ownChannelId;

    private final LocalChannelUrlDirectoryClient channelUrlClient;

    private final ObjectMapper objectMapper;

    private final IMessageReceivers messageReceivers;

    private HttpConstants httpConstants;

    @Inject
    public MessageSenderImpl(SendRequestScheduler sendRequestScheduler,
                             @Named(MessagingPropertyKeys.CHANNELID) String ownChannelId,
                             LocalChannelUrlDirectoryClient localChannelUrlClient,
                             MessagingSettings settings,
                             ObjectMapper objectMapper,
                             IMessageReceivers messageReceivers,
                             HttpConstants httpConstants) {
        this.sendRequestScheduler = sendRequestScheduler;
        this.ownChannelId = ownChannelId;
        this.channelUrlClient = localChannelUrlClient;
        this.settings = settings;
        this.objectMapper = objectMapper;
        this.messageReceivers = messageReceivers;
        this.httpConstants = httpConstants;
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.messaging.MessageSender#sendMessage(java.lang.String,
     * io.joynr.messaging.JoynrMessage, long)
     */
    @Override
    public void sendMessage(final String channelId, final JoynrMessage message) throws JoynrSendBufferFullException,
                                                                               JoynrMessageNotSentException,
                                                                               JsonGenerationException,
                                                                               JsonMappingException, IOException {
        if (Thread.currentThread().getName().startsWith(LongPollingMessageReceiver.MESSAGE_RECEIVER_THREADNAME_PREFIX)) {
            // throw new
            // JoynException("It is not allowed to send a joynrMessage in the MessageReceiver Thread. Please create a new Thread when sending messages");
            logger.error("It is not allowed to send a joynrMessage in the MessageReceiver Thread. This might lead to deadlocks and breaking joynr functionality. Please create a new Thread when sending messages.");
            StackTraceElement[] st = Thread.currentThread().getStackTrace();
            StringBuilder sb = new StringBuilder();
            for (StackTraceElement line : st) {
                sb.append(line.toString());
                sb.append("\n");
            }
            logger.error(sb.toString());
        }

        if (message.getType().equals(MESSAGE_TYPE_REQUEST)
                || message.getType().equals(MESSAGE_TYPE_SUBSCRIPTION_REQUEST)) {
            message.setReplyTo(getReplyToChannelId());
        }

        if (messageReceivers.contains(channelId)) {
            sendMessageLocally(message, channelId);
        } else {
            sendMessageViaHttpClient(channelId, message);
        }
    }

    private void sendMessageLocally(JoynrMessage message, String channelId) {
        logger.trace("starting sendMessageLocally");

        messageReceivers.getReceiverForChannelId(channelId).receive(message);
    }

    private void sendMessageViaHttpClient(final String channelId, final JoynrMessage message)
                                                                                             throws JsonGenerationException,
                                                                                             JsonMappingException,
                                                                                             IOException {
        if (message == null) {
            String errorMessage = "message cannot be null in sendMessage";
            logger.error(errorMessage);
            NullPointerException e = new NullPointerException(errorMessage);

            messageReceivers.getReceiverForChannelId(getReplyToChannelId()).onError(null, e);
            throw e;
        }

        logger.trace("starting sendMessageViaHttpClient");
        long currentTimeMillis = System.currentTimeMillis();
        long ttlExpirationDate_ms = message.getExpiryDate();

        if (ttlExpirationDate_ms <= currentTimeMillis) {
            String errorMessage = MessageFormat.format("ttl must be greater than 0 / ttl timestamp must be in the future: now: {0} abs_ttl: {1}",
                                                       currentTimeMillis,
                                                       ttlExpirationDate_ms);
            logger.error(errorMessage);
            throw new JoynrMessageNotSentException(errorMessage);
        }

        logger.trace("SEND messageId: {} from: {} to: {} starting lookup of channelUrl", new Object[]{
                message.getHeaderValue(JoynrMessage.HEADER_NAME_MESSAGE_ID),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID) });
        ChannelUrlInformation channelUrlInfo = channelUrlClient.getUrlsForChannel(channelId);
        logger.trace("SEND messsageId: {} from: {} to: {} finished lookup of channelUrl", new Object[]{
                message.getHeaderValue(JoynrMessage.HEADER_NAME_MESSAGE_ID),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID) });

        if (channelUrlInfo == null) {
            String errorMsg = "channelId: " + channelId + " does not exist";
            logger.error(errorMsg);
            JoynrMessageNotSentException joynrMessageNotSentException = new JoynrMessageNotSentException(errorMsg);
            messageReceivers.getReceiverForChannelId(getReplyToChannelId()).onError(message,
                                                                                    joynrMessageNotSentException);
            throw joynrMessageNotSentException;
        }

        List<String> urls = channelUrlInfo.getUrls();
        if (urls.isEmpty()) {
            String errorMsg = "no channelurl found for channelId: " + channelId;
            logger.error(errorMsg);
            JoynrMessageNotSentException joynrMessageNotSentException = new JoynrMessageNotSentException(errorMsg);
            messageReceivers.getReceiverForChannelId(getReplyToChannelId()).onError(message,
                                                                                    joynrMessageNotSentException);
            throw joynrMessageNotSentException;
        }
        String url = urls.get(0) + "message/"; // TODO handle trying multiple channelUrls

        final MessageContainer messageContainer = new MessageContainer(channelId,
                                                                       message,
                                                                       ttlExpirationDate_ms,
                                                                       url,
                                                                       httpConstants,
                                                                       objectMapper);

        final FailureAction failureAction = new FailureAction() {
            @Override
            public void execute(Throwable error) {
                logger.error("!!!! ERROR SENDING: messageId: {} on Channel: {}. Error: {}", new String[]{
                        message.getId(), channelId, error.getMessage() });
                if (error instanceof JoynrShutdownException) {
                    logger.error("Message not sent because joynr is already shutting down.");
                    return;
                }

                long delay_ms = settings.getSendMsgRetryIntervalMs();
                delay_ms += exponentialWait(messageContainer.getTries());

                // TODO Discuss how to report errors to the replycaller
                // This would remove the replyCaller and future replies will not be received
                // If the reply caller should be notified about all errors the implementation of onError has to be
                // modified
                // messageReceivers.getReceiverForChannelId(getReplyToChannelId()).onError(message, e);

                // Only reschedule the message if the delay would not cause the message to timeout.
                // Otherwise, just discard it now
                long rescheduleTime_absolute = System.currentTimeMillis() + delay_ms;
                if (rescheduleTime_absolute > messageContainer.getExpiryDate()) {
                    logger.warn("DROPPING MESSAGE Retry delay would cause message to expire. channelId: {} {} reschedleTime: "
                                        + DateFormatter.format(rescheduleTime_absolute)
                                        + "TTL deadline: "
                                        + DateFormatter.format(messageContainer.getExpiryDate()),
                                channelId,
                                message);
                    messageReceivers.getReceiverForChannelId(getReplyToChannelId()).onError(message, error);
                    return;
                }

                try {
                    logger.error("Rescheduling messageId: {} with delay " + delay_ms
                                         + " ms, new TTL expiration date: {}",
                                 messageContainer.getMessageId(),
                                 DateFormatter.format(messageContainer.getExpiryDate()));
                    sendRequestScheduler.scheduleRequest(messageContainer,
                                                         delay_ms,
                                                         this,
                                                         messageReceivers.getReceiverForChannelId(getReplyToChannelId()));
                    return;
                } catch (JoynrSendBufferFullException e) {
                    try {
                        logger.error("Rescheduling message: {} delayed {} ms because send buffer is full",
                                     delay_ms,
                                     messageContainer.getMessageId());
                        Thread.sleep(delay_ms);
                        this.execute(e);
                    } catch (InterruptedException e1) {
                        return;
                    }
                }
            }
        };
        // try to schedule a new message once, if the buffer is full, an
        // exception is thrown

        String replyToChannelId = getReplyToChannelId();
        MessageReceiver receiverForChannelId = messageReceivers.getReceiverForChannelId(replyToChannelId);

        logger.trace("SEND messageId: {} from: {} to: {} scheduleRequest", new Object[]{ message.getId(),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID) });
        sendRequestScheduler.scheduleRequest(messageContainer, 0, failureAction, receiverForChannelId);
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.messaging.MessageSender#shutdown()
     */
    @Override
    public void shutdown() {
        try {
            sendRequestScheduler.shutdown();
        } catch (Throwable e) {
            logger.error("Exception caught while shutting down");
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.messaging.MessageSender#getChannelId()
     */
    @Override
    public String getReplyToChannelId() {
        // TODO replace ownChannelId with the reply to channelId to support multiple channels
        return ownChannelId;
    }

    private long exponentialWait(int tries) {
        logger.debug("TRIES: " + tries);
        long millis = (2 ^ tries) * SECONDS;
        return millis;
    }
}

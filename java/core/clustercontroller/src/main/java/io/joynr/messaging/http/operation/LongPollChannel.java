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
package io.joynr.messaging.http.operation;

import java.io.IOException;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.ResponseHandler;
import org.apache.http.client.config.RequestConfig.Builder;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.util.EntityUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.exceptions.JoynrChannelMissingException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingSettings;
import io.joynr.messaging.datatypes.JoynrMessagingError;
import io.joynr.messaging.datatypes.JoynrMessagingErrorCode;
import io.joynr.messaging.util.Utilities;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import io.joynr.util.JoynrThreadFactory;
import joynr.ImmutableMessage;

/**
 * Callable to keep a long polling channel alive and to process incoming messages.
 */

public class LongPollChannel {
    private static final Logger logger = LoggerFactory.getLogger(LongPollChannel.class);
    final SimpleDateFormat format = new SimpleDateFormat("EEE, d MMM yyyy HH:mm:ss.SSS");

    public static final long LONGPOLLING_RETRY_INTERVAL_SECS = 5L;

    private CloseableHttpClient httpclient;
    private boolean shutdown = false;
    private MessageArrivedListener messageArrivedListener;
    private final ObjectMapper objectMapper;

    private String id = "";

    private Lock statusLock = new ReentrantLock();
    private Condition statusChanged = statusLock.newCondition();

    private Boolean longPollingDisabled;
    private MessagingSettings settings;
    private HttpConstants httpConstants;
    private String receiverId;
    private HttpGet httpget;
    protected int statusCode;
    private String statusText;
    private RequestConfig defaultRequestConfig;
    private HttpRequestFactory httpRequestFactory;
    private ExecutorService messageReceiverExecutor;

    // CHECKSTYLE:OFF
    public LongPollChannel(CloseableHttpClient httpclient,
                           RequestConfig defaultRequestConfig,
                           Boolean longPollingDisabled,
                           MessageArrivedListener messageArrivedListener,
                           ObjectMapper objectMapper,
                           MessagingSettings settings,
                           HttpConstants httpConstants,
                           String channelId,
                           String receiverId,
                           HttpRequestFactory httpRequestFactory) {
        // CHECKSTYLE:ON
        this.httpclient = httpclient;
        this.defaultRequestConfig = defaultRequestConfig;
        this.longPollingDisabled = longPollingDisabled;
        this.messageArrivedListener = messageArrivedListener;
        this.objectMapper = objectMapper;
        this.settings = settings;
        this.httpConstants = httpConstants;
        this.receiverId = receiverId;
        this.httpRequestFactory = httpRequestFactory;
        ThreadFactory namedThreadFactory = new JoynrThreadFactory("joynr.LongPollChannel");
        messageReceiverExecutor = Executors.newCachedThreadPool(namedThreadFactory);
    }

    /**
     * Start long polling loop throws ExecutionExeption in order to terminate channel properly returns Void (null) in
     * order to reconnect channel
     */
    public Void longPollLoop() throws JoynrShutdownException {
        logger.debug("LongPollingChannel OPENING CHANNEL: {} ", id);
        try {
            while (true) {
                // start long poll if it has not been disabled.
                // Checking the disable switch and request creation has
                // to be an atomic process.
                // When the request creation is done it can be canceled
                // by a call to suspend.
                if (shutdown) {
                    throw new JoynrShutdownException("shutting down");
                }

                if (longPollingDisabled == true) {
                    try {
                        statusLock.lockInterruptibly();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        throw new JoynrShutdownException("INTERRUPTED. Shutting down");
                    }
                    logger.trace("Waiting for long polling to be resumed.");
                    statusChanged.awaitUninterruptibly();

                } else {
                    longPoll();
                    try {

                        // prevent error when long polling connection was reestablished too soon after repsone being
                        // returned
                        Thread.sleep(10);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        throw new JoynrShutdownException("INTERRUPTED. Shutting down");
                    }
                }

            }
        } finally {
            // shutdown();
            logger.info("LongPollingChannel CHANNEL: " + id + " long poll loop exited");
        }
    }

    private void longPoll() {
        String responseBody = null;
        if (shutdown) {
            return;
        }

        final String asciiString = httpget.getURI().toASCIIString();
        try {
            responseBody = httpclient.execute(httpget, new ResponseHandler<String>() {

                @Override
                public String handleResponse(HttpResponse response) throws IOException {
                    HttpEntity entity = response.getEntity();
                    String body = entity == null ? null : EntityUtils.toString(entity, "UTF-8");
                    statusCode = response.getStatusLine().getStatusCode();
                    statusText = response.getStatusLine().getReasonPhrase();
                    logger.debug("Long poll returned: {} reason: url {}", statusCode, asciiString);
                    return body;
                }
            });
        } catch (IllegalStateException e) {
            logger.error("IllegalStateException in long poll: {} message: {}", asciiString, e.getMessage());
            throw new JoynrShutdownException(e.getMessage(), e);
        } catch (Exception e) {
            logger.debug("Exception in long poll: " + asciiString, e);
            delay();
            return;
        }

        switch (statusCode) {
        case HttpStatus.SC_OK:
            notifyDispatcher(responseBody);
            break;
        case HttpStatus.SC_NOT_FOUND:
            logger.error(responseBody);
            delay();
            throw new JoynrChannelMissingException("Not found");

        case HttpStatus.SC_BAD_REQUEST:
            if (responseBody != null) {
                try {
                    JoynrMessagingError error = objectMapper.readValue(responseBody, JoynrMessagingError.class);
                    JoynrMessagingErrorCode joynrMessagingErrorCode = JoynrMessagingErrorCode.getJoynrMessagingErrorCode(error.getCode());
                    logger.error(error.toString());
                    switch (joynrMessagingErrorCode) {
                    case JOYNRMESSAGINGERROR_CHANNELNOTFOUND:
                        throw new JoynrChannelMissingException(error.getReason());
                    default:
                        throw new JoynrCommunicationException(error.getReason());
                    }
                } catch (IOException e) {
                    throw new JoynrCommunicationException(statusText, e);
                }
            }

        default:
            delay();
            break;
        }

    }

    private void notifyDispatcher(String responseBody) {
        if (responseBody == null || responseBody.length() <= 0) {
            return;
        }

        // the response body could contain multiple SMRF messages
        List<ImmutableMessage> listOfMessages;
        try {
            listOfMessages = Utilities.splitSMRF(responseBody.getBytes(StandardCharsets.UTF_8));
        } catch (EncodingException | UnsuppportedVersionException e) {
            logger.error("Failed to split and deserialize SMRF messages: {}", e.getMessage());
            return;
        }

        logger.info("LongPollingChannel CHANNEL: {} messages received:", listOfMessages.size());

        for (final ImmutableMessage message : listOfMessages) {
            messageReceiverExecutor.execute(new Runnable() {

                @Override
                public void run() {
                    logger.info("ARRIVED {} messageId: {} type: {} from: {} to: {} header: {}",
                                new Object[]{ httpget.getURI().toString(), message.getId(), message.getType(),
                                        message.getSender(), message.getRecipient(), message.getHeaders().toString() });
                    logger.debug("\r\n<<<<<<<<<<<<<<<<<\r\n:{}", message);
                    messageArrivedListener.messageArrived(message);
                }
            });
        }
    }

    public void shutdown() {
        if (httpget != null && httpget.getURI() != null) {
            logger.info("SHUTTING down long poll for {}", httpget.getURI().toASCIIString());
        }
        shutdown = true;

        if (httpclient != null) {
            try {
                httpclient.close();
            } catch (IOException e) {
                logger.error("error closing http client", e);
            }
        }
        logger.debug("LongPollingChannel CHANNEL: {} SHUT DOWN", id);
    }

    public void resume() {
        try {
            statusLock.lockInterruptibly();
            httpget.reset();
            setLongPollingDisabled(false);
            statusChanged.signal();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return;
        } finally {
            statusLock.unlock();
        }
    }

    public void suspend() {
        try {

            statusLock.lockInterruptibly();
            setLongPollingDisabled(true);
            httpget.abort();
        } catch (InterruptedException e) {
            return;
        } finally {
            statusLock.unlock();
        }

    }

    public boolean isLongPollingDisabled() {
        return longPollingDisabled;
    }

    private void setLongPollingDisabled(boolean longPollingDisabled) {

        this.longPollingDisabled = longPollingDisabled;

    }

    public void setChannelUrl(String channelUrl) {
        this.httpget = httpRequestFactory.createHttpGet(URI.create(channelUrl));
        Builder requestConfigBuilder = RequestConfig.copy(defaultRequestConfig);
        httpget.setConfig(requestConfigBuilder.build());
        httpget.setHeader(httpConstants.getHEADER_X_ATMOSPHERE_TRACKING_ID(), receiverId);
        if (channelUrl.length() > 15) {
            this.id = "..." + channelUrl.substring(channelUrl.length() - 15);
        } else {
            this.id = channelUrl;
        }

    }

    private void delay() {
        try {
            Thread.sleep(settings.getLongPollRetryIntervalMs());
        } catch (InterruptedException e) {
            // exit
        }
    }
}

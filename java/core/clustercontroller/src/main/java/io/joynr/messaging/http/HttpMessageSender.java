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
package io.joynr.messaging.http;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URI;
import java.util.Optional;

import javax.inject.Inject;

import org.apache.http.HttpEntity;
import org.apache.http.StatusLine;
import org.apache.http.client.config.RequestConfig.Builder;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.message.BasicHeader;
import org.apache.http.protocol.BasicHttpContext;
import org.apache.http.protocol.HttpContext;
import org.apache.http.util.EntityUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.exceptions.JoynrChannelMissingException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.datatypes.JoynrMessagingError;
import io.joynr.messaging.datatypes.JoynrMessagingErrorCode;
import io.joynr.messaging.http.operation.HttpConstants;
import io.joynr.messaging.http.operation.HttpPost;
import io.joynr.messaging.http.operation.HttpRequestFactory;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.system.RoutingTypes.ChannelAddress;

public class HttpMessageSender implements ShutdownListener {
    private static final Logger logger = LoggerFactory.getLogger(HttpMessageSender.class);
    private static final int DELAY_RECEIVER_NOT_STARTED_MS = 100;
    private static final String RECEIVER_NOT_STARTED_REASON = "cannot send until receiver is started";
    private final UrlResolver urlResolver;
    private final HttpRequestFactory httpRequestFactory;
    private final HttpConstants httpConstants;
    private final CloseableHttpClient httpclient;
    private final RequestConfig defaultRequestConfig;
    private final ObjectMapper objectMapper;
    private MessageReceiver messageReceiver;

    @Inject
    // CHECKSTYLE:OFF
    public HttpMessageSender(MessageReceiver messageReceiver,
                             CloseableHttpClient httpclient,
                             HttpRequestFactory httpRequestFactory,
                             HttpConstants httpConstants,
                             RequestConfig defaultRequestConfig,
                             ObjectMapper objectMapper,
                             UrlResolver urlResolver,
                             ShutdownNotifier shutdownNotifier) {
        // CHECKSTYLE:ON
        this.messageReceiver = messageReceiver;
        this.httpclient = httpclient;
        this.httpRequestFactory = httpRequestFactory;
        this.httpConstants = httpConstants;
        this.defaultRequestConfig = defaultRequestConfig;
        this.objectMapper = objectMapper;
        this.urlResolver = urlResolver;
        shutdownNotifier.registerForShutdown(this);
    }

    public void sendMessage(ChannelAddress address,
                            byte[] serializedMessage,
                            SuccessAction successAction,
                            FailureAction failureAction) {
        // check if messageReceiver is ready to receive replies otherwise delay request by at least 100 ms
        if (!messageReceiver.isReady()) {
            long delay_ms = DELAY_RECEIVER_NOT_STARTED_MS;
            failureAction.execute(new JoynrDelayMessageException(delay_ms, RECEIVER_NOT_STARTED_REASON));
        }

        Optional<String> optionalSendUrl = urlResolver.getSendUrl(address.getMessagingEndpointUrl());
        String sendUrl = optionalSendUrl.isPresent() ? optionalSendUrl.get() : null;

        logger.trace("SENDING: channelId: {} message: {}", sendUrl, serializedMessage);

        HttpContext context = new BasicHttpContext();

        // execute http command to send
        CloseableHttpResponse response = null;
        try {

            HttpPost httpPost = httpRequestFactory.createHttpPost(URI.create(sendUrl));
            httpPost.addHeader(new BasicHeader(httpConstants.getHEADER_CONTENT_TYPE(),
                                               httpConstants.getAPPLICATION_JSON() + ";charset=UTF-8"));
            httpPost.setEntity(new ByteArrayEntity(serializedMessage));

            // Clone the default config
            Builder requestConfigBuilder = RequestConfig.copy(defaultRequestConfig);
            requestConfigBuilder.setConnectionRequestTimeout(httpConstants.getSEND_MESSAGE_REQUEST_TIMEOUT());
            httpPost.setConfig(requestConfigBuilder.build());

            response = httpclient.execute(httpPost, context);

            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();
            String statusText = statusLine.getReasonPhrase();

            switch (statusCode) {
            case HttpURLConnection.HTTP_OK:
            case HttpURLConnection.HTTP_CREATED:
                logger.trace("SENT: channelId {} message: {}", sendUrl, serializedMessage);
                successAction.execute();
                break;
            case HttpURLConnection.HTTP_BAD_REQUEST:
                HttpEntity entity = response.getEntity();
                if (entity == null) {
                    failureAction.execute(new JoynrCommunicationException("Error in HttpMessageSender. No further reason found in message body"));
                    return;
                }
                String body = EntityUtils.toString(entity, "UTF-8");

                JoynrMessagingError error = objectMapper.readValue(body, JoynrMessagingError.class);
                JoynrMessagingErrorCode joynrMessagingErrorCode = JoynrMessagingErrorCode.getJoynrMessagingErrorCode(error.getCode());
                logger.error(error.toString());
                switch (joynrMessagingErrorCode) {
                case JOYNRMESSAGINGERROR_CHANNELNOTFOUND:
                    failureAction.execute(new JoynrChannelMissingException("Channel does not exist. Status: "
                            + statusCode + " error: " + error.getCode() + "reason:" + error.getReason()));
                    break;
                default:
                    failureAction.execute(new JoynrCommunicationException("Error in HttpMessageSender: " + statusText
                            + body + " error: " + error.getCode() + "reason:" + error.getReason()));
                    break;
                }
                break;
            default:
                failureAction.execute(new JoynrCommunicationException("Unknown Error in HttpMessageSender: "
                        + statusText + " statusCode: " + statusCode));
                break;
            }
        } catch (JoynrShutdownException e) {
            failureAction.execute(new JoynrMessageNotSentException("Message not sent to: " + address, e));
        } catch (Exception e) {
            // An exception occured - this could still be a communication error (e.g Connection refused)
            failureAction.execute(new JoynrCommunicationException(e.getClass().getName()
                    + "Exception while communicating. error: " + e.getMessage()));
        } finally {
            if (response != null) {
                try {
                    response.close();
                } catch (IOException e) {
                }
            }
        }
    }

    @Override
    public void shutdown() {
        try {
            httpclient.close();
        } catch (IOException e) {
            logger.error("Error closing HTTP client: {}", e.getMessage());
        }
    }
}

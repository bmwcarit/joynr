package io.joynr.messaging.http;

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

import io.joynr.exceptions.JoynrChannelMissingException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.messaging.MessageContainer;
import io.joynr.messaging.datatypes.JoynrMessagingError;
import io.joynr.messaging.datatypes.JoynrMessagingErrorCode;
import io.joynr.messaging.http.operation.FailureAction;
import io.joynr.messaging.http.operation.HttpConstants;
import io.joynr.messaging.http.operation.HttpPost;
import io.joynr.messaging.http.operation.HttpRequestFactory;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URI;

import javax.inject.Inject;

import org.apache.http.HttpEntity;
import org.apache.http.StatusLine;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.config.RequestConfig.Builder;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.message.BasicHeader;
import org.apache.http.protocol.BasicHttpContext;
import org.apache.http.protocol.HttpContext;
import org.apache.http.util.EntityUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;

public class HttpMessageSender {
    private static final Logger logger = LoggerFactory.getLogger(HttpMessageSender.class);
    private final UrlResolver urlResolver;
    private final HttpRequestFactory httpRequestFactory;
    private final HttpConstants httpConstants;
    private final CloseableHttpClient httpclient;
    private final RequestConfig defaultRequestConfig;
    private final ObjectMapper objectMapper;

    @Inject
    public HttpMessageSender(CloseableHttpClient httpclient,
                             HttpRequestFactory httpRequestFactory,
                             HttpConstants httpConstants,
                             RequestConfig defaultRequestConfig,
                             ObjectMapper objectMapper,
                             UrlResolver urlResolver) {
        this.httpclient = httpclient;
        this.httpRequestFactory = httpRequestFactory;
        this.httpConstants = httpConstants;
        this.defaultRequestConfig = defaultRequestConfig;
        this.objectMapper = objectMapper;
        this.urlResolver = urlResolver;
    }

    public void sendMessage(final MessageContainer messageContainer, final FailureAction failureAction) {
        logger.trace("SEND messageId: {} channelId: {}",
                     messageContainer.getMessageId(),
                     messageContainer.getChannelId());

        HttpContext context = new BasicHttpContext();

        String channelId = messageContainer.getChannelId();
        String messageId = messageContainer.getMessageId();

        if (messageContainer.isExpired()) {
            logger.error("SEND executionQueue.run channelId: {}, messageId: {} TTL expired: ",
                         messageId,
                         messageContainer.getExpiryDate());
            failureAction.execute(new JoynrTimeoutException(messageContainer.getExpiryDate()));
            return;
        }

        // execute http command to send
        CloseableHttpResponse response = null;
        try {

            String serializedMessage = messageContainer.getSerializedMessage();
            final String sendUrl = urlResolver.getSendUrl(messageContainer.getChannelId());
            logger.debug("SENDING message channelId: {}, messageId: {} toUrl: {}", new String[]{ channelId, messageId,
                    sendUrl });
            if (sendUrl == null) {
                logger.error("SEND executionQueue.run channelId: {}, messageId: {} No channelId found",
                             messageId,
                             messageContainer.getExpiryDate());
                failureAction.execute(new JoynrMessageNotSentException("no channelId found"));
                return;
            }

            HttpPost httpPost = httpRequestFactory.createHttpPost(URI.create(sendUrl));
            httpPost.addHeader(new BasicHeader(httpConstants.getHEADER_CONTENT_TYPE(),
                                               httpConstants.getAPPLICATION_JSON() + ";charset=UTF-8"));
            httpPost.setEntity(new StringEntity(serializedMessage, "UTF-8"));

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
                logger.debug("SEND to ChannelId: {} messageId: {} completed successfully", channelId, messageId);
                break;
            case HttpURLConnection.HTTP_BAD_REQUEST:
                HttpEntity entity = response.getEntity();
                if (entity == null) {
                    logger.error("SEND to ChannelId: {} messageId: {} completed in error. No further reason found in message body",
                                 channelId,
                                 messageId);
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
                    logger.error("SEND error channelId: {}, messageId: {} error: {} code: {} reason: {} ",
                                 new Object[]{ channelId, messageId, statusText, error.getCode(), error.getReason() });
                    failureAction.execute(new JoynrCommunicationException("Http Error while communicating: "
                            + statusText + body + " error: " + error.getCode() + "reason:" + error.getReason()));
                    break;
                }
                break;
            default:
                logger.error("SEND to ChannelId: {} messageId: {} - unexpected response code: {} reason: {}",
                             new Object[]{ channelId, messageId, statusCode, statusText });
                break;
            }
        } catch (Exception e) {
            // An exception occured - this could still be a communication error (e.g Connection refused)
            logger.error("SEND error channelId: {}, messageId: {} error: {}", new Object[]{ channelId, messageId,
                    e.getMessage() });
            failureAction.execute(new JoynrCommunicationException("Exception while communicating error: "
                    + e.getMessage()));
        } finally {
            if (response != null) {
                try {
                    response.close();
                } catch (IOException e) {
                }
            }
        }
    }
}

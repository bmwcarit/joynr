/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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
package io.joynr.messaging.bounceproxy.controller;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URI;

import org.apache.http.StatusLine;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.CloseableHttpClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.bounceproxy.controller.exception.JoynrProtocolException;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.service.ChannelServiceConstants;

/**
 * Facade for a remote bounce proxy. This component encapsulates communication
 * with services of a bounce proxy.
 *
 * @author christina.strobel
 *
 */
public class RemoteBounceProxyFacade {

    private static final Logger logger = LoggerFactory.getLogger(RemoteBounceProxyFacade.class);

    private CloseableHttpClient httpclient;

    private long sendCreateChannelRetryIntervalMs;

    private int sendCreateChannelMaxRetries;

    @Inject
    public RemoteBounceProxyFacade(CloseableHttpClient httpclient,
                                   @Named(BounceProxyControllerPropertyKeys.PROPERTY_BPC_SEND_CREATE_CHANNEL_RETRY_INTERVAL_MS) long sendCreateChannelRetryIntervalMs,
                                   @Named(BounceProxyControllerPropertyKeys.PROPERTY_BPC_SEND_CREATE_CHANNEL_MAX_RETRY_COUNT) int sendCreateChannelMaxRetries) {
        this.httpclient = httpclient;
        this.sendCreateChannelRetryIntervalMs = sendCreateChannelRetryIntervalMs;
        this.sendCreateChannelMaxRetries = sendCreateChannelMaxRetries;
    }

    /**
     * Creates a channel on the remote bounce proxy.
     *
     * @param bpInfo information for a bounce proxy, including the cluster the
     * bounce proxy is running on
     * @param ccid the channel id
     * @param trackingId the tracking id
     * @return URI representing the channel
     * @throws JoynrProtocolException
     *             if the bounce proxy rejects channel creation
     */
    public URI createChannel(ControlledBounceProxyInformation bpInfo,
                             String ccid,
                             String trackingId) throws JoynrProtocolException {

        try {
            // Try to create a channel on a bounce proxy with maximum number of
            // retries.
            return createChannelLoop(bpInfo, ccid, trackingId, sendCreateChannelMaxRetries);
        } catch (JoynrProtocolException e) {
            logger.error("Unexpected bounce proxy behaviour: message: {}", e.getMessage());
            throw e;
        } catch (JoynrRuntimeException e) {
            logger.error("Channel creation on bounce proxy failed: message: {}", e.getMessage());
            throw e;
        } catch (Exception e) {
            logger.error("Uncaught exception in channel creation: message: {}", e.getMessage());
            throw new JoynrRuntimeException("Unknown exception when creating channel '" + ccid + "' on bounce proxy '"
                    + bpInfo.getId() + "'", e);
        }

    }

    /**
     * Starts a loop to send createChannel requests to a bounce proxy with a
     * maximum number of retries.
     *
     * @param bpInfo
     *            information about the bounce proxy to create the channel at
     * @param ccid
     *            the channel ID for the channel to create
     * @param trackingId
     *            a tracking ID necessary for long polling
     * @param retries
     *            the maximum number of retries
     * @return the url of the channel at the bounce proxy as it was returned by
     *         the bounce proxy itself
     * @throws JoynrProtocolException
     *             if the bounce proxy rejects channel creation
     */
    private URI createChannelLoop(ControlledBounceProxyInformation bpInfo,
                                  String ccid,
                                  String trackingId,
                                  int retries) throws JoynrProtocolException {

        while (retries > 0) {
            retries--;
            try {
                return sendCreateChannelHttpRequest(bpInfo, ccid, trackingId);
            } catch (IOException e) {
                // failed to establish communication with the bounce proxy
                logger.error("Creating a channel on bounce proxy {} failed due to communication errors: message: {}",
                             bpInfo.getId(),
                             e.getMessage());
            }
            // try again if creating the channel failed
            try {
                Thread.sleep(sendCreateChannelRetryIntervalMs);
            } catch (InterruptedException e) {
                throw new JoynrRuntimeException("creating a channel on bounce proxy " + bpInfo.getId()
                        + " was interrupted.");
            }
        }

        // the maximum number of retries passed, so channel creation failed
        throw new JoynrRuntimeException("creating a channel on bounce proxy " + bpInfo.getId() + " failed.");
    }

    private URI sendCreateChannelHttpRequest(ControlledBounceProxyInformation bpInfo,
                                             String ccid,
                                             String trackingId) throws IOException, JoynrProtocolException {

        // TODO jsessionid handling
        final String url = bpInfo.getLocationForBpc().toString() + "channels/?ccid=" + ccid;

        logger.debug("Using bounce proxy channel service URL: {}", url);

        HttpPost postCreateChannel = new HttpPost(url.trim());
        postCreateChannel.addHeader(ChannelServiceConstants.X_ATMOSPHERE_TRACKING_ID, trackingId);

        CloseableHttpResponse response = null;
        try {
            response = httpclient.execute(postCreateChannel);
            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();

            if (statusCode == HttpURLConnection.HTTP_CREATED) {
                // the channel was created successfully

                // check if bounce proxy ID header was sent correctly
                if (response.containsHeader("bp")) {
                    String bounceProxyId = response.getFirstHeader("bp").getValue();
                    if (bounceProxyId == null || !bounceProxyId.equals(bpInfo.getId())) {
                        throw new JoynrProtocolException("Bounce proxy ID '" + bounceProxyId
                                + "' returned by bounce proxy '" + bpInfo.getId() + "' does not match.");
                    }
                } else {
                    throw new JoynrProtocolException("No bounce proxy ID returned by bounce proxy '" + bpInfo.getId()
                            + "'");
                }

                // get URI of newly created channel
                if (!response.containsHeader("Location")) {
                    throw new JoynrProtocolException("No channel location returned by bounce proxy '" + bpInfo.getId()
                            + "'");
                }

                String locationValue = response.getFirstHeader("Location").getValue();

                if (locationValue == null || locationValue.isEmpty()) {
                    throw new JoynrProtocolException("Bounce proxy '" + bpInfo.getId()
                            + "' didn't return a channel location.");
                }
                try {
                    URI channelLocation = new URI(locationValue);

                    logger.info("Successfully created channel '{}' on bounce proxy '{}'", ccid, bpInfo.getId());
                    return channelLocation;
                } catch (Exception e) {
                    throw new JoynrProtocolException("Cannot parse channel location '" + locationValue
                            + "' returned by bounce proxy '" + bpInfo.getId() + "'", e);
                }
            }

            // the bounce proxy is not excepted to reject this call as it was
            // chosen based on performance measurements sent by it
            logger.error("Failed to create channel on bounce proxy '{}'. Response: {}", bpInfo.getId(), response);
            throw new JoynrProtocolException("Bounce Proxy " + bpInfo.getId() + " rejected channel creation (Response: "
                    + response + ")");

        } finally {
            if (response != null) {
                response.close();
            }
        }
    }
}

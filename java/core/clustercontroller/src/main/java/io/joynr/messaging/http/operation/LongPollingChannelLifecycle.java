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
import java.net.HttpURLConnection;
import java.net.URI;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadFactory;

import org.apache.http.Header;
import org.apache.http.StatusLine;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.util.EntityUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrChannelMissingException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingSettings;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.util.JoynrThreadFactory;

/**
 * The channel lifecycle callable is started in a new thread and loops infinitely until an exception is thrown:
 * <ul>
 * <li>creates the channel (in a looop, until it is created or maxRetries is exceeded
 * <li>longPolls (in a loop, until an exception is thrown or the system is being shutdown, ie started==false)
 * </ul>
 */
@Singleton
public class LongPollingChannelLifecycle {

    private static final Logger logger = LoggerFactory.getLogger(LongPollingChannelLifecycle.class);
    private String channelUrl = null;

    private final String channelId;

    @Inject
    private MessagingSettings settings;

    ThreadFactory namedThreadFactory = new JoynrThreadFactory("joynr.LongPoll");
    private ExecutorService channelMonitorExecutorService = Executors.newFixedThreadPool(1, namedThreadFactory);
    private LongPollChannel longPolling;
    private final ObjectMapper objectMapper;
    private boolean longPollingDisabled;
    private boolean started = false;
    private boolean channelCreated = false;
    private HttpConstants httpConstants;
    private String receiverId;
    private CloseableHttpClient httpclient;
    private RequestConfig defaultRequestConfig;
    private Future<Void> longPollingFuture;
    private HttpRequestFactory httpRequestFactory;

    @Inject
    public LongPollingChannelLifecycle(CloseableHttpClient httpclient,
                                       RequestConfig defaultRequestConfig,
                                       @Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                       @Named(MessagingPropertyKeys.RECEIVERID) String receiverId,
                                       ObjectMapper objectMapper,
                                       HttpConstants httpConstants,
                                       HttpRequestFactory httpRequestFactory) {
        this.httpclient = httpclient;
        this.defaultRequestConfig = defaultRequestConfig;
        this.channelId = channelId;
        this.receiverId = receiverId;
        this.objectMapper = objectMapper;
        this.httpConstants = httpConstants;
        this.httpRequestFactory = httpRequestFactory;
    }

    public synchronized void startLongPolling(final MessageArrivedListener messageArrivedListener,
                                              final ReceiverStatusListener... receiverStatusListeners) {
        if (channelMonitorExecutorService == null) {
            throw new JoynrShutdownException("Channel Monitor already shutdown");
        }

        if (started == true) {
            throw new IllegalStateException("only one long polling thread can be started per ChannelMonitor");
        }

        started = true;

        Callable<Void> channelLifecycleCallable = new Callable<Void>() {
            @Override
            public Void call() {
                try {
                    checkServerTime();
                    final int maxRetries = settings.getMaxRetriesCount();

                    while (true) {
                        channelCreated = false;
                        // Create the channel with maximal "maxRetries" retries
                        createChannelLoop(maxRetries);

                        // signal that the channel has been created
                        for (ReceiverStatusListener statusListener : receiverStatusListeners) {
                            statusListener.receiverStarted();
                        }

                        // If it was not possible to create the channel exit
                        if (!isChannelCreated()) {
                            String message = "registerMessageReceiver channelId: " + channelId
                                    + " error occured. Exiting.";
                            logger.error(message);
                            // signal that the channel is in exception
                            for (ReceiverStatusListener statusListener : receiverStatusListeners) {
                                statusListener.receiverException(new JoynrShutdownException(message));
                            }
                        }
                        // Start LONG POLL lifecycle. The future will only
                        // return when the long poll loop has ended,
                        // otherwise will terminate with a JoynrShutdownException
                        longPollLoop(messageArrivedListener, maxRetries);
                    }
                } finally {
                    started = false;
                }
            }
        };
        longPollingFuture = channelMonitorExecutorService.submit(channelLifecycleCallable);

    }

    public void checkServerTime() {

        CloseableHttpResponse response = null;
        try {
            String url = settings.getBounceProxyUrl().buildTimeCheckUrl();
            long localTime;
            long serverTime = 0L;
            long localTimeBeforeRequest = System.currentTimeMillis();

            HttpGet getTime = httpRequestFactory.createHttpGet(URI.create(url));
            getTime.setConfig(defaultRequestConfig);
            response = httpclient.execute(getTime);

            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();
            if (statusCode != 200) {
                logger.error("CheckServerTime: Bounce Proxy not reached: " + url + " Status " + statusCode + " Reason "
                        + statusLine.getReasonPhrase());
                return;
            }

            long localTimeAfterRequest = System.currentTimeMillis();
            localTime = (localTimeBeforeRequest + localTimeAfterRequest) / 2;
            try {
                serverTime = Long.parseLong(EntityUtils.toString(response.getEntity(), "UTF-8"));
            } catch (Exception e) {
                logger.error("CheckServerTime: could not parse server time: {}", e.getMessage());
                return;
            }

            long diff = Math.abs(serverTime - localTime);
            logger.debug("############ Server Time: " + serverTime + " vs. Local Time: " + localTime + " diff: "
                    + diff);
            if (Math.abs(diff) > 500) {
                logger.error("CheckServerTime: TIME DIFFERENCE TOO LARGE. PLEASE SYNC CLOCKS: diff=" + diff);
            } else {
                logger.info("CheckServerTime: time difference to server is " + diff);
            }
        } catch (Exception e) {
            logger.error("CheckServerTime: error {}", e.getMessage());
        } finally {
            if (response != null) {
                try {
                    response.close();
                } catch (IOException e) {
                    logger.error("CheckServerTime: error {}", e.getMessage());
                }
            }
        }

    }

    private int longPollLoop(final MessageArrivedListener messageArrivedListener,
                             int retries) throws JoynrShutdownException {
        while (retries > 0) {
            logger.info("LONG POLL LOOP: Start: retries: {}", retries);
            retries--;
            try {

                if (channelUrl == null) {
                    logger.error("openChannel channelId: {} channelUrl cannot be NULL", channelId);
                    throw new IllegalArgumentException("openChannel channelId: " + channelId
                            + " channelUrl cannot be NULL");
                }

                if (started == false) {
                    String errorMsg = "openChannel " + channelId + "failed: ChannelMonitor is shutdown";
                    logger.error(errorMsg);
                    throw new JoynrShutdownException(errorMsg);
                }

                // String id = getPrintableId(channelUrl);
                synchronized (this) {
                    this.longPolling = new LongPollChannel(httpclient,
                                                           defaultRequestConfig,
                                                           longPollingDisabled,
                                                           messageArrivedListener,
                                                           objectMapper,
                                                           settings,
                                                           httpConstants,
                                                           channelId,
                                                           receiverId,
                                                           httpRequestFactory);
                }
                longPolling.setChannelUrl(channelUrl);

                longPolling.longPollLoop();

                return retries;

                // A thrown ShutdownException is not caught here. Should be caught in MessageReceiver
            } catch (JoynrChannelMissingException e) {
                // JoynChannelMissingException means the channel needs to be recreated, so exit
                // the long poll loop and redo the whole lifecycle
                logger.error("LONG POLL LOOP: error in long poll: {}", e.getMessage());
                break;
            } catch (RejectedExecutionException e) {
                // Thrown when the thread pool cannot accept another runnable. Wait and try
                // again
                logger.error("LONG POLL LOOP: error in long poll: {}", e.getMessage());
                long waitMs = settings.getLongPollRetryIntervalMs();
                logger.info("LONG POLL LOOP: waiting for: {}", waitMs);
                try {
                    Thread.sleep(waitMs);
                } catch (InterruptedException e1) {
                    throw new JoynrShutdownException("INTERRUPT. Shutting down");
                }
            }

        }
        return retries;
    }

    private int createChannelLoop(int retries) {
        while (started && !channelCreated) {

            channelCreated = createChannel();

            if (channelCreated || retries <= 0) {
                break;
            }

            // retries
            try {
                long waitMs = settings.getLongPollRetryIntervalMs();
                Thread.sleep(waitMs);
            } catch (InterruptedException e) {
                // assume started will be set to false if shutting down
            }
            retries--;
        }
        return retries;
    }

    public synchronized void suspend() {
        this.longPollingDisabled = true;
        if (longPolling != null) {
            logger.debug("Suspending longPollingCallable.");
            longPolling.suspend();
        } else {
            logger.debug("Called suspend before longPollingCallable was created.");
        }
    }

    public synchronized void resume() {
        this.longPollingDisabled = false;
        if (longPolling != null) {
            longPolling.resume();
        }
    }

    public void shutdown() {
        started = false;
        logger.debug("ChannelMonitor channel: {} SHUTDOWN...", channelId);

        if (channelMonitorExecutorService != null) {
            logger.debug("ChannelMonitor channel: {} SHUTDOWN channelMonitorExecutorService", channelId);
            channelMonitorExecutorService.shutdownNow();
            channelMonitorExecutorService = null;
        }

        if (longPolling != null) {
            longPolling.shutdown();
        }

        if (longPollingFuture != null) {
            longPollingFuture.cancel(true);
        }

    }

    /**
     * Create a new channel for the given cluster controller id. If a channel already exists, it is returned instead.
     *
     * @return whether channel was created
     */
    private synchronized boolean createChannel() {

        final String url = settings.getBounceProxyUrl().buildCreateChannelUrl(channelId);

        HttpPost postCreateChannel = httpRequestFactory.createHttpPost(URI.create(url.trim()));
        postCreateChannel.setConfig(defaultRequestConfig);
        postCreateChannel.addHeader(httpConstants.getHEADER_X_ATMOSPHERE_TRACKING_ID(), receiverId);
        postCreateChannel.addHeader(httpConstants.getHEADER_CONTENT_TYPE(), httpConstants.getAPPLICATION_JSON());

        channelUrl = null;

        CloseableHttpResponse response = null;
        boolean created = false;
        try {
            response = httpclient.execute(postCreateChannel);
            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();
            String reasonPhrase = statusLine.getReasonPhrase();

            switch (statusCode) {
            case HttpURLConnection.HTTP_OK:
            case HttpURLConnection.HTTP_CREATED:
                try {
                    Header locationHeader = response.getFirstHeader(httpConstants.getHEADER_LOCATION());
                    channelUrl = locationHeader != null ? locationHeader.getValue() : null;
                } catch (Exception e) {
                    throw new JoynrChannelMissingException("channel url was null");
                }
                break;

            default:
                logger.error("createChannel channelId: {} failed. status: {} reason: {}",
                             channelId,
                             statusCode,
                             reasonPhrase);
                throw new JoynrChannelMissingException("channel url was null");
            }

            created = true;
            logger.info("createChannel channelId: {} returned channelUrl {}", channelId, channelUrl);

        } catch (ClientProtocolException e) {
            logger.error("createChannel ERROR reason: {}", e.getMessage());
        } catch (IOException e) {
            logger.error("createChannel ERROR reason: {}", e.getMessage());
        } finally {
            if (response != null) {
                try {
                    response.close();
                } catch (IOException e) {
                    logger.error("createChannel ERROR reason: {}", e.getMessage());
                }
            }
        }

        return created;
    }

    synchronized boolean deleteChannel(int retries) {

        // already deleted
        if (channelUrl == null) {
            return true;
        }

        if (retries < 0) {
            logger.info("delete channel: {} retries expired ", channelUrl);
            return false;
        }

        logger.info("trying to delete channel: " + channelUrl);

        try {
            while (retries > 0) {
                CloseableHttpResponse response = null;
                try {
                    // TODO JOYN-1079 need to unregister deleted channel
                    // channelUrlClient.unregisterChannelUrls(channelId);
                    HttpDelete httpDelete = httpRequestFactory.createHttpDelete(URI.create(channelUrl));
                    response = httpclient.execute(httpDelete);
                    int statusCode = response.getStatusLine().getStatusCode();
                    if (statusCode == HttpURLConnection.HTTP_OK || statusCode == HttpURLConnection.HTTP_NO_CONTENT) {
                        logger.debug("DELETE CHANNEL: {} completed successfully. status:{}", channelUrl, statusCode);
                        channelUrl = null;
                        return true;
                    }

                } catch (IllegalStateException e) {
                    logger.error("DELETE: CHANNEL " + channelUrl + "failed. Cannot retry: ", e.getMessage());
                    return false;
                } catch (Exception e) {
                    logger.error("DELETE: CHANNEL " + channelUrl + "failed retries: " + retries, e.getMessage());
                } finally {
                    if (response != null) {
                        try {
                            response.close();
                        } catch (IOException e) {
                        }
                    }
                }

                if (retries > 1) {
                    try {
                        Thread.sleep(settings.getLongPollRetryIntervalMs());
                    } catch (InterruptedException e) {
                        return false;
                    }
                }
                retries--;
            }
        } catch (IllegalArgumentException e) {
            logger.error("DELETE: CHANNEL {} attempted on channelUrl:\"{}\" which is not a valid channelUrl",
                         channelId,
                         channelUrl);
        }
        return false;
    }

    public String getChannelUrl() {
        return channelUrl;
    }

    public String getChannelId() {
        return channelId;
    }

    /**
     * Started is set as soon as startLongPolling is called, irrespective of whether the channel is created
     * @return started
     */
    public boolean isStarted() {
        return started;
    }

    /**
     * true if the channel has been created on the bounceproxy
     *
     * @return boolean value, true if the channel has been created on the bounceproxy
     */
    public boolean isChannelCreated() {
        return channelCreated;
    }

}

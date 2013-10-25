package io.joynr.messaging.httpoperation;

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

import io.joynr.exceptions.JoynrChannelMissingException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingSettings;

import java.io.IOException;
import java.lang.reflect.UndeclaredThrowableException;
import java.net.HttpURLConnection;
import java.util.Arrays;
import java.util.concurrent.Callable;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeoutException;

import javax.annotation.Nullable;

import joynr.types.ChannelUrlInformation;

import org.apache.http.Header;
import org.apache.http.StatusLine;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.util.EntityUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Creation of a request to create a channel on the bounce proxy server.
 * 
 * 
 */
@Singleton
public class ChannelMonitor {

    private static final Logger logger = LoggerFactory.getLogger(ChannelMonitor.class);
    protected static final long WAITTIME_MS = 10000;
    private String channelUrl = null;
    private Future<Void> longPollingLoopFuture;

    private final String channelId;

    @Inject
    private MessagingSettings settings;
    @Inject
    private LocalChannelUrlDirectoryClient channelUrlClient;

    ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("ChannelMonitor-%d").build();
    private ExecutorService channelMonitorExecutorService = Executors.newFixedThreadPool(2, namedThreadFactory);
    private LongPollingCallable longPollingCallable;
    private final ObjectMapper objectMapper;
    private boolean longPollingDisabled;
    private boolean started = false;
    private boolean channelCreated = false;
    private HttpConstants httpConstants;
    private String receiverId;
    private CloseableHttpClient httpclient;
    private RequestConfig defaultRequestConfig;

    @Inject
    @Nullable
    public ChannelMonitor(CloseableHttpClient httpclient,
                          RequestConfig defaultRequestConfig,
                          @Named(MessagingPropertyKeys.CHANNELID) String channelId,
                          @Named(MessagingPropertyKeys.RECEIVERID) String receiverId,
                          ObjectMapper objectMapper,
                          HttpConstants httpConstants) {
        this.httpclient = httpclient;
        this.defaultRequestConfig = defaultRequestConfig;
        this.channelId = channelId;
        this.receiverId = receiverId;
        this.objectMapper = objectMapper;
        this.httpConstants = httpConstants;
    }

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "RV_RETURN_VALUE_IGNORED_BAD_PRACTICE", justification = "We are not interested in result")
    public synchronized void startLongPolling(final MessageReceiver messageReceiver) {

        //
        if (channelMonitorExecutorService == null)
            throw new IllegalStateException("Channel Monitor already shutdown");

        if (started == true) {
            logger.error("only one long polling thread can be started per ChannelMonitor");
            return;
        }

        started = true;
        Callable<Boolean> channelMonitorCallable = new Callable<Boolean>() {
            @Override
            public Boolean call() {
                try {
                    checkServerTime();
                    final int maxRetries = settings.getMaxRetriesCount();

                    while (true) {
                        channelCreated = false;
                        // Create the channel with maximal "maxRetries" retries
                        // TODO check if maxRetries is correct here or if left over retries from long poll loop should
                        // be used
                        createChannelLoop(messageReceiver, maxRetries);

                        // If it was not possible to create the channel exit
                        if (!isChannelCreated()) {
                            logger.error("registerMessageReceiver channelId: " + channelId + " error occured. Exiting.");
                            return false;
                        }
                        // Start LONG POLL lifecycle. The future will only
                        // return when the long poll loop has ended
                        logger.debug("starting longpool withing callable");
                        longPollLoop(messageReceiver, maxRetries);
                        logger.debug("ending longpool withing callable");
                    }

                    // These exceptions are outside the while loop because
                    // they signal that we should shut down
                } catch (JoynrShutdownException e) {
                    return true;
                } catch (InterruptedException e) {
                    return true;
                } catch (CancellationException e) {
                    return true;
                } catch (Exception e) {
                    logger.error("Uncaught exception in startLongPoll. Shuting down long poll", e);
                    return false;
                } finally {
                    started = false;
                }
            }
        };
        channelMonitorExecutorService.submit(channelMonitorCallable);

    }

    public void checkServerTime() {

        CloseableHttpResponse response = null;
        try {
            String url = settings.getBounceProxyUrl().buildTimeCheckUrl();
            long localTime;
            long serverTime = 0L;
            long localTimeBeforeRequest = System.currentTimeMillis();

            HttpGet getTime = new HttpGet(url);
            response = httpclient.execute(getTime);

            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();
            if (statusCode != 200) {
                logger.error("CheckServerTime: Bounce Proxy not reached: " + statusCode + " "
                        + statusLine.getReasonPhrase());
                return;
            }

            long localTimeAfterRequest = System.currentTimeMillis();
            localTime = (localTimeBeforeRequest + localTimeAfterRequest) / 2;
            try {
                serverTime = Long.parseLong(EntityUtils.toString(response.getEntity(), "UTF-8"));
            } catch (Exception e) {
                logger.error("CheckServerTime: could not parse server time: {}", e.getMessage());
            }

            long diff = Math.abs(serverTime - localTime);
            logger.debug("############ Server Time: " + serverTime + " vs. Local Time: " + localTime + " diff: " + diff);
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
                }
            }
        }

    }

    private int longPollLoop(final MessageReceiver messageReceiver, int retries) throws InterruptedException {
        while (retries > 0) {
            logger.info("LONG POLL LOOP: Start: retries: {}", retries);
            retries--;
            try {

                longPollingLoopFuture = openChannel(getChannelUrl(), messageReceiver);
                longPollingLoopFuture.get();
                return retries;

                // Errors in the LongPollingCallable are packed in the ExecutionException
                // TODO examine which exceptions will be thrown and react
            } catch (ExecutionException e) {
                boolean wait = true;
                try {
                    throw e.getCause();
                } catch (JoynrShutdownException e1) {
                    logger.info("LONG POLL LOOP: shutting down");
                    wait = false;
                    throw e1;
                } catch (JoynrChannelMissingException e1) {
                    // JoynChannelMissingException means the channel needs to be recreated, so exit
                    // the long poll loop and redo the whole lifecycle
                    logger.error("LONG POLL LOOP: error in long poll: {}", e.getMessage());
                    break;
                } catch (RejectedExecutionException e1) {
                    // Thrown when the thread pool cannot accept another runnable. Wait and try
                    // again
                    logger.error("LONG POLL LOOP: error in long poll: {}", e1.getMessage());
                    continue;
                } catch (IllegalStateException e1) {
                    logger.error("LONG POLL LOOP: error in long poll", e1);
                } catch (Throwable e1) {
                    logger.error("LONG POLL LOOP: error in long poll", e1);
                } finally {
                    if (wait) {
                        long waitMs = settings.getLongPollRetryIntervalMs();
                        logger.info("LONG POLL LOOP: waiting for: {}", waitMs);
                        Thread.sleep(waitMs);
                    }
                }
            }

        }
        return retries;
    }

    private int createChannelLoop(final MessageReceiver messageReceiver, int retries) {
        while (retries > 0) {
            retries--;
            try {
                createChannel();
                // let the messageReceive know that we are ready for business
                // TODO: usually would not be ready until after register with UrlDirectory is finished
                synchronized (messageReceiver) {
                    channelCreated = true;
                    messageReceiver.notify();
                }
                break;
            } catch (Exception e) {
                logger.error("registerMessageReceiver channelId: {} error occured creating channel: {}",
                             channelId,
                             e.getMessage());
            }
            try {
                Thread.sleep(settings.getSendMsgRetryIntervalMs());
            } catch (InterruptedException e) {
                return 0;
            }
        }
        return retries;
    }

    public synchronized void suspend() {
        this.longPollingDisabled = true;
        if (longPollingCallable != null) {
            logger.debug("Suspending longPollingCallable.");
            longPollingCallable.suspend();
        } else {
            logger.debug("Called suspend before longPollingCallable was created.");
        }
    }

    public synchronized void resume() {
        this.longPollingDisabled = false;
        if (longPollingCallable != null) {
            longPollingCallable.resume();
        }
    }

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "IS2_INCONSISTENT_SYNC", justification = "Shutdown doesn't have to synchronize the access to channelMonitorExecutorService")
    public void shutdown() {
        started = false;
        logger.debug("ChannelMonitor channel: {} SHUTDOWN...", channelId);

        if (channelMonitorExecutorService != null) {
            logger.debug("ChannelMonitor channel: {} SHUTDOWN channelMonitorExecutorService", channelId);
            channelMonitorExecutorService.shutdownNow();
            channelMonitorExecutorService = null;
        }

        if (longPollingCallable != null) {
            longPollingCallable.shutdown();
        }
    }

    /**
     * Create a new channel for the given cluster controller id. If a channel already exists, it is returned instead.
     * 
     * @param maxRetries
     * @throws ClientProtocolException
     * @throws IOException
     * @throws TimeoutException
     * @throws ExecutionException
     */
    private synchronized void createChannel() throws ClientProtocolException, IOException {

        final String url = settings.getBounceProxyUrl().buildCreateChannelUrl(channelId);

        HttpPost postCreateChannel = new HttpPost(url.trim());
        postCreateChannel.setConfig(defaultRequestConfig);
        postCreateChannel.addHeader(httpConstants.getHEADER_CONTENT_TYPE(), httpConstants.getAPPLICATION_JSON()
                + ";charset=UTF-8");
        postCreateChannel.addHeader(httpConstants.getHEADER_X_ATMOSPHERE_TRACKING_ID(), receiverId);
        postCreateChannel.addHeader(httpConstants.getHEADER_CONTENT_TYPE(), httpConstants.getAPPLICATION_JSON());

        channelUrl = null;

        CloseableHttpResponse response = null;
        try {
            response = httpclient.execute(postCreateChannel);
            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();
            String reasonPhrase = statusLine.getReasonPhrase();

            switch (statusCode) {
            case HttpURLConnection.HTTP_CREATED:
                try {
                    Header locationHeader = response.getFirstHeader(httpConstants.getHEADER_LOCATION());
                    channelUrl = locationHeader != null ? locationHeader.getValue() : null;
                } catch (Exception e) {
                    throw new JoynrChannelMissingException("channel url was null");
                }
                break;

            default:
                logger.error("createChannel channelId: {} failed. status: {} reason: {}", new String[]{ channelId,
                        Integer.toString(statusCode), reasonPhrase });
                throw new JoynrChannelMissingException("channel url was null");
            }

            logger.info("createChannel channelId: {} returned channelUrl {}", channelId, channelUrl);

        } finally {
            if (response != null) {
                response.close();
            }
        }
    }

    /**
     * openChannel causes the long poll to reconnect. Exceptions are returned via the Future as well.
     * 
     * @param openChannelUrl
     * @param messageReceiver
     * @return
     * @throws IllegalArgumentException
     *             ,
     */
    synchronized Future<Void> openChannel(String openChannelUrl, MessageReceiver messageReceiver) {
        if (openChannelUrl == null) {
            logger.error("openChannel channelId: {} channelUrl cannot be NULL", channelId);
            return new ExceptionFuture(new IllegalArgumentException("openChannel channelId: " + channelId
                    + " channelUrl cannot be NULL"));
        }

        if (started == false) {
            String errorMsg = "openChannel " + channelId + "failed: ChannelMonitor is shutdown";
            logger.error(errorMsg);
            return new ExceptionFuture(new JoynrShutdownException(errorMsg));
        }

        // String id = getPrintableId(channelUrl);
        synchronized (this) {
            this.longPollingCallable = new LongPollingCallable(httpclient,
                                                               defaultRequestConfig,
                                                               longPollingDisabled,
                                                               messageReceiver,
                                                               objectMapper,
                                                               settings,
                                                               httpConstants,
                                                               channelId,
                                                               receiverId);
        }
        longPollingCallable.setChannelUrl(openChannelUrl);

        if (channelMonitorExecutorService == null || channelMonitorExecutorService.isShutdown()) {
            throw new JoynrShutdownException("channel monitor already shut down");
        }

        Future<Void> future = null;
        future = channelMonitorExecutorService.submit(longPollingCallable);

        // register is here because the registration response needs an open channel to be received.
        // Otherwise register fails.
        registerChannelUrl();

        return future;

    }

    private boolean registerChannelUrl() {
        if (channelUrl == null) {
            logger.error("REGISTER channelUrl, cannot be NULL");
            return false;
        }

        final ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        channelUrlInformation.setUrls(Arrays.asList(channelUrl));

        while (true) {
            try {
                channelUrlClient.registerChannelUrls(channelId, channelUrlInformation);
                return true;
            } catch (CancellationException e) {
                return false;
            } catch (IllegalStateException e) {
                logger.error("REGISTER Channel " + channelId + " failed: ", e.getMessage() != null ? e.getMessage() : e);
                return false;
            } catch (JoynrShutdownException e) {
                logger.error("REGISTER Channel " + channelId + " failed: Joyn is shutting down.");
                return false;
            } catch (Throwable e) {
                try {
                    String msg;
                    if (e.getCause() instanceof UndeclaredThrowableException) {
                        msg = ((UndeclaredThrowableException) e.getCause()).getUndeclaredThrowable()
                                                                           .getCause()
                                                                           .getMessage();
                    } else if (e.getCause() != null) {
                        msg = e.getCause().getMessage();
                    } else {
                        msg = e.toString();
                    }
                    logger.error("REGISTER channelUrl " + channelId + " failed: " + msg + " retrying in "
                            + settings.getSendMsgRetryIntervalMs() + " ms", e);

                    Thread.sleep(settings.getSendMsgRetryIntervalMs());
                } catch (InterruptedException e1) {
                    return false;
                }

            }
        }

    }

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "SWL_SLEEP_WITH_LOCK_HELD", justification = "Other synchronized methods should block while deleting a channel")
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
                    HttpDelete httpDelete = new HttpDelete(channelUrl);
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

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "IS2_INCONSISTENT_SYNC", justification = "isStarted is just a getter for the flag")
    public boolean isStarted() {
        return started;
    }

    public boolean isChannelCreated() {
        return channelCreated;
    }

}

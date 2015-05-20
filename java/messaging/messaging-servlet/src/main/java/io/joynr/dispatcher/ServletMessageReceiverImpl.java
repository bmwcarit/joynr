package io.joynr.dispatcher;

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

import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.http.operation.LongPollingMessageReceiver;

import java.util.Arrays;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;
import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.util.concurrent.Futures;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class ServletMessageReceiverImpl implements ServletMessageReceiver {
    private static final Logger logger = LoggerFactory.getLogger(ServletMessageReceiverImpl.class);

    private MessageArrivedListener messageListener;

    private final String channelId;

    private boolean started;

    private LocalChannelUrlDirectoryClient channelUrlDirectory;

    private boolean registered = false;
    private Object registeredSynchronizer = new Object();

    private LongPollingMessageReceiver longPollingReceiver;

    private String contextRoot;

    private int servletShutdownTimeout_ms;

    private String hostPath;

    private boolean skipLongPollForDeregistration = false;

    private void setRegistered(boolean registered) {
        this.registered = registered;
    }

    @Inject
    public ServletMessageReceiverImpl(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                      LocalChannelUrlDirectoryClient channelUrlDirectory,
                                      LongPollingMessageReceiver longPollingReceiver,
                                      @Named(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT) String contextRoot,
                                      @Named(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH) String hostPath,
                                      @Named(MessagingPropertyKeys.PROPERTY_SERVLET_SHUTDOWN_TIMEOUT) int servletShutdownTimeout_ms) {
        this.channelId = channelId;
        this.channelUrlDirectory = channelUrlDirectory;
        this.longPollingReceiver = longPollingReceiver;
        this.contextRoot = contextRoot;
        this.hostPath = hostPath;
        this.servletShutdownTimeout_ms = servletShutdownTimeout_ms;
        this.started = false;
    }

    @Inject(optional = true)
    public void setSkipLongPollForDeregistration(@Named(MessagingPropertyKeys.PROPERTY_SERVLET_SKIP_LONGPOLL_DEREGISTRATION) boolean skipLongPollForDeregistration) {
        this.skipLongPollForDeregistration = skipLongPollForDeregistration;
    }

    @Override
    public String getChannelId() {
        return channelId;
    }

    public void registerChannelUrl() {
        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        if (hostPath == null) {
            String message = "the system property hostPath must be set with name:port eg. http://localhost:8080";
            IllegalArgumentException illegalArgumentException = new IllegalArgumentException(message);
            logger.error(message, illegalArgumentException);
            throw illegalArgumentException;
        }

        String[] urls = { hostPath + contextRoot + "/channels/" + channelId + "/" };
        channelUrlInformation.setUrls(Arrays.asList(urls));

        synchronized (registeredSynchronizer) {
            channelUrlDirectory.registerChannelUrls(channelId, channelUrlInformation);
            setRegistered(true);
        }
    }

    @Override
    public void registerMessageListener(MessageArrivedListener registerMessageListener) {
        if (this.messageListener == registerMessageListener) {
            logger.warn("this messageListener {} is already registered", registerMessageListener);
            return;
        }
        if (this.messageListener == null && registerMessageListener != null) {
            this.messageListener = registerMessageListener;
        } else {
            throw new IllegalStateException();
        }

    }

    @Override
    public void shutdown(boolean clear) {
        ExecutorService executor = Executors.newSingleThreadExecutor();

        Callable<Void> unregisterChannelCallale = new Callable<Void>() {
            @Override
            public Void call() {
                unregisterChannel();
                return null;
            }
        };

        Future<Void> unregisterFuture = executor.submit(unregisterChannelCallale);
        try {
            unregisterFuture.get(5, TimeUnit.SECONDS);
        } catch (Exception e) {
            logger.error("Error while shutting down: ", e.getMessage());
        } finally {
            executor.shutdownNow();
        }
    }

    @Override
    public boolean deleteChannel() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isStarted() {
        return started;
    }

    @Override
    public void receive(JoynrMessage message) {
        if (message != null) {

            logger.info("\r\n########### CHANNEL: " + channelId + "\r\nARRIVED:\r\n{}", message);
            messageListener.messageArrived(message);
        } else {
            logger.warn("ServletMessageReceiver CHANNEL: {} message was null", channelId);
        }

    }

    @Override
    public void onError(JoynrMessage message, Throwable error) {
        if (messageListener == null) {
            logger.error("\r\n!!!! Dropped Message {}", message, error);
        } else {
            messageListener.error(message, error);
        }
    }

    @Override
    public void suspend() {
        // TODO Auto-generated method stub

    }

    @Override
    public void resume() {
        // TODO Auto-generated method stub

    }

    @Override
    public boolean isChannelCreated() {
        // TODO find a better way to check if the servlet is ready to receive messages
        return messageListener != null;
    }

    @Override
    public boolean switchToLongPolling() {
        if (skipLongPollForDeregistration) {
            logger.info("not using long poll for deregistration");
            return true;
        }

        try {
            // switching to longPolling before the servlet is destroyed, to be able to unregister
            longPollingReceiver.registerMessageListener(messageListener);
            Future<Void> startReceiver = longPollingReceiver.startReceiver();
            startReceiver.get(servletShutdownTimeout_ms, TimeUnit.MILLISECONDS);
            try {
                unregisterChannel();
                // catchinng the unregister separately to give the apps a chance to shutdown even if the unregister of
                // the channel did not work. ie will still return true since long polling is indeed already active here.
            } catch (Exception e) {
                logger.error("error unregistering servlet channelurl: {}", e.getMessage());
            }
        } catch (Exception e) {
            logger.error("error switching to long polling while shutting down servlet: {}", e.getMessage());
            return false;
        }
        logger.debug("switched to long polling.");
        return true;
    }

    private void unregisterChannel() {
        // TODO check if the unregisterChannelUrls works, as answers can not be received
        synchronized (registeredSynchronizer) {
            if (registered) {
                channelUrlDirectory.unregisterChannelUrls(channelId);
                setRegistered(false);
            }
        }
    }

    @Override
    public Future<Void> startReceiver(ReceiverStatusListener... statusListeners) {
        if (messageListener == null) {
            throw new IllegalStateException();
        }
        // this.messageListener must be set before calling registerChannelUrl,
        // otherwise the reply will not be able to be processed
        if (!registered) {
            registerChannelUrl();
        }

        this.started = true;

        for (ReceiverStatusListener receiverStatusListener : statusListeners) {
            receiverStatusListener.receiverStarted();
        }
        return Futures.immediateFuture(null);
    }

}

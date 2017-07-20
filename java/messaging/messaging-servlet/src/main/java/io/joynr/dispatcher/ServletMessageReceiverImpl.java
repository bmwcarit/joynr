package io.joynr.dispatcher;

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

import io.joynr.messaging.IServletMessageReceivers;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.http.operation.LongPollingMessageReceiver;

import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import joynr.ImmutableMessage;

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

    private LongPollingMessageReceiver longPollingReceiver;

    private int servletShutdownTimeout_ms;

    private boolean skipLongPollForDeregistration = false;

    @Inject
    public ServletMessageReceiverImpl(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                      LongPollingMessageReceiver longPollingReceiver,
                                      IServletMessageReceivers receivers,
                                      @Named(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT) String contextRoot,
                                      @Named(MessagingPropertyKeys.PROPERTY_SERVLET_SHUTDOWN_TIMEOUT) int servletShutdownTimeout_ms) {
        this.channelId = channelId;
        this.longPollingReceiver = longPollingReceiver;
        this.servletShutdownTimeout_ms = servletShutdownTimeout_ms;
        this.started = false;
        receivers.registerServletMessageReceiver(this, channelId);
    }

    @Inject(optional = true)
    public void setSkipLongPollForDeregistration(@Named(MessagingPropertyKeys.PROPERTY_SERVLET_SKIP_LONGPOLL_DEREGISTRATION) boolean skipLongPollForDeregistration) {
        this.skipLongPollForDeregistration = skipLongPollForDeregistration;
    }

    @Override
    public String getChannelId() {
        return channelId;
    }

    @Override
    public void shutdown(boolean clear) {
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
    public void receive(ImmutableMessage message) {
        if (message != null) {

            logger.debug("\r\n<<<<<<<< ARRIVED ON CHANNEL: " + channelId + " messageId: {}\r\n{}",
                         message.getId(),
                         message);
            messageListener.messageArrived(message);
        } else {
            logger.warn("ServletMessageReceiver CHANNEL: {} message was null", channelId);
        }

    }

    @Override
    public void onError(ImmutableMessage message, Throwable error) {
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
    public boolean isReady() {
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
            Future<Void> startReceiver = longPollingReceiver.start(messageListener);
            startReceiver.get(servletShutdownTimeout_ms, TimeUnit.MILLISECONDS);
        } catch (Exception e) {
            logger.debug("error switching to long polling while shutting down servlet.", e);
            return false;
        }
        logger.debug("switched to long polling.");
        return true;
    }

    @Override
    public Future<Void> start(MessageArrivedListener registerMessageListener, ReceiverStatusListener... statusListeners) {
        if (registerMessageListener == null) {
            throw new IllegalStateException();
        }

        this.messageListener = registerMessageListener;

        this.started = true;

        for (ReceiverStatusListener receiverStatusListener : statusListeners) {
            receiverStatusListener.receiverStarted();
        }
        return Futures.immediateFuture(null);
    }

}

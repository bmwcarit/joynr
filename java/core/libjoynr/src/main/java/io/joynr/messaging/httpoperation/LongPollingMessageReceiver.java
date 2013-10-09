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

import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingSettings;
import joynr.JoynrMessage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * The HTTP Communication Manager is responsible for opening and closing channels, managing long polls, and making HTTP
 * calls
 */
@Singleton
public class LongPollingMessageReceiver implements MessageReceiver {
    public static final String MESSAGE_RECEIVER_THREADNAME_PREFIX = "MessageReceiverThread";

    private static final Logger logger = LoggerFactory.getLogger(LongPollingMessageReceiver.class);

    protected final MessagingSettings settings;
    protected MessageArrivedListener messageListener = null;
    // private final MessageSender messageSender;
    protected final ChannelMonitor channelMonitor;

    private final String channelId;

    private boolean shutdown = false;

    private Object shutdownSynchronizer = new Object();

    private Object registerSynchronizer = new Object();

    @Inject
    public LongPollingMessageReceiver(ChannelMonitor channelMonitor,
                                      MessagingSettings settings,
                                      ObjectMapper objectMapper) {
        this.channelMonitor = channelMonitor;
        this.settings = settings;
        this.channelId = channelMonitor.getChannelId();
    }

    @Override
    public void registerMessageListener(MessageArrivedListener newMessageListener) {
        synchronized (shutdownSynchronizer) {
            if (shutdown) {
                throw new JoynrShutdownException("Cannot register Message Listener: " + messageListener
                        + ": LongPollingMessageReceiver is already shutting down");
            }
        }

        synchronized (registerSynchronizer) {
            if (this.messageListener == null) {
                this.messageListener = newMessageListener;
            } else {
                throw new IllegalStateException("MessageListener was already registered!");
            }
        }
    }

    @Override
    public synchronized void startReceiver() {
        if (!isStarted()) {
            channelMonitor.startLongPolling(this);
            while (!channelMonitor.isChannelCreated()) {
                try {
                    wait();
                    // TODO this may break our shutdown during channel creation. Use a JoynShutdown exception to signal
                    // shutdown instead
                } catch (InterruptedException e) {
                }
            }
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "IS2_INCONSISTENT_SYNC", justification = "shutdown is locked using the shutdownSynchronizer object")
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN long polling message receiver");

        synchronized (shutdownSynchronizer) {
            shutdown = true;
        }
        if (clear) {
            deleteChannel();
        }

        if (channelMonitor != null) {
            channelMonitor.shutdown();
        }

        this.messageListener = null;

    }

    @Override
    public String getChannelId() {
        return channelId;
    }

    @Override
    public boolean deleteChannel() {
        // stop the long polling before the channel is deleted
        if (channelMonitor != null) {
            return channelMonitor.deleteChannel(settings.getMaxRetriesCount());
        }
        return false;

    }

    @Override
    public boolean isStarted() {

        return channelMonitor.isStarted();
    }

    @Override
    public boolean isChannelCreated() {
        return channelMonitor.isChannelCreated();
    }

    @Override
    public void receive(JoynrMessage message) {
        if (message == null) {
            logger.info("ARRIVED on channelId: {} NULL message", channelId);
            return;
        }

        logger.info("ARRIVED on channelId: {} messageId: {} type: {} from: {} to: {} header: {}", new String[]{
                channelId, message.getId(), message.getType(),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID), message.getHeader().toString() });
        logger.debug("\r\n<<<<<<<<<<<<<<<<<\r\n:{}", message.toLogMessage());

        messageListener.messageArrived(message);

    }

    @Override
    public void suspend() {
        logger.info("Suspending channelMonitor");
        channelMonitor.suspend();

    }

    @Override
    public void resume() {
        channelMonitor.resume();

    }

    @Override
    public void onError(JoynrMessage message, Throwable error) {
        if (messageListener == null) {
            logger.error("\r\n!!!! Dropped Message {}", message, error);
        } else {
            messageListener.error(message, error);
        }
    }

}

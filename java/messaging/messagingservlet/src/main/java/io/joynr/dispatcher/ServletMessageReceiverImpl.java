package io.joynr.dispatcher;

/*
 * #%L
 * joynr::java::messaging::messagingservlet
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

import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.httpoperation.LongPollingMessageReceiver;

import java.util.Arrays;

import joynr.JoynrMessage;
import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class ServletMessageReceiverImpl implements ServletMessageReceiver {
    private static final Logger logger = LoggerFactory.getLogger(ServletMessageReceiverImpl.class);

    private MessageArrivedListener messageListener;

    private final String channelId;

    private LocalChannelUrlDirectoryClient channelUrlDirectory;

    private boolean registered = false;
    private Object registeredSynchronizer = new Object();

    private LongPollingMessageReceiver longPollingReceiver;

    private void setRegistered(boolean registered) {
        this.registered = registered;
    }

    @Inject
    public ServletMessageReceiverImpl(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                      LocalChannelUrlDirectoryClient channelUrlDirectory,
                                      LongPollingMessageReceiver longPollingReceiver) {
        this.channelId = channelId;
        this.channelUrlDirectory = channelUrlDirectory;
        this.longPollingReceiver = longPollingReceiver;

    }

    @Override
    public String getChannelId() {
        return channelId;
    }

    public void registerChannelUrl() {
        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        // TODO any better way to pass this in? It is coming from MessagingJettyLauncher. How could we inject?
        String hostPath = System.getProperty("hostPath");

        if (hostPath == null) {
            String message = "the system property hostPath must be set with name:port eg. http://localhost:8080";
            IllegalArgumentException illegalArgumentException = new IllegalArgumentException(message);
            logger.error(message, illegalArgumentException);
            throw illegalArgumentException;
        }

        String[] urls = { hostPath + "/channels/" + channelId + "/" };
        channelUrlInformation.setUrls(Arrays.asList(urls));

        synchronized (registeredSynchronizer) {
            channelUrlDirectory.registerChannelUrls(channelId, channelUrlInformation);
            setRegistered(true);
        }
    }

    @Override
    public void registerMessageListener(MessageArrivedListener registerMessageListener) {
        if (this.messageListener == null && registerMessageListener != null) {
            this.messageListener = registerMessageListener;
        } else {
            throw new IllegalStateException();
        }

    }

    @Override
    public void shutdown(boolean clear) {
        unregisterChannel();
        // if (clear)
        // messageListener = null;

    }

    @Override
    public boolean deleteChannel() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isStarted() {
        return messageListener != null;
    }

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
    public void switchToLongPolling() {
        // switching to longPolling before the servlet is destroyed, to be able to unregister
        unregisterChannel();
        longPollingReceiver.registerMessageListener(messageListener);
        longPollingReceiver.startReceiver();

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
    public void startReceiver() {
        if (messageListener == null) {
            throw new IllegalStateException();
        }
        // this.messageListener must be set before calling registerChannelUrl,
        // otherwise the reply will not be able to be processed
        if (!registered) {
            registerChannelUrl();
        }

    }

}

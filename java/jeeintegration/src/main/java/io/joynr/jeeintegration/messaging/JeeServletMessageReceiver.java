/**
 *
 */
package io.joynr.jeeintegration.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.concurrent.Callable;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import com.google.common.util.concurrent.Futures;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.httpbridge.HttpBridgeRegistryClient;
import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.ReceiverStatusListener;
import joynr.JoynrMessage;
import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static java.lang.String.format;

/**
 * Implementation of a servlet message receiver which is used to register / unregister the channel and also pass on
 * messages and errors to the message listener of the runtime.
 *
 * @author clive.jevons commissioned by MaibornWolff
 */
@Singleton
public class JeeServletMessageReceiver implements ServletMessageReceiver {

    private static final Logger LOG = LoggerFactory.getLogger(JeeServletMessageReceiver.class);

    private final String channelId;

    private final String hostPath;

    private final String contextRoot;

    private boolean registered = false;

    private final LocalChannelUrlDirectoryClient channelUrlDirectory;

    private final ExecutorService executorService;

    private MessageArrivedListener messageListener;

    private final HttpBridgeRegistryClient httpBridgeRegistryClient;

    @Inject
    public JeeServletMessageReceiver(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                     LocalChannelUrlDirectoryClient channelUrlDirectory,
                                     ExecutorService executorService,
                                     @Named(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT) String contextRoot,
                                     @Named(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH) String hostPath,
                                     HttpBridgeRegistryClient httpBridgeRegistryClient) {
        if (LOG.isDebugEnabled()) {
            LOG.debug(format("Initialising with:%n\tchannelId: %s%n\tchannelUrlDirectory: %s%n\texecutorService: %s%n\tcontextRoot: %s%n\thostPath: %s%n\thttpBridgeRegistryClient: %s",
                             channelId,
                             channelUrlDirectory,
                             executorService,
                             contextRoot,
                             hostPath,
                             httpBridgeRegistryClient));
        }
        this.channelId = channelId;
        this.hostPath = hostPath;
        this.contextRoot = contextRoot;
        this.channelUrlDirectory = channelUrlDirectory;
        this.executorService = executorService;
        this.httpBridgeRegistryClient = httpBridgeRegistryClient;
    }

    @Override
    public String getChannelId() {
        return channelId;
    }

    @Override
    public void shutdown(boolean clear) {
        LOG.info("Shutting down servlet message receiver.");
        if (registered) {
            try {
                executorService.submit(new Callable<Void>() {
                    @Override
                    public Void call() throws Exception {
                        channelUrlDirectory.unregisterChannelUrls(channelId);
                        registered = false;
                        return null;
                    }
                }).get(5, TimeUnit.SECONDS);
            } catch (JoynrRuntimeException | InterruptedException | ExecutionException | TimeoutException e) {
                LOG.error(format("Problem unregistering channel %s.", channelId));
            }
        }
    }

    @Override
    public boolean isReady() {
        return messageListener != null;
    }

    @Override
    public boolean deleteChannel() {
        LOG.info(format("Deleting channel %s", channelId));
        return false;
    }

    @Override
    public boolean isStarted() {
        if (LOG.isDebugEnabled()) {
            LOG.debug(format("isStarted called - returning %s", registered));
        }
        return registered;
    }

    @Override
    public void suspend() {
        LOG.info(format("Suspend called on channel %s, but functionality not supported. Ignoring.", channelId));
    }

    @Override
    public void resume() {
        LOG.info(format("Resume called on channel %s, but functionality not supported. Ignoring.", channelId));
    }

    @Override
    public Future<Void> start(MessageArrivedListener messageArrivedListener,
                              ReceiverStatusListener... receiverStatusListeners) {
        if (messageArrivedListener == null) {
            throw new IllegalStateException();
        }
        LOG.info("Starting JeeServletMessageReceiver with listener {}", messageArrivedListener);
        this.messageListener = messageArrivedListener;
        if (!registered) {
            registerChannelUrl();
        }
        return Futures.immediateFuture(null);
    }

    @Override
    public boolean switchToLongPolling() {
        LOG.info(format("JEE servlet message receiver does not support long polling. Ignoring."));
        return false;
    }

    @Override
    public void receive(JoynrMessage message) {
        if (message != null) {
            if (LOG.isDebugEnabled()) {
                LOG.debug(format(">>>>>> Message arrived on channel %s:%n\t%s%n", channelId, message));
            }
            messageListener.messageArrived(message);
        } else {
            LOG.warn(format("Received null message on channel %s", channelId));
        }
    }

    @Override
    public void onError(JoynrMessage message, Throwable error) {
        if (messageListener != null) {
            messageListener.error(message, error);
        } else {
            LOG.warn(format("Channel %s dropped message %s because no message listener available.%nError received: %s",
                            channelId,
                            message,
                            error));
        }
    }

    private synchronized void registerChannelUrl() {
        if (!registered) {
            ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
            if (hostPath == null) {
                String message = "The system property hostPath must be set with name:port eg. http://localhost:8080";
                IllegalArgumentException illegalArgumentException = new IllegalArgumentException(message);
                LOG.error(message, illegalArgumentException);
                throw illegalArgumentException;
            }
            String endpointUrl = hostPath + contextRoot + "/channels/" + channelId + "/";
            CompletionStage<Void> registrationResult = httpBridgeRegistryClient.register(endpointUrl, channelId);
            registrationResult.thenAccept((v) -> {
                channelUrlInformation.setUrls(new String[]{ endpointUrl });
                channelUrlDirectory.registerChannelUrls(channelId, channelUrlInformation);
                registered = true;
            }).exceptionally((t) -> {
                LOG.error("Unable to register channel URL.", t);
                return null;
            });
        }
    }
}

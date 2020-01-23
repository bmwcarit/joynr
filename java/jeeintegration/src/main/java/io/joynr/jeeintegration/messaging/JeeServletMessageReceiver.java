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
package io.joynr.jeeintegration.messaging;

import static io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys.JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY;
import static java.lang.String.format;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.Future;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.jeeintegration.httpbridge.HttpBridgeRegistryClient;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.ReceiverStatusListener;
import joynr.ImmutableMessage;

/**
 * Implementation of a servlet message receiver which is used to register / unregister the channel and also pass on
 * messages and errors to the message listener of the runtime.
 */
@Singleton
public class JeeServletMessageReceiver implements ServletMessageReceiver {

    private static final Logger LOG = LoggerFactory.getLogger(JeeServletMessageReceiver.class);

    private final String channelId;

    private final String hostPath;

    private final String contextRoot;

    private boolean registered = false;

    private MessageArrivedListener messageListener;

    private final HttpBridgeRegistryClient httpBridgeRegistryClient;

    private boolean httpBridgeEnabled;

    @Inject
    public JeeServletMessageReceiver(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                     @Named(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT) String contextRoot,
                                     @Named(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH) String hostPath,
                                     HttpBridgeRegistryClient httpBridgeRegistryClient,
                                     @Named(JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY) String httpBridgeEnabled) {
        if (LOG.isDebugEnabled()) {
            LOG.debug(format("Initialising with:%n\tchannelId: %s%n\tcontextRoot: %s%n\thostPath: %s%n\thttpBridgeRegistryClient: %s",
                             channelId,
                             contextRoot,
                             hostPath,
                             httpBridgeRegistryClient));
        }
        this.channelId = channelId;
        this.hostPath = hostPath;
        this.contextRoot = contextRoot;
        this.httpBridgeRegistryClient = httpBridgeRegistryClient;
        this.httpBridgeEnabled = Boolean.valueOf(httpBridgeEnabled);
    }

    @Override
    public String getChannelId() {
        return channelId;
    }

    @Override
    public void shutdown(boolean clear) {
        if (registered) {
            LOG.info("Shutting down servlet message receiver.");
            registered = false;
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
        return CompletableFuture.allOf();
    }

    @Override
    public boolean switchToLongPolling() {
        LOG.info(format("JEE servlet message receiver does not support long polling. Ignoring."));
        return false;
    }

    @Override
    public void receive(ImmutableMessage message) {
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
    public void onError(ImmutableMessage message, Throwable error) {
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
            if (hostPath == null) {
                String message = "The system property hostPath must be set with name:port eg. http://localhost:8080";
                IllegalArgumentException illegalArgumentException = new IllegalArgumentException(message);
                LOG.error(message, illegalArgumentException);
                throw illegalArgumentException;
            }
            if (httpBridgeEnabled) {
                LOG.debug("HTTP Bridge enabled - registering channel with endpoint registry.");
                String endpointUrl = hostPath + contextRoot + "/channels/" + channelId + "/";
                CompletionStage<Void> registrationResult = httpBridgeRegistryClient.register(endpointUrl, channelId);
                registrationResult.thenAccept((v) -> {
                    registered = true;
                }).exceptionally((t) -> {
                    LOG.error("Unable to register channel URL.", t);
                    return null;
                });
            } else {
                LOG.debug("HTTP Bridge disabled.");
                registered = true;
            }
        }
    }
}

package io.joynr.runtime;

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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.RegistrationFuture;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.dispatching.CallerDirectoryListener;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.subtypes.JoynrType;

import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

import joynr.BroadcastSubscriptionRequest;
import joynr.JoynrMessage;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.system.RoutingTypes.Address;

import org.reflections.Reflections;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

public class JoynrRuntimeImpl implements JoynrRuntime {

    private static final Logger logger = LoggerFactory.getLogger(JoynrRuntimeImpl.class);

    @Inject
    private CapabilitiesRegistrar capabilitiesRegistrar;
    @Inject
    private RequestReplyManager requestReplyManager;
    @Inject
    private PublicationManager publicationManager;
    private Dispatcher dispatcher;

    @Inject
    public ObjectMapper objectMapper;

    @Inject
    @Named(JOYNR_SCHEDULER_CLEANUP)
    ScheduledExecutorService cleanupScheduler;

    private final ProxyBuilderFactory proxyBuilderFactory;
    private final RequestCallerDirectory requestCallerDirectory;
    private final ReplyCallerDirectory replyCallerDirectory;
    private final MessageReceiver messageReceiver;

    private CallerDirectoryListener<RequestCaller> requestCallerDirectoryListener;
    private CallerDirectoryListener<ReplyCaller> replyCallerDirectoryListener;

    private boolean shutdown = false;
    private boolean registering = false;

    private IMessaging clusterControllerMessagingSkeleton;

    private class CallerDirectoryListenerImpl<T> implements CallerDirectoryListener<T> {

        @Override
        public void callerAdded(String participantId, T caller) {
            startReceiver();
        }

        public void callerRemoved(String participantId) {
        }
    }

    // CHECKSTYLE:OFF
    @Inject
    public JoynrRuntimeImpl(ObjectMapper objectMapper,
                            ProxyBuilderFactory proxyBuilderFactory,
                            RequestCallerDirectory requestCallerDirectory,
                            ReplyCallerDirectory replyCallerDirectory,
                            MessageReceiver messageReceiver,
                            Dispatcher dispatcher,
                            @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabilitiesDirectoryAddress,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS) Address channelUrlDirectoryAddress,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CLUSTERCONTROLER_MESSAGING_SKELETON) IMessaging clusterControllerMessagingSkeleton) {
        // CHECKSTYLE:ON
        this.requestCallerDirectory = requestCallerDirectory;
        this.replyCallerDirectory = replyCallerDirectory;
        this.messageReceiver = messageReceiver;
        this.dispatcher = dispatcher;
        this.clusterControllerMessagingSkeleton = clusterControllerMessagingSkeleton;
        Reflections reflections = new Reflections("joynr");
        Set<Class<? extends JoynrType>> subClasses = reflections.getSubTypesOf(JoynrType.class);
        objectMapper.registerSubtypes(subClasses.toArray(new Class<?>[subClasses.size()]));

        Class<?>[] messageTypes = new Class[]{ Request.class, Reply.class, SubscriptionRequest.class,
                SubscriptionStop.class, SubscriptionPublication.class, BroadcastSubscriptionRequest.class };
        objectMapper.registerSubtypes(messageTypes);
        this.proxyBuilderFactory = proxyBuilderFactory;
        if (libjoynrMessagingAddress instanceof InProcessAddress) {
            ((InProcessAddress) libjoynrMessagingAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        if (channelUrlDirectoryAddress instanceof InProcessAddress) {
            ((InProcessAddress) channelUrlDirectoryAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        if (capabilitiesDirectoryAddress instanceof InProcessAddress) {
            ((InProcessAddress) capabilitiesDirectoryAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        requestCallerDirectoryListener = new CallerDirectoryListenerImpl<RequestCaller>();
        replyCallerDirectoryListener = new CallerDirectoryListenerImpl<ReplyCaller>();
        requestCallerDirectory.addListener(requestCallerDirectoryListener);
        replyCallerDirectory.addListener(replyCallerDirectoryListener);
    }

    @Override
    public <T extends JoynrInterface> ProxyBuilder<T> getProxyBuilder(final String domain, final Class<T> interfaceClass) {

        if (domain == null || domain.isEmpty()) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: domain was not set");

        }

        if (interfaceClass == null) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: interfaceClass may not be NULL");
        }

        return proxyBuilderFactory.get(domain, interfaceClass);
    }

    @Override
    public RegistrationFuture registerProvider(String domain, JoynrProvider provider) {
        return capabilitiesRegistrar.registerProvider(domain, provider);
    }

    @Override
    public void unregisterProvider(String domain, JoynrProvider provider) {
        capabilitiesRegistrar.unregisterProvider(domain, provider);

    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN runtime");
        shutdown = true;
        try {
            capabilitiesRegistrar.shutdown(clear);
        } catch (Exception e) {
            logger.error("error clearing capabiltities while shutting down: {}", e.getMessage());
        }

        try {
            // TODO The channel is deleted but not deregistered from the Channel Url Directory
            requestReplyManager.shutdown();
            publicationManager.shutdown();
            dispatcher.shutdown(clear);
            requestCallerDirectory.removeListener(requestCallerDirectoryListener);
            replyCallerDirectory.removeListener(replyCallerDirectoryListener);
            try {
                messageReceiver.shutdown(clear);
            } catch (Exception e) {
                logger.error("error shutting down messageReceiver");
            }
        } catch (Exception e) {
            logger.error("error shutting down dispatcher: {}", e.getMessage());
        }

        try {
            requestReplyManager.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down message sender: {}", e.getMessage());
        }

        try {
            cleanupScheduler.shutdownNow();
        } catch (Exception e) {
            logger.error("error shutting down queue cleanup scheduler: {}", e.getMessage());
        }

    }

    private void startReceiver() {
        if (shutdown) {
            throw new JoynrShutdownException("cannot start receiver: dispatcher is already shutting down");
        }

        synchronized (messageReceiver) {
            if (registering == false) {
                registering = true;

                if (!messageReceiver.isStarted()) {
                    // The messageReceiver gets the message off the wire and passes it on to the message Listener.
                    // Starting the messageReceiver triggers a registration with the channelUrlDirectory, thus causing
                    // reply messages to be sent back to this message Receiver. It is therefore necessary to register
                    // the message receiver before registering the message listener.

                    // NOTE LongPollMessageReceiver creates a channel synchronously before returning

                    // TODO this will lead to a unique messageReceiver => all servlets share one channelId
                    messageReceiver.start(new MessageArrivedListener() {
                        @Override
                        public void messageArrived(JoynrMessage message) {
                            clusterControllerMessagingSkeleton.transmit(message);
                        }

                        @Override
                        public void error(JoynrMessage message, Throwable error) {
                            logger.error("Error in messageReceiver: {}. Arrived message with payload {} will be dropped",
                                         new Object[]{ error, message });
                        }
                    },
                                          new ReceiverStatusListener() {

                                              @Override
                                              public void receiverStarted() {
                                              }

                                              @Override
                                              // Exceptions that could not be resolved from within the receiver require a shutdown
                                              public void receiverException(Throwable e) {
                                                  // clear == false means that offboard resources (registrations, existing channels etc are
                                                  // not affected
                                                  shutdown(false);
                                              }
                                          });

                }
            }
        }
    }
}

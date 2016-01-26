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

import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.CallerDirectoryListener;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;

public class ClusterControllerRuntime extends JoynrRuntimeImpl {

    public static final Logger logger = LoggerFactory.getLogger(ClusterControllerRuntime.class);
    private final MessageReceiver messageReceiver;
    private boolean shutdown = false;
    private boolean registering = false;

    private IMessagingSkeleton clusterControllerMessagingSkeleton;

    private CallerDirectoryListener<RequestCaller> requestCallerDirectoryListener;
    private CallerDirectoryListener<ReplyCaller> replyCallerDirectoryListener;

    private class CallerDirectoryListenerImpl<T> implements CallerDirectoryListener<T> {

        @Override
        public void callerAdded(String participantId, T caller) {
            startReceiver();
        }

        @Override
        public void callerRemoved(String participantId) {
        }
    }

    // CHECKSTYLE:OFF
    @Inject
    public ClusterControllerRuntime(ObjectMapper objectMapper,
                                    ProxyBuilderFactory proxyBuilderFactory,
                                    RequestCallerDirectory requestCallerDirectory,
                                    ReplyCallerDirectory replyCallerDirectory,
                                    Dispatcher dispatcher,
                                    MessagingStubFactory messagingStubFactory,
                                    MessagingSkeletonFactory messagingSkeletonFactory,
                                    LocalDiscoveryAggregator localDiscoveryAggregator,
                                    @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                    @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabilitiesDirectoryAddress,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS) Address channelUrlDirectoryAddress,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS) Address domainAccessControllerAddress,
                                    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_CLUSTERCONTROLER_MESSAGING_SKELETON) IMessagingSkeleton clusterControllerMessagingSkeleton,
                                    CapabilitiesRegistrar capabilitiesRegistrar,
                                    LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                    MessageReceiver messageReceiver,
                                    MessageRouter messageRouter) {
        super(objectMapper,
              proxyBuilderFactory,
              requestCallerDirectory,
              replyCallerDirectory,
              dispatcher,
              messagingStubFactory,
              messagingSkeletonFactory,
              localDiscoveryAggregator,
              systemServicesDomain,
              dispatcherAddress,
              capabilitiesDirectoryAddress,
              channelUrlDirectoryAddress,
              domainAccessControllerAddress,
              discoveryProviderAddress);
        // CHECKSTYLE:ON
        this.messageReceiver = messageReceiver;

        requestCallerDirectoryListener = new CallerDirectoryListenerImpl<RequestCaller>();
        replyCallerDirectoryListener = new CallerDirectoryListenerImpl<ReplyCaller>();
        requestCallerDirectory.addListener(requestCallerDirectoryListener);
        replyCallerDirectory.addListener(replyCallerDirectoryListener);

        capabilitiesRegistrar.registerProvider(systemServicesDomain, localCapabilitiesDirectory);
        this.clusterControllerMessagingSkeleton = clusterControllerMessagingSkeleton;

        this.clusterControllerMessagingSkeleton.init();
        capabilitiesRegistrar.registerProvider(systemServicesDomain, messageRouter);
    }

    @Override
    public void shutdown(boolean clear) {
        super.shutdown(clear);
        shutdown = true;
        clusterControllerMessagingSkeleton.shutdown();
        try {
            requestCallerDirectory.removeListener(requestCallerDirectoryListener);
            replyCallerDirectory.removeListener(replyCallerDirectoryListener);
            messageReceiver.shutdown(clear);
        } catch (Exception e) {
            logger.error("error shutting down messageReceiver");
        }
    }

    void startReceiver() {
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
                            clusterControllerMessagingSkeleton.transmit(message, new FailureAction() {

                                @Override
                                public void execute(Throwable error) {
                                    logger.error("Failed to transmit message: ", error);
                                }
                            });
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

package io.joynr.runtime;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.ChildMessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.websocket.LibWebSocketMessagingStub;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.ProxyBuilderFactory;

import java.io.IOException;

import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketClientAddress;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

public class LibjoynrWebSocketRuntime extends LibjoynrRuntime<WebSocketClientAddress> {

    public static final Logger logger = LoggerFactory.getLogger(LibjoynrWebSocketRuntime.class);

    // CHECKSTYLE:OFF
    @Inject
    public LibjoynrWebSocketRuntime(ObjectMapper objectMapper,
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
                                    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address ccMessagingAddress,
                                    @Named(SystemServicesSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) WebSocketClientAddress libjoynrMessagingAddress,
                                    ChildMessageRouter messageRouter,
                                    @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String parentRoutingProviderParticipantId,
                                    @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_SKELETON) WebSocketMessagingSkeleton webSocketMessagingSkeleton) {
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
              discoveryProviderAddress,
              ccMessagingAddress,
              libjoynrMessagingAddress,
              messageRouter,
              parentRoutingProviderParticipantId);
        // CHECKSTYLE:ON
    }

    @Override
    protected void initMessagingStub(WebSocketClientAddress libjoynrMessagingAddress, IMessaging messagingStub) {
        if (messagingStub instanceof LibWebSocketMessagingStub) {
            initWebsocketStub(libjoynrMessagingAddress, (LibWebSocketMessagingStub) messagingStub);
        } else {
            throw new JoynrIllegalStateException("The messaging stub in the LibjoynrWebSockekRuntime has to be"
                    + " has to be of type " + LibWebSocketMessagingStub.class.getSimpleName());
        }
    }

    private void initWebsocketStub(WebSocketClientAddress libjoynrMessagingAddress,
                                   LibWebSocketMessagingStub messagingStub) {
        try {

            String serializedAddress = objectMapper.writeValueAsString(libjoynrMessagingAddress);
            messagingStub.sendString(serializedAddress, 30000);
        } catch (JsonProcessingException e) {
            logger.error("Error while serializing WebSocketClientAddress: ", e);
        } catch (IOException e) {
            logger.error("Error while sending websocket init message: ", e);
        }
    }
}

package io.joynr.runtime;

import java.util.UUID;

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

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebSocketMessagingStub;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

public class LibJoynrRuntime extends JoynrRuntimeImpl {

    public static final Logger logger = LoggerFactory.getLogger(LibJoynrRuntime.class);

    // CHECKSTYLE:OFF
    @Inject
    public LibJoynrRuntime(ObjectMapper objectMapper,
                           ProxyBuilderFactory proxyBuilderFactory,
                           RequestCallerDirectory requestCallerDirectory,
                           ReplyCallerDirectory replyCallerDirectory,
                           Dispatcher dispatcher,
                           LocalDiscoveryAggregator localDiscoveryAggregator,
                           @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                           @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress,
                           @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabilitiesDirectoryAddress,
                           @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS) Address channelUrlDirectoryAddress,
                           @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS) Address domainAccessControllerAddress,
                           @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_ADDRESS) Address discoveryProviderAddress,
                           @Named(ConfigurableMessagingSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address ccMessagingAddress,
                           WebSocketMessagingStubFactory webSocketMessagingStubFactory,
                           MessageRouter messageRouter,
                           @Named(ConfigurableMessagingSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String parentRoutingProviderParticipantId,
                           @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_SKELETON) WebSocketMessagingSkeleton webSocketMessagingSkeleton) {
        super(objectMapper,
              proxyBuilderFactory,
              requestCallerDirectory,
              replyCallerDirectory,
              dispatcher,
              localDiscoveryAggregator,
              systemServicesDomain,
              libjoynrMessagingAddress,
              capabilitiesDirectoryAddress,
              channelUrlDirectoryAddress,
              domainAccessControllerAddress,
              discoveryProviderAddress);
        // CHECKSTYLE:ON
        WebSocketClientAddress incommingAddress = null;
        if (ccMessagingAddress instanceof WebSocketAddress) {
            incommingAddress = initWebsocketStub((WebSocketAddress) ccMessagingAddress, webSocketMessagingStubFactory);
        } else {
            throw new JoynrIllegalStateException(ConfigurableMessagingSettings.PROPERTY_CC_MESSAGING_ADDRESS
                    + " has to be of type " + WebSocketAddress.class.getSimpleName());
        }
        webSocketMessagingSkeleton.initializeConnection();
        ProxyBuilder<RoutingProxy> proxyBuilder = getProxyBuilder(systemServicesDomain, RoutingProxy.class);
        RoutingProxy routingProxy = proxyBuilder.build();
        messageRouter.setIncommingAddress(incommingAddress);
        messageRouter.setParentRouter(routingProxy,
                                      ccMessagingAddress,
                                      parentRoutingProviderParticipantId,
                                      proxyBuilder.getParticipantId());
        messageRouter.addNextHop(discoveryProxyParticipantId, libjoynrMessagingAddress);

    }

    private WebSocketClientAddress initWebsocketStub(WebSocketAddress ccMessagingAddress,
                                                     WebSocketMessagingStubFactory webSocketMessagingStubFactory) {
        String messagingUUID = UUID.randomUUID().toString().replace("-", "");
        WebSocketClientAddress webSocketClientAddress = new WebSocketClientAddress("libjoynr.messaging.participantid_"
                + messagingUUID);
        try {

            String serializedAddress = objectMapper.writeValueAsString(webSocketClientAddress);
            WebSocketMessagingStub ccMessagingSocket = (WebSocketMessagingStub) webSocketMessagingStubFactory.create(ccMessagingAddress);
            ccMessagingSocket.sendString(serializedAddress, 30000);
        } catch (JsonProcessingException e) {
            logger.error("Error while serializing WebSocketClientAddress: ", e);
        }
        return webSocketClientAddress;
    }

    @Override
    void startReceiver() {
        logger.debug("LibJoynr has no receiver. Nothing to start.");
    }
}

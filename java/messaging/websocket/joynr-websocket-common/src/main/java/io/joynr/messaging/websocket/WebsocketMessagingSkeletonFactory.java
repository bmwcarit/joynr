/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.websocket;

import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.AbstractMessagingSkeletonFactory;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton.MainTransportFlagBearer;
import joynr.system.RoutingTypes.WebSocketAddress;

public class WebsocketMessagingSkeletonFactory extends AbstractMessagingSkeletonFactory {

    @Inject
    public WebsocketMessagingSkeletonFactory(@Named(WebsocketModule.WEBSOCKET_SERVER_ADDRESS) WebSocketAddress serverAddress,
                                             WebSocketEndpointFactory webSocketEndpointFactory,
                                             MessageRouter messageRouter,
                                             MainTransportFlagBearer mainTransportFlagBearer,
                                             Set<JoynrMessageProcessor> messageProcessors) {
        super();
        IMessagingSkeleton messagingSkeleton = new WebSocketMessagingSkeleton(serverAddress,
                                                                              webSocketEndpointFactory,
                                                                              messageRouter,
                                                                              mainTransportFlagBearer,
                                                                              messageProcessors);
        messagingSkeletonList.add(messagingSkeleton);
    }
}

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
package io.joynr.messaging.channel;

import java.util.Set;

import javax.inject.Named;

import com.google.inject.Inject;

import io.joynr.messaging.AbstractMessagingSkeletonFactory;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;

public class ChannelMessagingSkeletonFactory extends AbstractMessagingSkeletonFactory {

    @Inject
    public ChannelMessagingSkeletonFactory(MessageRouter messageRouter,
                                           MessageReceiver messageReceiver,
                                           Set<JoynrMessageProcessor> messageProcessors,
                                           @Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbidsArray,
                                           RoutingTable routingTable) {
        super();
        IMessagingSkeleton messagingSkeleton = new ChannelMessagingSkeleton(messageRouter,
                                                                            messageReceiver,
                                                                            messageProcessors,
                                                                            gbidsArray[0],
                                                                            routingTable);
        messagingSkeletonList.add(messagingSkeleton);
    }

}

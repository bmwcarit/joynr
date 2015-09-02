package io.joynr.proxy;

/*
 * #%L
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

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;

import javax.annotation.CheckForNull;

import joynr.system.routingtypes.Address;
import joynr.system.routingtypes.ChannelAddress;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;

@Singleton
public class ConnectorFactory {

    JoynrMessagingConnectorFactory joynrMessagingConnectorFactory;

    private static final Logger logger = LoggerFactory.getLogger(ConnectorFactory.class);

    private MessageRouter messageRouter;

    @Inject
    public ConnectorFactory(JoynrMessagingConnectorFactory joynrMessagingConnectorFactory, MessageRouter messageRouter) {
        this.joynrMessagingConnectorFactory = joynrMessagingConnectorFactory;
        this.messageRouter = messageRouter;
    }

    /**
     * Creates a new connector object using concrete connector factories chosen by the endpointAddress which is passed
     * in.
     * 
     * @param fromParticipantId origin participant id
     * @param arbitrationResult result of arbitration
     * @param qosSettings QOS settings
     * @return connector object
     */
    @CheckForNull
    public ConnectorInvocationHandler create(final String fromParticipantId,
                                             final ArbitrationResult arbitrationResult,
                                             final MessagingQos qosSettings) {

        for (Address endpointAddress : arbitrationResult.getEndpointAddress()) {
            if (endpointAddress instanceof ChannelAddress) {
                messageRouter.addNextHop(arbitrationResult.getParticipantId(), (ChannelAddress) endpointAddress);
                return joynrMessagingConnectorFactory.create(fromParticipantId,
                                                             arbitrationResult.getParticipantId(),
                                                             qosSettings);
            }

            logger.warn("Unknown EndpointAddress type: Did not create to create connector: " + endpointAddress);

        }
        return null;

    }
}

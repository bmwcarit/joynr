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
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.rpc.JoynrMessagingConnectorFactory;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.messaging.MessagingQos;

import javax.annotation.CheckForNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class ConnectorFactory {

    private ConnectorFactory() {

    }

    private static final Logger logger = LoggerFactory.getLogger(ConnectorFactory.class);

    /**
     * Creates a new connector object using concrete connector factories chosen by the endpointAddress which is passed
     * in.
     * 
     * @param clazz
     * @param dispatcher
     * @param messageSender
     * @param fromParticipantId
     * @param toParticipantId
     * @param qosSettings
     * @param endpointAddress
     * @return
     */
    @CheckForNull
    public static ConnectorInvocationHandler create(final RequestReplyDispatcher dispatcher,
                                                    final RequestReplySender messageSender,
                                                    final String fromParticipantId,
                                                    final ArbitrationResult arbitrationResult,
                                                    final MessagingQos qosSettings) {

        for (EndpointAddressBase endpointAddress : arbitrationResult.getEndpointAddress()) {
            if (endpointAddress instanceof JoynrMessagingEndpointAddress) {
                return JoynrMessagingConnectorFactory.create(dispatcher,
                                                             messageSender,
                                                             fromParticipantId,
                                                             arbitrationResult.getParticipantId(),
                                                             (JoynrMessagingEndpointAddress) endpointAddress,
                                                             qosSettings);
            }

            logger.warn("Unknown EndpointAddress type: Did not create to create connector: " + endpointAddress);

        }
        return null;

    }
}

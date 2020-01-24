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
package io.joynr.proxy;

import java.util.Optional;
import java.util.Set;

import javax.inject.Named;

import com.google.inject.Inject;
import com.google.inject.Singleton;

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.SystemServicesSettings;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.ProviderScope;

@Singleton
public class ConnectorFactory {

    JoynrMessagingConnectorFactory joynrMessagingConnectorFactory;

    private MessageRouter messageRouter;
    private Address libjoynrMessagingAddress;

    @Inject
    public ConnectorFactory(JoynrMessagingConnectorFactory joynrMessagingConnectorFactory,
                            MessageRouter messageRouter,
                            @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress) {
        this.joynrMessagingConnectorFactory = joynrMessagingConnectorFactory;
        this.messageRouter = messageRouter;
        this.libjoynrMessagingAddress = dispatcherAddress;
    }

    /**
     * Creates a new connector object using concrete connector factories chosen by the endpointAddress which is passed
     * in.
     *
     * @param fromParticipantId origin participant id
     * @param arbitrationResult result of arbitration
     * @param qosSettings QOS settings
     * @param statelessAsyncParticipantId
     * @return connector object
     */
    public Optional<ConnectorInvocationHandler> create(final String fromParticipantId,
                                                       final ArbitrationResult arbitrationResult,
                                                       final MessagingQos qosSettings,
                                                       String statelessAsyncParticipantId) {
        // iterate through  arbitrationResult.getDiscoveryEntries()
        // check if there is at least one Globally visible
        // set isGloballyVisible = true. otherwise = false
        boolean isGloballyVisible = false;
        Set<DiscoveryEntryWithMetaInfo> entries = arbitrationResult.getDiscoveryEntries();
        for (DiscoveryEntryWithMetaInfo entry : entries) {
            if (entry.getQos().getScope() == ProviderScope.GLOBAL) {
                isGloballyVisible = true;
            }
            messageRouter.setToKnown(entry.getParticipantId());
        }
        messageRouter.addNextHop(fromParticipantId, libjoynrMessagingAddress, isGloballyVisible);
        return Optional.ofNullable(joynrMessagingConnectorFactory.create(fromParticipantId,
                                                                         arbitrationResult.getDiscoveryEntries(),
                                                                         qosSettings,
                                                                         statelessAsyncParticipantId));
    }
}

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

import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.assistedinject.Assisted;

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.GarbageCollectionHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.ShutdownNotifier;
import joynr.types.DiscoveryEntryWithMetaInfo;

public class ProxyInvocationHandlerImpl extends ProxyInvocationHandler {
    private ConnectorFactory connectorFactory;
    private MessageRouter messageRouter;
    private GarbageCollectionHandler gcHandler;

    // CHECKSTYLE:OFF
    @Inject
    public ProxyInvocationHandlerImpl(@Assisted("domains") Set<String> domains,
                                      @Assisted("interfaceName") String interfaceName,
                                      @Assisted("proxyParticipantId") String proxyParticipantId,
                                      @Assisted DiscoveryQos discoveryQos,
                                      @Assisted MessagingQos messagingQos,
                                      @Assisted Optional<StatelessAsyncCallback> statelessAsyncCallback,
                                      @Assisted boolean separateReplyReceiver,
                                      ConnectorFactory connectorFactory,
                                      MessageRouter messageRouter,
                                      GarbageCollectionHandler gcHandler,
                                      ShutdownNotifier shutdownNotifier,
                                      StatelessAsyncIdCalculator statelessAsyncIdCalculator) {
        super(domains,
              interfaceName,
              proxyParticipantId,
              discoveryQos,
              messagingQos,
              statelessAsyncCallback,
              separateReplyReceiver,
              shutdownNotifier,
              statelessAsyncIdCalculator);
        // CHECKSTYLE:ON
        this.connectorFactory = connectorFactory;
        this.messageRouter = messageRouter;
        this.gcHandler = gcHandler;
    }

    /**
     * Sets the connector for this ProxyInvocationHandler after the DiscoveryAgent got notified about a successful
     * arbitration. Should be called from the DiscoveryAgent
     *
     * @param result from the previously invoked arbitration
     */
    @Override
    public void createConnector(ArbitrationResult result) {
        Optional<ConnectorInvocationHandler> connectorOptional = connectorFactory.create(proxyParticipantId,
                                                                                         result,
                                                                                         qosSettings,
                                                                                         statelessAsyncParticipantId);
        connector = connectorOptional.isPresent() ? connectorOptional.get() : null;
        setConnectorStatusSuccessAndSendQueuedRequests();

        // call registerProxyProviderParticipantIds for selected providers
        Set<String> providerParticipantIds = new HashSet<>();
        for (DiscoveryEntryWithMetaInfo selectedEntry : result.getDiscoveryEntries()) {
            providerParticipantIds.add(selectedEntry.getParticipantId());
        }
        gcHandler.registerProxyProviderParticipantIds(proxyParticipantId, providerParticipantIds);

        // decrease the RoutingEntry reference count for non-selected providers
        for (DiscoveryEntryWithMetaInfo nonSelectedEntry : result.getOtherDiscoveryEntries()) {
            messageRouter.removeNextHop(nonSelectedEntry.getParticipantId());
        }
    }

    @Override
    public void registerProxy(Object proxy) {
        gcHandler.registerProxy(proxy, proxyParticipantId, shutdownListener);
    }
}

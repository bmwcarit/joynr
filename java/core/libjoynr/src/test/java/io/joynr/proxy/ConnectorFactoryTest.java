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
package io.joynr.proxy;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;

import java.util.HashSet;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@RunWith(MockitoJUnitRunner.class)
public class ConnectorFactoryTest {

    private ConnectorFactory connectorFactory;

    @Mock
    private JoynrMessagingConnectorFactory joynrMessagingConnectorFactory;
    @Mock
    private MessageRouter messageRouter;
    @Mock
    private Address dispatcherAddress;

    private final String proxyParticipantId = "proxyParticipantId";
    private final String statelessAsyncParticipantId = "statelessAsyncParticipantId";
    private final MessagingQos messagingQos = new MessagingQos();
    @Mock
    private ArbitrationResult arbitrationResult;

    @Before
    public void setup() {
        connectorFactory = new ConnectorFactory(joynrMessagingConnectorFactory, messageRouter, dispatcherAddress);
    }

    @Test
    public void createAddsProxyAddress() {
        doReturn(new HashSet<>()).when(arbitrationResult).getDiscoveryEntries();
        connectorFactory.create(proxyParticipantId, arbitrationResult, messagingQos, statelessAsyncParticipantId);

        verify(messageRouter).addNextHop(proxyParticipantId, dispatcherAddress, false);
    }

    @Test
    public void createSetsIsGloballyVisibleForProxy() {
        final String providerParticipantId = "providerParticipantId";

        final Set<DiscoveryEntryWithMetaInfo> discoveredProviders = new HashSet<>();
        final DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo();
        discoveryEntry.setParticipantId(providerParticipantId);
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        discoveryEntry.setQos(providerQos);
        discoveredProviders.add(discoveryEntry);

        doReturn(discoveredProviders).when(arbitrationResult).getDiscoveryEntries();

        connectorFactory.create(proxyParticipantId, arbitrationResult, messagingQos, statelessAsyncParticipantId);
        verify(messageRouter).addNextHop(proxyParticipantId, dispatcherAddress, false);

        reset(messageRouter);

        providerQos.setScope(ProviderScope.GLOBAL);
        connectorFactory.create(proxyParticipantId, arbitrationResult, messagingQos, statelessAsyncParticipantId);
        verify(messageRouter).addNextHop(proxyParticipantId, dispatcherAddress, true);
    }

    @Test
    public void createAddsProviderAddresses() {
        final String provider1ParticipantId = "provider1ParticipantId";
        final String provider2ParticipantId = "provider2ParticipantId";

        final Set<DiscoveryEntryWithMetaInfo> discoveredProviders = new HashSet<>();
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo();
        discoveryEntry.setParticipantId(provider1ParticipantId);
        discoveredProviders.add(discoveryEntry);

        discoveryEntry = new DiscoveryEntryWithMetaInfo();
        discoveryEntry.setParticipantId(provider2ParticipantId);
        discoveredProviders.add(discoveryEntry);

        doReturn(discoveredProviders).when(arbitrationResult).getDiscoveryEntries();

        connectorFactory.create(proxyParticipantId, arbitrationResult, messagingQos, statelessAsyncParticipantId);
    }

}

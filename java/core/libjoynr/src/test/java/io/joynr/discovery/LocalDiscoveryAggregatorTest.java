package io.joynr.discovery;

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

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import joynr.system.Discovery;
import joynr.system.DiscoveryProxy;
import joynr.types.CommunicationMiddleware;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryQos;
import joynr.types.ProviderQos;

@RunWith(MockitoJUnitRunner.class)
public class LocalDiscoveryAggregatorTest {

    private String systemServicesDomain;
    private String discoveryProviderParticipantId;
    private CommunicationMiddleware clusterControllerConnection;
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    private DiscoveryEntry discoveryProviderEntry;

    @Mock
    DiscoveryProxy discoveryProxyMock;
    @Mock
    Callback<Void> addCallback;
    @Mock
    private Callback<DiscoveryEntry[]> lookupCallback;
    @Mock
    private Callback<DiscoveryEntry> lookupParticipantCallback;
    @Mock
    private Callback<Void> removeCallback;

    @Before
    public void setUp() {
        systemServicesDomain = "test.system.service.domain";
        discoveryProviderParticipantId = "test.discovery.provider.participant";
        clusterControllerConnection = CommunicationMiddleware.NONE;
        localDiscoveryAggregator = new LocalDiscoveryAggregator(systemServicesDomain,
                                                                discoveryProviderParticipantId,
                                                                clusterControllerConnection);
        localDiscoveryAggregator.setDiscoveryProxy(discoveryProxyMock);
        discoveryProviderEntry = new DiscoveryEntry(systemServicesDomain,
                                                    Discovery.INTERFACE_NAME,
                                                    discoveryProviderParticipantId,
                                                    new ProviderQos(),
                                                    new CommunicationMiddleware[]{ clusterControllerConnection });

    }

    @Test
    public void doesNotPassProvisionedEntry() {
        localDiscoveryAggregator.add(addCallback, discoveryProviderEntry);
        Mockito.verify(discoveryProxyMock, Mockito.never()).add(Mockito.any(Callback.class),
                                                                Mockito.any(DiscoveryEntry.class));
        Mockito.verify(addCallback).resolve();
    }

    @Test
    public void passesUnknownEntry() {
        DiscoveryEntry discoveryEntry = new DiscoveryEntry("anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           new CommunicationMiddleware[]{ clusterControllerConnection });
        localDiscoveryAggregator.add(addCallback, discoveryEntry);
        Mockito.verify(discoveryProxyMock, Mockito.times(1)).add(Mockito.any(Callback.class),
                                                                 Mockito.eq(discoveryEntry));
    }

    @Test
    public void findsProvisionedEntry() {
        localDiscoveryAggregator.lookup(lookupCallback,
                                        systemServicesDomain,
                                        Discovery.INTERFACE_NAME,
                                        new DiscoveryQos());
        Mockito.verify(lookupCallback).resolve(Mockito.eq(new DiscoveryEntry[]{ discoveryProviderEntry }));
    }

    @Test
    public void doesNotQueryProvisionedEntry() {
        localDiscoveryAggregator.lookup(lookupCallback,
                                        systemServicesDomain,
                                        Discovery.INTERFACE_NAME,
                                        new DiscoveryQos());
        Mockito.verify(discoveryProxyMock, Mockito.never()).lookup(Mockito.any(Callback.class),
                                                                   Mockito.anyString(),
                                                                   Mockito.anyString(),
                                                                   Mockito.any(DiscoveryQos.class));
    }

    @Test(expected = JoynrRuntimeException.class)
    public void addThrowsIfProxyNotSet() {
        localDiscoveryAggregator.setDiscoveryProxy(null);

        DiscoveryEntry discoveryEntry = new DiscoveryEntry("anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           new CommunicationMiddleware[]{ clusterControllerConnection });
        localDiscoveryAggregator.add(addCallback, discoveryEntry);
        Mockito.verify(addCallback, Mockito.never()).resolve();

    }

    @Test(expected = JoynrRuntimeException.class)
    public void lookupByParticipantThrowsIfProxyNotSet() {
        localDiscoveryAggregator.setDiscoveryProxy(null);
        localDiscoveryAggregator.lookup(lookupParticipantCallback, "someParticipant");
        Mockito.verify(lookupParticipantCallback, Mockito.never()).resolve(Mockito.any());
    }

    @Test(expected = JoynrRuntimeException.class)
    public void lookupByDomainThrowsIfProxyNotSet() {
        localDiscoveryAggregator.setDiscoveryProxy(null);
        localDiscoveryAggregator.lookup(lookupCallback, "anyDomain", "anyInterface", new DiscoveryQos());
        Mockito.verify(lookupCallback, Mockito.never()).resolve(Mockito.any());
    }

    @Test(expected = JoynrRuntimeException.class)
    public void removeThrowsIfProxyNotSet() {
        localDiscoveryAggregator.setDiscoveryProxy(null);
        localDiscoveryAggregator.remove(removeCallback, "anyParticipant");
        Mockito.verify(removeCallback, Mockito.never()).resolve();
    }
}

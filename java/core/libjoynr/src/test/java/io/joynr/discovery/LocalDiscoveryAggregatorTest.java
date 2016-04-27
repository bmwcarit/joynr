package io.joynr.discovery;

import static org.junit.Assert.assertEquals;

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
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import joynr.system.Discovery;
import joynr.system.DiscoveryProxy;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class LocalDiscoveryAggregatorTest {

    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private Long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    private String systemServicesDomain;
    private String discoveryProviderParticipantId;
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    private DiscoveryEntry discoveryProviderEntry;
    private String publicKeyId = "publicKeyId";

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
        localDiscoveryAggregator = new LocalDiscoveryAggregator(systemServicesDomain,
                                                                discoveryProviderParticipantId,
                                                                "routingProviderParticipantId");
        localDiscoveryAggregator.setDiscoveryProxy(discoveryProxyMock);
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        discoveryProviderEntry = new DiscoveryEntry(new Version(0, 0),
                                                    systemServicesDomain,
                                                    Discovery.INTERFACE_NAME,
                                                    discoveryProviderParticipantId,
                                                    providerQos,
                                                    System.currentTimeMillis(),
                                                    expiryDateMs,
                                                    publicKeyId);

    }

    @Test
    public void passesUnknownEntry() {
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(0, 0),
                                                           "anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        localDiscoveryAggregator.add(addCallback, discoveryEntry);
        Mockito.verify(discoveryProxyMock, Mockito.times(1)).add(Mockito.any(Callback.class),
                                                                 Mockito.eq(discoveryEntry));
    }

    @Test
    public void findsProvisionedEntry() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        localDiscoveryAggregator.lookup(lookupCallback, systemServicesDomain, Discovery.INTERFACE_NAME, discoveryQos);
        ArgumentCaptor<DiscoveryEntry[]> discoveryEntriesCaptor = ArgumentCaptor.forClass(DiscoveryEntry[].class);
        Mockito.verify(lookupCallback).resolve((Object) discoveryEntriesCaptor.capture());
        DiscoveryEntry[] discoveryEntriesPassed = discoveryEntriesCaptor.getValue();
        assertEquals(discoveryProviderEntry.getDomain(), discoveryEntriesPassed[0].getDomain());
        assertEquals(discoveryProviderEntry.getInterfaceName(), discoveryEntriesPassed[0].getInterfaceName());
        assertEquals(discoveryProviderEntry.getParticipantId(), discoveryEntriesPassed[0].getParticipantId());
        assertEquals(discoveryProviderEntry.getQos(), discoveryEntriesPassed[0].getQos());
        assertEquals(discoveryProviderEntry.getProviderVersion(), discoveryEntriesPassed[0].getProviderVersion());
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

        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(0, 0),
                                                           "anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
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

package io.joynr.discovery;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.never;

import java.util.Arrays;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
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

    private static final Logger logger = LoggerFactory.getLogger(LocalDiscoveryAggregatorTest.class);

    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private Long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    private String systemServicesDomain;
    private String anotherDomain;
    private String[] allDomains;
    private String[] oneDomain;
    private String discoveryProviderParticipantId;
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    private DiscoveryEntry discoveryProviderEntry;
    private DiscoveryEntry anotherDiscoveryProviderEntry;
    private DiscoveryEntry[] discoveryProviderEntries;
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
        anotherDomain = "anotherDomain";
        discoveryProviderParticipantId = "test.discovery.provider.participant";
        localDiscoveryAggregator = new LocalDiscoveryAggregator(systemServicesDomain,
                                                                discoveryProviderParticipantId,
                                                                "routingProviderParticipantId");
        localDiscoveryAggregator.setDiscoveryProxy(discoveryProxyMock);
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        discoveryProviderEntry = new DiscoveryEntry(new Version(0, 1),
                                                    systemServicesDomain,
                                                    Discovery.INTERFACE_NAME,
                                                    discoveryProviderParticipantId,
                                                    providerQos,
                                                    System.currentTimeMillis(),
                                                    expiryDateMs,
                                                    publicKeyId);
        anotherDiscoveryProviderEntry = new DiscoveryEntry(new Version(0, 0),
                                                           anotherDomain,
                                                           Discovery.INTERFACE_NAME,
                                                           discoveryProviderParticipantId,
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
    }

    @SuppressWarnings("unchecked")
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
        verify(discoveryProxyMock, times(1)).add(any(Callback.class), eq(discoveryEntry));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void findsProvisionedEntryForSingleDomain() {
        discoveryProviderEntries = new DiscoveryEntry[]{ discoveryProviderEntry };
        oneDomain = new String[]{ systemServicesDomain };
        // Double Decla. allDomains = new String[]{ anotherDomain, systemServicesDomain };
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        Future<DiscoveryEntry[]> discoveryEntryFuture = localDiscoveryAggregator.lookup(lookupCallback,
                                                                                        oneDomain,
                                                                                        Discovery.INTERFACE_NAME,
                                                                                        discoveryQos);
        ArgumentCaptor<DiscoveryEntry[]> discoveryEntriesCaptor = ArgumentCaptor.forClass(DiscoveryEntry[].class);
        verify(lookupCallback).resolve((Object) discoveryEntriesCaptor.capture());
        DiscoveryEntry[] discoveryEntriesPassed = discoveryEntriesCaptor.getValue();
        assertEquals(1, discoveryEntriesPassed.length);
        assertEquals(discoveryProviderEntry.getDomain(), discoveryEntriesPassed[0].getDomain());
        assertEquals(discoveryProviderEntry.getInterfaceName(), discoveryEntriesPassed[0].getInterfaceName());
        assertEquals(discoveryProviderEntry.getParticipantId(), discoveryEntriesPassed[0].getParticipantId());
        assertEquals(discoveryProviderEntry.getQos(), discoveryEntriesPassed[0].getQos());
        assertEquals(discoveryProviderEntry.getProviderVersion(), discoveryEntriesPassed[0].getProviderVersion());
        try {
            discoveryEntriesPassed = discoveryEntryFuture.get();
        } catch (Exception e) {
            Assert.fail("Got exception from future: " + e);
        }
        assertEquals(1, discoveryEntriesPassed.length);
        assertEquals(discoveryProviderEntry.getDomain(), discoveryEntriesPassed[0].getDomain());
        assertEquals(discoveryProviderEntry.getInterfaceName(), discoveryEntriesPassed[0].getInterfaceName());
        assertEquals(discoveryProviderEntry.getParticipantId(), discoveryEntriesPassed[0].getParticipantId());
        assertEquals(discoveryProviderEntry.getQos(), discoveryEntriesPassed[0].getQos());
        assertEquals(discoveryProviderEntry.getProviderVersion(), discoveryEntriesPassed[0].getProviderVersion());
        verify(discoveryProxyMock, never()).lookup(any(Callback.class),
                                                   any(String[].class),
                                                   anyString(),
                                                   any(DiscoveryQos.class));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void findsProvisionedEntryForMultipleDomains() throws Exception {
        discoveryProviderEntries = new DiscoveryEntry[]{ anotherDiscoveryProviderEntry };
        allDomains = new String[]{ systemServicesDomain, anotherDomain };
        String[] missingDomains = new String[]{ anotherDomain };
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                Callback<DiscoveryEntry[]> callback = (Callback<DiscoveryEntry[]>) invocation.getArguments()[0];
                callback.onSuccess(discoveryProviderEntries);
                return null;
            }
        }).when(discoveryProxyMock).lookup(any(Callback.class),
                                           any(String[].class),
                                           anyString(),
                                           any(DiscoveryQos.class));

        Future<DiscoveryEntry[]> discoveryEntriesFuture = localDiscoveryAggregator.lookup(lookupCallback,
                                                                                          allDomains,
                                                                                          Discovery.INTERFACE_NAME,
                                                                                          discoveryQos);

        assertNotNull(discoveryEntriesFuture);
        DiscoveryEntry[] result = discoveryEntriesFuture.get();
        logger.info("Got discovery entries: " + Arrays.toString(result));
        assertNotNull(result);
        assertEquals(2, result.length);
        assertTrue(containsByInterfaceDomain(result,
                                             discoveryProviderEntry.getInterfaceName(),
                                             discoveryProviderEntry.getDomain()));
        assertTrue(containsByInterfaceDomain(result,
                                             anotherDiscoveryProviderEntry.getInterfaceName(),
                                             anotherDiscoveryProviderEntry.getDomain()));
        verify(discoveryProxyMock).lookup(any(Callback.class),
                                          eq(missingDomains),
                                          eq(Discovery.INTERFACE_NAME),
                                          eq(discoveryQos));
    }

    private boolean containsByInterfaceDomain(DiscoveryEntry[] discoveryEntries, String interfaceName, String domain) {
        for (DiscoveryEntry discoveryEntry : discoveryEntries) {
            if (discoveryEntry != null && discoveryEntry.getDomain().equals(domain)
                    && discoveryEntry.getInterfaceName().equals(interfaceName)) {
                return true;
            }
        }
        return false;
    }

    @SuppressWarnings("unchecked")
    @Test
    public void doesNotQueryProvisionedEntry() {
        localDiscoveryAggregator.lookup(lookupCallback,
                                        new String[]{ systemServicesDomain },
                                        Discovery.INTERFACE_NAME,
                                        new DiscoveryQos());
        verify(discoveryProxyMock, never()).lookup(any(Callback.class),
                                                   any(String[].class),
                                                   anyString(),
                                                   any(DiscoveryQos.class));
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
        verify(addCallback, never()).resolve();

    }

    @Test(expected = JoynrRuntimeException.class)
    public void lookupByParticipantThrowsIfProxyNotSet() {
        localDiscoveryAggregator.setDiscoveryProxy(null);
        localDiscoveryAggregator.lookup(lookupParticipantCallback, "someParticipant");
        verify(lookupParticipantCallback, never()).resolve(any());
    }

    @Test(expected = JoynrRuntimeException.class)
    public void lookupByDomainThrowsIfProxyNotSet() {
        localDiscoveryAggregator.setDiscoveryProxy(null);
        localDiscoveryAggregator.lookup(lookupCallback, new String[]{ "anyDomain" }, "anyInterface", new DiscoveryQos());
        verify(lookupCallback, never()).resolve(any());
    }

    @Test(expected = JoynrRuntimeException.class)
    public void removeThrowsIfProxyNotSet() {
        localDiscoveryAggregator.setDiscoveryProxy(null);
        localDiscoveryAggregator.remove(removeCallback, "anyParticipant");
        verify(removeCallback, never()).resolve();
    }
}

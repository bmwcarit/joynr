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
package io.joynr.discovery;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Arrays;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.JoynrVersion;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.system.Discovery;
import joynr.system.DiscoveryProxy;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
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

    final private long epsilonMs = 10000;

    private String systemServicesDomain;
    private String anotherDomain;
    private String[] allDomains;
    private String[] oneDomain;
    private String discoveryProviderParticipantId;
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    private DiscoveryEntryWithMetaInfo discoveryProviderEntry;
    private DiscoveryEntryWithMetaInfo anotherDiscoveryProviderEntry;
    private DiscoveryEntryWithMetaInfo[] discoveryProviderEntries;
    private String publicKeyId = "publicKeyId";

    @Mock
    DiscoveryProxy discoveryProxyMock;
    @Mock
    ProxyBuilderFactory proxyBuilderFactory;
    @Mock
    ProxyBuilder<DiscoveryProxy> proxyBuilder;
    @Mock
    Callback<Void> callback;
    @Mock
    CallbackWithModeledError<Void, DiscoveryError> addCallbackWithModeledError;
    @Mock
    CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError> lookupCallbackWithModeledError;
    @Mock
    CallbackWithModeledError<DiscoveryEntryWithMetaInfo, DiscoveryError> lookupByParticipantIdCallbackWithModeledError;

    private String[] gbids = { "joynrdefaultgbid", "testGbid2", "testGbid3" };

    @Before
    public void setUp() {
        systemServicesDomain = "test.system.service.domain";
        anotherDomain = "anotherDomain";
        discoveryProviderParticipantId = "test.discovery.provider.participant";

        when(proxyBuilderFactory.get(anyString(), eq(DiscoveryProxy.class))).thenReturn(proxyBuilder);
        when(proxyBuilder.build()).thenReturn(discoveryProxyMock);
        when(proxyBuilder.setMessagingQos(any(MessagingQos.class))).thenReturn(proxyBuilder);

        localDiscoveryAggregator = new LocalDiscoveryAggregator(systemServicesDomain,
                                                                discoveryProviderParticipantId,
                                                                "routingProviderParticipantId",
                                                                proxyBuilderFactory);
        localDiscoveryAggregator.forceQueryOfDiscoveryProxy();
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        JoynrVersion interfaceVersion = Discovery.class.getAnnotation(JoynrVersion.class);

        discoveryProviderEntry = new DiscoveryEntryWithMetaInfo(new Version(interfaceVersion.major(),
                                                                            interfaceVersion.minor()),
                                                                systemServicesDomain,
                                                                Discovery.INTERFACE_NAME,
                                                                discoveryProviderParticipantId,
                                                                providerQos,
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                true);
        anotherDiscoveryProviderEntry = new DiscoveryEntryWithMetaInfo(new Version(0, 0),
                                                                       anotherDomain,
                                                                       Discovery.INTERFACE_NAME,
                                                                       discoveryProviderParticipantId,
                                                                       providerQos,
                                                                       System.currentTimeMillis(),
                                                                       expiryDateMs,
                                                                       publicKeyId,
                                                                       true);
    }

    @Test
    public void testAdd() {
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(0, 0),
                                                           "anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        localDiscoveryAggregator.add(callback, discoveryEntry);
        verify(discoveryProxyMock,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(discoveryEntry),
                             eq(false));
    }

    private void testAwaitGlobalRegistration(boolean awaitGlobalRegistration) {
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(0, 0),
                                                           "anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        localDiscoveryAggregator.add(callback, discoveryEntry, awaitGlobalRegistration);
        verify(discoveryProxyMock, times(1)).add(ArgumentMatchers.<Callback<Void>> any(),
                                                 eq(discoveryEntry),
                                                 eq(awaitGlobalRegistration));
    }

    @Test
    public void add_handlesAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = true;
        testAwaitGlobalRegistration(awaitGlobalRegistration);
    }

    @Test
    public void add_handlesDoNotAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = false;
        testAwaitGlobalRegistration(awaitGlobalRegistration);
    }

    private void testAwaitGlobalRegistrationWithGbids(boolean awaitGlobalRegistration) {
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(0, 0),
                                                           "anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        DiscoveryEntry expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        localDiscoveryAggregator.add(addCallbackWithModeledError,
                                     discoveryEntry,
                                     awaitGlobalRegistration,
                                     gbids.clone());
        verify(discoveryProxyMock,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(expectedDiscoveryEntry),
                             eq(awaitGlobalRegistration),
                             eq(gbids));
    }

    @Test
    public void add_handlesAwaitGlobalRegistrationWithGbids() {
        boolean awaitGlobalRegistration = true;
        testAwaitGlobalRegistrationWithGbids(awaitGlobalRegistration);
    }

    @Test
    public void add_handlesDoNotAwaitGlobalRegistrationWithGbids() {
        boolean awaitGlobalRegistration = false;
        testAwaitGlobalRegistrationWithGbids(awaitGlobalRegistration);
    }

    @Test
    public void addToAll_handlesAwaitGlobalRegistrationWithGbids() {
        boolean awaitGlobalRegistration = true;
        testAddToAllAwaitGlobalRegistration(awaitGlobalRegistration);
    }

    @Test
    public void addToAll_handlesDoNotAwaitGlobalRegistrationWithGbids() {
        boolean awaitGlobalRegistration = false;
        testAddToAllAwaitGlobalRegistration(awaitGlobalRegistration);
    }

    private void testAddToAllAwaitGlobalRegistration(boolean awaitGlobalRegistration) {
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(0, 0),
                                                           "anyDomain",
                                                           "anyInterface",
                                                           "anyParticipant",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);

        DiscoveryEntry expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);

        localDiscoveryAggregator.addToAll(addCallbackWithModeledError, discoveryEntry, awaitGlobalRegistration);
        verify(discoveryProxyMock,
               times(1)).addToAll(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                  eq(expectedDiscoveryEntry),
                                  eq(awaitGlobalRegistration));
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_callsProxyForNonProvisionedEntries() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        DiscoveryQos expectedDiscoveryQos = new DiscoveryQos(discoveryQos);
        localDiscoveryAggregator.lookup(lookupCallbackWithModeledError,
                                        new String[]{ anotherDomain },
                                        Discovery.INTERFACE_NAME,
                                        discoveryQos,
                                        gbids);

        verify(discoveryProxyMock,
               times(1)).lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                eq(new String[]{ anotherDomain }),
                                eq(Discovery.INTERFACE_NAME),
                                eq(expectedDiscoveryQos),
                                eq(gbids),
                                eq(new MessagingQos(expectedDiscoveryQos.getDiscoveryTimeout() + epsilonMs)));
    }

    @Test
    public void lookupByDomainInterface_callsProxyForNonProvisionedEntries() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        DiscoveryQos expectedDiscoveryQos = new DiscoveryQos(discoveryQos);
        localDiscoveryAggregator.lookup(lookupCallbackWithModeledError,
                                        new String[]{ anotherDomain },
                                        Discovery.INTERFACE_NAME,
                                        discoveryQos);

        verify(discoveryProxyMock,
               times(1)).lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                eq(new String[]{ anotherDomain }),
                                eq(Discovery.INTERFACE_NAME),
                                eq(expectedDiscoveryQos),
                                eq(new String[0]),
                                eq(new MessagingQos(expectedDiscoveryQos.getDiscoveryTimeout() + epsilonMs)));
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_findsProvisionedEntryForSingleDomain() {
        discoveryProviderEntries = new DiscoveryEntryWithMetaInfo[]{ discoveryProviderEntry };
        oneDomain = new String[]{ systemServicesDomain };
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        String[] gbids = new String[0];
        Future<DiscoveryEntryWithMetaInfo[]> discoveryEntryFuture = localDiscoveryAggregator.lookup(lookupCallbackWithModeledError,
                                                                                                    oneDomain,
                                                                                                    Discovery.INTERFACE_NAME,
                                                                                                    discoveryQos,
                                                                                                    gbids);
        ArgumentCaptor<DiscoveryEntry[]> discoveryEntriesCaptor = ArgumentCaptor.forClass(DiscoveryEntry[].class);
        verify(lookupCallbackWithModeledError).resolve((Object) discoveryEntriesCaptor.capture());
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
        verify(discoveryProxyMock,
               never()).lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                               any(String[].class),
                               anyString(),
                               any(DiscoveryQos.class),
                               Mockito.<String[]> any(),
                               any(MessagingQos.class));
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_findsProvisionedEntryForMultipleDomains() throws Exception {
        discoveryProviderEntries = new DiscoveryEntryWithMetaInfo[]{ anotherDiscoveryProviderEntry };
        allDomains = new String[]{ systemServicesDomain, anotherDomain };
        String[] missingDomains = new String[]{ anotherDomain };
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                @SuppressWarnings("unchecked")
                Callback<DiscoveryEntryWithMetaInfo[]> callback = (Callback<DiscoveryEntryWithMetaInfo[]>) invocation.getArguments()[0];
                callback.onSuccess(discoveryProviderEntries);
                return null;
            }
        }).when(discoveryProxyMock)
          .lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                  any(String[].class),
                  anyString(),
                  any(DiscoveryQos.class),
                  Mockito.<String[]> any(),
                  any(MessagingQos.class));

        String[] gbids = new String[0];
        Future<DiscoveryEntryWithMetaInfo[]> discoveryEntriesFuture = localDiscoveryAggregator.lookup(lookupCallbackWithModeledError,
                                                                                                      allDomains,
                                                                                                      Discovery.INTERFACE_NAME,
                                                                                                      discoveryQos,
                                                                                                      gbids);

        assertNotNull(discoveryEntriesFuture);
        DiscoveryEntry[] result = discoveryEntriesFuture.get();
        logger.info("Got discovery entries: {}", Arrays.toString(result));
        assertNotNull(result);
        assertEquals(2, result.length);
        assertTrue(containsByInterfaceDomain(result,
                                             discoveryProviderEntry.getInterfaceName(),
                                             discoveryProviderEntry.getDomain()));
        assertTrue(containsByInterfaceDomain(result,
                                             anotherDiscoveryProviderEntry.getInterfaceName(),
                                             anotherDiscoveryProviderEntry.getDomain()));
        verify(discoveryProxyMock).lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                          eq(missingDomains),
                                          eq(Discovery.INTERFACE_NAME),
                                          eq(discoveryQos),
                                          Mockito.<String[]> any(),
                                          eq(new MessagingQos(discoveryQos.getDiscoveryTimeout() + epsilonMs)));
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

    @Test
    public void lookupByDomainInterfaceWithGbids_doesNotQueryProxyForProvisionedEntry() {
        String[] gbids = new String[0];
        localDiscoveryAggregator.lookup(lookupCallbackWithModeledError,
                                        new String[]{ systemServicesDomain },
                                        Discovery.INTERFACE_NAME,
                                        new DiscoveryQos(),
                                        gbids);
        verify(discoveryProxyMock,
               never()).lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                               any(String[].class),
                               anyString(),
                               any(DiscoveryQos.class),
                               Mockito.<String[]> any(),
                               any(MessagingQos.class));
    }

    @Test
    public void lookupByParticipantIdWithGbids() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        DiscoveryQos expectedDiscoveryQos = new DiscoveryQos(discoveryQos);
        localDiscoveryAggregator.lookup(lookupByParticipantIdCallbackWithModeledError,
                                        discoveryProviderParticipantId,
                                        discoveryQos,
                                        gbids);

        verify(discoveryProxyMock,
               times(1)).lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo, DiscoveryError>> any(),
                                eq(discoveryProviderParticipantId),
                                eq(expectedDiscoveryQos),
                                eq(gbids),
                                eq(new MessagingQos(expectedDiscoveryQos.getDiscoveryTimeout() + epsilonMs)));
    }

    @Test
    public void lookupByParticipantId() {
        gbids = new String[0];

        localDiscoveryAggregator.lookup(lookupByParticipantIdCallbackWithModeledError, discoveryProviderParticipantId);

        verify(discoveryProxyMock,
               times(1)).lookup(ArgumentMatchers.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo, DiscoveryError>> any(),
                                eq(discoveryProviderParticipantId),
                                any(DiscoveryQos.class),
                                eq(gbids),
                                eq(new MessagingQos(new DiscoveryQos().getDiscoveryTimeout() + epsilonMs)));
    }

    @Test
    public void removeByParticipantId() {
        localDiscoveryAggregator.remove(callback, discoveryProviderParticipantId);
        verify(discoveryProxyMock, times(1)).remove(ArgumentMatchers.<Callback<Void>> any(),
                                                    eq(discoveryProviderParticipantId));
    }

    @Test
    public void forceQueryOfDiscoveryProxy() {
        localDiscoveryAggregator.forceQueryOfDiscoveryProxy();
        verify(proxyBuilderFactory).get(eq(systemServicesDomain), eq(DiscoveryProxy.class));
        MessagingQos expectedMessagingQos = new MessagingQos(MessagingQos.DEFAULT_TTL + 10000);
        verify(proxyBuilder).setMessagingQos(eq(expectedMessagingQos));
        verify(proxyBuilder).build();
    }
}

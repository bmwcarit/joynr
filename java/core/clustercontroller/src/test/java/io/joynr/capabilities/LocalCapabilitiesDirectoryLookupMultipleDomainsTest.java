/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.capabilities;

import static io.joynr.capabilities.CapabilityUtils.convertToDiscoveryEntrySet;
import static io.joynr.capabilities.CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.hamcrest.MockitoHamcrest.argThat;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.provider.Promise;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.system.DiscoveryProvider.Lookup1Deferred;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryLookupMultipleDomainsTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryLookupMultipleDomainsTest.class);

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_localOnly() throws InterruptedException {
        domains = new String[]{ DOMAIN_1, DOMAIN_2 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        final DiscoveryEntry firstEntry = createDiscoveryEntry(DOMAIN_1, PARTICIPANT_ID_1, new ProviderQos());
        final DiscoveryEntry secondEntry = createDiscoveryEntry(DOMAIN_2, PARTICIPANT_ID_2, new ProviderQos());
        final List<DiscoveryEntry> entries = List.of(firstEntry, secondEntry);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(entries);

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        final Object[] values = promiseChecker.checkPromiseSuccess(promise, MSG_LOOKUP_FAILED);
        assertEquals(2, ((DiscoveryEntryWithMetaInfo[]) values[0]).length);
        entries.forEach((entry) -> {
            verify(routingTable, times(1)).incrementReferenceCount(entry.getParticipantId());
            verify(routingTable, never()).put(eq(entry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        });
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_noneCached() throws InterruptedException {
        domains = new String[]{ DOMAIN_1, DOMAIN_2 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        when(globalDiscoveryEntryCacheMock.lookup(domains,
                                                  INTERFACE_NAME,
                                                  discoveryQos.getCacheMaxAge())).thenReturn(new ArrayList<>());
        doAnswer(answerCreateHelper.createLookupAnswer(new ArrayList<>())).when(globalCapabilitiesDirectoryClient)
                                                                          .lookup(any(),
                                                                                  argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                                                  eq(INTERFACE_NAME),
                                                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                                                  eq(knownGbids));

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise, 0);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_allCached() throws InterruptedException {
        domains = new String[]{ DOMAIN_1, DOMAIN_2 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        final List<GlobalDiscoveryEntry> entries = createGlobalDiscoveryEntriesForDomain(domains);

        when(globalDiscoveryEntryCacheMock.lookup(domains,
                                                  INTERFACE_NAME,
                                                  discoveryQos.getCacheMaxAge())).thenReturn(entries);

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(0, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise, 2); // 2 cached entries
        entries.forEach((entry) -> {
            verify(routingTable, never()).incrementReferenceCount(entry.getParticipantId());
            verify(routingTable, times(1)).put(eq(entry.getParticipantId()), any(Address.class), eq(true), anyLong());
        });
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_allLocalGlobal() throws InterruptedException {
        domains = new String[]{ "domain1", "domain2" };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        final List<GlobalDiscoveryEntry> entries = createGlobalDiscoveryEntriesForDomain(domains);
        final List<Promise<Add1Deferred>> promises = entries.stream().map(this::addEntry).collect(Collectors.toList());

        promises.forEach(promise -> {
            try {
                promiseChecker.checkPromiseSuccess(promise, MSG_ON_ADD_REJECT);
            } catch (final InterruptedException e) {
                fail(MSG_ON_ADD_REJECT + ": " + e);
            }
        });

        doReturn(entries).when(localDiscoveryEntryStoreMock).lookupGlobalEntries(domains, INTERFACE_NAME);

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(0, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise, 2); // 2 cached entries
        entries.forEach((entry) -> {
            verify(routingTable, times(1)).incrementReferenceCount(entry.getParticipantId());
            verify(routingTable, never()).put(eq(entry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        });
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_oneCached() throws InterruptedException {
        domains = new String[]{ DOMAIN_1, DOMAIN_2 };

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        final GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
        entry.setParticipantId(PARTICIPANT_ID_1);
        entry.setInterfaceName(INTERFACE_NAME);
        entry.setDomain(domains[0]);
        entry.setAddress(globalAddress1Serialized);
        doReturn(List.of(entry)).when(globalDiscoveryEntryCacheMock)
                                .lookup(domains, INTERFACE_NAME, discoveryQos.getCacheMaxAge());
        doReturn(Optional.of(entry)).when(globalDiscoveryEntryCacheMock).lookup(entry.getParticipantId(),
                                                                                Long.MAX_VALUE);
        doAnswer(answerCreateHelper.createLookupAnswer(Collections.emptyList())).when(globalCapabilitiesDirectoryClient)
                                                                                .lookup(any(),
                                                                                        eq(domains),
                                                                                        eq(INTERFACE_NAME),
                                                                                        eq(discoveryQos.getDiscoveryTimeout()),
                                                                                        eq(knownGbids));

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise, 1);
        verify(routingTable, never()).incrementReferenceCount(anyString());
        verify(routingTable, times(1)).put(eq(entry.getParticipantId()), any(Address.class), eq(true), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupMultipleDomains_localThenGlobal_oneLocalGlobalOneCached_sameParticipantIdsRemote() throws InterruptedException {
        final String localDomain = "localDomain";
        final String cachedDomain = "cachedDomain";
        final String remoteDomain = "remoteDomain";
        domains = new String[]{ localDomain, cachedDomain, remoteDomain };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        // local entry for participantId1 and domain1
        discoveryEntry.setParticipantId(PARTICIPANT_ID_1);
        discoveryEntry.setDomain(localDomain);
        doReturn(List.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                         .lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                 eq(INTERFACE_NAME));

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        // cached entry for participantId2 for cachedDomain
        final GlobalDiscoveryEntry cachedRemoteEntry = new GlobalDiscoveryEntry();
        cachedRemoteEntry.setParticipantId(PARTICIPANT_ID_2);
        cachedRemoteEntry.setInterfaceName(INTERFACE_NAME);
        cachedRemoteEntry.setDomain(cachedDomain);
        cachedRemoteEntry.setAddress(globalAddress1Serialized);
        doReturn(List.of(cachedRemoteEntry)).when(globalDiscoveryEntryCacheMock)
                                            .lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                    eq(INTERFACE_NAME),
                                                    eq(discoveryQos.getCacheMaxAge()));

        // remote entries for local provider and for remoteDomain for participantIds 2 and 3
        final GlobalDiscoveryEntry remoteEntry1 = discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                      globalAddressWithoutGbid);
        remoteEntry1.setDomain(remoteDomain);
        final GlobalDiscoveryEntry remoteEntry2 = new GlobalDiscoveryEntry(cachedRemoteEntry);
        remoteEntry2.setDomain(remoteDomain);
        remoteEntry2.setAddress(globalAddressWithoutGbidSerialized);
        final GlobalDiscoveryEntry remoteEntry3 = new GlobalDiscoveryEntry(cachedRemoteEntry);
        remoteEntry3.setParticipantId(PARTICIPANT_ID_3);
        remoteEntry3.setDomain(remoteDomain);
        remoteEntry3.setAddress(globalAddressWithoutGbidSerialized);
        doAnswer(answerCreateHelper.createLookupAnswer(List.of(remoteEntry1,
                                                               remoteEntry2,
                                                               remoteEntry3))).when(globalCapabilitiesDirectoryClient)
                                                                              .lookup(any(),
                                                                                      eq(domains),
                                                                                      eq(INTERFACE_NAME),
                                                                                      eq(discoveryQos.getDiscoveryTimeout()),
                                                                                      eq(knownGbids));

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        verify(localDiscoveryEntryStoreMock).lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                    eq(INTERFACE_NAME));
        verify(globalDiscoveryEntryCacheMock).lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                     eq(INTERFACE_NAME),
                                                     eq(discoveryQos.getCacheMaxAge()));
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(knownGbids));
        final Object[] values = verifyGcdLookupAndPromiseFulfillment(1,
                                                                     domains,
                                                                     discoveryQos.getDiscoveryTimeout(),
                                                                     knownGbids,
                                                                     promise,
                                                                     3);
        final DiscoveryEntryWithMetaInfo[] result = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(3, result.length);
        boolean discoveryEntryFound = false;
        boolean remoteEntry2Found = false;
        boolean remoteEntry3Found = false;
        for (final DiscoveryEntryWithMetaInfo entry : result) {
            if (isTheSame(entry, discoveryEntry, localDomain, true)) {
                discoveryEntryFound = true;
            }
            if (isTheSame(entry, remoteEntry2, remoteDomain, false)) {
                remoteEntry2Found = true;
            }
            if (isTheSame(entry, remoteEntry3, remoteDomain, false)) {
                remoteEntry3Found = true;
            }
        }
        verify(globalDiscoveryEntryCacheMock, never()).add(remoteEntry1);
        verify(globalDiscoveryEntryCacheMock).add(remoteEntry2);
        verify(globalDiscoveryEntryCacheMock).add(remoteEntry3);
        verify(routingTable, never()).put(eq(remoteEntry1.getParticipantId()),
                                          eq(globalAddressWithoutGbid),
                                          eq(true),
                                          anyLong());
        verify(routingTable, times(1)).incrementReferenceCount(remoteEntry1.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(remoteEntry2.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(remoteEntry3.getParticipantId());
        verify(routingTable).put(eq(remoteEntry2.getParticipantId()),
                                 eq(globalAddressWithoutGbid),
                                 eq(true),
                                 anyLong());
        verify(routingTable).put(eq(remoteEntry3.getParticipantId()),
                                 eq(globalAddressWithoutGbid),
                                 eq(true),
                                 anyLong());
        assertTrue(discoveryEntryFound && remoteEntry2Found && remoteEntry3Found);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_localThenGlobal() throws InterruptedException {
        domains = new String[]{ DOMAIN_1, DOMAIN_2, DOMAIN_3 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        final DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setParticipantId("participantIdLocal");
        localEntry.setDomain(domains[0]);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(List.of(localEntry));

        final GlobalDiscoveryEntry globalEntry = new GlobalDiscoveryEntry();
        globalEntry.setParticipantId("participantIdCached");
        globalEntry.setInterfaceName(INTERFACE_NAME);
        globalEntry.setDomain(domains[1]);
        globalEntry.setAddress(globalAddress1Serialized);
        doReturn(List.of(globalEntry)).when(globalDiscoveryEntryCacheMock)
                                      .lookup(domains, INTERFACE_NAME, discoveryQos.getCacheMaxAge());
        doReturn(Optional.of(globalEntry)).when(globalDiscoveryEntryCacheMock).lookup(globalEntry.getParticipantId(),
                                                                                      Long.MAX_VALUE);

        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(build0_0Version(),
                                                                                domains[2],
                                                                                INTERFACE_NAME,
                                                                                "participantIdRemote",
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                PUBLIC_KEY_ID,
                                                                                globalAddress1Serialized);

        mockGcdLookup(List.of(remoteGlobalEntry), knownGbids);

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(knownGbids));
        final Object[] values = promiseChecker.checkPromiseSuccess(promise, MSG_LOOKUP_FAILED);
        final Collection<DiscoveryEntry> captured = convertToDiscoveryEntrySet(List.of((DiscoveryEntryWithMetaInfo[]) values[0]));
        assertNotNull(captured);
        assertEquals(3, captured.size());
        assertTrue(captured.contains(localEntry));
        assertTrue(captured.contains(new DiscoveryEntry(globalEntry)));
        assertTrue(captured.contains(new DiscoveryEntry(remoteGlobalEntry)));
        verify(routingTable, times(1)).incrementReferenceCount(localEntry.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(globalEntry.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(remoteGlobalEntry.getParticipantId());
        verify(routingTable, never()).put(eq(localEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
        verify(routingTable, times(1)).put(eq(globalEntry.getParticipantId()), any(Address.class), eq(true), anyLong());
        verify(routingTable, times(1)).put(eq(remoteGlobalEntry.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupMultipleDomains_localThenGlobal_oneLocalAllCachedDomains_returnsLocalAndCachedEntries() throws InterruptedException {
        domains = new String[]{ DOMAIN_1, DOMAIN_2 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        final DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setParticipantId("participantIdLocal");
        localEntry.setDomain(domains[0]);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(List.of(localEntry));

        final GlobalDiscoveryEntry globalCachedEntry1 = new GlobalDiscoveryEntry();
        globalCachedEntry1.setParticipantId("participantIdCached1");
        globalCachedEntry1.setInterfaceName(INTERFACE_NAME);
        globalCachedEntry1.setDomain(domains[0]);
        globalCachedEntry1.setAddress(globalAddress1Serialized);

        final GlobalDiscoveryEntry globalCachedEntry2 = new GlobalDiscoveryEntry();
        globalCachedEntry2.setParticipantId("participantIdCached2");
        globalCachedEntry2.setInterfaceName(INTERFACE_NAME);
        globalCachedEntry2.setDomain(domains[1]);
        globalCachedEntry2.setAddress(globalAddress1Serialized);

        final Set<GlobalDiscoveryEntry> globalCachedEntries = Set.of(globalCachedEntry1, globalCachedEntry2);

        doReturn(globalCachedEntries).when(globalDiscoveryEntryCacheMock)
                                     .lookup(domains, INTERFACE_NAME, discoveryQos.getCacheMaxAge());

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);
        final Object[] values = promiseChecker.checkPromiseSuccess(promise, MSG_LOOKUP_FAILED);

        verify(localDiscoveryEntryStoreMock).lookup(domains, INTERFACE_NAME);
        verify(globalDiscoveryEntryCacheMock).lookup(domains, INTERFACE_NAME, ONE_DAY_IN_MS);
        verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any());

        final Collection<DiscoveryEntry> captured = convertToDiscoveryEntrySet(List.of((DiscoveryEntryWithMetaInfo[]) values[0]));
        assertEquals(3, captured.size());
        assertTrue(captured.contains(localEntry));
        assertTrue(captured.contains(new DiscoveryEntry(globalCachedEntry1)));
        assertTrue(captured.contains(new DiscoveryEntry(globalCachedEntry2)));
        verify(routingTable, times(1)).incrementReferenceCount(localEntry.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(globalCachedEntry1.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(globalCachedEntry2.getParticipantId());
        verify(routingTable, never()).put(eq(localEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
        verify(routingTable, times(1)).put(eq(globalCachedEntry1.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        verify(routingTable, times(1)).put(eq(globalCachedEntry2.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupMultipleDomains_localThenGlobal_allDomainsLocal_returnsOnlyLocalEntries() throws InterruptedException {
        domains = new String[]{ DOMAIN_1, DOMAIN_2 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        final DiscoveryEntry localEntry1 = new DiscoveryEntry();
        localEntry1.setParticipantId("participantIdLocal1");
        localEntry1.setDomain(domains[0]);

        final DiscoveryEntry localEntry2 = new DiscoveryEntry();
        localEntry2.setParticipantId("participantIdLocal2");
        localEntry2.setDomain(domains[1]);

        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(List.of(localEntry1,
                                                                                              localEntry2));

        final GlobalDiscoveryEntry globalCachedEntry = new GlobalDiscoveryEntry();
        globalCachedEntry.setParticipantId("participantIdCached1");
        globalCachedEntry.setInterfaceName(INTERFACE_NAME);
        globalCachedEntry.setDomain(domains[0]);
        globalCachedEntry.setAddress(globalAddress1Serialized);

        doReturn(List.of(globalCachedEntry)).when(globalDiscoveryEntryCacheMock)
                                            .lookup(domains, INTERFACE_NAME, discoveryQos.getCacheMaxAge());

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);
        final Object[] values = promiseChecker.checkPromiseSuccess(promise, MSG_LOOKUP_FAILED);

        verify(localDiscoveryEntryStoreMock).lookup(domains, INTERFACE_NAME);
        verify(globalDiscoveryEntryCacheMock).lookup(domains, INTERFACE_NAME, ONE_DAY_IN_MS);
        verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any());

        final Collection<DiscoveryEntry> captured = convertToDiscoveryEntrySet(List.of((DiscoveryEntryWithMetaInfo[]) values[0]));
        assertEquals(2, captured.size());
        assertTrue(captured.contains(localEntry1));
        assertTrue(captured.contains(localEntry2));
        assertFalse(captured.contains(new DiscoveryEntry(globalCachedEntry)));
        captured.forEach((entry) -> {
            verify(routingTable, times(1)).incrementReferenceCount(entry.getParticipantId());
            verify(routingTable, never()).put(eq(entry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        });
        verify(routingTable, never()).put(eq(globalCachedEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
    }

    private DiscoveryEntry createDiscoveryEntry(final String domain,
                                                final String participantId,
                                                final ProviderQos providerQos) {
        return new DiscoveryEntry(build0_0Version(),
                                  domain,
                                  INTERFACE_NAME,
                                  participantId,
                                  providerQos,
                                  System.currentTimeMillis(),
                                  expiryDateMs,
                                  INTERFACE_NAME);
    }

    private GlobalDiscoveryEntry createGlobalDiscoveryEntryForDomain(final String domain) {
        final GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
        entry.setParticipantId("participantIdFor-" + domain);
        entry.setDomain(domain);
        entry.setAddress(globalAddress1Serialized);
        return entry;
    }

    private List<GlobalDiscoveryEntry> createGlobalDiscoveryEntriesForDomain(final String[] domains) {
        return Arrays.stream(domains).map(this::createGlobalDiscoveryEntryForDomain).collect(Collectors.toList());
    }

    private Promise<Add1Deferred> addEntry(final GlobalDiscoveryEntry entry) {
        return localCapabilitiesDirectory.add(entry, true, knownGbids);
    }

    private boolean isTheSame(final DiscoveryEntryWithMetaInfo actual,
                              final DiscoveryEntry expected,
                              final String expectedDomain,
                              final boolean isLocal) {
        return Objects.equals(actual.getParticipantId(), expected.getParticipantId())
                && Objects.equals(actual.getDomain(), expectedDomain) && actual.getIsLocal() == isLocal;
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}

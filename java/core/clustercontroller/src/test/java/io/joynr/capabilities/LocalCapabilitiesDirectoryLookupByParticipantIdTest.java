/*
 * #%L
 * %%
 * Copyright (C) 2022-2023 BMW Car IT GmbH
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

import static io.joynr.capabilities.CapabilityUtils.convertToDiscoveryEntryWithMetaInfo;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.util.Optional;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.provider.Promise;
import io.joynr.proxy.Callback;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.system.DiscoveryProvider.Lookup3Deferred;
import joynr.system.DiscoveryProvider.Lookup4Deferred;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryLookupByParticipantIdTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryLookupByParticipantIdTest.class);

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantId_defaultScopeLocalAndGlobal_localEntry() throws InterruptedException {
        discoveryQos = new DiscoveryQos(Long.MAX_VALUE, Long.MAX_VALUE, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        final DiscoveryEntryWithMetaInfo expectedDiscoveryEntryWithMetaInfo = convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                  expectedDiscoveryEntry);

        // add locally registered entry
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        when(localDiscoveryEntryStoreMock.lookup(expectedDiscoveryEntry.getParticipantId(),
                                                 discoveryQos.getCacheMaxAge())).thenReturn(Optional.of(discoveryEntry));

        reset(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        final Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(expectedDiscoveryEntry.getParticipantId());

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo retrievedCapabilityEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(expectedDiscoveryEntryWithMetaInfo, retrievedCapabilityEntry);

        verify(localDiscoveryEntryStoreMock).lookup(expectedDiscoveryEntry.getParticipantId(), Long.MAX_VALUE);
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        verify(routingTable, times(1)).incrementReferenceCount(discoveryEntry.getParticipantId());
        verify(routingTable, never()).put(eq(discoveryEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantId_emptyGbid_replacesReturnedGbidsWithEmpty() throws InterruptedException {
        final String[] gbids = new String[]{ "" };

        Module injectionModule = Modules.override(createBaseInjectionModule()).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY)).toInstance(gbids);
            }
        });
        final LocalCapabilitiesDirectory localCapabilitiesDirectoryWithEmptyGbids = Guice.createInjector(injectionModule)
                                                                                         .getInstance(LocalCapabilitiesDirectory.class);

        discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        // add global entry
        final String globalParticipantId = "globalParticipant";
        final GlobalDiscoveryEntry capInfo = createGlobalDiscoveryEntry(globalParticipantId, new ProviderQos());
        mockGcdLookup(capInfo, globalParticipantId, discoveryQos.getDiscoveryTimeout(), gbids);
        final Promise<Lookup4Deferred> promise = localCapabilitiesDirectoryWithEmptyGbids.lookup(globalParticipantId,
                                                                                                 discoveryQos,
                                                                                                 new String[]{});
        promiseChecker.checkPromiseSuccess(promise, MSG_LOOKUP_FAILED);
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(globalParticipantId),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(gbids));
        verify(routingTable, never()).incrementReferenceCount(any());
        final ArgumentCaptor<Address> addressCaptor = ArgumentCaptor.forClass(Address.class);
        verify(routingTable, times(1)).put(eq(globalParticipantId), addressCaptor.capture(), eq(true), anyLong());
        final MqttAddress address = (MqttAddress) addressCaptor.getValue();
        assertEquals(gbids[0], address.getBrokerUri());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_globalOnly_filtersLocalEntriesByGbids() throws InterruptedException {
        discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        final DiscoveryEntry localEntry = createDiscoveryEntry(PARTICIPANT_ID_1, new ProviderQos());
        final DiscoveryEntry localStoreEntry = new DiscoveryEntry(localEntry);
        final DiscoveryEntryWithMetaInfo expectedLocalEntry = convertToDiscoveryEntryWithMetaInfo(true, localEntry);
        // register in knownGbids[1]
        final Promise<Add1Deferred> promiseAdd = addEntry(localEntry, true, knownGbids[1]);

        promiseChecker.checkPromiseSuccess(promiseAdd, MSG_ON_ADD_REJECT);
        reset(localDiscoveryEntryStoreMock, globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);

        doReturn(Optional.of(localStoreEntry)).when(localDiscoveryEntryStoreMock)
                                              .lookup(expectedLocalEntry.getParticipantId(), Long.MAX_VALUE);

        // lookup knownGbids[1], expect local entry
        final Promise<Lookup4Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(expectedLocalEntry.getParticipantId(),
                                                                                          discoveryQos,
                                                                                          new String[]{
                                                                                                  knownGbids[1] });

        final DiscoveryEntryWithMetaInfo result1 = (DiscoveryEntryWithMetaInfo) promiseChecker.checkPromiseSuccess(promiseLookup1,
                                                                                                                   MSG_LOOKUP_FAILED)[0];
        verify(localDiscoveryEntryStoreMock, times(1)).lookup(expectedLocalEntry.getParticipantId(), Long.MAX_VALUE);
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        assertEquals(expectedLocalEntry, result1);
        verify(routingTable, times(1)).incrementReferenceCount(expectedLocalEntry.getParticipantId());

        // lookup knownGbids[0], expect DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS
        final Promise<Lookup4Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(expectedLocalEntry.getParticipantId(),
                                                                                          discoveryQos,
                                                                                          new String[]{
                                                                                                  knownGbids[0] });

        promiseChecker.checkPromiseError(promiseLookup2, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
        verify(localDiscoveryEntryStoreMock, times(2)).lookup(expectedLocalEntry.getParticipantId(), Long.MAX_VALUE);
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        verify(routingTable, times(1)).incrementReferenceCount(expectedLocalEntry.getParticipantId());

        // lookup all gbids, expect local entry
        final Promise<Lookup4Deferred> promiseLookup3 = localCapabilitiesDirectory.lookup(expectedLocalEntry.getParticipantId(),
                                                                                          discoveryQos,
                                                                                          knownGbids);

        final DiscoveryEntryWithMetaInfo result3 = (DiscoveryEntryWithMetaInfo) promiseChecker.checkPromiseSuccess(promiseLookup3,
                                                                                                                   MSG_LOOKUP_FAILED)[0];
        verify(localDiscoveryEntryStoreMock, times(3)).lookup(expectedLocalEntry.getParticipantId(), Long.MAX_VALUE);
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        assertEquals(expectedLocalEntry, result3);
        verify(routingTable, times(2)).incrementReferenceCount(expectedLocalEntry.getParticipantId());
        verify(routingTable, never()).put(eq(expectedLocalEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        testLookupByParticipantIdWithGbids_globalOnly_allCached(gbidsForLookup);
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_emptyGbidsArray_allCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[0];

        testLookupByParticipantIdWithGbids_globalOnly_allCached(gbidsForLookup);
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        testLookupByParticipantIdWithGbids_globalOnly_noneCached(gbidsForLookup, gbidsForLookup.clone());

        final String[] gbidsForLookup2 = new String[]{ knownGbids[2], knownGbids[0] };

        testLookupByParticipantIdWithGbids_globalOnly_noneCached(gbidsForLookup2, gbidsForLookup2.clone());
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_emptyGbidsArray_noneCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[0];

        testLookupByParticipantIdWithGbids_globalOnly_noneCached(gbidsForLookup, knownGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdGbids_globalOnlyWithCache_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry() throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        lookupByParticipantIdGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry(10000L);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdGbids_globalOnlyNoCache_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry() throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        lookupByParticipantIdGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry(0L);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue_localEntry() throws Exception {
        final String participantId = "participantId";
        discoveryQos = new DiscoveryQos(Long.MAX_VALUE, Long.MAX_VALUE, DiscoveryScope.LOCAL_ONLY, false);

        // local DiscoveryEntry
        final String localDomain = "localDomain";
        final DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setDomain(localDomain);
        localEntry.setInterfaceName(INTERFACE_NAME);
        localEntry.setParticipantId(participantId);
        final DiscoveryEntryWithMetaInfo localEntryWithMetaInfo = convertToDiscoveryEntryWithMetaInfo(true, localEntry);
        when(localDiscoveryEntryStoreMock.lookup(participantId,
                                                 discoveryQos.getCacheMaxAge())).thenReturn(Optional.of(localEntry));

        final Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo capturedLocalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(localEntryWithMetaInfo, capturedLocalEntry);
        verify(routingTable, times(1)).incrementReferenceCount(capturedLocalEntry.getParticipantId());
        verify(routingTable, never()).put(eq(capturedLocalEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue_cachedEntry() throws Exception {
        final String participantId = discoveryEntry.getParticipantId();

        // cached global DiscoveryEntry
        final String globalDomain = "globalDomain";
        final GlobalDiscoveryEntry cachedGlobalEntry = new GlobalDiscoveryEntry();
        cachedGlobalEntry.setDomain(globalDomain);
        cachedGlobalEntry.setInterfaceName(INTERFACE_NAME);
        cachedGlobalEntry.setParticipantId(participantId);
        cachedGlobalEntry.setAddress(globalAddress1Serialized);
        final DiscoveryEntryWithMetaInfo cachedGlobalEntryWithMetaInfo = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                             cachedGlobalEntry);
        when(globalDiscoveryEntryCacheMock.lookup(participantId,
                                                  Long.MAX_VALUE)).thenReturn(Optional.of(cachedGlobalEntry));
        final Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo capturedCachedGlobalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(cachedGlobalEntryWithMetaInfo, capturedCachedGlobalEntry);
        verify(routingTable, never()).incrementReferenceCount(capturedCachedGlobalEntry.getParticipantId());
        verify(routingTable, times(1)).put(eq(capturedCachedGlobalEntry.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localOnly_noLocalEntry_doesNotInvokeGcd_returnsNoEntryForParticipant() throws Exception {
        final String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.LOCAL_ONLY, false);

        final Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        promiseChecker.checkPromiseError(lookupPromise, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localOnly_localGlobalEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_ONLY;
        final boolean localEntryAvailable = true;
        final boolean invokesGcd = false;
        final boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localThenGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        final boolean localEntryAvailable = false;
        final boolean invokesGcd = true;
        final boolean returnsLocalEntry = false;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localThenGlobal_localGlobalEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        final boolean localEntryAvailable = true;
        final boolean invokesGcd = false;
        final boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localAndGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        final boolean localEntryAvailable = false;
        final boolean invokesGcd = true;
        final boolean returnsLocalEntry = false;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localAndGlobal_localGlobalEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        final boolean localEntryAvailable = true;
        final boolean invokesGcd = false;
        final boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_globalOnly_localGlobalEntry_doesNotInvokeGcd_returnsLocalResult() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        final boolean localEntryAvailable = true;
        final boolean invokesGcd = false;
        final boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_globalOnly_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        final boolean localEntryAvailable = false;
        final boolean invokesGcd = true;
        final boolean returnsLocalEntry = false;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_globalOnly_localOnlyEntry_doesNotInvokeGcd_noEntryForParticipant() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.GLOBAL_ONLY;

        final String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, discoveryScope, false);
        // register local only
        setProviderQos(discoveryEntry, ProviderScope.LOCAL);
        final Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(discoveryEntry, true, knownGbids);
        promiseChecker.checkPromiseSuccess(promiseAdd, MSG_ON_ADD_REJECT);
        reset(localDiscoveryEntryStoreMock, globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(participantId, Long.MAX_VALUE);

        final Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        promiseChecker.checkPromiseError(lookupPromise, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(localDiscoveryEntryStoreMock).lookup(participantId, Long.MAX_VALUE);
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_respectsCacheMaxAge() {
        final String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        localCapabilitiesDirectory.lookup(participantId, discoveryQos, new String[0]);

        verify(globalDiscoveryEntryCacheMock).lookup(participantId, cacheMaxAge);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue_globalEntry() throws Exception {
        final String participantId = "participantId";
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        // remote global DiscoveryEntry
        final String remoteGlobalDomain = "remoteglobaldomain";
        final GlobalDiscoveryEntry remoteGlobalEntry = createGlobalDiscoveryEntry(remoteGlobalDomain, participantId);

        final DiscoveryEntryWithMetaInfo remoteGlobalEntryWithMetaInfo = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                             remoteGlobalEntry);
        doAnswer((Answer<Void>) invocation -> {
            @SuppressWarnings("unchecked")
            final Callback<GlobalDiscoveryEntry> callback = (Callback<GlobalDiscoveryEntry>) invocation.getArguments()[0];
            callback.onSuccess(remoteGlobalEntry);
            return null;
        }).when(globalCapabilitiesDirectoryClient).lookup(any(), eq(participantId), anyLong(), eq(knownGbids));

        final Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo capturedRemoteGlobalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(remoteGlobalEntryWithMetaInfo, capturedRemoteGlobalEntry);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, times(1)).put(eq(remoteGlobalEntryWithMetaInfo.getParticipantId()),
                                           any(Address.class),
                                           any(Boolean.class),
                                           anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryQosTtlIsUsed() throws Exception {
        final String participantId = "participantId";
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        final long discoveryTimeout = 1000000000;
        discoveryQos.setDiscoveryTimeout(discoveryTimeout);

        // remote global DiscoveryEntry
        final String remoteGlobalDomain = "remoteglobaldomain";
        final GlobalDiscoveryEntry remoteGlobalEntry = createGlobalDiscoveryEntry(remoteGlobalDomain, participantId);
        doAnswer((Answer<Void>) invocation -> {
            @SuppressWarnings("unchecked")
            final Callback<GlobalDiscoveryEntry> callback = (Callback<GlobalDiscoveryEntry>) invocation.getArguments()[0];
            callback.onSuccess(remoteGlobalEntry);
            return null;
        }).when(globalCapabilitiesDirectoryClient).lookup(any(), eq(participantId), anyLong(), eq(knownGbids));

        final Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(participantId), eq(discoveryTimeout), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, times(1)).put(eq(participantId), any(Address.class), any(Boolean.class), anyLong());
    }

    private void lookupByParticipantIdDiscoveryScopeTest(final DiscoveryScope discoveryScope,
                                                         final boolean localEntryAvailable,
                                                         final boolean invokesGcd,
                                                         final boolean returnsLocalEntry) throws Exception {
        final String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, discoveryScope, false);
        if (localEntryAvailable) {
            // register in all gbids
            final Promise<Add1Deferred> promiseAdd = addEntry(discoveryEntry, true, knownGbids);
            promiseChecker.checkPromiseSuccess(promiseAdd, MSG_ON_ADD_REJECT);

            doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(participantId,
                                                                                            Long.MAX_VALUE);
        }
        mockGcdLookup(globalDiscoveryEntry);

        final Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo capturedDiscoveryEntry = (DiscoveryEntryWithMetaInfo) values[0];
        if (invokesGcd) {
            verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(participantId), eq(discoveryTimeout), any());
        } else {
            verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any());
        }
        if (returnsLocalEntry) {
            final DiscoveryEntryWithMetaInfo expectedLocalDiscoveryEntry = convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                               discoveryEntry);
            assertEquals(expectedLocalDiscoveryEntry, capturedDiscoveryEntry);
            verify(routingTable, times(1)).incrementReferenceCount(expectedLocalDiscoveryEntry.getParticipantId());
            verify(routingTable, never()).put(eq(expectedLocalDiscoveryEntry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        } else {
            final DiscoveryEntryWithMetaInfo expectedGlobalDiscoveryEntry = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                globalDiscoveryEntry);
            assertEquals(expectedGlobalDiscoveryEntry, capturedDiscoveryEntry);
            verify(routingTable, never()).incrementReferenceCount(any());
            verify(routingTable, times(1)).put(eq(expectedGlobalDiscoveryEntry.getParticipantId()),
                                               any(Address.class),
                                               eq(true),
                                               anyLong());
        }
    }

    private void lookupByParticipantIdGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry(final long cacheMaxAge) throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);
        mockGcdLookup(globalDiscoveryEntry);

        doAnswer(new Answer<Optional<DiscoveryEntry>>() {
            // simulate provider registration after remote lookup has been triggered
            boolean firstCall = true;

            @Override
            public Optional<DiscoveryEntry> answer(final InvocationOnMock invocation) {
                if (firstCall) {
                    firstCall = false;
                    return Optional.empty();
                }
                return Optional.of(discoveryEntry);
            }
        }).when(localDiscoveryEntryStoreMock).lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        final Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(discoveryEntry.getParticipantId(),
                                                                                         discoveryQos,
                                                                                         new String[0]);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo capturedDiscoveryEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertTrue(capturedDiscoveryEntry.getIsLocal());
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(discoveryEntry.getParticipantId()),
                                                         eq(discoveryTimeout),
                                                         any());
        verify(localDiscoveryEntryStoreMock, times(2)).lookup(eq(discoveryEntry.getParticipantId()), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(routingTable, times(1)).incrementReferenceCount(discoveryEntry.getParticipantId());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    private void testLookupByParticipantIdWithGbids_globalOnly_noneCached(final String[] gbidsForLookup,
                                                                          final String[] expectedGbids) throws InterruptedException {
        final String participantId = discoveryEntry.getParticipantId();
        discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        doReturn(Optional.empty()).when(globalDiscoveryEntryCacheMock).lookup(eq(participantId), anyLong());
        mockGcdLookup(globalDiscoveryEntry, participantId, discoveryQos.getDiscoveryTimeout(), expectedGbids);

        doReturn(true).when(routingTable).put(eq(participantId), any(Address.class), any(Boolean.class), anyLong());

        final Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                         discoveryQos,
                                                                                         gbidsForLookup);

        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(participantId),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(expectedGbids));
        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo foundEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(participantId, foundEntry.getParticipantId());
        verify(routingTable, times(1)).put(eq(foundEntry.getParticipantId()), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(anyString());
        reset((Object) routingTable);
    }

    private void testLookupByParticipantIdWithGbids_globalOnly_allCached(final String[] gbidsForLookup) throws InterruptedException {
        final String participantId = discoveryEntry.getParticipantId();
        discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        doReturn(Optional.of(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock).lookup(eq(participantId),
                                                                                               anyLong());

        final Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                         discoveryQos,
                                                                                         gbidsForLookup);
        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo foundEntry = (DiscoveryEntryWithMetaInfo) values[0];

        assertEquals(participantId, foundEntry.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(globalDiscoveryEntry.getParticipantId());
        verify(routingTable, times(1)).put(eq(globalDiscoveryEntry.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        reset((Object) routingTable);

        verify(globalCapabilitiesDirectoryClient, times(0)).lookup(any(), anyString(), anyLong(), any(String[].class));
    }

    @SuppressWarnings("SameParameterValue")
    private GlobalDiscoveryEntry createGlobalDiscoveryEntry(final String domain, final String participantId) {
        return new GlobalDiscoveryEntry(build0_0Version(),
                                        domain,
                                        INTERFACE_NAME,
                                        participantId,
                                        new ProviderQos(),
                                        System.currentTimeMillis(),
                                        System.currentTimeMillis() + 10000L,
                                        PUBLIC_KEY_ID,
                                        globalAddress1Serialized);
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
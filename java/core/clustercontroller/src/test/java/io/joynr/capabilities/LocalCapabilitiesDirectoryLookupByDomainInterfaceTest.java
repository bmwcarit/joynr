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

import static io.joynr.capabilities.CapabilityUtils.convertToDiscoveryEntryWithMetaInfo;
import static io.joynr.capabilities.CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry;
import static java.util.Collections.emptyList;
import static java.util.Collections.singletonList;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.capabilities.helpers.DiscoveryEntryMatchHelper;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.system.DiscoveryProvider.Lookup1Deferred;
import joynr.system.DiscoveryProvider.Lookup2Deferred;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.util.Modules;
import com.google.inject.Module;
import com.google.inject.name.Names;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryLookupByDomainInterfaceTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryLookupByDomainInterfaceTest.class);

    private static final String LOCAL_PARTICIPANT_ID = "localParticipant";
    private static final String GLOBAL_PARTICIPANT_ID = "globalParticipant";
    private static final String GLOBAL_PARTICIPANT_2_ID = "globalParticipant2";

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_globalOnly() throws InterruptedException {
        final List<GlobalDiscoveryEntry> caps = new ArrayList<>();

        when(globalDiscoveryEntryCacheMock.lookup(domains,
                                                  INTERFACE_NAME,
                                                  discoveryQos.getCacheMaxAge())).thenReturn(new ArrayList<>());

        mockGcdLookup(caps, domains, discoveryQos, knownGbids);
        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise, 0);
        verify(routingTable, never()).incrementReferenceCount(any());

        // add local entry
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        final DiscoveryEntry discoveryEntry = createDiscoveryEntry(LOCAL_PARTICIPANT_ID, providerQos);
        final boolean awaitGlobalRegistration = true;
        final Promise<DeferredVoid> promiseAdd = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promiseAdd, MSG_ON_ADD_REJECT);
        final Promise<Lookup1Deferred> promise2 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise2, 0);
        verify(routingTable, never()).incrementReferenceCount(any());

        // even deleting local cap entries shall have no effect, the global cap dir shall be invoked
        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(localDiscoveryEntryStoreMock).remove(discoveryEntry.getParticipantId());
        final Promise<Lookup1Deferred> promise3 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise3, 0);
        verify(routingTable, never()).incrementReferenceCount(any());

        // add global entry
        final GlobalDiscoveryEntry capInfo = createGlobalDiscoveryEntry(GLOBAL_PARTICIPANT_ID, new ProviderQos());
        caps.add(capInfo);

        mockGcdLookup(caps, domains, discoveryQos, knownGbids);
        final Promise<Lookup1Deferred> promise4 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise4, 1); // 1 global entry
        verify(routingTable, times(1)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(any());

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        reset((Object) globalDiscoveryEntryCacheMock);
        reset((Object) routingTable);

        when(globalDiscoveryEntryCacheMock.lookup(domains,
                                                  INTERFACE_NAME,
                                                  discoveryQos.getCacheMaxAge())).thenReturn(List.of(capInfo));

        doReturn(true).when(routingTable).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());
        final Promise<Lookup1Deferred> promise5 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise5, 1); // 1 cached entry
        verify(routingTable, times(1)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(any());
        reset((Object) routingTable);

        // and now, invalidate the existing cached global values, resulting in another call to globalCapClient
        discoveryQos.setCacheMaxAge(0L);
        sleep1ms();

        doReturn(true).when(routingTable).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());
        // now, another lookup call shall call the globalCapabilitiesDirectoryClient, as the global cap dir is expired
        final Promise<Lookup1Deferred> promise6 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(5, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise6, 1); // 1 global entry
        verify(routingTable, times(1)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(any());
        reset(globalCapabilitiesDirectoryClient);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_localThenGlobal() throws InterruptedException {
        final List<GlobalDiscoveryEntry> caps = new ArrayList<>();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        mockGcdLookup(caps, domains, discoveryQos, knownGbids);
        final Promise<Lookup1Deferred> promise1 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise1, 0);
        verify(routingTable, never()).incrementReferenceCount(any());

        // add local entry
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        final DiscoveryEntry discoveryEntry = createDiscoveryEntry(LOCAL_PARTICIPANT_ID, providerQos);
        reset((Object) localDiscoveryEntryStoreMock);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(List.of(discoveryEntry));
        final Promise<Lookup1Deferred> promise2 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise2, 1); // 1 local entry
        verify(routingTable, times(1)).incrementReferenceCount(LOCAL_PARTICIPANT_ID);

        // add global entry
        final GlobalDiscoveryEntry capInfo = createGlobalDiscoveryEntry(GLOBAL_PARTICIPANT_ID, new ProviderQos());
        caps.add(capInfo);
        final Promise<Lookup1Deferred> promise3 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise3, 1); // 1 local entry
        verify(routingTable, times(2)).incrementReferenceCount(LOCAL_PARTICIPANT_ID);
        verify(routingTable, never()).put(anyString(), any(Address.class), eq(true), anyLong());

        // without local entry, the global cap dir is called
        reset((Object) localDiscoveryEntryStoreMock);
        when(localDiscoveryEntryStoreMock.lookup(anyString(), anyLong())).thenReturn(Optional.empty());
        final Promise<Lookup1Deferred> promise4 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise4, 1); // 1 global entry
        verify(routingTable, times(1)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        when(globalDiscoveryEntryCacheMock.lookup(domains,
                                                  INTERFACE_NAME,
                                                  discoveryQos.getCacheMaxAge())).thenReturn(List.of(capInfo));
        final Promise<Lookup1Deferred> promise5 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise5, 1); // 1 cached entry
        verify(routingTable, times(2)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());

        // and now, invalidate the existing cached global values, resulting in another call to globalCapClient
        discoveryQos.setCacheMaxAge(0L);
        sleep1ms();

        final Promise<Lookup1Deferred> promise6 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise6, 1); // 1 global entry
        verify(routingTable, times(3)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_localAndGlobal() throws InterruptedException {
        final List<GlobalDiscoveryEntry> globalEntries = new ArrayList<>();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_AND_GLOBAL);

        mockGcdLookup(globalEntries, domains, discoveryQos, knownGbids);
        final Promise<Lookup1Deferred> promise1 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise1, 0);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        // add local entry
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        final DiscoveryEntry discoveryEntry = createDiscoveryEntry(LOCAL_PARTICIPANT_ID, providerQos);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(List.of(discoveryEntry));
        final Promise<Lookup1Deferred> promise2 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise2, 1); // 1 local entry
        verify(routingTable, times(1)).incrementReferenceCount(LOCAL_PARTICIPANT_ID);
        verify(routingTable, never()).put(anyString(), any(Address.class), eq(true), anyLong());

        // add global entry
        final GlobalDiscoveryEntry capInfo = createGlobalDiscoveryEntry(GLOBAL_PARTICIPANT_ID, new ProviderQos());
        globalEntries.add(capInfo);
        mockGcdLookup(globalEntries, domains, discoveryQos, knownGbids);
        final Promise<Lookup1Deferred> promise3 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise3, 2); // 1 local, 1 global entry
        verify(routingTable, times(2)).incrementReferenceCount(LOCAL_PARTICIPANT_ID);
        verify(routingTable, times(1)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        when(globalDiscoveryEntryCacheMock.lookup(domains,
                                                  INTERFACE_NAME,
                                                  discoveryQos.getCacheMaxAge())).thenReturn(List.of(capInfo));
        final Promise<Lookup1Deferred> promise4 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise4, 2); // 1 local, 1 cached entry
        verify(routingTable, times(3)).incrementReferenceCount(LOCAL_PARTICIPANT_ID);
        verify(routingTable, times(2)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());

        // and now, invalidate the existing cached global values, resulting in another call to globalCapClient
        discoveryQos.setCacheMaxAge(0L);
        sleep1ms();

        final Promise<Lookup1Deferred> promise5 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4, domains, discoveryQos.getDiscoveryTimeout(), knownGbids, promise5, 2); // 1 local, 1 global entry
        verify(routingTable, times(4)).incrementReferenceCount(LOCAL_PARTICIPANT_ID);
        verify(routingTable, times(3)).put(eq(GLOBAL_PARTICIPANT_ID), any(Address.class), eq(true), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_emptyGbid_replacesReturnedGbidsWithEmpty() throws InterruptedException {
        final String[] gbids = new String[]{ "" };

        Module injectionModule = Modules.override(createBaseInjectionModule()).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY)).toInstance(gbids);
            }
        });
        final LocalCapabilitiesDirectory localCapabilitiesDirectoryWithEmptyGbids = Guice.createInjector(injectionModule)
                                                                                         .getInstance(LocalCapabilitiesDirectory.class);

        final List<GlobalDiscoveryEntry> globalEntries = new ArrayList<>();
        discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        // add global entries
        GlobalDiscoveryEntry capInfo = createGlobalDiscoveryEntry(GLOBAL_PARTICIPANT_ID, new ProviderQos());
        globalEntries.add(capInfo);

        final GlobalDiscoveryEntry capInfo2 = createGlobalDiscoveryEntry(GLOBAL_PARTICIPANT_2_ID, new ProviderQos());
        globalEntries.add(capInfo2);

        mockGcdLookup(globalEntries, domains, discoveryQos, gbids);
        final Promise<Lookup2Deferred> promise = localCapabilitiesDirectoryWithEmptyGbids.lookup(domains,
                                                                                                 INTERFACE_NAME,
                                                                                                 discoveryQos,
                                                                                                 new String[]{});
        verifyGcdLookupAndPromiseFulfillment(1, domains, discoveryQos.getDiscoveryTimeout(), gbids, promise, 2);
        verify(routingTable, never()).incrementReferenceCount(any());
        final ArgumentCaptor<Address> addressCaptor = ArgumentCaptor.forClass(Address.class);
        verify(routingTable, times(1)).put(eq(GLOBAL_PARTICIPANT_ID), addressCaptor.capture(), eq(true), anyLong());
        final MqttAddress address1 = (MqttAddress) addressCaptor.getValue();
        assertEquals(gbids[0], address1.getBrokerUri());
        verify(routingTable, times(1)).put(eq(GLOBAL_PARTICIPANT_2_ID), addressCaptor.capture(), eq(true), anyLong());
        final MqttAddress address2 = (MqttAddress) addressCaptor.getValue();
        assertEquals(gbids[0], address2.getBrokerUri());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_globalOnly_filtersRemoteCachedEntriesByGbids() throws InterruptedException {
        discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        final GlobalDiscoveryEntry cachedEntryForGbid1 = createGlobalDiscoveryEntry(PARTICIPANT_ID_1,
                                                                                    new ProviderQos());
        final GlobalDiscoveryEntry cachedEntryForGbid2 = new GlobalDiscoveryEntry(cachedEntryForGbid1);
        cachedEntryForGbid2.setParticipantId(PARTICIPANT_ID_2);
        cachedEntryForGbid2.setAddress(globalAddress2Serialized);
        final DiscoveryEntryWithMetaInfo expectedEntry1 = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                              cachedEntryForGbid1);
        final DiscoveryEntryWithMetaInfo expectedEntry2 = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                              cachedEntryForGbid2);

        doReturn(Arrays.asList(cachedEntryForGbid1, cachedEntryForGbid2)).when(globalDiscoveryEntryCacheMock)
                                                                         .lookup(domains,
                                                                                 INTERFACE_NAME,
                                                                                 discoveryQos.getCacheMaxAge());

        final Promise<Lookup2Deferred> promise1 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[1] });

        final DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promise1,
                                                                                                                       MSG_LOOKUP_FAILED)[0];
        verify(routingTable, times(1)).put(eq(expectedEntry2.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        verify(routingTable, never()).put(eq(expectedEntry1.getParticipantId()),
                                          any(Address.class),
                                          eq(true),
                                          anyLong());
        verify(routingTable, never()).incrementReferenceCount(anyString());
        assertEquals(1, result1.length);
        assertEquals(expectedEntry2, result1[0]);

        reset((Object) routingTable);

        doReturn(true).when(routingTable)
                      .put(eq(expectedEntry1.getParticipantId()), any(Address.class), eq(true), anyLong());
        final Promise<Lookup2Deferred> promise2 = localCapabilitiesDirectory.lookup(domains,
                                                                                    INTERFACE_NAME,
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[0] });

        final DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promise2,
                                                                                                                       MSG_LOOKUP_FAILED)[0];
        verify(routingTable, times(1)).put(eq(expectedEntry1.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        verify(routingTable, never()).put(eq(expectedEntry2.getParticipantId()),
                                          any(Address.class),
                                          eq(true),
                                          anyLong());
        verify(routingTable, never()).incrementReferenceCount(anyString());
        assertEquals(1, result2.length);
        assertEquals(expectedEntry1, result2[0]);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_globalOnly_filtersLocalEntriesByGbids() throws InterruptedException {
        discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        final DiscoveryEntry localEntry1 = createDiscoveryEntry(PARTICIPANT_ID_1, new ProviderQos());
        final DiscoveryEntryWithMetaInfo expectedEntry1 = convertToDiscoveryEntryWithMetaInfo(true, localEntry1);
        final DiscoveryEntry localEntry2 = new DiscoveryEntry(localEntry1);
        localEntry2.setParticipantId(PARTICIPANT_ID_2);
        final DiscoveryEntryWithMetaInfo expectedEntry2 = new DiscoveryEntryWithMetaInfo(expectedEntry1);
        expectedEntry2.setParticipantId(localEntry2.getParticipantId());

        doReturn(Arrays.asList(localEntry1, localEntry2)).when(localDiscoveryEntryStoreMock)
                                                         .lookupGlobalEntries(domains, INTERFACE_NAME);

        final Promise<Add1Deferred> firstPromiseAdd = localCapabilitiesDirectory.add(localEntry1, true, knownGbids);
        promiseChecker.checkPromiseSuccess(firstPromiseAdd, MSG_ON_ADD_REJECT);
        final Promise<Add1Deferred> secondPromiseAdd = localCapabilitiesDirectory.add(localEntry2,
                                                                                      true,
                                                                                      new String[]{ knownGbids[1] });
        promiseChecker.checkPromiseSuccess(secondPromiseAdd, MSG_ON_ADD_REJECT);

        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock).add(localEntry1);
        verify(localDiscoveryEntryStoreMock).add(localEntry2);

        final Promise<Lookup2Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(domains,
                                                                                          INTERFACE_NAME,
                                                                                          discoveryQos,
                                                                                          new String[]{
                                                                                                  knownGbids[1] });

        final DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promiseLookup1,
                                                                                                                       MSG_LOOKUP_FAILED)[0];

        assertEquals(2, result1.length);
        checkEntries(expectedEntry1, expectedEntry2, result1);
        verify(routingTable, times(1)).incrementReferenceCount(expectedEntry1.getParticipantId());
        verify(routingTable, times(1)).incrementReferenceCount(expectedEntry2.getParticipantId());

        final Promise<Lookup2Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(domains,
                                                                                          INTERFACE_NAME,
                                                                                          discoveryQos,
                                                                                          new String[]{
                                                                                                  knownGbids[0] });

        final DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promiseLookup2,
                                                                                                                       MSG_LOOKUP_FAILED)[0];
        assertEquals(1, result2.length);
        assertTrue(DiscoveryEntryMatchHelper.discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry1,
                                                                                                      result2[0]));
        verify(routingTable, times(2)).incrementReferenceCount(expectedEntry1.getParticipantId());

        final Promise<Lookup2Deferred> promiseLookup3 = localCapabilitiesDirectory.lookup(domains,
                                                                                          INTERFACE_NAME,
                                                                                          discoveryQos,
                                                                                          knownGbids);

        final DiscoveryEntryWithMetaInfo[] result3 = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promiseLookup3,
                                                                                                                       MSG_LOOKUP_FAILED)[0];
        assertEquals(2, result3.length);
        checkEntries(expectedEntry1, expectedEntry2, result3);
        verify(routingTable, times(3)).incrementReferenceCount(expectedEntry1.getParticipantId());
        verify(routingTable, times(2)).incrementReferenceCount(expectedEntry2.getParticipantId());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_allCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        final DiscoveryEntry entryForGbid1 = new DiscoveryEntry(discoveryEntry);
        final DiscoveryEntry entryForGbid2 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2.setParticipantId("participantId2");
        final DiscoveryEntry entryForGbid3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid3.setParticipantId("participantId3");
        final DiscoveryEntry entryForGbid2And3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2And3.setParticipantId("participantId4");

        final Set<String> expectedParticipantIds = Set.of(entryForGbid1.getParticipantId(),
                                                          entryForGbid3.getParticipantId(),
                                                          entryForGbid2And3.getParticipantId());

        testLookupByDomainInterfaceWithGbids_globalOnly_allLocal(gbidsForLookup,
                                                                 entryForGbid1,
                                                                 entryForGbid2,
                                                                 entryForGbid3,
                                                                 entryForGbid2And3,
                                                                 expectedParticipantIds);
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsArray_allCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[0];

        final DiscoveryEntry entryForGbid1 = new DiscoveryEntry(discoveryEntry);
        final DiscoveryEntry entryForGbid2 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2.setParticipantId("participantId2");
        final DiscoveryEntry entryForGbid3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid3.setParticipantId("participantId3");
        final DiscoveryEntry entryForGbid2And3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2And3.setParticipantId("participantId4");

        final Set<String> expectedParticipantIds = Set.of(entryForGbid1.getParticipantId(),
                                                          entryForGbid2.getParticipantId(),
                                                          entryForGbid3.getParticipantId(),
                                                          entryForGbid2And3.getParticipantId());

        testLookupByDomainInterfaceWithGbids_globalOnly_allLocal(gbidsForLookup,
                                                                 entryForGbid1,
                                                                 entryForGbid2,
                                                                 entryForGbid3,
                                                                 entryForGbid2And3,
                                                                 expectedParticipantIds);
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_noneCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(gbidsForLookup, gbidsForLookup.clone());

        final String[] gbidsForLookup2 = new String[]{ knownGbids[2], knownGbids[0] };

        testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(gbidsForLookup2, gbidsForLookup2.clone());
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsArray_noneCached() throws InterruptedException {
        final String[] gbidsForLookup = new String[0];

        testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(gbidsForLookup, knownGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceGbids_localAndGlobal_localGlobalEntry_invokesGcd_filtersCombinedResult() throws Exception {
        domains = new String[]{ discoveryEntry.getDomain() };
        final List<DiscoveryEntry> localDiscoveryEntries = singletonList(discoveryEntry);
        final List<GlobalDiscoveryEntry> globalDiscoveryEntries = singletonList(globalDiscoveryEntry);
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.LOCAL_AND_GLOBAL, false);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(localDiscoveryEntries);
        mockGcdLookup(globalDiscoveryEntries);

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        final Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                         INTERFACE_NAME,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(1, capturedDiscoveryEntries.length);
        assertTrue(capturedDiscoveryEntries[0].getIsLocal());
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryTimeout),
                                                         any());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(routingTable, times(1)).incrementReferenceCount(capturedDiscoveryEntries[0].getParticipantId());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
        verify(localDiscoveryEntryStoreMock).lookup(domains, INTERFACE_NAME);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry() throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        domains = new String[]{ discoveryEntry.getDomain() };
        final List<GlobalDiscoveryEntry> globalDiscoveryEntries = singletonList(globalDiscoveryEntry);
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);
        mockGcdLookup(globalDiscoveryEntries);

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        final Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                         INTERFACE_NAME,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(1, capturedDiscoveryEntries.length);
        assertEquals(convertToDiscoveryEntryWithMetaInfo(true, discoveryEntry), capturedDiscoveryEntries[0]);
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryTimeout),
                                                         any());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(routingTable, times(1)).incrementReferenceCount(capturedDiscoveryEntries[0].getParticipantId());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
        verify(localDiscoveryEntryStoreMock).lookup(eq(discoveryEntry.getParticipantId()), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localOnly_noLocalEntry_doesNotInvokeGcd_returnsEmptyArray() throws Exception {
        domains = new String[]{ discoveryEntry.getDomain() };
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.LOCAL_ONLY, false);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(new ArrayList<>());
        final Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                         INTERFACE_NAME,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(0, capturedDiscoveryEntries.length);
        verify(globalCapabilitiesDirectoryClient,
               never()).lookup(any(), any(String[].class), anyString(), anyLong(), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localOnly_localEntries_doesNotInvokeGcd_returnsLocalEntries() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_ONLY;
        final boolean localEntriesAvailable = true;
        final boolean invokesGcd = false;
        final boolean returnsLocalEntry = true;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localThenGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        final boolean localEntriesAvailable = false;
        final boolean invokesGcd = true;
        final boolean returnsLocalEntry = false;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localThenGlobal_localEntries_doesNotInvokeGcd_returnsLocalEntries() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        final boolean localEntriesAvailable = true;
        final boolean invokesGcd = false;
        final boolean returnsLocalEntry = true;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localAndGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        final DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        final boolean localEntriesAvailable = false;
        final boolean invokesGcd = true;
        final boolean returnsLocalEntry = false;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_respectsCacheMaxAge() {
        domains = new String[]{ discoveryEntry.getDomain() };
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        localCapabilitiesDirectory.lookup(domains, INTERFACE_NAME, discoveryQos, new String[0]);

        verify(globalDiscoveryEntryCacheMock).lookup(domains, INTERFACE_NAME, cacheMaxAge);
    }

    @Test
    public void lookupDomIntf_globalOnlyWithCache_localGlobalEntryNoCachedEntry_doesNotInvokeGcd() throws Exception {
        final long cacheMaxAge = 1L;
        final long discoveryTimeout = 5000L;
        domains = new String[]{ discoveryEntry.getDomain() };
        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);

        // register in all gbids
        final Promise<Add1Deferred> promiseAdd = addEntry(discoveryEntry, true, knownGbids);
        promiseChecker.checkPromiseSuccess(promiseAdd, MSG_ON_ADD_REJECT);
        reset(localDiscoveryEntryStoreMock, globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);

        doReturn(Collections.singleton(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                                       .lookupGlobalEntries(domains, INTERFACE_NAME);

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        final DiscoveryEntryWithMetaInfo[] values = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promise,
                                                                                                                      MSG_LOOKUP_FAILED)[0];
        assertEquals(1, values.length);
        assertEquals(true, values[0].getIsLocal());
        verify(routingTable, times(1)).incrementReferenceCount(discoveryEntry.getParticipantId());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
    }

    @Test
    public void lookupDomIntf_globalOnlyNoCache_localGlobalEntryNoCachedEntry_invokesGcd() throws Exception {
        final long cacheMaxAge = 0L;
        final long discoveryTimeout = 5000L;
        domains = new String[]{ discoveryEntry.getDomain() };
        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);

        // register in all gbids
        final Promise<Add1Deferred> promiseAdd = addEntry(discoveryEntry, true, knownGbids);
        promiseChecker.checkPromiseSuccess(promiseAdd, MSG_ON_ADD_REJECT);
        reset(localDiscoveryEntryStoreMock, globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);

        mockGcdLookup(emptyList());

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        final DiscoveryEntryWithMetaInfo[] values = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promise,
                                                                                                                      MSG_LOOKUP_FAILED)[0];
        assertEquals(0, values.length);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(knownGbids));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterface_DiscoveryEntriesWithMetaInfoContainExpectedIsLocalValue_localCachedAndGlobalEntries() throws InterruptedException {
        final String globalDomain = "globaldomain";
        final String remoteGlobalDomain = "remoteglobaldomain";
        domains = new String[]{ "localdomain", globalDomain, remoteGlobalDomain };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        // local DiscoveryEntry
        final DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setParticipantId("participantIdLocal");
        localEntry.setDomain(domains[0]);
        final DiscoveryEntryWithMetaInfo localEntryWithMetaInfo = convertToDiscoveryEntryWithMetaInfo(true, localEntry);
        when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(List.of(localEntry));

        // cached global DiscoveryEntry
        final GlobalDiscoveryEntry cachedGlobalEntry = new GlobalDiscoveryEntry();
        cachedGlobalEntry.setParticipantId("participantIdCached");
        cachedGlobalEntry.setInterfaceName(INTERFACE_NAME);
        cachedGlobalEntry.setDomain(globalDomain);
        cachedGlobalEntry.setAddress(globalAddress1Serialized);
        final DiscoveryEntryWithMetaInfo cachedGlobalEntryWithMetaInfo = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                             cachedGlobalEntry);
        doReturn(List.of(cachedGlobalEntry)).when(globalDiscoveryEntryCacheMock)
                                            .lookup(domains, INTERFACE_NAME, discoveryQos.getCacheMaxAge());
        doReturn(Optional.of(cachedGlobalEntry)).when(globalDiscoveryEntryCacheMock)
                                                .lookup(cachedGlobalEntry.getParticipantId(), Long.MAX_VALUE);

        // remote global DiscoveryEntry
        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(build0_0Version(),
                                                                                remoteGlobalDomain,
                                                                                INTERFACE_NAME,
                                                                                "participantIdRemote",
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                PUBLIC_KEY_ID,
                                                                                globalAddress1Serialized);
        final DiscoveryEntryWithMetaInfo remoteGlobalEntryWithMetaInfo = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                             remoteGlobalEntry);
        mockGcdLookup(List.of(remoteGlobalEntry), domains, knownGbids);

        final Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos);

        final Object[] values = promiseChecker.checkPromiseSuccess(promise, MSG_LOOKUP_FAILED);
        final List<DiscoveryEntryWithMetaInfo> capabilities = Arrays.asList((DiscoveryEntryWithMetaInfo[]) values[0]);
        assertEquals(3, capabilities.size());
        assertTrue(capabilities.contains(localEntryWithMetaInfo));
        verify(routingTable, times(1)).incrementReferenceCount(localEntryWithMetaInfo.getParticipantId());
        verify(routingTable, never()).put(eq(localEntryWithMetaInfo.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
        assertTrue(capabilities.contains(cachedGlobalEntryWithMetaInfo));
        verify(routingTable, never()).incrementReferenceCount(cachedGlobalEntryWithMetaInfo.getParticipantId());
        verify(routingTable, times(1)).put(eq(cachedGlobalEntryWithMetaInfo.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        assertTrue(capabilities.contains(remoteGlobalEntryWithMetaInfo));
        verify(routingTable, never()).incrementReferenceCount(remoteGlobalEntryWithMetaInfo.getParticipantId());
        verify(routingTable, times(1)).put(eq(remoteGlobalEntryWithMetaInfo.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
    }

    private void lookupByDomainInterfaceDiscoveryScopeTest(DiscoveryScope discoveryScope,
                                                           boolean localEntriesAvailable,
                                                           boolean invokesGcd,
                                                           boolean returnsLocalEntry) throws Exception {
        domains = new String[]{ discoveryEntry.getDomain() };
        final List<DiscoveryEntry> discoveryEntries = singletonList(discoveryEntry);
        final List<GlobalDiscoveryEntry> globalDiscoveryEntries = singletonList(globalDiscoveryEntry);
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, discoveryScope, false);
        if (localEntriesAvailable) {
            when(localDiscoveryEntryStoreMock.lookup(domains, INTERFACE_NAME)).thenReturn(discoveryEntries);
        }
        mockGcdLookup(globalDiscoveryEntries);

        final Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                         INTERFACE_NAME,
                                                                                         discoveryQos,
                                                                                         new String[0]);

        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        if (invokesGcd) {
            verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                             eq(domains),
                                                             eq(INTERFACE_NAME),
                                                             eq(discoveryTimeout),
                                                             any());
        } else {
            verify(globalCapabilitiesDirectoryClient,
                   never()).lookup(any(), any(String[].class), anyString(), anyLong(), any());
        }
        if (returnsLocalEntry) {
            final DiscoveryEntryWithMetaInfo expectedLocalDiscoveryEntry = convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                               discoveryEntry);
            assertEquals(expectedLocalDiscoveryEntry, capturedDiscoveryEntries[0]);
            verify(routingTable, times(1)).incrementReferenceCount(expectedLocalDiscoveryEntry.getParticipantId());
            verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
        } else {
            final DiscoveryEntryWithMetaInfo expectedGlobalDiscoveryEntry = convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                globalDiscoveryEntry);
            assertEquals(expectedGlobalDiscoveryEntry, capturedDiscoveryEntries[0]);
            verify(routingTable, never()).incrementReferenceCount(any());
            verify(routingTable, times(1)).put(eq(expectedGlobalDiscoveryEntry.getParticipantId()),
                                               any(Address.class),
                                               any(Boolean.class),
                                               anyLong());
        }
    }

    private void testLookupByDomainInterfaceWithGbids_globalOnly_allLocal(final String[] gbidsForLookup,
                                                                          final DiscoveryEntry entryForGbid1,
                                                                          final DiscoveryEntry entryForGbid2,
                                                                          final DiscoveryEntry entryForGbid3,
                                                                          final DiscoveryEntry entryForGbid2And3,
                                                                          final Set<String> expectedParticipantIds) throws InterruptedException {
        final String[] domainsForLookup = new String[]{ discoveryEntry.getDomain() };
        final DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);
        final boolean awaitGlobalRegistration = true;
        final Promise<Add1Deferred> promise1 = addEntry(entryForGbid1, awaitGlobalRegistration, knownGbids[0]);
        promiseChecker.checkPromiseSuccess(promise1, MSG_ON_ADD_REJECT);
        final Promise<Add1Deferred> promise2 = addEntry(entryForGbid2, awaitGlobalRegistration, knownGbids[1]);
        promiseChecker.checkPromiseSuccess(promise2, MSG_ON_ADD_REJECT);
        final Promise<Add1Deferred> promise3 = addEntry(entryForGbid3, awaitGlobalRegistration, knownGbids[2]);
        promiseChecker.checkPromiseSuccess(promise3, MSG_ON_ADD_REJECT);
        final Promise<Add1Deferred> promise4 = addEntry(entryForGbid2And3,
                                                        awaitGlobalRegistration,
                                                        knownGbids[1],
                                                        knownGbids[2]);
        promiseChecker.checkPromiseSuccess(promise4, MSG_ON_ADD_REJECT);

        doReturn(Arrays.asList(entryForGbid1,
                               entryForGbid2,
                               entryForGbid3,
                               entryForGbid2And3)).when(localDiscoveryEntryStoreMock)
                                                  .lookupGlobalEntries(domainsForLookup, INTERFACE_NAME);
        doReturn(new ArrayList<GlobalDiscoveryEntry>()).when(globalDiscoveryEntryCacheMock)
                                                       .lookup(eq(domainsForLookup), eq(INTERFACE_NAME), anyLong());

        final Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                         INTERFACE_NAME,
                                                                                         discoveryQos,
                                                                                         gbidsForLookup);
        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo[] foundEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(expectedParticipantIds.size(), foundEntries.length);

        final Set<String> foundParticipantIds = Arrays.stream(foundEntries)
                                                      .map(DiscoveryEntry::getParticipantId)
                                                      .collect(Collectors.toSet());
        assertEquals(expectedParticipantIds, foundParticipantIds);
        expectedParticipantIds.forEach((participantId) -> {
            verify(routingTable, times(1)).incrementReferenceCount(participantId);
            verify(routingTable, never()).put(eq(participantId), any(Address.class), any(Boolean.class), anyLong());
        });
        reset((Object) routingTable);

        verify(globalCapabilitiesDirectoryClient,
               times(0)).lookup(any(), any(String[].class), anyString(), anyLong(), any(String[].class));
    }

    private void testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(final String[] gbidsForLookup,
                                                                                   final String[] expectedGbids) throws InterruptedException {
        final String[] domainsForLookup = new String[]{ discoveryEntry.getDomain() };
        final String[] expectedDomains = domainsForLookup.clone();
        final DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        final List<GlobalDiscoveryEntry> globalEntries = new ArrayList<>();
        globalEntries.add(globalDiscoveryEntry);
        final DiscoveryEntry entry2 = new DiscoveryEntry(discoveryEntry);
        entry2.setParticipantId("participantId2");
        globalEntries.add(discoveryEntry2GlobalDiscoveryEntry(entry2, globalAddressWithoutGbid));

        doReturn(new ArrayList<GlobalDiscoveryEntry>()).when(globalDiscoveryEntryCacheMock)
                                                       .lookup(eq(expectedDomains), eq(INTERFACE_NAME), anyLong());
        doAnswer(answerCreateHelper.createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                                      .lookup(any(),
                                                                              eq(expectedDomains),
                                                                              eq(INTERFACE_NAME),
                                                                              eq(discoveryQos.getDiscoveryTimeout()),
                                                                              eq(expectedGbids));

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        final Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                         INTERFACE_NAME,
                                                                                         discoveryQos,
                                                                                         gbidsForLookup);

        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(expectedDomains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(expectedGbids));
        final Object[] values = promiseChecker.checkPromiseSuccess(lookupPromise, MSG_LOOKUP_FAILED);
        final DiscoveryEntryWithMetaInfo[] foundEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(2, foundEntries.length);
        Arrays.asList(foundEntries)
              .forEach(entry -> verify(routingTable, times(1)).put(eq(entry.getParticipantId()),
                                                                   any(Address.class),
                                                                   eq(true),
                                                                   anyLong()));

        verify(routingTable, never()).incrementReferenceCount(anyString());
        reset((Object) routingTable);
    }

    private void checkEntries(final DiscoveryEntryWithMetaInfo expectedEntry1,
                              final DiscoveryEntryWithMetaInfo expectedEntry2,
                              final DiscoveryEntryWithMetaInfo[] result) {
        final int actualEntry1 = expectedEntry1.getParticipantId().equals(result[0].getParticipantId()) ? 0 : 1;
        final int actualEntry2 = (actualEntry1 + 1) % 2;
        assertTrue(DiscoveryEntryMatchHelper.discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry1,
                                                                                                      result[actualEntry1]));
        assertTrue(DiscoveryEntryMatchHelper.discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry2,
                                                                                                      result[actualEntry2]));
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
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

import static io.joynr.capabilities.CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.atMostOnce;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.util.Collections;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatchers;
import org.mockito.InOrder;
import org.mockito.junit.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.helpers.DiscoveryEntryMatchHelper;
import io.joynr.capabilities.helpers.DiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.capabilities.helpers.GlobalDiscoveryEntryWithParticipantIdMatcher;
import io.joynr.capabilities.helpers.GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.DiscoveryProvider;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryAddTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryAddTest.class);

    private static final String MSG_ON_REMOVE_REJECT = "remove failed";

    private static final long RE_ADD_INTERVAL_DAYS = 7L;
    private static final long DEFAULT_TTL_ADD_AND_REMOVE = MessagingQos.DEFAULT_TTL;

    @Test(timeout = TEST_TIMEOUT)
    public void add_global_invokesGcdAndStore() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        final String[] expectedGbids = knownGbids;

        final Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                         awaitGlobalRegistration);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_singleNonDefaultGbid_invokesGcdAndStore() throws InterruptedException {
        final String[] gbids = new String[]{ knownGbids[1] };
        final String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;

        final Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                         awaitGlobalRegistration,
                                                                                                                         gbids);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_multipleGbids_invokesGcdAndStore() throws InterruptedException {
        // expectedGbids element order intentionally differs from knownGbids element order
        final String[] gbids = new String[]{ knownGbids[1], knownGbids[0] };
        final String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;

        final Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                         awaitGlobalRegistration,
                                                                                                                         gbids);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_emptyGbidArray_addsToKnownBackends() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        final String[] gbids = new String[0];
        final String[] expectedGbids = knownGbids;

        final Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                         awaitGlobalRegistration,
                                                                                                                         gbids);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addToAll_global_invokesGcdAndStore() throws InterruptedException {
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;

        final Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                                                              awaitGlobalRegistration);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testReAddAllGlobalDiscoveryEntriesPeriodically() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        final DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        setProviderQos(discoveryEntry1, ProviderScope.GLOBAL);
        discoveryEntry1.setParticipantId(participantId1);

        final DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry1);
        discoveryEntry2.setParticipantId(participantId2);

        final GlobalDiscoveryEntry globalDiscoveryEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                                               globalAddress1);
        final GlobalDiscoveryEntry globalDiscoveryEntry2 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                                               globalAddress1);

        final boolean awaitGlobalRegistration = true;
        final String[] gbids1 = new String[]{ knownGbids[0] };
        final String[] expectedGbids1 = gbids1.clone();
        final String[] gbids2 = new String[]{ knownGbids[1] };
        final String[] expectedGbids2 = gbids2.clone();
        final Promise<DiscoveryProvider.Add1Deferred> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry1,
                                                                                                   awaitGlobalRegistration,
                                                                                                   gbids1);
        final Promise<DiscoveryProvider.Add1Deferred> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                                                   awaitGlobalRegistration,
                                                                                                   gbids2);

        promiseChecker.checkPromiseSuccess(promiseAdd1, "add failed");
        promiseChecker.checkPromiseSuccess(promiseAdd2, "add failed");

        reset(globalCapabilitiesDirectoryClient);

        final CountDownLatch cdlReAdd = new CountDownLatch(2);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdlReAdd)).when(globalCapabilitiesDirectoryClient)
                                                                      .add(any(),
                                                                           argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                                                                           anyLong(),
                                                                           eq(gbids1));

        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdlReAdd)).when(globalCapabilitiesDirectoryClient)
                                                                      .add(any(),
                                                                           argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                                                                           anyLong(),
                                                                           eq(gbids2));

        final Set<DiscoveryEntry> globalEntries = new HashSet<>();
        globalEntries.add(discoveryEntry1);
        globalEntries.add(discoveryEntry2);
        when(localDiscoveryEntryStoreMock.getAllGlobalEntries()).thenReturn(globalEntries);

        verify(globalCapabilitiesDirectoryClient, times(0)).add(any(), any(), anyLong(), any());

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(RE_ADD_INTERVAL_DAYS),
                                                                        eq(RE_ADD_INTERVAL_DAYS),
                                                                        eq(TimeUnit.DAYS));

        // capture the runnable and execute it to schedule the re-add task
        final Runnable runnable = runnableCaptor.getValue();
        runnable.run();

        assertTrue(cdlReAdd.await(DEFAULT_TTL_ADD_AND_REMOVE, TimeUnit.MILLISECONDS));

        // check whether add method has been called for 2 non expired entries
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                             eq(DEFAULT_TTL_ADD_AND_REMOVE),
                             eq(expectedGbids1));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                             eq(DEFAULT_TTL_ADD_AND_REMOVE),
                             eq(expectedGbids2));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addSameGbidTwiceInARow() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        final String[] gbids = new String[]{ knownGbids[0] };
        final String[] expectedGbids = gbids.clone();
        final DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .add(any(),
                                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                                                                      anyLong(),
                                                                      eq(expectedGbids));

        final Promise<DiscoveryProvider.Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                               awaitGlobalRegistration,
                                                                                               gbids);

        promiseChecker.checkPromiseSuccess(promise, "add failed");
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids));
        checkRemainingTtl(remainingTtlCaptor);

        sleep1ms(); // make sure that the lastSeenDate of expected entry 2 is larger than the lastSeenDateMs of expected entry 1
        final DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        final GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(expectedGlobalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        doReturn(Optional.of(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock)
                                                   .lookup(eq(expectedDiscoveryEntry.getParticipantId()), anyLong());

        final Promise<DiscoveryProvider.Add1Deferred> promise2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                                                awaitGlobalRegistration,
                                                                                                gbids);

        promiseChecker.checkPromiseSuccess(promise2, "add failed");

        // entry is added again (with newer lastSeenDateMs)
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids));
        checkRemainingTtl(remainingTtlCaptor);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addDifferentGbidsAfterEachOther() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        final String[] gbids1 = new String[]{ knownGbids[0] };
        final String[] expectedGbids1 = gbids1.clone();
        final String[] gbids2 = new String[]{ knownGbids[1] };
        final String[] expectedGbids2 = gbids2.clone();
        final DiscoveryEntryWithMetaInfo expectedEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                         discoveryEntry);
        final DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);

        doAnswer(answerCreateHelper.createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                                              .add(any(),
                                                                   any(GlobalDiscoveryEntry.class),
                                                                   anyLong(),
                                                                   any());

        final Promise<DiscoveryProvider.Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                               awaitGlobalRegistration,
                                                                                               gbids1);
        promiseChecker.checkPromiseSuccess(promise, "add failed");

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids1));
        checkRemainingTtl(remainingTtlCaptor);

        sleep1ms(); // make sure that the lastSeenDate of expected entry 2 is larger than the lastSeenDateMs of expected entry 1
        final DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        final GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(expectedGlobalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        verify(localDiscoveryEntryStoreMock,
               times(0)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));

        final Promise<DiscoveryProvider.Add1Deferred> promise2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                                                awaitGlobalRegistration,
                                                                                                gbids2);
        promiseChecker.checkPromiseSuccess(promise2, "add failed");

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids2));
        checkRemainingTtl(remainingTtlCaptor);

        // provider is now registered for both GBIDs
        doReturn(Collections.singletonList(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                                           .lookupGlobalEntries(new String[]{
                                                                   expectedDiscoveryEntry.getDomain() },
                                                                                INTERFACE_NAME);

        final DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        final Promise<DiscoveryProvider.Lookup2Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(new String[]{
                expectedDiscoveryEntry.getDomain() }, expectedDiscoveryEntry.getInterfaceName(), discoveryQos, gbids1);
        final Promise<DiscoveryProvider.Lookup2Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(new String[]{
                expectedDiscoveryEntry.getDomain() }, expectedDiscoveryEntry.getInterfaceName(), discoveryQos, gbids2);

        final DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promiseLookup1,
                                                                                                                       "lookup failed")[0];
        assertEquals(1, result1.length);
        assertTrue(DiscoveryEntryMatchHelper.discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntryWithMetaInfo,
                                                                                                      result1[0]));
        final DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) promiseChecker.checkPromiseSuccess(promiseLookup2,
                                                                                                                       "lookup failed")[0];
        assertEquals(1, result2.length);
        assertTrue(DiscoveryEntryMatchHelper.discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntryWithMetaInfo,
                                                                                                      result2[0]));

        verify(globalCapabilitiesDirectoryClient, times(0)).lookup(any(), any(), anyString(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_removesCachedEntryWithSameParticipantId_ProviderScope_LOCAL() throws InterruptedException {
        checkAddRemovesCachedEntryWithSameParticipantId(ProviderScope.LOCAL);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_removesCachedEntryWithSameParticipantId_ProviderScope_GLOBAL() throws InterruptedException {
        checkAddRemovesCachedEntryWithSameParticipantId(ProviderScope.GLOBAL);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAll() throws InterruptedException {
        boolean awaitGlobalRegistration = true;
        final Promise<DiscoveryProvider.AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                                        awaitGlobalRegistration);

        final ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        promiseChecker.checkPromiseSuccess(promise, "addToAll failed");
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                                                      remainingTtlCapture.capture(),
                                                      eq(knownGbids));
        checkRemainingTtl(remainingTtlCapture);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addRemoveAddInSequence_awaitGlobalRegistration_true() throws InterruptedException {
        // A sequence of add-remove-add for the same provider could lead to a non-registered provider in earlier versions
        final boolean awaitGlobalRegistration = true;
        final String participantId = discoveryEntry.getParticipantId();
        setProviderQos(discoveryEntry, ProviderScope.GLOBAL);
        expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress1);

        // checked in remove
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        final Semaphore addSemaphore1 = new Semaphore(0);
        final Semaphore addSemaphore2 = new Semaphore(0);
        mockGcdAdd(addSemaphore1, addSemaphore2, globalDiscoveryEntry);
        final Semaphore removeSemaphore1 = new Semaphore(0);
        final Semaphore removeSemaphore2 = new Semaphore(0);
        mockGcdRemove(removeSemaphore1, removeSemaphore2, participantId);

        // 3 actions. 1 global add 1 lcd.remove and 1 global add
        // add1
        final Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                 awaitGlobalRegistration);
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        addSemaphore2.release();
        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        // remove
        final Promise<DeferredVoid> promiseRemove = localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        assertTrue(removeSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId), any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(participantId);
        removeSemaphore2.release();
        promiseChecker.checkPromiseSuccess(promiseRemove, MSG_ON_REMOVE_REJECT);
        verify(localDiscoveryEntryStoreMock, timeout(DEFAULT_WAIT_TIME_MS).times(1)).remove(participantId);

        // add2
        final Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                 awaitGlobalRegistration);
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(2)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(1)).add(any(DiscoveryEntry.class));
        addSemaphore2.release();
        promiseChecker.checkPromiseSuccess(promiseAdd2, MSG_ON_ADD_REJECT);
        verify(localDiscoveryEntryStoreMock,
               times(2)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        final InOrder inOrderGlobal = inOrder(globalCapabilitiesDirectoryClient);
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId), any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));

        final InOrder inOrderLocal = inOrder(localDiscoveryEntryStoreMock);
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1)).remove(participantId);
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addRemoveAddInSequence_awaitGlobalRegistration_false() throws InterruptedException {
        // In previous versions a sequence of add-remove-add for the same provider could lead to a non-registered provider
        // This is no longer a case because when awaitGlobalRegistration = false, the entry will be removed before scheduling
        // second add call
        final boolean awaitGlobalRegistration = false;
        final String participantId = discoveryEntry.getParticipantId();
        setProviderQos(discoveryEntry, ProviderScope.GLOBAL);
        expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress1);

        // checked in remove
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        final Semaphore addSemaphore1 = new Semaphore(0);
        final Semaphore addSemaphore2 = new Semaphore(0);
        mockGcdAdd(addSemaphore1, addSemaphore2, globalDiscoveryEntry);
        final Semaphore removeSemaphore1 = new Semaphore(0);
        final Semaphore removeSemaphore2 = new Semaphore(0);
        mockGcdRemove(removeSemaphore1, removeSemaphore2, participantId);

        // 3 actions. 1 global add 1 lcd.remove and 1 global add
        final Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                 awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        final Promise<DeferredVoid> promiseRemove = localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        promiseChecker.checkPromiseSuccess(promiseRemove, MSG_ON_REMOVE_REJECT);

        final Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                 awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promiseAdd2, MSG_ON_ADD_REJECT);
        verify(localDiscoveryEntryStoreMock,
               times(2)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        // add1
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      any(String[].class));
        addSemaphore2.release();

        // remove
        assertTrue(removeSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(participantId);
        verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId), any(String[].class));
        removeSemaphore2.release();
        // already called before removeSemaphore2.release();
        verify(localDiscoveryEntryStoreMock, atMostOnce()).remove(participantId);

        // add2
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(2)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any(String[].class));
        addSemaphore2.release();

        final InOrder inOrderGlobal = inOrder(globalCapabilitiesDirectoryClient);
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId), any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));

        final InOrder inOrderLocal = inOrder(localDiscoveryEntryStoreMock);
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1)).remove(participantId);
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
    }

    private void checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(final String[] expectedGbids,
                                                                               final Function<Void, Promise<? extends AbstractDeferred>> addFunction) throws InterruptedException {
        final ArgumentCaptor<GlobalDiscoveryEntry> argumentCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);
        final ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);
        final Semaphore successCallbackSemaphore1 = new Semaphore(0);
        final Semaphore successCallbackSemaphore2 = new Semaphore(0);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(successCallbackSemaphore1,
                                                            successCallbackSemaphore2)).when(globalCapabilitiesDirectoryClient)
                                                                                       .add(any(),
                                                                                            any(GlobalDiscoveryEntry.class),
                                                                                            anyLong(),
                                                                                            any());

        final Promise<? extends AbstractDeferred> promise = addFunction.apply(null);
        assertTrue(successCallbackSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        successCallbackSemaphore2.release();
        promiseChecker.checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argumentCaptor.capture(),
                                                      remainingTtlCapture.capture(),
                                                      eq(expectedGbids));
        final GlobalDiscoveryEntry capturedGlobalDiscoveryEntry = argumentCaptor.getValue();
        assertNotNull(capturedGlobalDiscoveryEntry);

        checkRemainingTtl(remainingTtlCapture);

        assertTrue(DiscoveryEntryMatchHelper.globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(expectedGlobalDiscoveryEntry,
                                                                                                capturedGlobalDiscoveryEntry));

        verify(localDiscoveryEntryStoreMock).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verifyNoMoreInteractions(accessController);
    }

    private void checkAddRemovesCachedEntryWithSameParticipantId(final ProviderScope scope) throws InterruptedException {
        setProviderQos(discoveryEntry, scope);
        setProviderQos(expectedDiscoveryEntry, scope);

        doReturn(false).when(localDiscoveryEntryStoreMock)
                       .hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        doReturn(Optional.of(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock)
                                                   .lookup(expectedDiscoveryEntry.getParticipantId(), Long.MAX_VALUE);

        final Promise<DiscoveryProvider.Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                               true,
                                                                                               knownGbids);

        promiseChecker.checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(localDiscoveryEntryStoreMock, never()).lookup(any(), any());

        verify(globalDiscoveryEntryCacheMock, times(1)).lookup(expectedGlobalDiscoveryEntry.getParticipantId(),
                                                               Long.MAX_VALUE);
        verify(globalDiscoveryEntryCacheMock, times(1)).remove(expectedGlobalDiscoveryEntry.getParticipantId());

        final int calls = (scope == ProviderScope.GLOBAL ? 1 : 0);
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, times(calls)).add(any(), any(), anyLong(), any());
    }

    private void mockGcdAdd(final Semaphore semaphore1,
                            final Semaphore semaphore2,
                            final GlobalDiscoveryEntry globalDiscoveryEntry) {
        doAnswer(answerCreateHelper.createAnswerWithSuccess(semaphore1,
                                                            semaphore2)).when(globalCapabilitiesDirectoryClient)
                                                                        .add(any(),
                                                                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry)),
                                                                             anyLong(),
                                                                             any(String[].class));
    }

    private void mockGcdRemove(final Semaphore semaphore1, final Semaphore semaphore2, final String participantId) {
        doAnswer(answerCreateHelper.createAnswerWithSuccess(semaphore1,
                                                            semaphore2)).when(globalCapabilitiesDirectoryClient)
                                                                        .remove(any(),
                                                                                eq(participantId),
                                                                                any(String[].class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void localAddRemoveAddInSequence_doesNotInvokeGcdAndCache() throws InterruptedException {
        final String participantId = discoveryEntry.getParticipantId();
        setProviderQos(discoveryEntry, ProviderScope.LOCAL);
        setProviderQos(expectedDiscoveryEntry, ProviderScope.LOCAL);
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        // 3 actions. 1 global add 1 lcd.remove and 1 global add
        final Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry);
        final Promise<DeferredVoid> promiseRemove = localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        final Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry);

        promiseChecker.checkPromiseSuccess(promiseAdd1, "add failed");
        promiseChecker.checkPromiseSuccess(promiseRemove, "remove failed");
        promiseChecker.checkPromiseSuccess(promiseAdd2, "add failed");

        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalDiscoveryEntryCacheMock, never()).remove(anyString());
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);

        final InOrder inOrder = inOrder(localDiscoveryEntryStoreMock);
        inOrder.verify(localDiscoveryEntryStoreMock, times(1))
               .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrder.verify(localDiscoveryEntryStoreMock, times(1)).remove(participantId);
        inOrder.verify(localDiscoveryEntryStoreMock, times(1))
               .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_local_doesNotInvokeGcdAndCache() throws InterruptedException {
        setProviderQos(discoveryEntry, ProviderScope.LOCAL);
        setProviderQos(expectedDiscoveryEntry, ProviderScope.LOCAL);

        sleep(100);
        final Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry);
        promiseChecker.checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalCapabilitiesDirectoryClient, never()).add(any(),
                                                               any(GlobalDiscoveryEntry.class),
                                                               anyLong(),
                                                               any());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddKnownLocalEntryDoesNothing() throws InterruptedException {
        setProviderQos(discoveryEntry, ProviderScope.LOCAL);
        setProviderQos(expectedDiscoveryEntry, ProviderScope.LOCAL);
        doReturn(true).when(localDiscoveryEntryStoreMock)
                      .hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(expectedDiscoveryEntry.getParticipantId(), Long.MAX_VALUE);

        final Promise<DiscoveryProvider.Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                               false,
                                                                                               knownGbids);
        promiseChecker.checkPromiseSuccess(promise, "add failed");

        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(localDiscoveryEntryStoreMock).lookup(expectedDiscoveryEntry.getParticipantId(), Long.MAX_VALUE);

        verify(globalDiscoveryEntryCacheMock, never()).lookup(anyString(), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).remove(anyString());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, never()).add(any(), any(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddKnownLocalEntryWithDifferentExpiryDateAddsAgain() throws InterruptedException {
        final DiscoveryEntry newDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        newDiscoveryEntry.setExpiryDateMs(discoveryEntry.getExpiryDateMs() + 1);
        setProviderQos(newDiscoveryEntry, ProviderScope.LOCAL);
        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(newDiscoveryEntry);
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(newDiscoveryEntry.getParticipantId(), Long.MAX_VALUE);

        final Promise<DiscoveryProvider.Add1Deferred> promise = localCapabilitiesDirectory.add(newDiscoveryEntry,
                                                                                               false,
                                                                                               knownGbids);

        promiseChecker.checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(newDiscoveryEntry);
        verify(localDiscoveryEntryStoreMock).lookup(newDiscoveryEntry.getParticipantId(), Long.MAX_VALUE);
        verify(localDiscoveryEntryStoreMock).add(newDiscoveryEntry);
        // check whether the local entry is in the global cache (unlikely). If so, then remove it
        verify(globalDiscoveryEntryCacheMock, times(1)).lookup(anyString(), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).remove(anyString());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, never()).add(any(), any(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAll_local() throws InterruptedException {
        setProviderQos(discoveryEntry, ProviderScope.LOCAL);
        setProviderQos(expectedDiscoveryEntry, ProviderScope.LOCAL);
        final boolean awaitGlobalRegistration = true;

        final Promise<DiscoveryProvider.AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                                        awaitGlobalRegistration);

        promiseChecker.checkPromiseSuccess(promise, "addToAll failed");
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, never()).add(any(),
                                                               any(GlobalDiscoveryEntry.class),
                                                               anyLong(),
                                                               any());
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addGlobalCapSucceeds_NextAddShallAddGlobalAgain() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        final DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);
        final Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        promiseChecker.checkPromiseSuccess(promise, "add failed");

        final ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             remainingTtlCapture.capture(),
                             any());

        checkRemainingTtl(remainingTtlCapture);

        sleep1ms(); // make sure that the lastSeenDate of expected entry 2 is larger than the lastSeenDateMs of expected entry 1
        final DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        final GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(expectedGlobalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        verify(localDiscoveryEntryStoreMock,
               times(0)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(anyString(), anyLong());

        final Promise<DeferredVoid> promise2 = localCapabilitiesDirectory.add(discoveryEntry2, awaitGlobalRegistration);

        promiseChecker.checkPromiseSuccess(promise2, "add failed");

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCapture.capture(),
                             any());
        checkRemainingTtl(remainingTtlCapture);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addGlobalCapFails_NextAddShallAddGlobalAgain() throws InterruptedException {
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
        final String participantId = this.getClass().getName() + ".addLocalAndThanGlobalShallWork";
        final String domain = "testDomain";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis(),
                                                                 expiryDateMs,
                                                                 PUBLIC_KEY_ID);
        final DiscoveryEntry expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        PUBLIC_KEY_ID,
                                                        globalAddress1Serialized);

        final ProviderRuntimeException exception = new ProviderRuntimeException("add failed");
        doAnswer(answerCreateHelper.createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                                             .add(ArgumentMatchers.any(),
                                                                                  argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                  anyLong(),
                                                                                  any());

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        promiseChecker.checkPromiseException(promise, new ProviderRuntimeException(exception.toString()));
        final ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      remainingTtlCaptor.capture(),
                                                      any());
        checkRemainingTtl(remainingTtlCaptor);

        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());

        reset(globalCapabilitiesDirectoryClient, localDiscoveryEntryStoreMock);

        doAnswer(answerCreateHelper.createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                                              .add(any(),
                                                                   argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                   anyLong(),
                                                                   any());

        final DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(globalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());

        promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        promiseChecker.checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                                                      remainingTtlCaptor.capture(),
                                                      any());
        checkRemainingTtl(remainingTtlCaptor);
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withoutAwaitGlobalRegistration_retryAfterTimeout() throws InterruptedException {
        final CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(answerCreateHelper.createVoidAnswerWithException(cdl,
                                                                  new JoynrTimeoutException(0))).when(globalCapabilitiesDirectoryClient)
                                                                                                .add(any(),
                                                                                                     argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                                     anyLong(),
                                                                                                     any());

        final boolean awaitGlobalRegistration = false;
        final Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               atLeast(2)).add(any(),
                               argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                               anyLong(),
                               any());
        promiseChecker.checkPromiseSuccess(promise, "add failed");
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withAwaitGlobalRegistration_noRetryAfterTimeout() throws InterruptedException {
        final JoynrTimeoutException timeoutException = new JoynrTimeoutException(0);
        final ProviderRuntimeException expectedException = new ProviderRuntimeException(timeoutException.toString());

        doAnswer(answerCreateHelper.createVoidAnswerWithException(timeoutException)).when(globalCapabilitiesDirectoryClient)
                                                                                    .add(any(),
                                                                                         argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                         anyLong(),
                                                                                         any());

        final boolean awaitGlobalRegistration = true;
        final Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        promiseChecker.checkPromiseException(promise, expectedException);

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withoutAwaitGlobalRegistration_noRetryAfterRuntimeException() throws InterruptedException {
        final JoynrRuntimeException runtimeException = new JoynrRuntimeException("custom runtime exception");

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createVoidAnswerWithException(cdl,
                                                                  runtimeException)).when(globalCapabilitiesDirectoryClient)
                                                                                    .add(any(),
                                                                                         argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                         anyLong(),
                                                                                         any());

        final boolean awaitGlobalRegistration = false;
        final Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(globalDiscoveryEntry.getParticipantId());
        promiseChecker.checkPromiseSuccess(promise, "add failed");
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withoutAwaitGlobalRegistration_noRetryAfterDiscoveryError() throws InterruptedException {
        final DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createVoidAnswerWithDiscoveryError(cdl,
                                                                       expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                                      .add(any(),
                                                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                           anyLong(),
                                                                                           any());

        final boolean awaitGlobalRegistration = false;
        final Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(globalDiscoveryEntry.getParticipantId());
        promiseChecker.checkPromiseSuccess(promise, "add failed");
    }

    @Test(timeout = TEST_TIMEOUT)
    public void getAwaitGlobalRegistration_returnFalseForNonExistingParticipant() {
        final String nonExistingParticipantId = "non-existing-participant";

        assertFalse(localCapabilitiesDirectory.getAwaitGlobalRegistration(nonExistingParticipantId));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void getAwaitGlobalRegistration_returnLastStoredValue() throws InterruptedException {
        Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry, false);
        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        assertFalse(localCapabilitiesDirectory.getAwaitGlobalRegistration(discoveryEntry.getParticipantId()));

        Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry, true);
        promiseChecker.checkPromiseSuccess(promiseAdd2, MSG_ON_ADD_REJECT);
        assertTrue(localCapabilitiesDirectory.getAwaitGlobalRegistration(discoveryEntry.getParticipantId()));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void getAwaitGlobalRegistration_removeLastStoredValue() throws InterruptedException {
        final boolean awaitGlobalRegistration = false;
        final String participantId = discoveryEntry.getParticipantId();
        setProviderQos(discoveryEntry, ProviderScope.GLOBAL);
        expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress1);

        // checked in remove
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        final Semaphore addSemaphore1 = new Semaphore(0);
        final Semaphore addSemaphore2 = new Semaphore(0);
        mockGcdAdd(addSemaphore1, addSemaphore2, globalDiscoveryEntry);
        final Semaphore removeSemaphore1 = new Semaphore(0);
        final Semaphore removeSemaphore2 = new Semaphore(0);
        mockGcdRemove(removeSemaphore1, removeSemaphore2, participantId);

        // 2 actions. 1 global add 1 lcd.remove
        final Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                 awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        final Promise<DeferredVoid> promiseRemove = localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        promiseChecker.checkPromiseSuccess(promiseRemove, MSG_ON_REMOVE_REJECT);

        // add1
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      any(String[].class));
        addSemaphore2.release();

        // remove
        assertTrue(removeSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(participantId);
        verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId), any(String[].class));
        removeSemaphore2.release();
        // already called before removeSemaphore2.release();
        verify(localDiscoveryEntryStoreMock, atMostOnce()).remove(participantId);

        final InOrder inOrderGlobal = inOrder(globalCapabilitiesDirectoryClient);
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId), any(String[].class));

        final InOrder inOrderLocal = inOrder(localDiscoveryEntryStoreMock);
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1)).remove(participantId);

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);

        assertFalse(localCapabilitiesDirectory.getAwaitGlobalRegistration(participantId));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withAwaitGlobalRegistration_usesCorrectRemainingTtl() throws InterruptedException {
        globalAddUsesCorrectRemainingTtl(true);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withoutAwaitGlobalRegistration_usesCorrectRemainingTtl() throws InterruptedException {
        globalAddUsesCorrectRemainingTtl(false);
    }

    private void globalAddUsesCorrectRemainingTtl(boolean awaitGlobalRegistration) throws InterruptedException {
        final int defaultTtl = MessagingQos.DEFAULT_TTL;

        final DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry1.setParticipantId("participantId1");
        final DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry2.setParticipantId("participantId2");

        final GlobalDiscoveryEntry globalDiscoveryEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                                               globalAddress1);
        final GlobalDiscoveryEntry globalDiscoveryEntry2 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                                               globalAddress1);

        final ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        final CountDownLatch startOfFirstAddCdl = new CountDownLatch(1);
        final CountDownLatch endOfFirstAddCdl = new CountDownLatch(1);
        final long sleepTime = 1000L;
        doAnswer(answerCreateHelper.createAnswerWithDelayedSuccess(startOfFirstAddCdl,
                                                                   endOfFirstAddCdl,
                                                                   sleepTime)).when(globalCapabilitiesDirectoryClient)
                                                                              .add(any(),
                                                                                   argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry1)),
                                                                                   anyLong(),
                                                                                   any());

        final CountDownLatch secondAddCdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(secondAddCdl)).when(globalCapabilitiesDirectoryClient)
                                                                          .add(any(),
                                                                               argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry2)),
                                                                               anyLong(),
                                                                               any());

        localCapabilitiesDirectory.add(discoveryEntry1, awaitGlobalRegistration);
        localCapabilitiesDirectory.add(discoveryEntry2, awaitGlobalRegistration);
        assertTrue(startOfFirstAddCdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry1)),
                             remainingTtlCapture.capture(),
                             any());

        final long firstNow = System.currentTimeMillis();
        final long capturedFirstAddRemainingTtl = remainingTtlCapture.getValue();

        assertTrue(endOfFirstAddCdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        assertTrue(secondAddCdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry2)),
                             remainingTtlCapture.capture(),
                             any());

        final long secondNow = System.currentTimeMillis();
        final long delta = secondNow - firstNow;
        final long capturedSecondAddRemainingTtl = remainingTtlCapture.getValue();
        final long epsilon = 300;
        if (awaitGlobalRegistration) {
            assertTrue(capturedFirstAddRemainingTtl <= defaultTtl);
            assertTrue(capturedFirstAddRemainingTtl > defaultTtl - epsilon);
            assertTrue(capturedSecondAddRemainingTtl <= defaultTtl - delta + epsilon);
            assertTrue(capturedSecondAddRemainingTtl > defaultTtl - delta - epsilon);
        } else {
            assertEquals(capturedFirstAddRemainingTtl, defaultTtl);
            assertEquals(capturedSecondAddRemainingTtl, defaultTtl);
        }
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}

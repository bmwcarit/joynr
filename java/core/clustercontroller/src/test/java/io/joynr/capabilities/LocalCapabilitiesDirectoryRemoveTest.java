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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryRemoveTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryRemoveTest.class);

    @Test(timeout = TEST_TIMEOUT)
    public void remove_globallyRegistered_GcdCalled_awaitGlobalRegistration_true() throws InterruptedException {
        remove_globallyRegistered_GcdCalled(true);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_globallyRegistered_GcdCalled_awaitGlobalRegistration_false() throws InterruptedException {
        remove_globallyRegistered_GcdCalled(false);
    }

    private void remove_globallyRegistered_GcdCalled(boolean awaitGlobalRegistration) throws InterruptedException {
        final Promise<DeferredVoid> addPromise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(addPromise, "add failed");

        final CountDownLatch cdlStart = new CountDownLatch(1);
        final CountDownLatch cdlDone = new CountDownLatch(1);
        mockGcdRemove(cdlStart, cdlDone, 1500, globalDiscoveryEntry.getParticipantId());

        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(globalDiscoveryEntry.getParticipantId());

        assertTrue(cdlStart.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient).remove(any(),
                                                         eq(discoveryEntry.getParticipantId()),
                                                         any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(awaitGlobalRegistration ? 0 : 1)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(any(String.class));
        assertTrue(cdlDone.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(discoveryEntry.getParticipantId());
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_localProvider_GcdNotCalled() throws InterruptedException {
        setProviderQos(discoveryEntry, ProviderScope.LOCAL);

        final boolean awaitGlobalRegistration = true;
        final Promise<DeferredVoid> addPromise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(addPromise, MSG_ON_ADD_REJECT);

        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());

        sleep(500);

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient, times(0)).remove(any(), anyString(), any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(1)).remove(discoveryEntry.getParticipantId());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_participantNotRegisteredNoGbids_GcdNotCalled() throws InterruptedException {
        // awaitGlobalRegistration = false
        final String participantId = "unknownparticipantId";
        final CountDownLatch cdl = new CountDownLatch(1);

        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        localCapabilitiesDirectory.remove(participantId);

        assertFalse(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient, never()).remove(any(), any(String.class), any(String[].class));
        verify(localDiscoveryEntryStoreMock, never()).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, never()).remove(any(String.class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_participantNotRegisteredGbidsMapped_GcdCalled() throws InterruptedException {
        // this test assumes that the participant gets registered by a queued add task after enqueuing the remove task
        // while awaitGlobalRegistration = false
        final CountDownLatch cdl = new CountDownLatch(1);
        mockGcdRemove(cdl, provisionedGlobalDiscoveryEntry.getParticipantId());

        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock)
                                  .lookup(eq(provisionedGlobalDiscoveryEntry.getParticipantId()), anyLong());
        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(any(), eq(provisionedGlobalDiscoveryEntry.getParticipantId()), any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(provisionedGlobalDiscoveryEntry.getParticipantId());
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_TimeoutException() throws InterruptedException {
        final CountDownLatch cdl = new CountDownLatch(2);

        mockGcdRemoveException(cdl, new JoynrTimeoutException(0), provisionedGlobalDiscoveryEntry.getParticipantId());

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verifyNoMoreInteractions(routingTable);
        verify(localDiscoveryEntryStoreMock, times(1)).remove(anyString());
        verify(globalCapabilitiesDirectoryClient,
               atLeast(2)).remove(any(), eq(provisionedGlobalDiscoveryEntry.getParticipantId()), any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_NonTimeoutException() throws InterruptedException {
        final CountDownLatch cdl = new CountDownLatch(1);

        mockGcdRemoveException(cdl,
                               new JoynrCommunicationException(),
                               provisionedGlobalDiscoveryEntry.getParticipantId());

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(localDiscoveryEntryStoreMock, times(1)).remove(anyString());
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(any(), eq(provisionedGlobalDiscoveryEntry.getParticipantId()), any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_DiscoveryError_NoEntry() throws InterruptedException {
        // covers NO_ENTRY_FOR_PARTICIPANT as well as NO_ENTRY_FOR_SELECTED_BACKENDS
        // while awaitGlobalRegistration = false
        final CountDownLatch cdl = new CountDownLatch(1);

        mockGcdRemoveError(cdl,
                           DiscoveryError.NO_ENTRY_FOR_PARTICIPANT,
                           provisionedGlobalDiscoveryEntry.getParticipantId());

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(any(), eq(provisionedGlobalDiscoveryEntry.getParticipantId()), any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(1)).remove(provisionedGlobalDiscoveryEntry.getParticipantId());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_DiscoveryError_InvalidGbid() throws InterruptedException {
        // This test uses awaitGlobalRegistration = false
        // Also covers UNKNOWN_GBID and INTERNAL_ERROR
        final CountDownLatch cdl = new CountDownLatch(1);

        mockGcdRemoveError(cdl, DiscoveryError.INVALID_GBID, provisionedGlobalDiscoveryEntry.getParticipantId());

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(localDiscoveryEntryStoreMock, times(1)).remove(anyString());
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(any(), eq(provisionedGlobalDiscoveryEntry.getParticipantId()), any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testRemoveUsesSameGbidOrderAsAdd() throws InterruptedException {
        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[0] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[1] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[0], knownGbids[1] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[1], knownGbids[0] });
    }

    @Test
    public void removeStaleProvidersOfClusterController_invokesGcdClient() {
        // Test whether removeStale() of GlobalCapabilitiesDirectoryClient is called once for all known backends
        // and captured argument of maxLastSeenDateMs differs from current time less than threshold.
        final long currentDateMs = System.currentTimeMillis();
        final ArgumentCaptor<Long> maxLastSeenDateCaptor = ArgumentCaptor.forClass(Long.class);
        final long toleranceMs = 200L;

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();
        final ArgumentCaptor<String> gbidCaptor = ArgumentCaptor.forClass(String.class);
        verify(globalCapabilitiesDirectoryClient, times(knownGbids.length)).removeStale(any(),
                                                                                        maxLastSeenDateCaptor.capture(),
                                                                                        gbidCaptor.capture());

        assertTrue(maxLastSeenDateCaptor.getValue() <= currentDateMs);
        assertTrue(currentDateMs - maxLastSeenDateCaptor.getValue() <= toleranceMs);
        final List<String> actualGbids = gbidCaptor.getAllValues();
        assertEquals(Arrays.asList(knownGbids), actualGbids);
    }

    @Test
    public void removeStaleProvidersOfClusterController_callsItselfOnCallbackFailure() {
        // Test whether removeStaleProvidersOfClusterController() is calling itself n-times
        // when callback function is calling onFailure(exception) function.
        final int numberOfOnFailureCalls = 2;
        final JoynrRuntimeException exception = new JoynrRuntimeException("removeStale failed");

        for (final String gbid : knownGbids) {
            doAnswer(new Answer<Future<Void>>() {
                private int count = 0;

                @Override
                public Future<Void> answer(final InvocationOnMock invocation) {
                    Future<Void> result = new Future<>();
                    @SuppressWarnings("unchecked")
                    final Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                    if (count++ == numberOfOnFailureCalls) {
                        callback.onSuccess(null);
                        result.onSuccess(null);
                        return result;
                    }
                    callback.onFailure(exception);
                    result.onSuccess(null);
                    return result;
                }
            }).when(globalCapabilitiesDirectoryClient).removeStale(any(), anyLong(), eq(gbid));
        }

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        final int numberOfCalls = numberOfOnFailureCalls + 1; // one time success

        for (final String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient, times(numberOfCalls)).removeStale(any(), anyLong(), eq(gbid));
        }
    }

    @Test
    public void removeStaleProvidersOfClusterController_calledOnceIfMessageNotSent() {
        // Test whether removeStale() of GlobalCapabilitiesDirectoryClient is called once when exception
        // in a gbid has a type JoynrMessageNotSentException and contains "Address type not supported" message
        final JoynrRuntimeException exception = new JoynrMessageNotSentException("Address type not supported");

        doAnswer((Answer<Future<Void>>) invocation -> {
            final Future<Void> result = new Future<>();
            @SuppressWarnings("unchecked")
            final Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
            callback.onFailure(exception);
            result.onSuccess(null);
            return result;
        }).when(globalCapabilitiesDirectoryClient).removeStale(any(), anyLong(), anyString());

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        for (final String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient, times(1)).removeStale(any(), anyLong(), eq(gbid));
        }
    }

    @Test
    public void removeStaleProvidersOfClusterController_noRetryIfRetryDurationExceeded() {
        final long removeStaleMaxRetryMs = 3600000;
        // Set a custom value of cluster controller start time to simulate timeout for removeStale retries
        final long ccStartUpDateMs = removeStaleMaxRetryMs + 1;
        try {
            setFieldValue(localCapabilitiesDirectory, "ccStartUpDateInMs", ccStartUpDateMs);
        } catch (Exception e) {
            fail("Couldn't set start date of cluster controller in milliseconds.");
        }

        final JoynrRuntimeException exception = new JoynrRuntimeException("removeStale failed");
        for (final String gbid : knownGbids) {
            doAnswer((Answer<Future<Void>>) invocation -> {
                final Future<Void> result = new Future<>();
                @SuppressWarnings("unchecked")
                final Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                callback.onFailure(exception);
                result.onSuccess(null);
                return result;
            }).when(globalCapabilitiesDirectoryClient).removeStale(any(), anyLong(), eq(gbid));
        }

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        for (final String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient, times(1)).removeStale(any(), eq(ccStartUpDateMs), eq(gbid));
        }
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testExpiredDiscoveryEntryCacheCleanerIsInitializedCorrectly() {
        verify(expiredDiscoveryEntryCacheCleaner).scheduleCleanUpForCaches(any(ExpiredDiscoveryEntryCacheCleaner.CleanupAction.class),
                                                                           eq(globalDiscoveryEntryCacheMock),
                                                                           eq(localDiscoveryEntryStoreMock));
    }

    private void testRemoveUsesSameGbidOrderAsAdd(final String[] selectedGbids) throws InterruptedException {
        final String[] expectedGbids = selectedGbids.clone();
        final String participantId = this.getClass().getName() + ".removeUsesSameGbidOrderAsAdd."
                + Arrays.toString(selectedGbids);
        final String domain = "testDomain";
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(build47_11Version(),
                                                        domain,
                                                        INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        PUBLIC_KEY_ID,
                                                        globalAddress1Serialized);

        final boolean awaitGlobalRegistration = true;
        final Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(globalDiscoveryEntry,
                                                                             awaitGlobalRegistration,
                                                                             selectedGbids);
        promiseChecker.checkPromiseSuccess(promise, "add failed in testRemoveUsesSameGbidOrderAsAdd");

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .remove(any(), any(String.class), any(String[].class));

        when(localDiscoveryEntryStoreMock.lookup(globalDiscoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(globalDiscoveryEntry));
        localCapabilitiesDirectory.remove(globalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient).remove(any(), any(String.class), eq(expectedGbids));
        verifyNoMoreInteractions(routingTable);
    }

    @SuppressWarnings("SameParameterValue")
    private void mockGcdRemove(final CountDownLatch cdlStart,
                               final CountDownLatch cdlDone,
                               final long delay,
                               final String participantId) {
        doAnswer(answerCreateHelper.createAnswerWithDelayedSuccess(cdlStart,
                                                                   cdlDone,
                                                                   delay)).when(globalCapabilitiesDirectoryClient)
                                                                          .remove(any(),
                                                                                  eq(participantId),
                                                                                  any(String[].class));
    }

    private void mockGcdRemove(final CountDownLatch cdl, final String participantId) {
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .remove(any(), eq(participantId), any(String[].class));
    }

    private void mockGcdRemoveException(final CountDownLatch cdl,
                                        final JoynrRuntimeException exception,
                                        final String participantId) {
        doAnswer(answerCreateHelper.createVoidAnswerWithException(cdl,
                                                                  exception)).when(globalCapabilitiesDirectoryClient)
                                                                             .remove(any(),
                                                                                     eq(participantId),
                                                                                     any(String[].class));
    }

    private void mockGcdRemoveError(final CountDownLatch cdl, final DiscoveryError error, final String participantId) {
        doAnswer(answerCreateHelper.createVoidAnswerWithDiscoveryError(cdl,
                                                                       error)).when(globalCapabilitiesDirectoryClient)
                                                                              .remove(any(),
                                                                                      eq(participantId),
                                                                                      any(String[].class));
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}

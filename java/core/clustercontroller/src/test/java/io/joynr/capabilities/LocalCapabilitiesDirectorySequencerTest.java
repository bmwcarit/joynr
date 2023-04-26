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
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import io.joynr.capabilities.helpers.GlobalDiscoveryEntryWithParticipantIdMatcher;
import io.joynr.provider.DeferredVoid;
import joynr.exceptions.ProviderRuntimeException;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderScope;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatchers;
import org.mockito.InOrder;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.capabilities.helpers.GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.provider.Promise;
import io.joynr.proxy.CallbackWithModeledError;
import joynr.system.DiscoveryProvider.AddToAllDeferred;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.Version;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectorySequencerTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectorySequencerTest.class);

    private static final String FIELD_NAME = "gcdTaskSequencer";

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerDoesNotCrashOnExceptionAfterAddTaskFinished() throws InterruptedException {
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);

        final CountDownLatch cdl1 = new CountDownLatch(1);
        final CountDownLatch cdl2 = new CountDownLatch(1);

        class TestGde extends GlobalDiscoveryEntry {
            TestGde(final GlobalDiscoveryEntry gde) {
                super(gde);
            }

            @Override
            public Version getProviderVersion() {
                cdl1.countDown();
                try {
                    // block GcdTaskSequencer until taskFinished has been called
                    cdl2.await();
                } catch (final InterruptedException e) {
                    // ignore
                }
                return super.getProviderVersion();
            }
        }
        final AtomicBoolean cbCalled = new AtomicBoolean();
        final TestGde gde = new TestGde(globalDiscoveryEntry);
        final GcdTask.CallbackCreator callbackCreator = new GcdTask.CallbackCreator() {
            @Override
            public CallbackWithModeledError<Void, DiscoveryError> createCallback() {
                return new CallbackWithModeledError<>() {
                    @Override
                    public void onFailure(final DiscoveryError errorEnum) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called, DiscoveryError {}", errorEnum);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onFailure(final JoynrRuntimeException runtimeException) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called:", runtimeException);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onSuccess(final Void result) {
                        // taskFinished is called manually
                        logger.error("onSuccess callback called");
                        cbCalled.set(true);
                    }
                };
            }
        };
        final GcdTask task = GcdTask.createAddTask(callbackCreator, gde, expiryDateMs, knownGbids, true);
        gcdTaskSequencer.addTask(task);

        assertTrue(cdl1.await(DEFAULT_WAIT_TIME_MS * 100, TimeUnit.MILLISECONDS));
        // call taskFinished while task is processed
        gcdTaskSequencer.taskFinished();
        cdl2.countDown();

        verify(globalCapabilitiesDirectoryClient,
               timeout(DEFAULT_WAIT_TIME_MS).times(1)).add(any(),
                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                                                           anyLong(),
                                                           eq(expectedGbids));

        // check that GcdTaskSequencer is still alive
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        verify(globalCapabilitiesDirectoryClient, timeout(DEFAULT_WAIT_TIME_MS).times(2)).add(any(),
                                                                                              any(),
                                                                                              anyLong(),
                                                                                              eq(expectedGbids));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
        assertFalse(cbCalled.get());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterAddSuccess() throws InterruptedException, IllegalAccessException {
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, FIELD_NAME, gcdTaskSequencerSpy);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((Answer<Void>) invocation -> {
            cdl.countDown();
            return null;
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS * 100, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(callbackCaptor.capture(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             anyLong(),
                             eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onSuccess(null);
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterAddTimeoutOnDisabledRetry() throws InterruptedException,
                                                                         IllegalAccessException {
        final String[] expectedGbids = knownGbids.clone();
        // Retries are disabled when awaitGlobalRegistration is true
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, FIELD_NAME, gcdTaskSequencerSpy);

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((Answer<Void>) invocation -> {
            cdl.countDown();
            return null;
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(callbackCaptor.capture(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             anyLong(),
                             eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerRetriesAddOnJoynrTimeoutExceptionOnly() throws InterruptedException,
                                                                     IllegalAccessException {
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = false;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, FIELD_NAME, gcdTaskSequencerSpy);

        final Semaphore semaphore = new Semaphore(0);
        doAnswer((Answer<Void>) invocation -> {
            semaphore.release();
            return null;
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        assertTrue(semaphore.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).add(callbackCaptor.capture(),
                                                                any(),
                                                                anyLong(),
                                                                eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verify(gcdTaskSequencerSpy).retryTask();
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        assertTrue(semaphore.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(2)).add(callbackCaptor.capture(),
                                                                any(),
                                                                anyLong(),
                                                                eq(expectedGbids));

        verify(gcdTaskSequencerSpy, never()).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterAddDiscoveryError() throws InterruptedException, IllegalAccessException {
        final String[] expectedGbids = new String[]{ knownGbids[0] };
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, FIELD_NAME, gcdTaskSequencerSpy);

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((Answer<Void>) invocation -> {
            cdl.countDown();
            return null;
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, expectedGbids);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).add(callbackCaptor.capture(),
                                                                any(),
                                                                anyLong(),
                                                                eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(gcdTaskSequencerSpy).taskFinished();

        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerDoesNotCrashOnExceptionAfterRemoveTaskFinished() throws InterruptedException {
        /// We have to add before we can remove anything
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        final Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                      awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promise, MSG_ON_ADD_REJECT);
        verify(globalCapabilitiesDirectoryClient, times(1)).add(any(), any(), anyLong(), eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        final CountDownLatch cdl1 = new CountDownLatch(1);
        final CountDownLatch cdl2 = new CountDownLatch(1);

        final AtomicBoolean cbCalled = new AtomicBoolean();
        GcdTask.CallbackCreator callbackCreator = new GcdTask.CallbackCreator() {
            @Override
            public CallbackWithModeledError<Void, DiscoveryError> createCallback() {
                return new CallbackWithModeledError<>() {
                    @Override
                    public void onFailure(final DiscoveryError errorEnum) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called, DiscoveryError {}", errorEnum);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onFailure(final JoynrRuntimeException runtimeException) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called:", runtimeException);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onSuccess(final Void result) {
                        // taskFinished is called manually
                        logger.error("onSuccess callback called");
                        cbCalled.set(true);
                    }
                };
            }
        };
        class TestGcdRemoveTask extends GcdTask {
            public TestGcdRemoveTask(final CallbackCreator callbackCreator,
                                     final String participantId,
                                     String[] gbids) {
                super(MODE.REMOVE, callbackCreator, participantId, null, gbids, 0L, true);
            }

            @Override
            public String getParticipantId() {
                cdl1.countDown();
                try {
                    // block GcdTaskSequencer until taskFinished has been called
                    cdl2.await();
                } catch (InterruptedException e) {
                    // ignore
                }
                return super.getParticipantId();
            }
        }

        final TestGcdRemoveTask task = new TestGcdRemoveTask(callbackCreator,
                                                             globalDiscoveryEntry.getParticipantId(),
                                                             expectedGbids);
        gcdTaskSequencer.addTask(task);

        assertTrue(cdl1.await(DEFAULT_WAIT_TIME_MS * 100, TimeUnit.MILLISECONDS));
        // call taskFinished while task is processed
        gcdTaskSequencer.taskFinished();
        cdl2.countDown();

        verify(globalCapabilitiesDirectoryClient,
               timeout(DEFAULT_WAIT_TIME_MS).times(1)).remove(any(),
                                                              eq(globalDiscoveryEntry.getParticipantId()),
                                                              eq(expectedGbids));

        // check that GcdTaskSequencer is still alive
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        verify(globalCapabilitiesDirectoryClient, timeout(DEFAULT_WAIT_TIME_MS).times(1)).add(any(),
                                                                                              any(),
                                                                                              anyLong(),
                                                                                              eq(expectedGbids));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
        assertFalse(cbCalled.get());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterRemoveSuccess() throws InterruptedException, IllegalAccessException {
        /// We have to add before we can remove anything
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        final Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                      awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promise, MSG_ON_ADD_REJECT);
        verify(globalCapabilitiesDirectoryClient, times(1)).add(any(), any(), anyLong(), eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        setFieldValue(localCapabilitiesDirectory, FIELD_NAME, gcdTaskSequencerSpy);
        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((Answer<Void>) invocation -> {
            cdl.countDown();
            return null;
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.REMOVE.equals(arg.getMode())));
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));

        callbackCaptor.getValue().onSuccess(null);
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerRetriesRemoveOnJoynrTimeoutExceptionOnly() throws InterruptedException,
                                                                        IllegalAccessException {
        ///We need to add before we can remove
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promise, MSG_ON_ADD_REJECT);
        verify(globalCapabilitiesDirectoryClient, times(1)).add(any(), any(), anyLong(), eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        setFieldValue(localCapabilitiesDirectory, FIELD_NAME, gcdTaskSequencerSpy);
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((Answer<Void>) invocation -> {
            cdl.countDown();
            return null;
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.REMOVE.equals(arg.getMode())));
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));

        final CountDownLatch cdl2 = new CountDownLatch(1);
        doAnswer((Answer<Void>) invocation -> {
            cdl2.countDown();
            return null;
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verify(gcdTaskSequencerSpy).retryTask();
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        assertTrue(cdl2.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(2)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        verify(gcdTaskSequencerSpy).taskFinished();
        // After handling a non-timeout exception, the callback is 'disabled'
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterRemoveDiscoveryError() throws InterruptedException,
                                                                    IllegalAccessException {
        ///We need to add before we can remove
        final String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        final Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                      awaitGlobalRegistration);
        promiseChecker.checkPromiseSuccess(promise, MSG_ON_ADD_REJECT);
        verify(globalCapabilitiesDirectoryClient, times(1)).add(ArgumentMatchers.any(),
                                                                any(),
                                                                anyLong(),
                                                                eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        setFieldValue(localCapabilitiesDirectory, FIELD_NAME, gcdTaskSequencerSpy);
        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((Answer<Void>) invocation -> {
            cdl.countDown();
            return null;
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.REMOVE.equals(arg.getMode())));
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));

        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testProcessingExpiredQueuedGcdActions() throws Exception {
        reset(globalCapabilitiesDirectoryClient);

        // defaultTtlAddAndRemove = 60000ms (MessagingQos.DEFAULT_TTL) is too long, we reduce it to 1000ms for the test
        setNewDefaultTtlAddAndRemove(1000);

        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        final DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry1.getQos().setScope(ProviderScope.GLOBAL);
        discoveryEntry1.setParticipantId(participantId1);

        final DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry1);
        discoveryEntry2.setParticipantId(participantId2);

        final GlobalDiscoveryEntry globalDiscoveryEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                                               globalAddress1);
        final GlobalDiscoveryEntry globalDiscoveryEntry2 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                                               globalAddress1);

        final long delay = 1500;
        final CountDownLatch cdlAddDelayStarted = new CountDownLatch(1);
        final CountDownLatch cdlAddDone = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithDelayedSuccess(cdlAddDelayStarted,
                                                                   cdlAddDone,
                                                                   delay)).when(globalCapabilitiesDirectoryClient)
                                                                          .add(any(),
                                                                               argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                                                                               anyLong(),
                                                                               any(String[].class));
        CountDownLatch cdlRemove = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdlRemove)).when(globalCapabilitiesDirectoryClient)
                                                                       .remove(any(),
                                                                               eq(participantId1),
                                                                               any(String[].class));

        // 3 actions. 2 lcd.add and 1 lcd.remove
        final Boolean awaitGlobalRegistration = true;
        final Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry1,
                                                                                 awaitGlobalRegistration);
        final Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                                 awaitGlobalRegistration);

        assertTrue(cdlAddDelayStarted.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        final JoynrRuntimeException expectedException = new JoynrRuntimeException("Failed to process global registration in time, please try again");
        promiseChecker.checkPromiseException(promiseAdd2, new ProviderRuntimeException(expectedException.toString()));

        // second add failed before first add has finished, remove not yet executed
        assertEquals(1, cdlAddDone.getCount());
        assertEquals(1, cdlRemove.getCount());

        promiseChecker.checkPromiseSuccess(promiseAdd1, "add failed");
        assertTrue(cdlAddDone.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        localCapabilitiesDirectory.remove(discoveryEntry1.getParticipantId());
        assertTrue(cdlRemove.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        final InOrder inOrder = inOrder(globalCapabilitiesDirectoryClient);

        inOrder.verify(globalCapabilitiesDirectoryClient, times(1))
               .add(any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                    anyLong(),
                    any(String[].class));

        inOrder.verify(globalCapabilitiesDirectoryClient, times(1))
               .remove(any(), eq(participantId1), any(String[].class));

        verify(globalCapabilitiesDirectoryClient,
               times(0)).add(any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                             anyLong(),
                             any(String[].class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addAndRemoveAreCalledInOrder() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        final DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry1.getQos().setScope(ProviderScope.GLOBAL);
        discoveryEntry1.setParticipantId(participantId1);

        final DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry2.setParticipantId(participantId2);

        final GlobalDiscoveryEntry globalDiscoveryEntry1 = discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                               globalAddress1);
        final GlobalDiscoveryEntry globalDiscoveryEntry2 = discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                               globalAddress1);

        final boolean awaitGlobalRegistration = true;
        final Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry1,
                                                                                 awaitGlobalRegistration);
        final Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                                 awaitGlobalRegistration);

        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        promiseChecker.checkPromiseSuccess(promiseAdd2, MSG_ON_ADD_REJECT);

        final InOrder inOrder = inOrder(globalCapabilitiesDirectoryClient);

        final ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        inOrder.verify(globalCapabilitiesDirectoryClient)
               .add(any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                    remainingTtlCapture.capture(),
                    any(String[].class));

        checkRemainingTtl(remainingTtlCapture);

        inOrder.verify(globalCapabilitiesDirectoryClient)
               .add(any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                    remainingTtlCapture.capture(),
                    any(String[].class));

        checkRemainingTtl(remainingTtlCapture);

        final CountDownLatch cdl = new CountDownLatch(2);
        mockGcdRemove(cdl);

        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry2.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry2));
        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry1.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry1));
        localCapabilitiesDirectory.remove(discoveryEntry2.getParticipantId());
        localCapabilitiesDirectory.remove(discoveryEntry1.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        inOrder.verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId2), any(String[].class));
        inOrder.verify(globalCapabilitiesDirectoryClient).remove(any(), eq(participantId1), any(String[].class));
    }

    @SuppressWarnings("SameParameterValue")
    private void setNewDefaultTtlAddAndRemove(final long defaultTtlMs) throws ReflectiveOperationException {
        final Field defaultTtlMsField = LocalCapabilitiesDirectoryImpl.class.getDeclaredField("defaultTtlAddAndRemove");
        defaultTtlMsField.setAccessible(true);
        defaultTtlMsField.set(localCapabilitiesDirectory, defaultTtlMs);
    }

    private void mockGcdRemove(final CountDownLatch cdl) {
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .remove(any(), anyString(), any(String[].class));
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
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

import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderScope;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryTouchTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryTouchTest.class);

    @Test(timeout = TEST_TIMEOUT)
    public void callTouchForGlobalParticipantIds() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        final long toleranceMs = FRESHNESS_UPDATE_INTERVAL_MS * 2 / 3;

        final GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);

        final GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(entry1);
        entry2.setParticipantId(participantId2);

        final Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(entry1, true);
        final Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(entry2, true);
        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        promiseChecker.checkPromiseSuccess(promiseAdd2, MSG_ON_ADD_REJECT);

        final ArgumentCaptor<Long> lastSeenDateCaptor = ArgumentCaptor.forClass(Long.class);
        final ArgumentCaptor<Long> expiryDateCaptor = ArgumentCaptor.forClass(Long.class);

        final String[] touchedParticipantIds = new String[]{ participantId1, participantId2 };
        final String[] expectedParticipantIds = touchedParticipantIds.clone();
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(touchedParticipantIds);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(TimeUnit.MILLISECONDS));

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .touch(any(), eq(expectedParticipantIds), anyString());
        sleep(FRESHNESS_UPDATE_INTERVAL_MS); // make sure that the initial delay has expired before starting the runnable
        final long expectedLastSeenDateMs = System.currentTimeMillis();
        final long expectedExpiryDateMs = expectedLastSeenDateMs + DEFAULT_EXPIRY_TIME_MS;
        final Runnable runnable = runnableCaptor.getValue();
        runnable.run();

        verify(localDiscoveryEntryStoreMock, times(1)).touchDiscoveryEntries(lastSeenDateCaptor.capture(),
                                                                             expiryDateCaptor.capture());

        assertTrue(Math.abs(lastSeenDateCaptor.getValue() - expectedLastSeenDateMs) <= toleranceMs);
        assertTrue(Math.abs(expiryDateCaptor.getValue() - expectedExpiryDateMs) <= toleranceMs);

        verify(globalDiscoveryEntryCacheMock, times(1)).touchDiscoveryEntries(expectedParticipantIds,
                                                                              lastSeenDateCaptor.getValue(),
                                                                              expiryDateCaptor.getValue());

        assertTrue(Math.abs(lastSeenDateCaptor.getValue() - expectedLastSeenDateMs) <= toleranceMs);
        assertTrue(Math.abs(expiryDateCaptor.getValue() - expectedExpiryDateMs) <= toleranceMs);

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(any(), eq(expectedParticipantIds), anyString());
    }

    @Test
    public void touchNotCalled_noParticipantIdsToTouch() {
        final String[] participantIdsToTouch = new String[0];
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(TimeUnit.MILLISECONDS));

        final Runnable runnable = runnableCaptor.getValue();
        runnable.run();

        verify(globalCapabilitiesDirectoryClient, times(0)).touch(any(), any(), anyString());
    }

    @Test
    public void touchCalledOnce_multipleParticipantIdsForSingleGbid() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        final String gbid = knownGbids[1];
        final String[] gbids = { gbid };

        final GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);
        entry1.setExpiryDateMs(0L);
        entry1.setLastSeenDateMs(0L);

        final GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(entry1);
        entry2.setParticipantId(participantId2);

        final Promise<Add1Deferred> promiseAdd1 = localCapabilitiesDirectory.add(entry1, true, gbids);
        final Promise<Add1Deferred> promiseAdd2 = localCapabilitiesDirectory.add(entry2, true, gbids);
        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        promiseChecker.checkPromiseSuccess(promiseAdd2, MSG_ON_ADD_REJECT);

        // Mock return values of localDiscoveryEntryStore.touchDiscoveryEntries
        final String[] participantIdsToTouch = new String[]{ participantId1, participantId2 };
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(TimeUnit.MILLISECONDS));

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .touch(any(), eq(participantIdsToTouch), anyString());
        sleep(FRESHNESS_UPDATE_INTERVAL_MS); // make sure that the initial delay has expired before starting the runnable
        final Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(any(), eq(participantIdsToTouch), eq(gbid));
    }

    @Test
    public void touchCalledOnce_singleParticipantIdForMultipleGbids() throws InterruptedException {
        final String participantId1 = "participantId1";

        final String gbid1 = knownGbids[1];
        final String gbid2 = knownGbids[2];
        final String[] gbids = { gbid1, gbid2 };

        final GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);
        entry1.setExpiryDateMs(0L);
        entry1.setLastSeenDateMs(0L);

        final Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(entry1, true, gbids);
        promiseChecker.checkPromiseSuccess(promiseAdd, MSG_ON_ADD_REJECT);

        // Mock return values of localDiscoveryEntryStore.touchDiscoveryEntries
        final String[] participantIdsToTouch = new String[]{ participantId1 };
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(TimeUnit.MILLISECONDS));

        final CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .touch(any(), eq(participantIdsToTouch), anyString());
        sleep(FRESHNESS_UPDATE_INTERVAL_MS); // make sure that the initial delay has expired before starting the runnable
        final Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).touch(any(), eq(participantIdsToTouch), eq(gbid1));
    }

    @Test
    public void touchCalledTwice_twoParticipantIdsForDifferentGbids() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        final String gbid1 = knownGbids[1];
        final String gbid2 = knownGbids[2];
        final String[] gbids1 = { gbid1 };
        final String[] gbids2 = { gbid2 };

        final GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);
        entry1.setExpiryDateMs(0L);
        entry1.setLastSeenDateMs(0L);

        final GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(entry1);
        entry2.setParticipantId(participantId2);

        final Promise<Add1Deferred> promiseAdd1 = localCapabilitiesDirectory.add(entry1, true, gbids1);
        final Promise<Add1Deferred> promiseAdd2 = localCapabilitiesDirectory.add(entry2, true, gbids2);
        promiseChecker.checkPromiseSuccess(promiseAdd1, MSG_ON_ADD_REJECT);
        promiseChecker.checkPromiseSuccess(promiseAdd2, MSG_ON_ADD_REJECT);

        // Mock return values of localDiscoveryEntryStore.touchDiscoveryEntries
        final String[] participantIdsToTouch = new String[]{ participantId1, participantId2 };
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(FRESHNESS_UPDATE_INTERVAL_MS),
                                                                        eq(TimeUnit.MILLISECONDS));

        final String[] expectedParticipantIds1 = new String[]{ participantId1 };
        final String[] expectedParticipantIds2 = new String[]{ participantId2 };

        final CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(answerCreateHelper.createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                                                 .touch(any(), any(), anyString());
        sleep(FRESHNESS_UPDATE_INTERVAL_MS); // make sure that the initial delay has expired before starting the runnable
        final Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(any(), eq(expectedParticipantIds1), eq(gbid1));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(any(), eq(expectedParticipantIds2), eq(gbid2));
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
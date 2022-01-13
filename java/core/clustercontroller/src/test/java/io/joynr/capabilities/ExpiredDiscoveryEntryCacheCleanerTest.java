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
package io.joynr.capabilities;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.types.DiscoveryEntry;

/**
 * Unit tests for the {@link ExpiredDiscoveryEntryCacheCleaner}.
 */
@RunWith(MockitoJUnitRunner.class)
public class ExpiredDiscoveryEntryCacheCleanerTest {

    @Mock
    private ScheduledExecutorService scheduledExecutorService;

    @Mock
    private DiscoveryEntryStore<DiscoveryEntry> cache;

    @Mock
    private ExpiredDiscoveryEntryCacheCleaner.CleanupAction cleanupAction;

    @Captor
    private ArgumentCaptor<Runnable> runnableArgumentCaptor;

    private ExpiredDiscoveryEntryCacheCleaner subject;

    @Mock
    private ShutdownNotifier shutdownNotifier;
    private ShutdownListener shutdownListener;

    @Before
    public void setup() {
        subject = new ExpiredDiscoveryEntryCacheCleaner(scheduledExecutorService, 1, shutdownNotifier);
        lenient().doAnswer(new Answer() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                shutdownListener = (ShutdownListener) invocation.getArguments()[0];
                return null;
            }
        }).when(shutdownNotifier).registerForShutdown(any());
    }

    @Test
    public void testScheduleCacheForCleanup() {
        subject.scheduleCleanUpForCaches(cleanupAction, cache);
        verify(scheduledExecutorService).scheduleAtFixedRate(any(Runnable.class), eq(1L), eq(1L), eq(TimeUnit.MINUTES));
    }

    @Test
    public void testExpiredEntriesCleanedUp() {
        Set<DiscoveryEntry> allEntries = new HashSet<>();

        DiscoveryEntry liveEntry = mock(DiscoveryEntry.class);
        when(liveEntry.getExpiryDateMs()).thenReturn(System.currentTimeMillis() + 1000L);
        allEntries.add(liveEntry);

        final String expiredParticipantId = "expiredParticipantId";
        DiscoveryEntry expiredEntry = mock(DiscoveryEntry.class);
        lenient().when(expiredEntry.getExpiryDateMs()).thenReturn(System.currentTimeMillis() - 1000L);
        lenient().when(expiredEntry.getParticipantId()).thenReturn(expiredParticipantId);
        allEntries.add(expiredEntry);

        when(cache.getAllDiscoveryEntries()).thenReturn(allEntries);

        subject.scheduleCleanUpForCaches(cleanupAction, cache);

        verify(scheduledExecutorService).scheduleAtFixedRate(runnableArgumentCaptor.capture(),
                                                             eq(1L),
                                                             eq(1L),
                                                             eq(TimeUnit.MINUTES));
        Runnable cleanupTask = runnableArgumentCaptor.getValue();
        cleanupTask.run();
        verify(cache).getAllDiscoveryEntries();
        verify(cleanupAction).cleanup(new HashSet<DiscoveryEntry>(Arrays.asList(expiredEntry)));
    }
}

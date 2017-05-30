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
package io.joynr.messaging.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.lang.Thread.State;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class BoundedDelayQueueTest {
    BoundedDelayQueue<TimedDelayed> boundedDelayQueue;
    private TimedDelayed delayed1;
    private TimedDelayed delayed2;
    final Collection<TimedDelayed> delayedTaken = new ArrayList<>();

    @Before
    public void setUp() throws Exception {
        delayed1 = new TimedDelayed() {
        };
        delayed2 = new TimedDelayed() {
        };
    }

    @After
    public void tearDown() throws Exception {
    }

    Thread runInThread(Runnable runnable) {
        Thread thread = new Thread(runnable);
        thread.setDaemon(true);
        thread.start();
        return thread;
    }

    @Test
    public void testTakingOnEmptyBlocks() throws InterruptedException {
        delayed1 = new TimedDelayed(0);

        boundedDelayQueue = new BoundedDelayQueue<TimedDelayed>(2);
        Thread runInThread = runInThread(new Runnable() {
            @Override
            public void run() {
                try {
                    delayedTaken.add(boundedDelayQueue.take());
                } catch (InterruptedException e) {
                    fail("unexpected interruption");
                }
            }
        });
        Thread.sleep(1);
        State state = runInThread.getState();
        assertEquals(State.WAITING, state);

        boundedDelayQueue.putBounded(delayed1);
        Thread.sleep(1);
        assertTrue(delayedTaken.contains(delayed1));
        assertTrue(boundedDelayQueue.isEmpty());

    }

    @Test
    public void testPuttingWhenFullBlocks() throws InterruptedException {
        delayed1 = new TimedDelayed(0);
        delayed2 = new TimedDelayed(0);
        boundedDelayQueue = new BoundedDelayQueue<TimedDelayed>(1);
        boundedDelayQueue.putBounded(delayed1);
        assertEquals(1, boundedDelayQueue.size());

        Thread runInThread = runInThread(new Runnable() {
            @Override
            public void run() {
                try {
                    boundedDelayQueue.putBounded(delayed2);
                } catch (Exception e) {
                    fail("unexpected interruption");
                }
            }
        });
        Thread.sleep(1);
        State state = runInThread.getState();
        assertEquals(State.WAITING, state);
        assertEquals(1, boundedDelayQueue.size());

        delayedTaken.add(boundedDelayQueue.take());
        Thread.sleep(1);
        assertEquals(1, boundedDelayQueue.size());

        delayedTaken.add(boundedDelayQueue.take());

        assertTrue(delayedTaken.contains(delayed1));
        assertTrue(delayedTaken.contains(delayed2));
    }

    @Test
    public void testMessageIsDelayed() throws InterruptedException {
        boundedDelayQueue = new BoundedDelayQueue<TimedDelayed>(1);
        delayed1 = new TimedDelayed(100);
        boundedDelayQueue.putBounded(delayed1);
        assertEquals(1, boundedDelayQueue.size());

        Thread runInThread = runInThread(new Runnable() {
            @Override
            public void run() {
                try {
                    delayedTaken.add(boundedDelayQueue.take());
                } catch (Exception e) {
                    fail("unexpected interruption");
                }
            }
        });
        Thread.sleep(1);
        State state = runInThread.getState();
        assertEquals(State.TIMED_WAITING, state);
        assertEquals(1, boundedDelayQueue.size());

        Thread.sleep(50);
        assertEquals(1, boundedDelayQueue.size());
        assertFalse(delayedTaken.contains(delayed1));

        Thread.sleep(60);
        assertTrue(delayedTaken.contains(delayed1));
        assertEquals(0, boundedDelayQueue.size());

    }

    @Test
    public void testBlockingOnDelayUnblocksOnNewPut() throws InterruptedException {
        delayed1 = new TimedDelayed(100);
        delayed2 = new TimedDelayed(0);
        boundedDelayQueue = new BoundedDelayQueue<TimedDelayed>(2);
        boundedDelayQueue.putBounded(delayed1);
        Thread runInThread = runInThread(new Runnable() {
            @Override
            public void run() {
                try {
                    delayedTaken.add(boundedDelayQueue.take());
                    delayedTaken.add(boundedDelayQueue.take());
                } catch (Exception e) {
                    fail("unexpected interruption");
                }
            }
        });
        Thread.sleep(1);
        State state = runInThread.getState();
        assertEquals(State.TIMED_WAITING, state);
        delayed2 = new TimedDelayed(0);
        boundedDelayQueue.putBounded(delayed2);
        Thread.sleep(1);
        assertEquals(1, boundedDelayQueue.size());

        assertTrue(delayedTaken.contains(delayed2));
        Thread.sleep(120);
        assertTrue(delayedTaken.contains(delayed1));
    }

}

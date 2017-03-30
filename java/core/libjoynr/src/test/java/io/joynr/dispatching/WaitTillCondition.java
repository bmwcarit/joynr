package io.joynr.dispatching;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.Collection;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * This is used for testing purposes, and waits for a given amount of time for an expected number of responses before
 * returning to the caller.
 * <p/>
 * This method also provides a few assert methods to simplify testing.
 */
public abstract class WaitTillCondition {

    protected Semaphore semaphoreMessages;
    protected Semaphore semaphoreErrors;
    protected final int numberOfMessagesExpected;
    protected final int numberOfErrorsExpected;

    public WaitTillCondition(int numberOfMessagesExpected) {
        numberOfErrorsExpected = 0;
        this.numberOfMessagesExpected = numberOfMessagesExpected;
        semaphoreMessages = new Semaphore(numberOfMessagesExpected);
        try {
            semaphoreMessages.acquire(numberOfMessagesExpected);
        } catch (InterruptedException e) {
            throw new RuntimeException("Error occurred while attempting to acquire all permits for the semaphore.", e);
        }
    }

    public WaitTillCondition(int numberOfMessagesExpected, int numberOfErrorsExpected) {
        this.numberOfMessagesExpected = numberOfMessagesExpected;
        this.numberOfErrorsExpected = numberOfErrorsExpected;

        // Messages
        semaphoreMessages = new Semaphore(numberOfMessagesExpected);
        try {
            semaphoreMessages.acquire(numberOfMessagesExpected);
        } catch (InterruptedException e) {
            throw new RuntimeException("Error occurred while attempting to acquire all permits for the semaphore.", e);
        }

        // Errors
        semaphoreErrors = new Semaphore(numberOfErrorsExpected);
        try {
            semaphoreErrors.acquire(numberOfErrorsExpected);
        } catch (InterruptedException e) {
            throw new RuntimeException("Error occurred while attempting to acquire all permits for the semaphore.", e);
        }
    }

    protected abstract Collection<Object> getReceivedPayloads();

    protected void releaseSemaphorePermit() {
        semaphoreMessages.release();
    }

    protected void releaseErrorSemaphorePermit() {
        semaphoreErrors.release();
    }

    public boolean waitForMessage(long timeOutMs) {
        try {
            if (!semaphoreMessages.tryAcquire(numberOfMessagesExpected, timeOutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("Error occurred while attempting to acquire all permits for the semaphore.", e);
        }
        semaphoreMessages.release(numberOfMessagesExpected);
        return true;
    }

    public boolean waitForErrors(long timeOutMs) {
        try {
            if (!semaphoreErrors.tryAcquire(numberOfErrorsExpected, timeOutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("Error occurred while attempting to acquire all permits for the semaphore.", e);
        }
        semaphoreMessages.release(numberOfMessagesExpected);
        return true;
    }

    public void assertAllPayloadsReceived(long timeOutMs) {
        assertTrue("wait for " + numberOfMessagesExpected + " messages did not succeed. "
                           + semaphoreMessages.availablePermits() + " messages received in time.",
                   waitForMessage(timeOutMs));
    }

    public void assertAllErrorsReceived(long timeOutMs) {
        assertTrue(waitForErrors(timeOutMs));
    }

    public void assertReceivedPayloadsContains(Object... payloads) {
        assertEquals(payloads.length, getReceivedPayloads().size());
        for (Object payload : payloads) {
            Collection<Object> receivedPayloads = getReceivedPayloads();
            assertTrue(receivedPayloads.contains(payload));
        }
    }

    public void assertReceivedPayloadsContainsNot(Object... payloads) {
        for (Object payload : payloads) {
            assertTrue(!getReceivedPayloads().contains(payload));
        }
    }

}

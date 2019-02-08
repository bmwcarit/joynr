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
package io.joynr.performance;

import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicInteger;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;

/**
 * Response callback for async requests. Is used to wait until all responses
 * arrived (this includes failures as well).
 */
public class AsyncResponseCounterCallback<type> extends Callback<type> {

    /**
     * Counts all responses (including failures)
     */
    private AtomicInteger responseCounter = new AtomicInteger(0);

    /**
     * Counts how many requests could not be transmitted
     */
    private AtomicInteger failureCounter = new AtomicInteger(0);

    private Semaphore responseCounterSemaphore = new Semaphore(0);

    @Override
    public void onFailure(JoynrRuntimeException runtimeException) {
        responseCounter.incrementAndGet();
        failureCounter.incrementAndGet();
        responseCounterSemaphore.release();
    }

    @Override
    public void onSuccess(type result) {
        responseCounter.incrementAndGet();
        responseCounterSemaphore.release();
    }

    public void acquire() {
        try {
            responseCounterSemaphore.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    public void waitForNumberOfResponses(int numExpectedResponses, int sleepIntervalMilliSec) {
        while (responseCounter.get() < numExpectedResponses) {
            try {
                Thread.sleep(sleepIntervalMilliSec);
            } catch (InterruptedException e) {
                // The thread was interrupted/stopped. We can ignore this
                // exception
            }
        }
    }

    public void waitForNumberOfResponses(int numExpectedResponses) {
        try {
            responseCounterSemaphore.acquire(numExpectedResponses);
        } catch (InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    public void release(int permits) {
        responseCounterSemaphore.release(permits);
    }

    public boolean failuresOccured() {
        return failureCounter.get() > 0;
    }

    public int getNumberOfFailures() {
        return failureCounter.get();
    }

    public int getNumberOfRespones() {
        return responseCounter.get();
    }
}

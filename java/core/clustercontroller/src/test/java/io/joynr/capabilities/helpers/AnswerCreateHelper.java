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
package io.joynr.capabilities.helpers;

import static org.junit.Assert.fail;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.mockito.stubbing.Answer;
import org.slf4j.Logger;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

public class AnswerCreateHelper {

    private final Logger logger;
    private final int timeout;

    public AnswerCreateHelper(final Logger logger, final int timeout) {
        this.logger = logger;
        this.timeout = timeout;
    }

    public Answer<Void> createAnswerWithSuccess() {
        return createAnswerWithSuccess(null, null);
    }

    public Answer<Void> createAnswerWithSuccess(final Semaphore successCallbackSemaphore1,
                                                final Semaphore successCallbackSemaphore2) {
        return invocation -> {
            final Object[] args = invocation.getArguments();
            if (successCallbackSemaphore1 != null) {
                logDebugMessage("success callback called");
                successCallbackSemaphore1.release();
            }
            if (successCallbackSemaphore2 != null) {
                logDebugMessage("waiting for Semaphore in success callback");
                final var result = successCallbackSemaphore2.tryAcquire(timeout, TimeUnit.MILLISECONDS);
                logDebugMessage("waiting for Semaphore in success callback result {}", result);
            }
            @SuppressWarnings("unchecked")
            final Callback<Void> argument = (Callback<Void>) args[0];
            argument.onSuccess(null);
            return null;
        };
    }

    private void logDebugMessage(final String messagePattern, final Object... arguments) {
        if (logger != null) {
            logger.debug(messagePattern, arguments);
        }
    }

    public Answer<Void> createAnswerWithDelayedSuccess(final CountDownLatch cdlStart,
                                                       final CountDownLatch cdlDone,
                                                       final long delay) {
        return invocation -> {
            @SuppressWarnings("unchecked")
            final Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
            new Thread(() -> {
                cdlStart.countDown();
                try {
                    Thread.sleep(delay);
                } catch (Exception e) {
                    fail("SLEEP INTERRUPTED");
                }
                callback.onSuccess(null);
                cdlDone.countDown();
            }).start();
            return null;
        };
    }

    public Answer<Void> createAnswerWithSuccess(final CountDownLatch cdl) {
        return invocation -> {
            final Object[] args = invocation.getArguments();
            @SuppressWarnings("unchecked")
            final Callback<Void> argument = (Callback<Void>) args[0];
            argument.onSuccess(null);
            cdl.countDown();
            return null;
        };
    }

    public Answer<Future<List<GlobalDiscoveryEntry>>> createLookupAnswer(final List<GlobalDiscoveryEntry> caps) {
        return invocation -> {
            final Future<List<GlobalDiscoveryEntry>> result = new Future<>();
            @SuppressWarnings("unchecked")
            final Callback<List<GlobalDiscoveryEntry>> callback = (Callback<List<GlobalDiscoveryEntry>>) invocation.getArguments()[0];
            callback.onSuccess(caps);
            result.onSuccess(caps);
            return result;
        };
    }

    public Answer<Future<GlobalDiscoveryEntry>> createLookupAnswer(final GlobalDiscoveryEntry caps) {
        return invocation -> {
            final Future<GlobalDiscoveryEntry> result = new Future<>();
            @SuppressWarnings("unchecked")
            final Callback<GlobalDiscoveryEntry> callback = (Callback<GlobalDiscoveryEntry>) invocation.getArguments()[0];
            callback.onSuccess(caps);
            result.onSuccess(caps);
            return result;
        };
    }

    public Answer<Void> createVoidAnswerWithDiscoveryError(final CountDownLatch cdl, final DiscoveryError error) {
        return invocation -> {
            final Object[] args = invocation.getArguments();
            @SuppressWarnings("unchecked")
            CallbackWithModeledError<Void, DiscoveryError> callback = ((CallbackWithModeledError<Void, DiscoveryError>) args[0]);
            callback.onFailure(error);
            cdl.countDown();
            return null;
        };
    }

    public Answer<Void> createVoidAnswerWithDiscoveryError(final DiscoveryError error) {
        final CountDownLatch cdl = new CountDownLatch(0);
        return createVoidAnswerWithDiscoveryError(cdl, error);
    }

    public Answer<Void> createVoidAnswerWithException(final JoynrRuntimeException exception) {
        final CountDownLatch cdl = new CountDownLatch(0);
        return createVoidAnswerWithException(cdl, exception);
    }

    public Answer<Void> createVoidAnswerWithException(final CountDownLatch cdl, final JoynrRuntimeException exception) {
        return invocation -> {
            @SuppressWarnings("unchecked")
            final Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
            callback.onFailure(exception);
            cdl.countDown();
            return null;
        };
    }
}

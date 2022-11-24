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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import io.joynr.exceptions.JoynrException;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.types.DiscoveryError;

public class PromiseChecker {

    private static final String MSG_UNEXPECTED_SUCCESS = "Unexpected fulfillment when expecting rejection.";

    private final int waitTimeMs;

    public PromiseChecker(final int waitTimeMs) {
        this.waitTimeMs = waitTimeMs;
    }

    public Object[] checkPromiseSuccess(final Promise<? extends AbstractDeferred> promise,
                                        final String onRejectionMessage) throws InterruptedException {
        final List<Object> result = new ArrayList<>();
        final CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {
            @Override
            public void onRejection(final JoynrException error) {
                fail(onRejectionMessage + ": " + error);
            }

            @Override
            public void onFulfillment(final Object... values) {
                result.addAll(Arrays.asList(values));
                countDownLatch.countDown();
            }
        });
        assertTrue(onRejectionMessage + ": promise timeout", countDownLatch.await(waitTimeMs, TimeUnit.MILLISECONDS));
        return result.toArray(new Object[0]);
    }

    public void checkPromiseErrorInProviderRuntimeException(final Promise<?> promise,
                                                            final DiscoveryError expectedError) throws InterruptedException {
        final CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {
            @Override
            public void onRejection(final JoynrException exception) {
                if (exception instanceof ProviderRuntimeException) {
                    assertTrue(((ProviderRuntimeException) exception).getMessage().contains(expectedError.name()));
                    countDownLatch.countDown();
                } else {
                    fail("Did not receive a ProviderRuntimeException on rejection.");
                }
            }

            @Override
            public void onFulfillment(final Object... values) {
                fail(MSG_UNEXPECTED_SUCCESS);
            }
        });
        assertTrue(countDownLatch.await(waitTimeMs, TimeUnit.MILLISECONDS));
    }

    public void checkPromiseError(final Promise<?> promise,
                                  final DiscoveryError expectedError) throws InterruptedException {
        final CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {
            @Override
            public void onRejection(final JoynrException exception) {
                if (exception instanceof ApplicationException) {
                    final DiscoveryError error = ((ApplicationException) exception).getError();
                    assertEquals(expectedError, error);
                    countDownLatch.countDown();
                } else {
                    fail("Did not receive an ApplicationException on rejection.");
                }
            }

            @Override
            public void onFulfillment(final Object... values) {
                fail(MSG_UNEXPECTED_SUCCESS);
            }
        });
        assertTrue(countDownLatch.await(waitTimeMs, TimeUnit.MILLISECONDS));
    }

    public void checkPromiseException(final Promise<?> promise,
                                      final Exception expectedException) throws InterruptedException {
        final CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {
            @Override
            public void onRejection(final JoynrException exception) {
                assertTrue(expectedException.getClass().isInstance(exception));
                assertEquals(expectedException, exception);
                countDownLatch.countDown();
            }

            @Override
            public void onFulfillment(Object... values) {
                fail(MSG_UNEXPECTED_SUCCESS);
            }
        });
        assertTrue(countDownLatch.await(waitTimeMs, TimeUnit.MILLISECONDS));
    }
}

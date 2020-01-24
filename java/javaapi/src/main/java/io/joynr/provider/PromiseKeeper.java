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
package io.joynr.provider;

import java.util.Arrays;
import java.util.Optional;

import io.joynr.exceptions.JoynrException;

public class PromiseKeeper implements PromiseListener {
    private enum State {
        PENDING, FULFILLED, REJECTED
    }

    private State state = State.PENDING;
    private Object[] values = null;
    private JoynrException error = null;

    @Override
    public void onFulfillment(Object... values) {
        synchronized (this) {
            this.values = values;
            state = State.FULFILLED;
            notifyAll();
        }
    }

    @Override
    public void onRejection(JoynrException error) {
        synchronized (this) {
            this.error = error;
            state = State.REJECTED;
            notifyAll();
        }
    }

    /**
     * Get the resolved values of the promise. If the promise is not settled,
     * the call blocks until the promise is settled.
     *
     * @return the resolved values or null in case of timeout.
     * @throws InterruptedException if the thread is interrupted.
     */
    public Optional<Object[]> getValues() throws InterruptedException {
        return getValues(0);
    }

    /**
     * Get the resolved values of the promise. If the promise is not settled,
     * the call blocks until the promise is settled or timeout is reached.
     *
     * @param timeout the maximum time to wait in milliseconds.
     * @return the resolved values or null in case of timeout.
     * @throws InterruptedException if the thread is interrupted.
     */
    public Optional<Object[]> getValues(long timeout) throws InterruptedException {
        if (!isSettled()) {
            synchronized (this) {
                wait(timeout);
            }
        }
        if (values == null) {
            return Optional.ofNullable(null);
        }
        return Optional.of(Arrays.copyOf(values, values.length));
    }

    /**
     * Get the error causing rejection of the promise. If the promise is not
     * settled, the call blocks until the promise is settled.
     *
     * @return the error causing rejection or null in case of timeout.
     * @throws InterruptedException if the thread is interrupted.
     */
    public Optional<JoynrException> getError() throws InterruptedException {
        return getError(0);
    }

    /**
     * Get the error causing rejection of the promise. If the promise is not
     * settled, the call blocks until the promise is settled or timeout is
     * reached.
     *
     * @param timeout the maximum time to wait in milliseconds.
     * @return the error causing rejection or null in case of timeout.
     * @throws InterruptedException if the thread is interrupted.
     */
    public Optional<JoynrException> getError(long timeout) throws InterruptedException {
        if (!isSettled()) {
            synchronized (this) {
                wait(timeout);
            }
        }
        return Optional.ofNullable(error);
    }

    /**
     * Blocks until the promise is settled.
     *
     * @throws InterruptedException if the thread is interrupted.
     */
    public void waitForSettlement() throws InterruptedException {
        waitForSettlement(0);
    }

    /**
     * Blocks until the promise is settled or timeout is reached.
     *
     * @param timeout the maximum time to wait in milliseconds.
     * @throws InterruptedException if the thread is interrupted.
     */
    public void waitForSettlement(long timeout) throws InterruptedException {
        if (!isSettled()) {
            synchronized (this) {
                wait(timeout);
            }
        }
    }

    public boolean isFulfilled() {
        return state == State.FULFILLED;
    }

    public boolean isRejected() {
        return state == State.REJECTED;
    }

    public boolean isSettled() {
        return isFulfilled() || isRejected();
    }
}

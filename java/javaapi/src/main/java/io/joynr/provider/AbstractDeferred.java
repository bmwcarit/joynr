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

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import io.joynr.exceptions.JoynrException;
import joynr.exceptions.ProviderRuntimeException;

public abstract class AbstractDeferred {
    private enum State {
        PENDING, FULFILLED, REJECTED
    };

    private State state = State.PENDING;
    private JoynrException error = null;
    private Object[] values = null;

    private List<DeferredListener> listeners = new ArrayList<DeferredListener>();

    public AbstractDeferred() {
    }

    /**
     * Resolves the promise. NOTE: The thread resolving the promise will be
     * used to execute waiting listeners.
     * @param values the result which resolves the Deferred.
     * @return true if the promise is resolved; false in case the promise is
     *      already settled.
     */
    protected synchronized boolean resolve(Object... values) {
        if (isSettled()) {
            return false;
        }
        this.values = values;
        state = State.FULFILLED;
        notifyListeners();
        return true;
    }

    /**
     * Rejects the promise. NOTE: The thread rejecting the promise will be used
     * to execute waiting listeners.
     * @param error the reason that caused the rejection.
     * @return true if the promise is rejected; false in case the promise is
     *      already settled.
     */
    protected synchronized boolean reject(JoynrException error) {
        if (isSettled()) {
            return false;
        }
        state = State.REJECTED;
        this.error = error;
        notifyListeners();
        return true;
    }

    /**
     * Rejects the promise. NOTE: The thread rejecting the promise will be used
     * to execute waiting listeners.
     * @param error the reason that caused the rejection.
     * @return true if the promise is rejected; false in case the promise is
     *      already settled.
     */
    public synchronized boolean reject(ProviderRuntimeException error) {
        return this.reject((JoynrException) error);
    }

    /**
     * @return the error that caused the rejection of the deferred; null if the
     *      deferred is not in rejected state.
     */
    public Optional<JoynrException> getError() {
        return Optional.ofNullable(error);
    }

    /**
     * @return the values that caused the fulfillment of the deferred; null if
     *      the deferred is not in fulfilled state.
     */
    public Optional<Object[]> getValues() {
        return Optional.ofNullable(values);
    }

    private void notifyListeners() {
        for (DeferredListener listener : listeners) {
            listener.onSettlement();
        }
    }

    /**
     * Adds a listener that is called once the deferred is settled. NOTE: If the
     * deferred is already settled when adding the listener, the thread adding
     * the listener will be used to execute the listener.
     * @param listener the listener to add.
     */
    public synchronized void addListener(DeferredListener listener) {
        listeners.add(listener);
        if (isSettled()) {
            listener.onSettlement();
        }
    }

    public boolean isFulfilled() {
        return state == State.FULFILLED;
    }

    public boolean isRejected() {
        return state == State.REJECTED;
    }

    /**
     * @return true if the promise is fulfilled or rejected; false if the
     *      promise is pending.
     */
    public boolean isSettled() {
        return state == State.FULFILLED || state == State.REJECTED;
    }
}

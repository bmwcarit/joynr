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

import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrException;

public class Promise<T extends AbstractDeferred> {
    private T deferred;
    private List<PromiseListener> listeners = new ArrayList<PromiseListener>();

    public Promise(T deferred) {
        this.deferred = deferred;
        this.deferred.addListener(new DeferredListener() {

            @Override
            public void onSettlement() {
                notifyListeners();
            }
        });
    }

    /**
     * Adds a listener that is called once the promise is settled. NOTE: If the
     * promise is already settled when adding the listener, the thread adding
     * the listener will be used to execute the listener.
     * @param listener the listener that will be called once the promise is settled
     */
    public void then(PromiseListener listener) {
        if (listener == null) {
            return;
        }
        listeners.add(listener);
        if (isSettled()) {
            notifyListener(listener);
        }
    }

    private void notifyListeners() {
        for (PromiseListener listener : listeners) {
            notifyListener(listener);
        }
    }

    private void notifyListener(PromiseListener listener) {
        if (isFulfilled()) {
            Optional<Object[]> values = deferred.getValues();
            if (values.isPresent()) {
                listener.onFulfillment(values.get());
            } else {
                listener.onFulfillment(null);
            }
        } else if (isRejected()) {
            Optional<JoynrException> error = deferred.getError();
            if (error.isPresent()) {
                listener.onRejection(error.get());
            } else {
                listener.onRejection(new JoynrCommunicationException("Promise rejected unexpectedly."));
            }
        }
    }

    public boolean isFulfilled() {
        return deferred.isFulfilled();
    }

    public boolean isRejected() {
        return deferred.isRejected();
    }

    /**
     * @return true if the promise is fulfilled or rejected; false if the
     *      promise is pending.
     */
    public boolean isSettled() {
        return deferred.isSettled();
    }
}

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
package io.joynr.proxy;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import io.joynr.dispatcher.rpc.RequestStatus;
import io.joynr.dispatcher.rpc.RequestStatusCode;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import joynr.exceptions.ApplicationException;

public class Future<T> {

    private T value;
    private AtomicReference<JoynrException> exception = new AtomicReference<>(null);
    RequestStatus status = new RequestStatus(RequestStatusCode.IN_PROGRESS);
    private Lock statusLock = new ReentrantLock();
    private Condition statusLockChangedCondition = statusLock.newCondition();

    /**
     * This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If the request finishes successfully, it retrieves the return
     * value for the request if one exists, otherwise a JoynrException is thrown.
     *
     * @param timeoutMs
     *            The maximum number of milliseconds to wait before this request times out
     * @return the result of the request
     * @throws InterruptedException if the thread is interrupted.
     * @throws JoynrWaitExpiredException
     *             if timeout_ms expires
     * @throws ApplicationException if the request failed with a ApplicationException
     * @throws JoynrRuntimeException if the request failed with a JoynrRuntimeException
     */
    public T get(long timeoutMs) throws InterruptedException, JoynrWaitExpiredException, ApplicationException,
                                 JoynrRuntimeException {
        try {
            statusLock.lock();
            if (this.status.getCode() == RequestStatusCode.OK) {
                return value;
            }

            JoynrException e = exception.get();
            if (e != null) {
                if (e instanceof ApplicationException) {
                    throw (ApplicationException) e;
                }
                throw (JoynrRuntimeException) e;
            }

            while (RequestStatusCode.IN_PROGRESS.equals(status.getCode())) {
                boolean awaitOk = statusLockChangedCondition.await(timeoutMs, TimeUnit.MILLISECONDS);
                if (!awaitOk) {
                    this.status.setCode(RequestStatusCode.ERROR);
                    throw new JoynrWaitExpiredException();
                }
            }

            // check if an exception has arrived while waiting
            e = exception.get();
            if (e != null) {
                if (e instanceof ApplicationException) {
                    throw (ApplicationException) e;
                }
                throw (JoynrRuntimeException) e;
            }

            return value;
        } finally {
            statusLock.unlock();
        }
    }

    /**
     * This is a blocking call which waits until the request finishes/an error
     * occurs. If the request finishes successfully, it retrieves the return
     * value for the request if one exists, otherwise a JoynrException is thrown.
     *
     * @return the result of the request
     * @throws InterruptedException if the thread is interrupted.
     * @throws JoynrWaitExpiredException
     *             if timeout_ms expires
     * @throws ApplicationException if the request failed with a ApplicationException
     * @throws JoynrRuntimeException if the request failed with a JoynrRuntimeException
     */
    public T get() throws InterruptedException, JoynrWaitExpiredException, ApplicationException, JoynrRuntimeException {
        return this.get(Long.MAX_VALUE);
    }

    public RequestStatus getStatus() {
        return new RequestStatus(status);
    }

    /**
     * Resolves the future using the given result
     *
     * @param result
     *            the result of the asynchronous call
     */
    public void onSuccess(T result) {
        try {
            statusLock.lock();
            value = result;
            status = new RequestStatus(RequestStatusCode.OK);
            statusLockChangedCondition.signalAll();
        } catch (Exception e) {
            status = new RequestStatus(RequestStatusCode.ERROR);
            exception.set(new JoynrRuntimeException(e));
        } finally {
            statusLock.unlock();
        }
    }

    /**
     * Terminates the future in error
     *
     * @param newException
     *            that caused the failure
     */
    public void onFailure(JoynrException newException) {
        status = new RequestStatus(RequestStatusCode.ERROR);
        try {
            statusLock.lock();
            exception.set(newException);
            statusLockChangedCondition.signalAll();
        } finally {
            statusLock.unlock();
        }
    }

    @SuppressWarnings("unchecked")
    public void resolve(Object... outParameters) {
        if (outParameters.length == 0) {
            onSuccess(null);
        } else {
            onSuccess((T) outParameters[0]);
        }
    }
}

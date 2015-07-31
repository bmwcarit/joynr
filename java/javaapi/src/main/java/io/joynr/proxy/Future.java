package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.dispatcher.rpc.RequestStatus;
import io.joynr.dispatcher.rpc.RequestStatusCode;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrWaitExpiredException;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class Future<T> {

    private T reply;
    private JoynrRuntimeException exception = null;
    RequestStatus status = new RequestStatus(RequestStatusCode.IN_PROGRESS);
    private Lock statusLock = new ReentrantLock();
    private Condition statusLockChangedCondition = statusLock.newCondition();

    /**
     *
     * @param timeout_ms
     *            time to wait until throwing a JoynWaitExpiredException
     * @return the result of the method call
     * @throws InterruptedException if the thread is interrupted.
     * @throws JoynrWaitExpiredException
     *             if timeout_ms expires
     */
    public T getReply(long timeout_ms) throws InterruptedException, JoynrWaitExpiredException {
        try {
            statusLock.lock();
            if (this.status.getCode() == RequestStatusCode.OK) {
                return reply;
            }

            if (exception != null) {
                throw exception;
            }

            boolean awaitOk = statusLockChangedCondition.await(timeout_ms, TimeUnit.MILLISECONDS);

            // check if an exception has arrived while waiting
            if (exception != null) {
                throw exception;
            }

            if (!awaitOk) {
                this.status.setCode(RequestStatusCode.ERROR);
                throw new JoynrWaitExpiredException();
            }

            return reply;
        } finally {
            statusLock.unlock();
        }
    }

    /**
     *
     * @return the result of the method call
     * @throws InterruptedException
     *             - if the current thread is interrupted (and interruption of thread suspension is supported)
     */
    public T getReply() throws InterruptedException {
        return this.getReply(Long.MAX_VALUE);
    }

    public RequestStatus getStatus() {
        return status;
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
            reply = result;
            status = new RequestStatus(RequestStatusCode.OK);
            statusLockChangedCondition.signalAll();
        } catch (Throwable e) {
            status = new RequestStatus(RequestStatusCode.ERROR);
            exception = new JoynrRuntimeException(e);
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
    public void onFailure(JoynrRuntimeException newException) {
        exception = newException;
        status = new RequestStatus(RequestStatusCode.ERROR);
        try {
            statusLock.lock();
            statusLockChangedCondition.signalAll();
        } finally {
            statusLock.unlock();
        }
    }

    @SuppressWarnings("unchecked")
    public void resolve(Object... response) {
        if (response.length == 0) {
            onSuccess(null);
        } else if (response[0] instanceof JoynrRuntimeException) {
            onFailure((JoynrRuntimeException) response[0]);
        } else {
            onSuccess((T) response[0]);
        }
    }
}

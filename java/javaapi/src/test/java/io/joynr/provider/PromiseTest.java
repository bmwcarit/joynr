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

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.tests.testProvider;

@RunWith(MockitoJUnitRunner.class)
public class PromiseTest {

    @Test
    public void promiseStateIsCorrectOnRejection() {
        AbstractDeferred deferred = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promise = new Promise<AbstractDeferred>(deferred);

        Assert.assertFalse(promise.isSettled());
        Assert.assertFalse(promise.isRejected());
        Assert.assertFalse(promise.isFulfilled());
        deferred.reject(new JoynrRuntimeException());

        Assert.assertTrue(promise.isSettled());
        Assert.assertTrue(promise.isRejected());
        Assert.assertFalse(promise.isFulfilled());
    }

    @Test
    public void promiseStateIsCorrectOnFulfillment() {
        testProvider.MethodWithNoInputParametersDeferred deferred = new testProvider.MethodWithNoInputParametersDeferred();
        Promise<testProvider.MethodWithNoInputParametersDeferred> promise = new Promise<testProvider.MethodWithNoInputParametersDeferred>(deferred);

        Assert.assertFalse(promise.isSettled());
        Assert.assertFalse(promise.isRejected());
        Assert.assertFalse(promise.isFulfilled());
        deferred.resolve(42);

        Assert.assertTrue(promise.isSettled());
        Assert.assertFalse(promise.isRejected());
        Assert.assertTrue(promise.isFulfilled());
    }

    @Test
    public void addingNullListenerDoesNotThrow() {
        AbstractDeferred deferred = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promise = new Promise<AbstractDeferred>(deferred);

        promise.then(null);
        deferred.reject(new JoynrRuntimeException("test exception"));
    }

    @Test
    public void promiseNotifiesListenersOnRejection() {
        AbstractDeferred deferred = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promise = new Promise<AbstractDeferred>(deferred);
        PromiseListener listener = Mockito.mock(PromiseListener.class);

        JoynrRuntimeException expectedError = new JoynrRuntimeException("test exception");

        promise.then(listener);
        Assert.assertFalse(promise.isSettled());
        deferred.reject(expectedError);

        Assert.assertTrue(promise.isRejected());
        Mockito.verify(listener).onRejection(expectedError);
    }

    @Test
    public void rejectedPromiseNotifiesListener() {
        AbstractDeferred deferred = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promise = new Promise<AbstractDeferred>(deferred);
        PromiseListener listener = Mockito.mock(PromiseListener.class);

        JoynrRuntimeException expectedError = new JoynrRuntimeException("test exception");

        deferred.reject(expectedError);
        Assert.assertTrue(promise.isRejected());
        promise.then(listener);

        Mockito.verify(listener).onRejection(expectedError);
    }

    @Test
    public void promiseNotifiesListenersOnFulfillment() {
        testProvider.MethodWithNoInputParametersDeferred deferred = new testProvider.MethodWithNoInputParametersDeferred();
        Promise<testProvider.MethodWithNoInputParametersDeferred> promise = new Promise<testProvider.MethodWithNoInputParametersDeferred>(deferred);
        PromiseListener listener = Mockito.mock(PromiseListener.class);

        Integer expectedValue = 42;

        promise.then(listener);
        Assert.assertFalse(promise.isSettled());
        deferred.resolve(expectedValue);

        Assert.assertTrue(promise.isFulfilled());
        Mockito.verify(listener).onFulfillment(new Object[]{ expectedValue });
    }

    @Test
    public void fulfilledPromiseNotifiesListener() {
        testProvider.MethodWithNoInputParametersDeferred deferred = new testProvider.MethodWithNoInputParametersDeferred();
        Promise<testProvider.MethodWithNoInputParametersDeferred> promise = new Promise<testProvider.MethodWithNoInputParametersDeferred>(deferred);
        PromiseListener listener = Mockito.mock(PromiseListener.class);

        Integer expectedValue = 42;

        deferred.resolve(expectedValue);
        Assert.assertTrue(promise.isFulfilled());
        promise.then(listener);

        Mockito.verify(listener).onFulfillment(new Object[]{ expectedValue });
    }
}

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

@RunWith(MockitoJUnitRunner.class)
public class DeferredVoidTest {

    @Test
    public void promiseStateIsCorrectOnFulfillment() {
        DeferredVoid deferred = new DeferredVoid();
        Promise<DeferredVoid> promise = new Promise<DeferredVoid>(deferred);

        Assert.assertFalse(promise.isSettled());
        Assert.assertFalse(promise.isRejected());
        Assert.assertFalse(promise.isFulfilled());
        deferred.resolve();

        Assert.assertTrue(promise.isSettled());
        Assert.assertFalse(promise.isRejected());
        Assert.assertTrue(promise.isFulfilled());
    }

    @Test
    public void promiseNotifiesListenersOnFulfillment() {
        DeferredVoid deferred = new DeferredVoid();
        Promise<DeferredVoid> promise = new Promise<DeferredVoid>(deferred);
        PromiseListener listener = Mockito.mock(PromiseListener.class);

        promise.then(listener);
        Assert.assertFalse(promise.isSettled());
        deferred.resolve();

        Assert.assertTrue(promise.isFulfilled());
        Mockito.verify(listener).onFulfillment(new Object[]{});
    }

    @Test
    public void fulfilledPromiseNotifiesListener() {
        DeferredVoid deferred = new DeferredVoid();
        Promise<DeferredVoid> promise = new Promise<DeferredVoid>(deferred);
        PromiseListener listener = Mockito.mock(PromiseListener.class);

        deferred.resolve();
        Assert.assertTrue(promise.isFulfilled());
        promise.then(listener);

        Mockito.verify(listener).onFulfillment(new Object[]{});
    }
}

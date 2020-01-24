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

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.tests.testProvider;

@RunWith(MockitoJUnitRunner.class)
public class PromiseKeeperTest {

    static ExecutorService executer = Executors.newSingleThreadExecutor();

    @Test
    public void keeperStateIsCorrectOnRejection() {
        AbstractDeferred deferred = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promise = new Promise<AbstractDeferred>(deferred);
        PromiseKeeper keeper = new PromiseKeeper();
        promise.then(keeper);

        Assert.assertFalse(keeper.isSettled());
        Assert.assertFalse(keeper.isRejected());
        Assert.assertFalse(keeper.isFulfilled());
        deferred.reject(new JoynrRuntimeException());

        Assert.assertTrue(keeper.isSettled());
        Assert.assertTrue(keeper.isRejected());
        Assert.assertFalse(keeper.isFulfilled());
    }

    @Test
    public void keeperStateIsCorrectOnFulfillment() {
        testProvider.MethodWithNoInputParametersDeferred deferred = new testProvider.MethodWithNoInputParametersDeferred();
        Promise<testProvider.MethodWithNoInputParametersDeferred> promise = new Promise<testProvider.MethodWithNoInputParametersDeferred>(deferred);
        PromiseKeeper keeper = new PromiseKeeper();

        promise.then(keeper);

        Assert.assertFalse(keeper.isSettled());
        Assert.assertFalse(keeper.isRejected());
        Assert.assertFalse(keeper.isFulfilled());
        deferred.resolve(42);

        Assert.assertTrue(keeper.isSettled());
        Assert.assertFalse(keeper.isRejected());
        Assert.assertTrue(keeper.isFulfilled());
    }

    @Test
    public void unsettledKeeperTimesOut() throws InterruptedException {
        AbstractDeferred deferred = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promise = new Promise<AbstractDeferred>(deferred);
        PromiseKeeper keeper = new PromiseKeeper();

        promise.then(keeper);

        Assert.assertFalse(keeper.getError(10).isPresent());
        Assert.assertFalse(keeper.getValues(10).isPresent());
    }

    @Test
    public void getErrorWaitsForRejection() throws InterruptedException {
        final AbstractDeferred deferred = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promise = new Promise<AbstractDeferred>(deferred);
        PromiseKeeper keeper = new PromiseKeeper();
        final JoynrRuntimeException expectedError = new JoynrRuntimeException("test exception");

        promise.then(keeper);
        executer.submit(new Runnable() {

            @Override
            public void run() {
                // give test some time to proceed, so getError runs into wait
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    // ignore
                }
                deferred.reject(expectedError);
            }
        });
        JoynrException error = keeper.getError().get();

        Assert.assertEquals(expectedError, error);
    }

    @Test
    public void getValuesWaitsForFulfillment() throws InterruptedException {
        final testProvider.MethodWithNoInputParametersDeferred deferred = new testProvider.MethodWithNoInputParametersDeferred();
        Promise<testProvider.MethodWithNoInputParametersDeferred> promise = new Promise<testProvider.MethodWithNoInputParametersDeferred>(deferred);
        PromiseKeeper keeper = new PromiseKeeper();
        final Integer expectedValue = 42;

        promise.then(keeper);
        executer.submit(new Runnable() {

            @Override
            public void run() {
                // give test some time to proceed, so getValues runs into wait
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    // ignore
                }
                deferred.resolve(expectedValue);
            }
        });
        Object[] values = keeper.getValues().get();

        Assert.assertArrayEquals(new Object[]{ expectedValue }, values);
    }

    @Test
    public void waitForSettlementBlocks() throws InterruptedException {
        final AbstractDeferred deferredToResolve = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promiseToResolve = new Promise<AbstractDeferred>(deferredToResolve);
        PromiseKeeper keeperToResolve = new PromiseKeeper();

        promiseToResolve.then(keeperToResolve);
        executer.submit(new Runnable() {

            @Override
            public void run() {
                // give test some time to proceed, so getValues runs into wait
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    // ignore
                }
                deferredToResolve.resolve();
            }
        });
        keeperToResolve.waitForSettlement();

        Assert.assertTrue(keeperToResolve.isSettled());

        final AbstractDeferred deferredToReject = new AbstractDeferred() {
        };
        Promise<AbstractDeferred> promiseToReject = new Promise<AbstractDeferred>(deferredToReject);
        PromiseKeeper keeperToReject = new PromiseKeeper();

        promiseToReject.then(keeperToReject);
        executer.submit(new Runnable() {

            @Override
            public void run() {
                // give test some time to proceed, so getValues runs into wait
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    // ignore
                }
                deferredToReject.reject(null);
            }
        });
        keeperToReject.waitForSettlement();

        Assert.assertTrue(keeperToReject.isSettled());
    }
}

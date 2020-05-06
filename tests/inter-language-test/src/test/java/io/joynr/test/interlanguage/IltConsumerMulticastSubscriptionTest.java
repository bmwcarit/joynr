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
package io.joynr.test.interlanguage;

import static org.junit.Assert.fail;

import java.util.concurrent.Semaphore;

import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.SubscriptionException;
import io.joynr.proxy.Future;
import joynr.MulticastSubscriptionQos;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleEnumerationParameterBroadcastAdapter;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;

public class IltConsumerMulticastSubscriptionTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerMulticastSubscriptionTest.class);

    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
    Semaphore callbackCalledSemaphore = new Semaphore(0);

    @BeforeClass
    public static void setUp() throws Exception {
        logger.info("setUp: Entering");
        setupConsumerRuntime(false);
        logger.info("setUp: Leaving");
    }

    @AfterClass
    public static void tearDown() throws InterruptedException {
        logger.info("tearDown: Entering");
        generalTearDown();
        logger.info("tearDown: Leaving");
    }

    @Test
    public void doNotReceivePublicationsForOtherPartitions() {

        String[] subscribeToPartitions = new String[]{ "partitions0", "partitions1" };
        String[] broadcastPartitions = new String[]{ "otherPartition" };

        try {
            BroadcastWithSingleEnumerationParameterBroadcastAdapter adapter = new BroadcastWithSingleEnumerationParameterBroadcastAdapter() {
                @Override
                public void onReceive(ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = true;
                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                    synchronized (callbackCalledSemaphore) {
                        callbackCalledSemaphore.notify();
                    }
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                    synchronized (callbackCalledSemaphore) {
                        callbackCalledSemaphore.notify();
                    }
                }
            };

            Future<String> subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithSingleEnumerationParameterBroadcast(adapter,
                                                                                                                                 new MulticastSubscriptionQos(),
                                                                                                                                 subscribeToPartitions);

            String subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);

            logger.info(name.getMethodName() + " - Invoking fire method with not matching partitions");
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter(broadcastPartitions);

            synchronized (callbackCalledSemaphore) {
                callbackCalledSemaphore.wait(2000);
            }

            Assert.assertEquals(false, subscribeBroadcastWithSingleEnumerationParameterCallbackDone);

            logger.info(name.getMethodName() + " - Invoking fire method with matching partitions");
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter(subscribeToPartitions);

            synchronized (callbackCalledSemaphore) {
                callbackCalledSemaphore.wait(1000);
            }

            Assert.assertEquals(true, subscribeBroadcastWithSingleEnumerationParameterCallbackDone);
            Assert.assertEquals(true, subscribeBroadcastWithSingleEnumerationParameterCallbackResult);
            logger.info(name.getMethodName() + " - received expected broadcast");

            testInterfaceProxy.unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(subscriptionId);
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }
}

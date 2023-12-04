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

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized.Parameters;
import org.junit.runners.Parameterized;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.SubscriptionException;
import io.joynr.proxy.Future;
import joynr.MulticastSubscriptionQos;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultipleArrayParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultipleByteBufferParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultipleEnumerationParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultiplePrimitiveParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultipleStructParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleArrayParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleByteBufferParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleEnumerationParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSinglePrimitiveParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleStructParameterBroadcastAdapter;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;

@RunWith(Parameterized.class)
public class IltConsumerBroadcastSubscriptionTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerBroadcastSubscriptionTest.class);

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

    /*
     * BROADCAST SUBSCRIPTIONS
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;

    private String[] partitions;

    public IltConsumerBroadcastSubscriptionTest(String[] partitions) {
        this.partitions = partitions;
    }

    @Parameters
    public static List<Object[]> data() {
        return Arrays.asList(new Object[][]{ { new String[]{} }, { new String[]{ "partition0", "partition1" } } });
    }

    @Test
    public void callSubscribeBroadcastWithSinglePrimitiveParameter() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(new BroadcastWithSinglePrimitiveParameterBroadcastAdapter() {
                @Override
                public void onReceive(String stringOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (!stringOut.equals("boom")) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = true;
                    }
                    subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
                    subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSinglePrimitiveParameter(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSinglePrimitiveParameterCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithSinglePrimitiveParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            } else if (!subscribeBroadcastWithSinglePrimitiveParameterCallbackResult) {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on subscribe: " + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultiplePrimitiveParameters() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;
        boolean result;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithMultiplePrimitiveParametersBroadcast(new BroadcastWithMultiplePrimitiveParametersBroadcastAdapter() {
                @Override
                public void onReceive(Double doubleOut, String stringOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (!stringOut.equals("boom") || !IltUtil.cmpDouble(doubleOut, 1.1d)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = true;
                    }
                    subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
                    subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultiplePrimitiveParameters(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            } else if (!subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultiplePrimitiveParametersBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleArrayParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleArrayParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSingleArrayParameter() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithSingleArrayParameterBroadcast(new BroadcastWithSingleArrayParameterBroadcastAdapter() {
                @Override
                public void onReceive(String[] stringArrayOut) {
                    //
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    String[] stringArray = { "Hello", "World" };
                    if (!Arrays.equals(stringArray, stringArrayOut)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithSingleArrayParameterCallbackResult = true;
                    }
                    subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
                    subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleArrayParameter(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleArrayParameterCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithSingleArrayParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            } else if (!subscribeBroadcastWithSingleArrayParameterCallbackResult) {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleArrayParameterBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleArrayParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultipleArrayParameters() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithMultipleArrayParametersBroadcast(new BroadcastWithMultipleArrayParametersBroadcastAdapter() {
                @Override
                public void onReceive(Long[] uInt64ArrayOut, StructWithStringArray[] structWithStringArrayArrayOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (!IltUtil.checkUInt64Array(uInt64ArrayOut)
                            || !IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithMultipleArrayParametersCallbackResult = true;
                    }
                    subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
                    subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleArrayParameters(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleArrayParametersCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithMultipleArrayParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            } else if (!subscribeBroadcastWithMultipleArrayParametersCallbackResult) {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication event");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleArrayParametersBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleByteBufferParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSingleByteBufferParameter() {
        final Semaphore resultsAvailable = new Semaphore(0);
        Future<String> subscriptionIdFuture;
        String subscriptionId;
        boolean result;

        final Byte[] expectedByteBuffer = { -128, 0, 127 };

        logger.info(name.getMethodName());

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithSingleByteBufferParameterBroadcast(new BroadcastWithSingleByteBufferParameterBroadcastAdapter() {
                @Override
                public void onReceive(Byte[] byteBufferOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (!java.util.Objects.deepEquals(byteBufferOut, expectedByteBuffer)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithSingleByteBufferParameterCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithSingleByteBufferParameterCallbackResult = true;
                    }
                    resultsAvailable.release();
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithSingleByteBufferParameterCallbackResult = false;
                    resultsAvailable.release();
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);

            logger.info(name.getMethodName() + " - Invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleByteBufferParameter(expectedByteBuffer, partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // wait for results from callback
            Assert.assertTrue(name.getMethodName() + " - FAILED - callback was not received in time",
                              resultsAvailable.tryAcquire(2, TimeUnit.SECONDS));
            logger.info(name.getMethodName() + " - results received");

            // check results from callback
            Assert.assertTrue(name.getMethodName()
                    + " - FAILED - callback got called but received unexpected error or publication event",
                              subscribeBroadcastWithSingleByteBufferParameterCallbackResult);

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleByteBufferParameterBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleByteBufferParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultipleByteBufferParameters() {
        final Semaphore resultsAvailable = new Semaphore(0);
        Future<String> subscriptionIdFuture;
        String subscriptionId;
        boolean result;

        final Byte[] expectedByteBuffer1 = { -5, 125 };
        final Byte[] expectedByteBuffer2 = { 78, 0 };

        logger.info(name.getMethodName());

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithMultipleByteBufferParametersBroadcast(new BroadcastWithMultipleByteBufferParametersBroadcastAdapter() {
                @Override
                public void onReceive(Byte[] byteBufferOut1, Byte[] byteBufferOut2) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (!java.util.Objects.deepEquals(byteBufferOut1, expectedByteBuffer1)
                            || !java.util.Objects.deepEquals(byteBufferOut2, expectedByteBuffer2)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithMultipleByteBufferParametersCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithMultipleByteBufferParametersCallbackResult = true;
                    }
                    resultsAvailable.release();
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithMultipleByteBufferParametersCallbackResult = false;
                    resultsAvailable.release();
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);

            logger.info(name.getMethodName() + " - Invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleByteBufferParameters(expectedByteBuffer1,
                                                                                     expectedByteBuffer2,
                                                                                     partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // wait for results from callback
            Assert.assertTrue(name.getMethodName() + " - FAILED - callback was not received in time",
                              resultsAvailable.tryAcquire(2, TimeUnit.SECONDS));

            // check results from callback
            Assert.assertTrue(name.getMethodName()
                    + " - FAILED - callback got called but received unexpected error or publication event",
                              subscribeBroadcastWithMultipleByteBufferParametersCallbackResult);

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleByteBufferParametersBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSingleEnumerationParameter() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;
        boolean result;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithSingleEnumerationParameterBroadcast(new BroadcastWithSingleEnumerationParameterBroadcastAdapter() {
                @Override
                public void onReceive(ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = true;
                    }
                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleEnumerationParameterCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithSingleEnumerationParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            } else if (!subscribeBroadcastWithSingleEnumerationParameterCallbackResult) {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultipleEnumerationParameters() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithMultipleEnumerationParametersBroadcast(new BroadcastWithMultipleEnumerationParametersBroadcastAdapter() {
                @Override
                public void onReceive(ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut,
                                      Enumeration enumerationOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (extendedEnumerationOut != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
                            || enumerationOut != Enumeration.ENUM_0_VALUE_1) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = true;
                    }
                    subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
                    subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleEnumerationParameters(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleEnumerationParametersCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithMultipleEnumerationParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            } else if (!subscribeBroadcastWithMultipleEnumerationParametersCallbackResult) {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleEnumerationParametersBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleStructParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleStructParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSingleStructParameter() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithSingleStructParameterBroadcast(new BroadcastWithSingleStructParameterBroadcastAdapter() {
                @Override
                public void onReceive(ExtendedStructOfPrimitives extendedStructOfPrimitivesOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    if (!IltUtil.checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithSingleStructParameterCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithSingleStructParameterCallbackResult = true;
                    }
                    subscribeBroadcastWithSingleStructParameterCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithSingleStructParameterCallbackResult = false;
                    subscribeBroadcastWithSingleStructParameterCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleStructParameter(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleStructParameterCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithSingleStructParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
            } else if (!subscribeBroadcastWithSingleStructParameterCallbackResult) {
                fail(name.getMethodName()
                        + " - FAILED - callback got callback but received unexpected error or publication content");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleStructParameterBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleStructParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleStructParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultipleStructParameters() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;
        boolean result;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithMultipleStructParametersBroadcast(new BroadcastWithMultipleStructParametersBroadcastAdapter() {
                @Override
                public void onReceive(BaseStructWithoutElements baseStructWithoutElementsOut,
                                      ExtendedExtendedBaseStruct extendedExtendedBaseStructOut) {
                    logger.info(name.getMethodName() + " - callback - got broadcast");
                    ExtendedExtendedBaseStruct extendedExtendedBaseStruct = IltUtil.createExtendedExtendedBaseStruct();
                    if (!IltUtil.checkBaseStructWithoutElements(baseStructWithoutElementsOut)
                            || !extendedExtendedBaseStructOut.equals(extendedExtendedBaseStruct)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithMultipleStructParametersCallbackResult = true;
                    }
                    subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
                    subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
                }
            }, new MulticastSubscriptionQos(), partitions);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleStructParameters(partitions);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleStructParametersCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            if (!subscribeBroadcastWithMultipleStructParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get callback in time");
            } else if (!subscribeBroadcastWithMultipleStructParametersCallbackResult) {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
            }
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleStructParametersBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }
}

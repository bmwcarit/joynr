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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Arrays;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;

import io.joynr.exceptions.SubscriptionException;
import io.joynr.proxy.Future;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithFilteringBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithFilteringBroadcastFilterParameters;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;

public class IltConsumerFilteredBroadcastSubscriptionTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerFilteredBroadcastSubscriptionTest.class);

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

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithFilteringCallbackDone = false;
    volatile boolean subscribeBroadcastWithFilteringCallbackResult = false;

    @SuppressWarnings("checkstyle:methodlength")
    @Test
    public void callSubscribeBroadcastWithFiltering() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        logger.info(name.getMethodName());

        try {
            BroadcastWithFilteringBroadcastFilterParameters filterParameters = new BroadcastWithFilteringBroadcastFilterParameters();
            String stringOfInterest = "fireBroadcast";
            filterParameters.setStringOfInterest(stringOfInterest);

            String[] stringArrayOfInterest = { "Hello", "World" };
            String json;
            try {
                logger.info(name.getMethodName() + " - objectMapper is " + objectMapper);
                logger.info(name.getMethodName() + " - objectMapper stringArrayOfInterest " + stringArrayOfInterest);
                json = objectMapper.writeValueAsString(stringArrayOfInterest);
            } catch (JsonProcessingException je) {
                throw new RuntimeException(getExceptionMessage(je, "stringArrayOfInterest"));
            }
            filterParameters.setStringArrayOfInterest(json);

            ExtendedTypeCollectionEnumerationInTypeCollection enumerationOfInterest = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
            try {
                json = objectMapper.writeValueAsString(enumerationOfInterest);
            } catch (JsonProcessingException je) {
                throw new RuntimeException(getExceptionMessage(je, "enumerationOfInterest"));
            }
            filterParameters.setEnumerationOfInterest(json);

            StructWithStringArray structWithStringArrayOfInterest = IltUtil.createStructWithStringArray();
            try {
                json = objectMapper.writeValueAsString(structWithStringArrayOfInterest);
            } catch (JsonProcessingException je) {
                throw new RuntimeException(getExceptionMessage(je, "structWithStringArrayOfInterest"));
            }
            filterParameters.setStructWithStringArrayOfInterest(json);

            StructWithStringArray[] structWithStringArrayArrayOfInterest = IltUtil.createStructWithStringArrayArray();
            try {
                json = objectMapper.writeValueAsString(structWithStringArrayArrayOfInterest);
            } catch (JsonProcessingException je) {
                throw new RuntimeException(getExceptionMessage(je, "structWithStringArrayArrayOfInterest"));
            }
            filterParameters.setStructWithStringArrayArrayOfInterest(json);

            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithFilteringBroadcast(new BroadcastWithFilteringBroadcastAdapter() {
                @Override
                public void onReceive(String stringOut,
                                      String[] stringArrayOut,
                                      ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut,
                                      StructWithStringArray structWithStringArrayOut,
                                      StructWithStringArray[] structWithStringArrayArrayOut) {

                    logger.info(name.getMethodName() + " - callback - got broadcast");

                    String[] stringArray = { "Hello", "World" };
                    if (!Arrays.equals(stringArray, stringArrayOut)) {
                        subscribeBroadcastWithFilteringCallbackResult = false;
                    } else if (enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithFilteringCallbackResult = false;
                    } else if (!IltUtil.checkStructWithStringArray(structWithStringArrayOut)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithFilteringCallbackResult = false;
                    } else if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
                        logger.info(name.getMethodName() + " - callback - invalid content");
                        subscribeBroadcastWithFilteringCallbackResult = false;
                    } else {
                        logger.info(name.getMethodName() + " - callback - content OK");
                        subscribeBroadcastWithFilteringCallbackResult = true;
                    }
                    subscribeBroadcastWithFilteringCallbackDone = true;
                }

                @Override
                public void onError(SubscriptionException error) {
                    logger.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithFilteringCallbackResult = false;
                    subscribeBroadcastWithFilteringCallbackDone = true;
                }
            }, subscriptionQos, filterParameters);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            logger.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            logger.info(name.getMethodName() + " - Wait done, invoking fire method");
            String stringArg = "fireBroadcast";
            testInterfaceProxy.methodToFireBroadcastWithFiltering(stringArg);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (!subscribeBroadcastWithFilteringCallbackDone) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            assertTrue(name.getMethodName() + " - FAILED - callback did not get called in time",
                       subscribeBroadcastWithFilteringCallbackDone);
            assertTrue(name.getMethodName()
                    + " - FAILED - callback got called but received unexpected error or publication content",
                       subscribeBroadcastWithFilteringCallbackResult);
            logger.info(name.getMethodName() + " - callback got called and received expected publication");

            // reset counter for 2nd test
            subscribeBroadcastWithFilteringCallbackResult = false;
            subscribeBroadcastWithFilteringCallbackDone = false;

            logger.info(name.getMethodName() + " - invoking fire method with wrong stringArg");
            stringArg = "doNotfireBroadcast";
            testInterfaceProxy.methodToFireBroadcastWithFiltering(stringArg);
            logger.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (!subscribeBroadcastWithFilteringCallbackDone) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }

            assertFalse(name.getMethodName() + " - FAILED - callback got called unexpectedly",
                        subscribeBroadcastWithFilteringCallbackDone);
            logger.info(name.getMethodName() + " - callback did not get called in time (expected)");

            // try to unsubscribe
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
            }

            logger.info(name.getMethodName() + " - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            StringWriter stringWriter = new StringWriter();
            PrintWriter printWriter = new PrintWriter(stringWriter);
            e.printStackTrace(printWriter);
            printWriter.flush();

            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + stringWriter.toString());
        }
    }

    private String getExceptionMessage(JsonProcessingException exception, String what) {
        return name.getMethodName() + " - FAILED - got exception when serializing " + what + exception.getMessage();
    }
}

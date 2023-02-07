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

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;

public class IltConsumerAttributeSubscriptionTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerAttributeSubscriptionTest.class);

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
     * ATTRIBUTE SUBSCRIPTIONS
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeAttributeEnumerationCallbackDone = false;
    volatile boolean subscribeAttributeEnumerationCallbackResult = false;

    @Test
    public void callSubscribeAttributeEnumeration() {
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
        boolean result;

        logger.info(name.getMethodName() + "");

        try {
            // must set the value before it can be retrieved again via subscription
            Enumeration enumerationArg = Enumeration.ENUM_0_VALUE_2;
            testInterfaceProxy.setAttributeEnumeration(enumerationArg);

            subscriptionIdFuture = testInterfaceProxy.subscribeToAttributeEnumeration(new AttributeSubscriptionAdapter<Enumeration>() {
                @Override
                public void onReceive(Enumeration value) {
                    if (value == Enumeration.ENUM_0_VALUE_2) {
                        logger.info(name.getMethodName() + " - callback - got publication with correct value");
                        subscribeAttributeEnumerationCallbackResult = true;
                    } else {
                        subscribeAttributeEnumerationCallbackResult = false;
                        logger.info(name.getMethodName() + " - callback - got publication with invalid value");
                    }
                    subscribeAttributeEnumerationCallbackDone = true;
                }

                @Override
                public void onError(JoynrRuntimeException error) {
                    logger.info(name.getMethodName() + " - callback - got unexpected exception");
                    subscribeAttributeEnumerationCallbackResult = false;
                    subscribeAttributeEnumerationCallbackDone = true;
                }
            }, subscriptionQos);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeAttributeEnumerationCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }
            if (subscribeAttributeEnumerationCallbackDone && subscribeAttributeEnumerationCallbackResult) {
                result = true;
            } else {
                fail(name.getMethodName() + " - FAILED - callback NOT done");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromAttributeEnumeration(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
                result = false;
            }

            if (!result) {
                logger.info(name.getMethodName() + " - FAILED");
            } else {
                logger.info(name.getMethodName() + " - OK");
            }
            return;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeAttributeWithExceptionFromGetterCallbackDone = false;
    volatile boolean subscribeAttributeWithExceptionFromGetterCallbackResult = false;

    @Test
    public void callSubscribeAttributeWithExceptionFromGetter() {
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
        boolean result;

        logger.info(name.getMethodName() + "");

        try {
            subscriptionIdFuture = testInterfaceProxy.subscribeToAttributeWithExceptionFromGetter(new AttributeSubscriptionAdapter<Boolean>() {
                @Override
                public void onReceive(Boolean value) {
                    logger.info(name.getMethodName() + " - callback - got unexpected publication");
                    subscribeAttributeWithExceptionFromGetterCallbackResult = false;
                    subscribeAttributeWithExceptionFromGetterCallbackDone = true;
                }

                @Override
                public void onError(JoynrRuntimeException error) {
                    if (error instanceof ProviderRuntimeException) {
                        if (((ProviderRuntimeException) error).getMessage()
                                                              .equals("Exception from getAttributeWithExceptionFromGetter")) {
                            logger.info(name.getMethodName() + " - callback - got expected exception "
                                    + ((JoynrRuntimeException) error).getMessage());
                            subscribeAttributeWithExceptionFromGetterCallbackResult = true;
                            subscribeAttributeWithExceptionFromGetterCallbackDone = true;
                            return;
                        }
                        logger.info(name.getMethodName() + " - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else if (error instanceof JoynrRuntimeException) {
                        logger.info(name.getMethodName() + " - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        logger.info(name.getMethodName() + " - callback - caught invalid exception ");
                    }
                    subscribeAttributeWithExceptionFromGetterCallbackResult = false;
                    subscribeAttributeWithExceptionFromGetterCallbackDone = true;
                }
            }, subscriptionQos);
            subscriptionId = subscriptionIdFuture.get(10000);
            logger.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeAttributeWithExceptionFromGetterCallbackDone == false) {
                logger.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                logger.info(name.getMethodName() + " - wait for callback is over");
            } else {
                logger.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeAttributeWithExceptionFromGetterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeAttributeWithExceptionFromGetterCallbackResult) {
                logger.info(name.getMethodName() + " - callback got called and received expected exception");
                result = true;
            } else {
                fail(name.getMethodName() + " - FAILED - callback got called but received unexpected result");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromAttributeWithExceptionFromGetter(subscriptionId);
                logger.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: "
                        + e.getMessage());
                result = false;
            }

            if (!result) {
                logger.info(name.getMethodName() + " - FAILED");
            } else {
                logger.info(name.getMethodName() + " - OK");
            }
            return;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            logger.info(name.getMethodName() + " - caught unexpected exception");
            logger.info(name.getMethodName() + " - FAILED");
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }
}

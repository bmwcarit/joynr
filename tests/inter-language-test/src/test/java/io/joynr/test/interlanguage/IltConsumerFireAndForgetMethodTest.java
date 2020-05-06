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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.doAnswer;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import joynr.OnChangeSubscriptionQos;

/**
 * Class to test fireAndForget methods.
 *
 * FireAndForget methods do not have a return value and the calling proxy does not receive an answer to a fireAndForget method call.
 * The attribute attributeFireAndForget is used in fireAndForget method calls to check if the method is called at the provider.
 * The provider will change the attribute to a (fireAndForget) method specific value which will be checked in the subscription listener.
 */
@RunWith(MockitoJUnitRunner.class)
public class IltConsumerFireAndForgetMethodTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerFireAndForgetMethodTest.class);
    private Integer attributeFireAndForgetValue = -1;
    private OnChangeSubscriptionQos subscriptionQos;
    //private Future<String> attributeFireAndForgetSubscriptionId = new Future<String>();
    private String attributeFireAndForgetSubscriptionId;
    private Semaphore publicationReceivedSemaphore;
    private Answer<Void> releaseSemaphore = new Answer<Void>() {
        @Override
        public Void answer(InvocationOnMock invocation) throws Throwable {
            Integer value = (Integer) invocation.getArguments()[0];
            attributeFireAndForgetValue = value;
            publicationReceivedSemaphore.release();
            return null;
        }
    };
    private Answer<Void> failWithException = new Answer<Void>() {
        @Override
        public Void answer(InvocationOnMock invocation) throws Throwable {
            JoynrRuntimeException error = (JoynrRuntimeException) invocation.getArguments()[0];
            publicationReceivedSemaphore.release();
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + error.getMessage());
            return null;
        }
    };

    @Mock
    private AttributeSubscriptionAdapter<Integer> attributeFireAndForgetListener;

    @BeforeClass
    public static void testClassSetUp() throws Exception {
        logger.info("testClassSetUp: Entering");
        setupConsumerRuntime(false);
        logger.info("testClassSetUp: Leaving");
    }

    @Before
    public void setUp() {
        publicationReceivedSemaphore = new Semaphore(0);
        doAnswer(releaseSemaphore).when(attributeFireAndForgetListener).onReceive(anyInt());
        doAnswer(failWithException).when(attributeFireAndForgetListener).onError(any(JoynrRuntimeException.class));
    }

    @AfterClass
    public static void testClassTearDown() throws InterruptedException {
        logger.info("testClassTearDown: Entering");
        generalTearDown();
        logger.info("testClassTearDown: Leaving");
    }

    @After
    public void tearDown() {
        unsubscribeFromAttributeFireAndForget();
    }

    /*
     * FIRE AND FORGET METHOD CALLS
     */

    /**
     * Subscribe to attributeFireAndForget to get notified when the attribute attributeFireAndForget is changed.
     * AttributeFireAndForget is set to 0 first since it might have been set to the expected value by another test.
     */
    private void subscribeToAttributeFireAndForget() {
        int minIntervalMs = 0;
        long expiryDateMs = System.currentTimeMillis() + 60000;
        int publicationTtlMs = 5000;
        subscriptionQos = new OnChangeSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                       .setExpiryDateMs(expiryDateMs)
                                                       .setPublicationTtlMs(publicationTtlMs);

        logger.info(name.getMethodName() + ": subscribeToAttributeFireAndForget");

        try {
            // set attributeFireAndForget do a defined value
            Integer expected = 0;
            testInterfaceProxy.setAttributeFireAndForget(expected);

            // subscribe to attributeFireAndForget
            Future<String> myFuture;
            myFuture = testInterfaceProxy.subscribeToAttributeFireAndForget(attributeFireAndForgetListener,
                                                                            subscriptionQos);

            attributeFireAndForgetSubscriptionId = myFuture.get(10000);
            logger.info(name.getMethodName() + ": subscribeToAttributeFireAndForget - subscription successful");
            assertTrue(publicationReceivedSemaphore.tryAcquire(5000, TimeUnit.MILLISECONDS));
            assertEquals(expected, attributeFireAndForgetValue);

            logger.info(name.getMethodName() + ": subscribeToAttributeFireAndForget - first publication received");
        } catch (Exception e) {
            fail(name.getMethodName() + ": subscribeToAttributeFireAndForget - FAILED - caught unexpected exception: "
                    + e.getMessage());
        }
    }

    private void unsubscribeFromAttributeFireAndForget() {
        logger.info(name.getMethodName() + ": unsubscribeFromAttributeFireAndForget");
        try {
            testInterfaceProxy.unsubscribeFromAttributeFireAndForget(attributeFireAndForgetSubscriptionId);
            // wait some time for unsubscribe call to to be processed at the provider
            Thread.sleep(2000);
            logger.info(name.getMethodName() + ": unsubscribeFromAttributeFireAndForget - OK");
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName()
                    + ": unsubscribeFromAttributeFireAndForget - FAILED - caught unexpected exception: "
                    + e.getMessage());
        }
    }

    @Test
    public void callMethodFireAndForgetWithoutParameter() {
        subscribeToAttributeFireAndForget();
        Integer expected = attributeFireAndForgetValue + 1;
        try {
            logger.info(name.getMethodName() + " - CALL");
            testInterfaceProxy.methodFireAndForgetWithoutParameter();
            assertTrue(publicationReceivedSemaphore.tryAcquire(5000, TimeUnit.MILLISECONDS));
            assertEquals(expected, attributeFireAndForgetValue);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodFireAndForgetWithInputParameter() {
        subscribeToAttributeFireAndForget();
        Integer expected = 1337;
        try {
            logger.info(name.getMethodName() + " - CALL");
            testInterfaceProxy.methodFireAndForgetWithInputParameter(expected);
            assertTrue(publicationReceivedSemaphore.tryAcquire(5000, TimeUnit.MILLISECONDS));
            assertEquals(expected, attributeFireAndForgetValue);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

}

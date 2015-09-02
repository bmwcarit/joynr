package io.joynr.dispatching.subscription;

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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCaller;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

@RunWith(MockitoJUnitRunner.class)
public class PublicationTimersTest {

    private static final Logger LOG = LoggerFactory.getLogger(PublicationTimersTest.class);
    private final String attributeName = "testAttribute";

    interface TestRequestCaller extends RequestCaller {
        String getTestAttribute();
    }

    @Mock
    private TestRequestCaller requestCaller;

    @Mock
    private Dispatcher dispatcher;
    @Mock
    private AttributePollInterpreter attributePollInterpreter;

    private ScheduledExecutorService cleanupScheduler = Executors.newSingleThreadScheduledExecutor();

    @Mock
    private PubSubTestProviderImpl provider;

    @Before
    public void setUp() {
        Deferred<String> testAttributeDeferred = new Deferred<String>();
        testAttributeDeferred.resolve("testAttributeValue");
        Promise<Deferred<String>> testAttributePromise = new Promise<Deferred<String>>(testAttributeDeferred);
        Mockito.doReturn(testAttributePromise).when(attributePollInterpreter).execute(any(RequestCaller.class),
                                                                                      any(Method.class));
    }

    @Test(timeout = 4000)
    public void publicationsSentUntilExpiryDate() throws InterruptedException, JoynrSendBufferFullException,
                                                 JoynrMessageNotSentException, JsonGenerationException,
                                                 JsonMappingException, IOException {
        LOG.debug("Starting PublicationTimersTest.timerIsStoppedWhenEnddateIsReached test");
        int period = 500;
        int subscriptionLength = 1100;
        long expiryDate = System.currentTimeMillis() + subscriptionLength;
        int publicationTtl = 1000;
        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos(period, expiryDate, publicationTtl);
        String subscriptionId = "subscriptionId";
        String proxyId = "proxyId";
        String providerId = "providerId";

        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
        PublicationManager publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                                           dispatcher,
                                                                           cleanupScheduler);
        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest, requestCaller);

        Thread.sleep(subscriptionLength + period / 2);

        int publicationTimes = 1 + (subscriptionLength / period);
        verify(dispatcher, times(publicationTimes)).sendSubscriptionPublication(eq(providerId),
                                                                                eq(proxyId),
                                                                                any(SubscriptionPublication.class),
                                                                                any(MessagingQos.class));

        Thread.sleep(subscriptionLength);
        verifyNoMoreInteractions(dispatcher);
    }

}

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

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.pubsub.SubscriptionQos;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

@RunWith(MockitoJUnitRunner.class)
public class PushingPublicationTest {

    private static final Logger logger = LoggerFactory.getLogger(PushingPublicationTest.class);

    private PubSubTestProviderImpl provider;
    private PublicationManager publicationManager;

    @Mock
    Dispatcher dispatcher;
    @Mock
    private RequestCallerDirectory requestCallerDirectory;
    @Mock
    private AttributePollInterpreter attributePollInterpreter;

    private ScheduledExecutorService cleanupScheduler = Executors.newSingleThreadScheduledExecutor();

    private SubscriptionRequest subscriptionRequest;
    private String subscriptionId;
    private String proxyId;
    private String providerId;
    private String attributeName;
    private SubscriptionQos qos;
    private RequestCaller requestCaller;
    private RequestCallerFactory requestCallerFactory;
    int testAttribute = 123;
    SubscriptionPublication publication;

    @Before
    public void setUp() throws JoynrSendBufferFullException, JoynrMessageNotSentException, JsonGenerationException,
                       JsonMappingException, IOException {
        provider = new PubSubTestProviderImpl();

        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        requestCallerDirectory,
                                                        cleanupScheduler);
        subscriptionId = "subscriptionId";
        proxyId = "proxyId";
        providerId = "providerId";
        attributeName = "testAttribute";
        publication = new SubscriptionPublication(Arrays.asList(testAttribute), subscriptionId);

        requestCallerFactory = new RequestCallerFactory();
        requestCaller = requestCallerFactory.create(provider);
        setupMocks();
    }

    void setupPureOnChangedQos() {
        long maxInterval_ms = SubscriptionQos.IGNORE_VALUE;

        long endDate = System.currentTimeMillis() + 19000;
        long publicationTtl_ms = 1000;
        qos = new OnChangeSubscriptionQos(maxInterval_ms, endDate, publicationTtl_ms);
        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
    }

    void setupMixedQos() {
        long minInterval_ms = 10;
        long maxInterval_ms = 3000; // TODO Also write this test with -1

        long endDate = System.currentTimeMillis() + 19000;
        long alertInterval_ms = 500;
        long publicationTtl_ms = 1000;
        qos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                       maxInterval_ms,
                                                       endDate,
                                                       alertInterval_ms,
                                                       publicationTtl_ms);
        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
    }

    void setupMocks() throws JoynrSendBufferFullException, JoynrMessageNotSentException, JsonGenerationException,
                     JsonMappingException, IOException {
        Deferred<Integer> testAttributeDeferred = new Deferred<Integer>();
        testAttributeDeferred.resolve(testAttribute);
        Promise<Deferred<Integer>> testAttributePromise = new Promise<Deferred<Integer>>(testAttributeDeferred);
        Mockito.doReturn(testAttributePromise).when(attributePollInterpreter).execute(any(RequestCaller.class),
                                                                                      any(Method.class));

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                StringBuilder sb = new StringBuilder();
                sb.append("sendPublication called with the following Arguments: ");
                for (Object argument : invocation.getArguments()) {
                    if (argument != null) {
                        sb.append(argument.toString());
                    } else {
                        sb.append("null");
                    }
                    sb.append(" , ");
                }
                logger.trace(sb.toString());
                return null;
            }
        }).when(dispatcher).sendSubscriptionPublication(any(String.class),
                                                        any(String.class),
                                                        any(SubscriptionPublication.class),
                                                        any(MessagingQos.class));

    }

    @Test
    public void settingAttributeSendsPublication() throws InterruptedException, JoynrSendBufferFullException,
                                                  JoynrMessageNotSentException, JsonGenerationException,
                                                  JsonMappingException, IOException {
        setupMixedQos();

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest, requestCaller);
        Thread.sleep(100);
        provider.setTestAttribute(testAttribute);
        Thread.sleep(1500);

        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(providerId),
                                                                 eq(proxyId),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));
        verify(attributePollInterpreter, times(1)).execute(any(RequestCaller.class), any(Method.class));

    }

    @Test
    public void settingAttributeSendsPublicationOnPureOnChangedSubscription() throws InterruptedException,
                                                                             JoynrSendBufferFullException,
                                                                             JoynrMessageNotSentException,
                                                                             JsonGenerationException,
                                                                             JsonMappingException, IOException {
        setupPureOnChangedQos();

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest, requestCaller);
        provider.setTestAttribute(testAttribute);

        ArgumentCaptor<SubscriptionPublication> sentPublication = ArgumentCaptor.forClass(SubscriptionPublication.class);
        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(providerId),
                                                                 eq(proxyId),
                                                                 sentPublication.capture(),
                                                                 any(MessagingQos.class));
        assertEquals(publication.getResponse(), sentPublication.getValue().getResponse());
        assertEquals(publication.getSubscriptionId(), sentPublication.getValue().getSubscriptionId());
        verify(attributePollInterpreter, times(1)).execute(any(RequestCaller.class), any(Method.class));

    }

}

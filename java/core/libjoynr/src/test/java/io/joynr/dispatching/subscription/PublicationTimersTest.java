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
package io.joynr.dispatching.subscription;

import static org.hamcrest.Matchers.contains;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

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

import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerFactory;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import io.joynr.provider.ProviderContainer;
import io.joynr.runtime.ShutdownNotifier;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.tests.DefaulttestProvider;

@RunWith(MockitoJUnitRunner.class)
public class PublicationTimersTest {

    private static final Logger logger = LoggerFactory.getLogger(PublicationTimersTest.class);
    private final String attributeName = "notifyReadWrite";
    private static final boolean SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED = true;

    @Mock
    private RequestCaller requestCaller;

    @Mock
    private AbstractSubscriptionPublisher subscriptionPublisher;

    @Mock
    private ProviderContainer providerContainer;

    @Mock
    private Dispatcher dispatcher;
    @Mock
    private AttributePollInterpreter attributePollInterpreter;

    private ScheduledExecutorService cleanupScheduler = Executors.newSingleThreadScheduledExecutor();

    @Mock
    private SubscriptionTestsProviderImpl provider;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Before
    public void setUp() {
        requestCaller = new RequestCallerFactory().create(new DefaulttestProvider());
        when(providerContainer.getProviderProxy()).thenReturn(requestCaller.getProxy());
        when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);

        Deferred<String> testAttributeDeferred = new Deferred<String>();
        testAttributeDeferred.resolve("testAttributeValue");
        Promise<Deferred<String>> testAttributePromise = new Promise<Deferred<String>>(testAttributeDeferred);
        Mockito.doReturn(Optional.of(testAttributePromise))
               .when(attributePollInterpreter)
               .execute(any(ProviderContainer.class), any(Method.class));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 4000)
    public void publicationsSentUntilExpiryDate() throws InterruptedException, JoynrSendBufferFullException,
                                                  JoynrMessageNotSentException, JsonGenerationException,
                                                  JsonMappingException, IOException {
        logger.debug("Starting PublicationTimersTest.timerIsStoppedWhenEnddateIsReached test");
        int period = 500;
        int subscriptionLength = 1100;
        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos();
        qos.setPeriodMs(period).setValidityMs(subscriptionLength).setPublicationTtlMs(1000);
        String subscriptionId = "subscriptionId";
        String proxyId = "proxyId";
        String providerId = "providerId";

        ProviderDirectory providerDirectory = Mockito.mock(ProviderDirectory.class);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
        PublicationManager publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                                           dispatcher,
                                                                           providerDirectory,
                                                                           cleanupScheduler,
                                                                           Mockito.mock(SubscriptionRequestStorage.class),
                                                                           shutdownNotifier,
                                                                           SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        when(providerDirectory.get(eq(providerId))).thenReturn(providerContainer);
        when(providerDirectory.contains(eq(providerId))).thenReturn(true);

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest);

        Thread.sleep(subscriptionLength + period / 2);

        int publicationTimes = 1 + (subscriptionLength / period);
        verify(dispatcher,
               times(publicationTimes)).sendSubscriptionPublication(eq(providerId),
                                                                    (Set<String>) argThat(contains(proxyId)),
                                                                    any(SubscriptionPublication.class),
                                                                    any(MessagingQos.class));

        Thread.sleep(subscriptionLength);
        verify(dispatcher).sendSubscriptionReply(eq(providerId),
                                                 eq(proxyId),
                                                 any(SubscriptionReply.class),
                                                 any(MessagingQos.class));
        verifyNoMoreInteractions(dispatcher);
    }

}

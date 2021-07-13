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
import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

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

import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCallerFactory;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import io.joynr.provider.ProviderContainer;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.runtime.ShutdownNotifier;
import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.tests.testSubscriptionPublisherImpl;

@RunWith(MockitoJUnitRunner.class)
public class PushingPublicationTest {

    private static final Logger logger = LoggerFactory.getLogger(PushingPublicationTest.class);

    private SubscriptionTestsProviderImpl provider;
    private PublicationManager publicationManager;

    @Mock
    Dispatcher dispatcher;

    @Mock
    private ProviderDirectory providerDirectory;

    @Mock
    private ProviderContainer providerContainer;

    @Mock
    private AttributePollInterpreter attributePollInterpreter;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    private ScheduledExecutorService cleanupScheduler = Executors.newSingleThreadScheduledExecutor();

    private SubscriptionRequest subscriptionRequest;
    private String subscriptionId;
    private String proxyId;
    private String providerId;
    private String attributeName;
    int testAttribute = 123;
    SubscriptionPublication publication;

    @Before
    public void setUp() throws JoynrSendBufferFullException, JoynrMessageNotSentException, JsonGenerationException,
                        JsonMappingException, IOException {
        provider = new SubscriptionTestsProviderImpl();

        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        providerDirectory,
                                                        Mockito.mock(RoutingTable.class),
                                                        cleanupScheduler,
                                                        shutdownNotifier);
        subscriptionId = "subscriptionId";
        proxyId = "proxyId";
        providerId = "providerId";
        attributeName = "testAttribute";
        publication = new SubscriptionPublication(Arrays.asList(testAttribute), subscriptionId);

        testSubscriptionPublisherImpl testSubscriptionPublisher = new testSubscriptionPublisherImpl();
        provider.setSubscriptionPublisher(testSubscriptionPublisher);
        when(providerContainer.getProviderProxy()).thenReturn(new RequestCallerFactory().create(provider).getProxy());
        when(providerContainer.getSubscriptionPublisher()).thenReturn(testSubscriptionPublisher);
        setupMocks();
    }

    void setupPureOnChangedQos() {
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(SubscriptionQos.IGNORE_VALUE).setValidityMs(19000).setPublicationTtlMs(1000);
        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
    }

    void setupMixedQos() {
        OnChangeWithKeepAliveSubscriptionQos qos = new OnChangeWithKeepAliveSubscriptionQos();
        qos.setMinIntervalMs(10);
        qos.setMaxIntervalMs(3000); // TODO Also write this test with -1
        qos.setValidityMs(19000);
        qos.setAlertAfterIntervalMs(500);
        qos.setPublicationTtlMs(1000);
        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
    }

    @SuppressWarnings("unchecked")
    void setupMocks() throws JoynrSendBufferFullException, JoynrMessageNotSentException, JsonGenerationException,
                      JsonMappingException, IOException {
        Deferred<Integer> testAttributeDeferred = new Deferred<Integer>();
        testAttributeDeferred.resolve(testAttribute);
        Promise<Deferred<Integer>> testAttributePromise = new Promise<Deferred<Integer>>(testAttributeDeferred);
        Mockito.doReturn(Optional.of(testAttributePromise))
               .when(attributePollInterpreter)
               .execute(any(ProviderContainer.class), any(Method.class));

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
                                                        any(Set.class),
                                                        any(SubscriptionPublication.class),
                                                        any(MessagingQos.class));

        when(providerDirectory.get(eq(providerId))).thenReturn(providerContainer);

    }

    @SuppressWarnings("unchecked")
    @Test
    public void settingAttributeSendsPublication() throws InterruptedException, JoynrSendBufferFullException,
                                                   JoynrMessageNotSentException, JsonGenerationException,
                                                   JsonMappingException, IOException {
        setupMixedQos();

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest);
        Thread.sleep(100);
        provider.setTestAttribute(testAttribute);
        Thread.sleep(1500);

        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(providerId),
                                                                 (Set<String>) argThat(contains(proxyId)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));
        verify(attributePollInterpreter, times(1)).execute(any(ProviderContainer.class), any(Method.class));

    }

    @SuppressWarnings("unchecked")
    @Test
    public void settingAttributeSendsPublicationOnPureOnChangedSubscription() throws InterruptedException,
                                                                              JoynrSendBufferFullException,
                                                                              JoynrMessageNotSentException,
                                                                              JsonGenerationException,
                                                                              JsonMappingException, IOException {
        setupPureOnChangedQos();

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest);
        provider.setTestAttribute(testAttribute);

        ArgumentCaptor<SubscriptionPublication> sentPublication = ArgumentCaptor.forClass(SubscriptionPublication.class);
        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(providerId),
                                                                 (Set<String>) argThat(contains(proxyId)),
                                                                 sentPublication.capture(),
                                                                 any(MessagingQos.class));
        assertEquals(publication.getResponse(), sentPublication.getValue().getResponse());
        assertEquals(publication.getSubscriptionId(), sentPublication.getValue().getSubscriptionId());
        verify(attributePollInterpreter, times(1)).execute(any(ProviderContainer.class), any(Method.class));

    }

}

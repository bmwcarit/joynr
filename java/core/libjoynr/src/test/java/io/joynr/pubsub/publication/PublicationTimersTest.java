package io.joynr.pubsub.publication;

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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.pubsub.PubSubTestProviderImpl;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.tests.testProxy;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
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
public class PublicationTimersTest {

    private static final Logger LOG = LoggerFactory.getLogger(PublicationTimersTest.class);

    private SubscriptionRequest subscriptionRequest;

    private RequestCaller requestCaller;
    @Mock
    private RequestReplySender messageSender;
    @Mock
    private AttributePollInterpreter attributePollInterpreter;

    private ScheduledExecutorService cleanupScheduler = Executors.newSingleThreadScheduledExecutor();

    private PeriodicSubscriptionQos qos;

    private PublicationManager publicationManager;
    private String attributeName;
    private String providerId;
    private String proxyId;
    private String subscriptionId;

    private RequestCallerFactory requestCallerFactory;
    @Mock
    private PubSubTestProviderImpl provider;
    // @Mock
    private SubscriptionPublication publication = new SubscriptionPublication(null, "subscriptionId");

    private static final int period = 100;
    private int numberOfPublications = 5;
    private int missedPublicationAlertDelay = 10;
    private long alertAfterInterval;

    private long expiryDate;

    @Before
    public void setUp() {
        alertAfterInterval = period + missedPublicationAlertDelay;

        expiryDate = getExpiryDate();
        qos = new PeriodicSubscriptionQos(period, expiryDate, alertAfterInterval, 1000);

        subscriptionId = "subscriptionId";
        proxyId = "proxyId";
        providerId = "providerId";
        attributeName = "testAttribute";

        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
        requestCallerFactory = new RequestCallerFactory();

        requestCaller = requestCallerFactory.create(provider, testProxy.class);
        publicationManager = new PublicationManagerImpl(attributePollInterpreter, messageSender, cleanupScheduler);
    }

    private long getExpiryDate() {
        return System.currentTimeMillis() // the publication should start now
                // time needed to publish numberOfMissedPublications publications
                + period * numberOfPublications
                // add enough time to the subscription to prevent minor timing gliches from failing the test, but not so
                // much as to cause a further publication
                + (period / 2);
    }

    final class DummyAnswer implements Answer<Void> {

        int count = 0;

        @Override
        public Void answer(InvocationOnMock invocation) throws Throwable {
            count++;
            return null;
        }
    }

    @Test(timeout = 3000)
    public void timerIsStoppedWhenEnddateIsReached() throws InterruptedException, JoynrSendBufferFullException,
                                                    JoynrMessageNotSentException, JsonGenerationException,
                                                    JsonMappingException, IOException {
        LOG.debug("Starting PublicationTimersTest.timerIsStoppedWhenEnddateIsReached test");

        DummyAnswer attributePollInterpreterAnswer = new DummyAnswer();
        DummyAnswer messageSenderAnswer = new DummyAnswer();
        Mockito.doAnswer(attributePollInterpreterAnswer)
               .when(attributePollInterpreter)
               .execute(Mockito.any(RequestCaller.class), Mockito.any(Method.class));
        Mockito.doAnswer(messageSenderAnswer)
               .when(messageSender)
               .sendSubscriptionPublication(Mockito.eq(providerId),
                                            Mockito.eq(proxyId),
                                            Mockito.eq(publication),
                                            Mockito.any(MessagingQos.class));

        qos.setExpiryDate(getExpiryDate());
        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest, requestCaller);

        // the publication timer will send one publ ication at t=0, and then 5 further publications within the time
        // alloted
        // the publication will be sent at t=0, and then 5 further publications within the time alloted
        while ((attributePollInterpreterAnswer.count != (numberOfPublications + 1) && messageSenderAnswer.count != (numberOfPublications + 1))
                || (qos.getExpiryDate() + period < System.currentTimeMillis())) {
            Thread.sleep(50);
        }

        LOG.debug("Number of attributePollInterpreter calls counted: " + attributePollInterpreterAnswer.count);
        int oldAttributePollInterpreterCallCount = attributePollInterpreterAnswer.count;
        int oldMessageSenderCallCount = messageSenderAnswer.count;
        Thread.sleep(2 * period);
        Assert.assertEquals(oldAttributePollInterpreterCallCount, attributePollInterpreterAnswer.count);
        Assert.assertEquals(oldMessageSenderCallCount, messageSenderAnswer.count);
        Mockito.verify(attributePollInterpreter, Mockito.times(oldAttributePollInterpreterCallCount))
               .execute(Mockito.any(RequestCaller.class), Mockito.any(Method.class));
        Mockito.verify(messageSender, Mockito.times(oldMessageSenderCallCount))
               .sendSubscriptionPublication(Mockito.eq(providerId),
                                            Mockito.eq(proxyId),
                                            Mockito.eq(publication),
                                            Mockito.any(MessagingQos.class));
        Mockito.verifyNoMoreInteractions(attributePollInterpreter);
    }

}

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

import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.tests.TestProxy;

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

    private SubscriptionRequest subscriptionRequest;

    private RequestCaller requestCaller;
    @Mock
    private RequestReplySender messageSender;
    @Mock
    private AttributePollInterpreter attributePollInterpreter;
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

    private int period = 500;
    private int numberOfPublications = 5;
    private int missedPublicationAlertDelay = 10;
    private long alertAfterInterval;

    private long expiryDate;

    @Before
    public void setUp() {
        alertAfterInterval = period + missedPublicationAlertDelay;

        expiryDate = System.currentTimeMillis() // the publication should start now
                + period * numberOfPublications // time needed to publish numberOfMissedPublications
                // publications
                + 100;
        qos = new PeriodicSubscriptionQos(period, expiryDate, alertAfterInterval, 1000);

        subscriptionId = "subscriptionId";
        proxyId = "proxyId";
        providerId = "providerId";
        attributeName = "testAttribute";

        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
        requestCallerFactory = new RequestCallerFactory();

        requestCaller = requestCallerFactory.create(provider, TestProxy.class);
        publicationManager = new PublicationManagerImpl(attributePollInterpreter);
    }

    @Test
    public void timerPollsAndSendsPublication() throws InterruptedException, JoynrSendBufferFullException,
                                               JoynrMessageNotSentException, JsonGenerationException,
                                               JsonMappingException, IOException {
        LOG.debug("Starting PublicationTimersTest.timerPollsAndSendsPublication test");

        publicationManager.addSubscriptionRequest(proxyId,
                                                  providerId,
                                                  subscriptionRequest,
                                                  requestCaller,
                                                  messageSender);

        Thread.sleep(expiryDate - System.currentTimeMillis());

        // the publication timer will send one publ ication at t=0, and then 5 further publications within the time
        // alloted
        Mockito.verify(attributePollInterpreter, Mockito.times(numberOfPublications + 1))
               .execute(Mockito.any(RequestCaller.class), Mockito.any(Method.class));

        // the publication will be sent at t=0, and then 5 further publications within the time alloted
        Mockito.verify(messageSender, Mockito.times(numberOfPublications + 1))
               .sendSubscriptionPublication(Mockito.eq(providerId),
                                            Mockito.eq(proxyId),
                                            Mockito.eq(publication),
                                            Mockito.any(MessagingQos.class));
    }

    @Test
    public void timerIsStoppedWhenEnddateIsReached() throws InterruptedException, JoynrSendBufferFullException,
                                                    JoynrMessageNotSentException, JsonGenerationException,
                                                    JsonMappingException, IOException {
        LOG.debug("Starting PublicationTimersTest.timerIsStoppedWhenEnddateIsReached test");

        publicationManager.addSubscriptionRequest(proxyId,
                                                  providerId,
                                                  subscriptionRequest,
                                                  requestCaller,
                                                  messageSender);

        Thread.sleep(expiryDate - System.currentTimeMillis() + 100);

        // the publication timer will send one publication at t=0, and then 5 further publications within the time
        // alloted
        Mockito.verify(attributePollInterpreter, Mockito.times(numberOfPublications + 1))
               .execute(Mockito.any(RequestCaller.class), Mockito.any(Method.class));

        // the publication will be sent at t=0, and then 5 further publications within the time alloted
        Mockito.verify(messageSender, Mockito.times(numberOfPublications + 1))
               .sendSubscriptionPublication(Mockito.eq(providerId),
                                            Mockito.eq(proxyId),
                                            Mockito.eq(publication),
                                            Mockito.any(MessagingQos.class));

        // wait some additional time to see whether there are unwanted publications
        Thread.sleep(2 * period);

        Mockito.verifyNoMoreInteractions(attributePollInterpreter);

    }

}

package io.joynr.pubsub.subscription;

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

import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.atMost;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import io.joynr.pubsub.PubSubState;

import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;

import joynr.PeriodicSubscriptionQos;

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

import com.fasterxml.jackson.core.type.TypeReference;
import com.google.common.collect.Maps;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionTimersTest {
    private static final Logger LOG = LoggerFactory.getLogger(SubscriptionTimersTest.class);

    private SubscriptionManager subscriptionManager;
    private ConcurrentMap<String, SubscriptionListener<?>> attributeSubscriptionDirectory;
    private ConcurrentMap<String, PubSubState> subscriptionStates;
    private ScheduledExecutorService subscriptionEndScheduler;
    ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers;
    ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;

    private String attributeName;
    @Mock
    private SubscriptionListener<?> attributeSubscriptionCallback;

    @Mock
    private ConcurrentMap<String, Class<? extends TypeReference<?>>> subscriptionAttributeTypes;
    private String subscriptionId;

    private int period = 100;
    private int numberOfPublications = 5;
    private int missedPublicationAlertDelay = 100;
    private long alertAfterInterval;

    class IntegerReference extends TypeReference<Integer> {
    }

    @Before
    public void setUp() {
        attributeSubscriptionDirectory = Maps.newConcurrentMap();
        subscriptionStates = Maps.newConcurrentMap();
        missedPublicationTimers = Maps.newConcurrentMap();
        subscriptionEndFutures = Maps.newConcurrentMap();
        subscriptionEndScheduler = Executors.newScheduledThreadPool(10);
        subscriptionManager = new SubscriptionManagerImpl(attributeSubscriptionDirectory,
                                                          subscriptionStates,
                                                          missedPublicationTimers,
                                                          subscriptionEndFutures,
                                                          subscriptionAttributeTypes,
                                                          subscriptionEndScheduler);

        attributeName = "testAttribute";

        alertAfterInterval = period + missedPublicationAlertDelay;
    }

    @Test(timeout = 3000)
    public void missedPublicationRunnableIsStopped() throws InterruptedException {
        LOG.debug("Starting missedPublicationRunnableIsStopped test");

        final int[] count = new int[]{ 0 };
        Answer<Void> answer = new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                count[0]++;
                return null;
            }
        };

        Mockito.doAnswer(answer).when(attributeSubscriptionCallback).publicationMissed();

        long expiryDate = System.currentTimeMillis() // the publication should start now 
                + alertAfterInterval * numberOfPublications; // time needed to raise numberOfPublications missed publication alerts
        // Note: we will receive numberOfPublications-1 missed publication alerts, since it needs also some time
        // to execute an alert and thus the last alert will be expired (due to endDate) before execution

        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos(period, expiryDate, alertAfterInterval, 1000);

        // register a subscription
        subscriptionId = subscriptionManager.registerAttributeSubscription(attributeName,
                                                                           IntegerReference.class,
                                                                           attributeSubscriptionCallback,
                                                                           qos);

        while (count[0] != numberOfPublications - 1) {
            Thread.sleep(50);
        }
        verify(attributeSubscriptionCallback, times(numberOfPublications - 1)).publicationMissed();

        // wait some additional time to see whether there are unwanted publications
        Thread.sleep(2 * period);

        // verify callback is not called
        // Note: we will receive numberOfPublications-1 missed publication alerts, since it needs also some time
        // to execute an alert and thus the last alert will be expired (due to endDate) before execution
        verifyNoMoreInteractions(attributeSubscriptionCallback);
    }

    @Test(timeout = 3000)
    public void noMissedPublicationWarningWhenPublicationIsReceived() throws InterruptedException {
        LOG.debug("Starting noMissedPublicationWarningWhenPublicationIsReceived test");

        // there should be at least one successful publication, so (numberOfPublications-1)
        //        int numberOfMissedPublications = (int) (Math.random() * (numberOfPublications - 1));
        int numberOfMissedPublications = 0;
        int numberOfSuccessfulPublications = numberOfPublications - numberOfMissedPublications;

        // TODO JOYN-1097 missed publications are currently relative to last missed publication alert.
        long expiryDate = System.currentTimeMillis() // the publication should start now 
                + alertAfterInterval * numberOfMissedPublications // time needed to raise numberOfMissedPublications missed publication alerts
                + period * numberOfSuccessfulPublications; // time needed to raise numberOfSuccessfulPublications successful publications

        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos(period, expiryDate, alertAfterInterval, 1000);
        qos.setPublicationTtl(period);
        qos.setExpiryDate(expiryDate);
        // alert 10 ms after a publication should have been received
        qos.setAlertAfterInterval(alertAfterInterval);
        qos.setPublicationTtl(1000);

        // register a subscription
        subscriptionId = subscriptionManager.registerAttributeSubscription(attributeName,
                                                                           IntegerReference.class,
                                                                           attributeSubscriptionCallback,
                                                                           qos);

        boolean lastPublicationIsMissedPublication = false;
        int missedPublicationsCounter = 0;
        int successfulPublicationsCounter = 0;
        for (int i = 0; i < numberOfPublications; i++) {
            // choose randomly whether the current publication is successful or missed
            if ((Math.random() < 0.5 && successfulPublicationsCounter < numberOfSuccessfulPublications)
                    || missedPublicationsCounter == numberOfMissedPublications) {
                Thread.sleep(period);
                // publication successfully received
                subscriptionManager.touchSubscriptionState(subscriptionId);
                successfulPublicationsCounter++;
                LOG.trace("\nSUCCESSFUL publication");
            } else {
                Thread.sleep(alertAfterInterval);
                // publication missed
                missedPublicationsCounter++;
                // Note: if the last publication is a missed publication, in _MOST_ cases we will not receive the last missed publication alert,
                // since it needs also some time to execute an alert and thus the last alert will be expired (due to endDate)
                // before execution
                if (i == numberOfPublications - 1) {
                    lastPublicationIsMissedPublication = true;
                }
                LOG.trace("\nMISSED publication");
            }
        }

        LOG.trace("No more calls are expected now.");

        // wait some additional time to see whether there are unwanted publications
        Thread.sleep(2 * period);

        // Note: see comment above about last missed publication alert
        int missedPublicationAlerts = (lastPublicationIsMissedPublication) ? missedPublicationsCounter - 1
                : missedPublicationsCounter;
        verify(attributeSubscriptionCallback, atLeast(missedPublicationAlerts)).publicationMissed();
        verify(attributeSubscriptionCallback, atMost(missedPublicationsCounter)).publicationMissed();
        // verify callback is not called
        verifyNoMoreInteractions(attributeSubscriptionCallback);
        LOG.trace("finishing test.");
    }
}

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

import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.atMost;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;

import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.util.MulticastWildcardRegexFactory;
import io.joynr.proxy.Future;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.PeriodicSubscriptionQos;
import joynr.exceptions.PublicationMissedException;
import joynr.types.DiscoveryEntryWithMetaInfo;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionTimersTest {
    private static final Logger logger = LoggerFactory.getLogger(SubscriptionTimersTest.class);

    private SubscriptionManager subscriptionManager;

    private ScheduledExecutorService subscriptionEndScheduler;
    ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers;
    ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;

    private String attributeName;
    @Mock
    private AttributeSubscriptionListener<?> attributeSubscriptionCallback;

    @Mock
    private Dispatcher dispatcher;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Mock
    private MulticastWildcardRegexFactory multicastWildcardRegexFactory;

    @Mock
    private MulticastReceiverRegistrar mockMulticastReceiverRegistrar;

    private String subscriptionId;

    private int period = 100;
    private long alertAfterInterval = 120;
    private int numberOfPublications = 5;
    private long subscriptionLength = period * numberOfPublications + alertAfterInterval;

    private String fromParticipantId;
    private String toParticipantId;
    private DiscoveryEntryWithMetaInfo toDiscoveryEntry;
    private Future<String> future;

    class IntegerReference extends TypeReference<Integer> {
    }

    @Before
    public void setUp() {
        subscriptionEndScheduler = Executors.newScheduledThreadPool(10);
        subscriptionManager = new SubscriptionManagerImpl(subscriptionEndScheduler,
                                                          dispatcher,
                                                          multicastWildcardRegexFactory,
                                                          shutdownNotifier,
                                                          mockMulticastReceiverRegistrar);
        attributeName = "testAttribute";
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        toDiscoveryEntry.setParticipantId(toParticipantId);
        future = new Future<String>();
    }

    @Test(timeout = 3000)
    public void missedPublicationRunnableIsStopped() throws InterruptedException, JoynrSendBufferFullException,
                                                     JoynrMessageNotSentException, JsonGenerationException,
                                                     JsonMappingException, IOException {
        logger.debug("Starting missedPublicationRunnableIsStopped test");

        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos().setPeriodMs(period)
                                                                   .setValidityMs(subscriptionLength)
                                                                   .setAlertAfterIntervalMs(alertAfterInterval)
                                                                   .setPublicationTtlMs(1000);

        // register a subscription
        AttributeSubscribeInvocation subscriptionRequest = new AttributeSubscribeInvocation(attributeName,
                                                                                            IntegerReference.class,
                                                                                            attributeSubscriptionCallback,
                                                                                            qos,
                                                                                            future);
        subscriptionManager.registerAttributeSubscription(fromParticipantId,
                                                          new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                          subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();
        Thread.sleep(subscriptionLength);
        verify(attributeSubscriptionCallback,
               times(numberOfPublications)).onError(new PublicationMissedException(subscriptionId));

        // wait some additional time to see whether there are unwanted publications
        Thread.sleep(2 * period);

        // verify callback is not called
        verifyNoMoreInteractions(attributeSubscriptionCallback);
    }

    @Test(timeout = 3000)
    public void noMissedPublicationWarningWhenPublicationIsReceived() throws InterruptedException,
                                                                      JoynrSendBufferFullException,
                                                                      JoynrMessageNotSentException,
                                                                      JsonGenerationException, JsonMappingException,
                                                                      IOException {
        logger.debug("Starting noMissedPublicationWarningWhenPublicationIsReceived test");

        // there should be at least one successful publication, so (numberOfPublications-1)
        int numberOfMissedPublications = (int) (Math.random() * (numberOfPublications - 1));
        // int numberOfMissedPublications = 5;
        int numberOfSuccessfulPublications = numberOfPublications - numberOfMissedPublications;

        long validityMs = period * numberOfPublications // usual length of the subscription
                + (alertAfterInterval - period); // plus time for the last possible alertAfterInterval to arrive

        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos().setPeriodMs(period)
                                                                   .setValidityMs(validityMs)
                                                                   .setAlertAfterIntervalMs(alertAfterInterval)
                                                                   .setPublicationTtlMs(1000);

        // register a subscription
        AttributeSubscribeInvocation subscriptionRequest = new AttributeSubscribeInvocation(attributeName,
                                                                                            IntegerReference.class,
                                                                                            attributeSubscriptionCallback,
                                                                                            qos,
                                                                                            future);
        subscriptionManager.registerAttributeSubscription(fromParticipantId,
                                                          new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                          subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

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
                logger.trace("\nSUCCESSFUL publication");
            } else {
                Thread.sleep(period);
                // publication missed
                missedPublicationsCounter++;
                // Note: if the last publication is a missed publication, in _MOST_ cases we will not receive the last
                // missed publication alert,
                // since it needs also some time to execute an alert and thus the last alert will be expired (due to
                // endDate)
                // before execution
                if (i == numberOfPublications - 1) {
                    lastPublicationIsMissedPublication = true;
                }
                logger.trace("\nMISSED publication");
            }
        }

        logger.trace("No more calls are expected now.");

        // wait some additional time to see whether there are unwanted publications
        Thread.sleep(2 * period);

        int missedPublicationAlerts = (lastPublicationIsMissedPublication) ? missedPublicationsCounter - 1
                : missedPublicationsCounter;
        verify(attributeSubscriptionCallback,
               atLeast(missedPublicationAlerts)).onError(new PublicationMissedException(subscriptionId));
        verify(attributeSubscriptionCallback,
               atMost(missedPublicationsCounter)).onError(new PublicationMissedException(subscriptionId));
        // verify callback is not called
        verifyNoMoreInteractions(attributeSubscriptionCallback);
        logger.trace("finishing test.");
    }
}

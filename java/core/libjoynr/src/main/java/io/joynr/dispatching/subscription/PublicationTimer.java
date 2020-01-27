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

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Optional;
import java.util.TimerTask;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.dispatching.subscription.PublicationManagerImpl.PublicationInformation;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.provider.ProviderContainer;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.SubscriptionQos;
import joynr.OnChangeSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.exceptions.ProviderRuntimeException;

/**
 * Timer object to handle periodic subscriptions
 * @author david.katz
 */
public class PublicationTimer extends PubSubTimerBase {

    private static final Logger logger = LoggerFactory.getLogger(PublicationTimer.class);
    private final PublicationInformation publicationInformation;
    private final ProviderContainer providerContainer;
    private final AttributePollInterpreter attributePollInterpreter;
    public Method method;

    private final long publicationTtl;
    private final long minInterval;
    private final long period;
    private boolean pendingPublication;
    private final PublicationManager publicationManager;

    /**
     * Constructor for PublicationTimer object, see (@link PublicationTimer)
     * @param publicationInformation information about the requested subscription, see {@link io.joynr.dispatching.subscription.PublicationManagerImpl.PublicationInformation}
     * @param method method to be invoked to retrieve the requested information
     * @param providerContainer request caller
     * @param publicationManager publication manager to send publication messages
     * @param attributePollInterpreter attribute poll interpreter to execute method
     */
    public PublicationTimer(PublicationInformation publicationInformation,
                            Method method,
                            ProviderContainer providerContainer,
                            PublicationManager publicationManager,
                            AttributePollInterpreter attributePollInterpreter) {
        super(publicationInformation.getQos().getExpiryDateMs(), publicationInformation.getState());
        this.publicationManager = publicationManager;

        SubscriptionQos qos = publicationInformation.getQos();

        this.publicationInformation = publicationInformation;

        this.publicationTtl = publicationInformation.getQos().getPublicationTtlMs();

        boolean hasSubscriptionHeartBeat = qos instanceof HeartbeatSubscriptionInformation;
        boolean isOnChangeSubscription = qos instanceof OnChangeSubscriptionQos;

        this.period = hasSubscriptionHeartBeat ? ((HeartbeatSubscriptionInformation) qos).getPeriodMs() : 0;
        this.minInterval = isOnChangeSubscription ? ((OnChangeSubscriptionQos) qos).getMinIntervalMs() : 0;

        this.providerContainer = providerContainer;
        this.attributePollInterpreter = attributePollInterpreter;
        this.method = method;
        this.pendingPublication = false;
    }

    class PublicationTask extends TimerTask {

        @Override
        public void run() {
            logger.trace("Running PublicationTask");
            if (publicationTtl > 0 && !state.isStopped() && !state.isInterrupted()) {

                long timeSinceLast = System.currentTimeMillis() - state.getTimeOfLastPublication();
                long delayUntilNextPublication;

                if (timeSinceLast < period) {
                    logger.debug("no publication necessary. MaxInterval: " + period + "TimeSinceLast: "
                            + timeSinceLast);
                    delayUntilNextPublication = period - timeSinceLast;
                    assert (delayUntilNextPublication >= 0);

                } else {
                    logger.debug("run: executing attributePollInterpreter for attribute "
                            + publicationInformation.getSubscribedToName());
                    try {
                        Optional<Promise<?>> optionalPromise = attributePollInterpreter.execute(providerContainer,
                                                                                                method);
                        if (!optionalPromise.isPresent()) {
                            throw new ProviderRuntimeException("Unexpected exception while calling getter for attribute "
                                    + publicationInformation.getSubscribedToName());
                        }
                        Promise<?> attributeGetterPromise = optionalPromise.get();
                        attributeGetterPromise.then(new PromiseListener() {

                            @Override
                            public void onRejection(JoynrException error) {
                                if (error instanceof JoynrRuntimeException) {
                                    sendPublicationError((JoynrRuntimeException) error);
                                } else {
                                    sendPublicationError(new ProviderRuntimeException("Unexpected exception while calling getter for attribute "
                                            + publicationInformation.getSubscribedToName()));
                                }
                            }

                            @Override
                            public void onFulfillment(Object... values) {
                                // attribute getters only return a single value
                                SubscriptionPublication publication = new SubscriptionPublication(Arrays.asList(values[0]),
                                                                                                  publicationInformation.getSubscriptionId());
                                sendPublication(publication);
                            }
                        });
                    } catch (JoynrRuntimeException error) {
                        sendPublicationError(error);
                    }

                    delayUntilNextPublication = period;
                }

                if (delayUntilNextPublication >= 0) {
                    logger.debug("Rescheduling PublicationTimer with delay: " + delayUntilNextPublication);
                    rescheduleTimer(delayUntilNextPublication);
                } else {
                    logger.info("Negative maxInterval: PublicationTimer is not scheduled. Publications will be sent on change only.");
                }
            }
        }
    }

    private void sendPublicationError(JoynrRuntimeException error) {
        SubscriptionPublication publication = new SubscriptionPublication(error,
                                                                          publicationInformation.getSubscriptionId());
        sendPublication(publication);
    }

    protected void sendPublication(final SubscriptionPublication publication) {
        long timeSinceLast = System.currentTimeMillis() - state.getTimeOfLastPublication();

        if (timeSinceLast >= minInterval) {
            // publish
            logger.trace("sending subscriptionreply");
            try {
                publicationManager.sendSubscriptionPublication(publication, publicationInformation);
            } catch (IOException e) {
                logger.error("sendPublication error.", e);
            }
            synchronized (PublicationTimer.this) {
                if (pendingPublication) {
                    pendingPublication = false;
                    PublicationTimer.this.notify();
                }
            }
            logger.trace("sent subscriptionreply @ " + state.getTimeOfLastPublication());
        } else {
            synchronized (PublicationTimer.this) {
                if (!pendingPublication) {
                    pendingPublication = true;
                    final long timeToWait = minInterval - timeSinceLast;
                    logger.trace("TimeToWait for subscription {}: {}", publication.getSubscriptionId(), timeToWait);
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            synchronized (PublicationTimer.this) {
                                try {
                                    PublicationTimer.this.wait(timeToWait);
                                    if (pendingPublication) {
                                        sendPublication(publication);
                                    }
                                } catch (InterruptedException e) {
                                    Thread.currentThread().interrupt();
                                    logger.trace("DelayedPublicationThread interrupted. No publication is sent.");
                                }
                            }

                        }
                    }).start();
                } else {
                    logger.trace("ignored attribute change. Mininterval {} not yet reached since timeSinceLast: {}",
                                 minInterval,
                                 timeSinceLast);
                }
            }
        }
    }

    @Override
    protected TimerTask getTimerTask() {
        return new PublicationTask();
    }

    @Override
    public void startTimer() {
        if (period > 0) {
            super.startTimer(period);
        }
    }

    public void sendPublicationNow(SubscriptionPublication publication) {

        // Only send the publication if the subscription has not expired
        // and the TTL is in the future.
        if (publicationTtl < 0) {
            logger.info("sendPublicationNow, dropping publication because TTL is in the past");
            return;
        }

        sendPublication(publication);

    }
}

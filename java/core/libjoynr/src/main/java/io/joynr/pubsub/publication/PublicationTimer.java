package io.joynr.pubsub.publication;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.PubSubTimerBase;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.publication.PublicationManagerImpl.PublicationInformation;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.TimerTask;

import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.SubscriptionPublication;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

/**
 * Timer object to handle periodic subscriptions
 * @author david.katz
 */
public class PublicationTimer extends PubSubTimerBase {

    private static final Logger logger = LoggerFactory.getLogger(PublicationTimer.class);
    private final PublicationInformation publicationInformation;
    private final RequestCaller requestCaller;
    private final RequestReplySender requestReplySender;
    private final AttributePollInterpreter attributePollInterpreter;
    public Method method;

    private final long publicationTtl;
    private final long minInterval;
    private final long period;

    /**
     * Constructor for PublicationTimer object, see (@link PublicationTimer)
     * @param publicationInformation information about the requested subscription, see {@link io.joynr.pubsub.publication.PublicationManagerImpl.PublicationInformation}
     * @param method method to be invoked to retrieve the requested information
     * @param requestCaller request caller
     * @param requestReplySender request reply sender to send publication messages
     * @param attributePollInterpreter attribute poll interpreter to execute method
     */
    public PublicationTimer(PublicationInformation publicationInformation,
                            Method method,
                            RequestCaller requestCaller,
                            RequestReplySender requestReplySender,
                            AttributePollInterpreter attributePollInterpreter) {
        super(publicationInformation.getQos().getExpiryDate(), publicationInformation.getState());

        SubscriptionQos qos = publicationInformation.getQos();

        this.publicationInformation = publicationInformation;
        this.publicationTtl = qos.getPublicationTtl();

        boolean hasSubscriptionHeartBeat = qos instanceof HeartbeatSubscriptionInformation;
        boolean isKeepAliveSubscription = qos instanceof OnChangeWithKeepAliveSubscriptionQos;

        this.period = hasSubscriptionHeartBeat ? ((HeartbeatSubscriptionInformation) qos).getHeartbeat() : 0;
        this.minInterval = isKeepAliveSubscription ? ((OnChangeWithKeepAliveSubscriptionQos) qos).getMinInterval() : 0;

        this.requestCaller = requestCaller;
        this.requestReplySender = requestReplySender;
        this.attributePollInterpreter = attributePollInterpreter;
        this.method = method;
    }

    class PublicationTask extends TimerTask {

        @Override
        public void run() {
            logger.trace("Running PublicationTask");
            if (publicationTtl > 0 && !state.isStopped() && !state.isInterrupted()) {

                long timeSinceLast = System.currentTimeMillis() - state.getTimeOfLastPublication();
                long delayUntilNextPublication;

                if (timeSinceLast < period) {
                    logger.debug("no publication necessary. MaxInterval: " + period + "TimeSinceLast: " + timeSinceLast);
                    delayUntilNextPublication = period - timeSinceLast;
                    assert (delayUntilNextPublication >= 0);

                } else {
                    logger.debug("run: executing attributePollInterpreter for attribute "
                            + publicationInformation.getSubscribedToName());
                    Promise<?> attributeGetterPromise = attributePollInterpreter.execute(requestCaller, method);
                    attributeGetterPromise.then(new PromiseListener() {

                        @Override
                        public void onRejection(JoynrRuntimeException error) {
                            // TODO Auto-generated method stub

                        }

                        @Override
                        public void onFulfillment(Object... values) {
                            // attribute getters only return a single value
                            SubscriptionPublication publication = new SubscriptionPublication(values[0],
                                                                                              publicationInformation.getSubscriptionId());
                            sendPublication(publication);
                        }
                    });

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

    protected void sendPublication(SubscriptionPublication publication) {

        long timeSinceLast = System.currentTimeMillis() - state.getTimeOfLastPublication();

        if (timeSinceLast > minInterval) {
            // publish
            logger.trace("sending subscriptionreply");
            MessagingQos messagingQos = new MessagingQos();
            messagingQos.setTtl_ms(publicationTtl);

            try {
                requestReplySender.sendSubscriptionPublication(publicationInformation.getProviderParticipantId(),
                                                               publicationInformation.getProxyParticipantId(),
                                                               publication,
                                                               messagingQos);
                // TODO handle exceptions during publication
            } catch (JoynrSendBufferFullException e) {
                logger.error("sendPublication error: {}", e.getMessage());
            } catch (JoynrMessageNotSentException e) {
                logger.error("sendPublication error: {}", e.getMessage());
            } catch (JsonGenerationException e) {
                logger.error("sendPublication error: {}", e.getMessage());
            } catch (JsonMappingException e) {
                logger.error("sendPublication error: {}", e.getMessage());
            } catch (IOException e) {
                logger.error("sendPublication error: {}", e.getMessage());
            }
            state.updateTimeOfLastPublication();
            logger.trace("sent subscriptionreply @ " + state.getTimeOfLastPublication());
        } else {
            logger.trace("igored attribute change. Mininterval {} not yet reached since timeSinceLast: {}",
                         minInterval,
                         timeSinceLast);
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

    public void sendPublicationNow(Object value) {

        // Only send the publication if the subscription has not expired
        // and the TTL is in the future.
        if (publicationTtl < 0) {
            logger.info("sendPublicationNow, dropping publication because TTL is in the past");
            return;
        }

        SubscriptionPublication publication = new SubscriptionPublication(value,
                                                                          publicationInformation.getSubscriptionId());
        sendPublication(publication);

    }
}

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
import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.PubSubTimerBase;
import io.joynr.pubsub.SubscriptionQos;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.TimerTask;

import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

/**
 * @author david.katz
 * 
 */
public class PublicationTimer extends PubSubTimerBase {

    private static final Logger logger = LoggerFactory.getLogger(PublicationTimer.class);
    private final SubscriptionRequest subscriptionRequest;
    private final RequestCaller requestCaller;
    private final RequestReplySender requestReplySender;
    private final AttributePollInterpreter attributePollInterpreter;
    public Method method;
    private final String providerParticipantId;
    private final String proxyParticipantId;

    private final long publicationTtl;
    private final long minInterval;
    private final long period;

    /**
     * 
     * @param providerParticipantId
     * @param proxyParticipantId
     * @param state
     * @param subscriptionRequest
     * @param requestCaller
     * @param messageSender
     * @param attributePollInterpreter
     * @throws NoSuchMethodException
     */
    public PublicationTimer(String providerParticipantId,
                            String proxyParticipantId,
                            PubSubState state,
                            SubscriptionRequest subscriptionRequest,
                            RequestCaller requestCaller,
                            RequestReplySender messageSender,
                            AttributePollInterpreter attributePollInterpreter) throws NoSuchMethodException {
        super(subscriptionRequest.getQos().getExpiryDate(), state);

        SubscriptionQos qos = subscriptionRequest.getQos();

        this.publicationTtl = qos.getPublicationTtl();

        if (qos instanceof PeriodicSubscriptionQos) {
            this.period = ((PeriodicSubscriptionQos) qos).getPeriod();
            this.minInterval = 0;
        } else if (qos instanceof OnChangeWithKeepAliveSubscriptionQos) {
            this.period = ((OnChangeWithKeepAliveSubscriptionQos) qos).getMaxInterval();
            this.minInterval = ((OnChangeWithKeepAliveSubscriptionQos) qos).getMinInterval();
        } else {
            period = 0;
            minInterval = 0;
        }

        this.proxyParticipantId = proxyParticipantId;
        this.providerParticipantId = providerParticipantId;
        this.subscriptionRequest = subscriptionRequest;
        this.requestCaller = requestCaller;
        this.requestReplySender = messageSender;
        this.attributePollInterpreter = attributePollInterpreter;
        findGetterForAttributeName();

    }

    private void findGetterForAttributeName() throws NoSuchMethodException {
        String attributeName = subscriptionRequest.getAttributeName();
        String attributeGetterName = "get" + attributeName.toUpperCase().charAt(0)
                + attributeName.subSequence(1, attributeName.length());
        method = ReflectionUtils.findMethodByParamTypes(requestCaller.getClass(), attributeGetterName, new Class[]{});

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
                            + subscriptionRequest.getAttributeName());

                    sendPublication();
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

    protected void sendPublication() {
        SubscriptionPublication publication = new SubscriptionPublication(attributePollInterpreter.execute(requestCaller,
                                                                                                           method),
                                                                          subscriptionRequest.getSubscriptionId());
        sendPublication(publication);
    }

    protected void sendPublication(SubscriptionPublication publication) {

        long timeSinceLast = System.currentTimeMillis() - state.getTimeOfLastPublication();

        if (timeSinceLast > minInterval) {
            // publish
            logger.trace("sending subscriptionreply");
            MessagingQos messagingQos = new MessagingQos();
            messagingQos.setTtl_ms(publicationTtl);

            try {
                requestReplySender.sendSubscriptionPublication(providerParticipantId,
                                                               proxyParticipantId,
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

    public void sendInitialPublication() {
        sendPublication();
    }

    public void sendPublicationNow(Object value) {

        // Only send the publication if the subscription has not expired
        // and the TTL is in the future.
        if (publicationTtl < 0) {
            logger.info("sendPublicationNow, dropping publication because TTL is in the past");
            return;
        }

        // TODO dka serialization issue JOYN-1351
        SubscriptionPublication publication = new SubscriptionPublication(value,
                                                                          subscriptionRequest.getSubscriptionId());
        sendPublication(publication);

    }
}

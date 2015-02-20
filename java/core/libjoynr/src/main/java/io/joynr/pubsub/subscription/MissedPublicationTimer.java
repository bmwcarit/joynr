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

import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.PubSubTimerBase;

import java.util.TimerTask;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MissedPublicationTimer extends PubSubTimerBase {

    private final AttributeSubscriptionListener<?> callback;
    private final long alertAfterInterval_ms;
    private long expectedInterval_ms;
    private static final Logger logger = LoggerFactory.getLogger(MissedPublicationTimer.class);

    public MissedPublicationTimer(long expiryDate,
                                  long expectedInterval_ms,
                                  long alertAfterInterval_ms,
                                  AttributeSubscriptionListener<?> callback,
                                  PubSubState state) {
        super(expiryDate, state);
        this.expectedInterval_ms = expectedInterval_ms;
        this.alertAfterInterval_ms = alertAfterInterval_ms;
        this.callback = callback;
        startTimer();
    }

    class MissedPublicationTask extends TimerTask {

        @Override
        public void run() {
            if (!isExpiredInMs(0) && !state.isStopped()) {
                long delay = 0;
                long timeSinceLastPublication = System.currentTimeMillis() - state.getTimeOfLastPublication();
                boolean publicationInTime = timeSinceLastPublication < alertAfterInterval_ms;
                if (publicationInTime) {
                    logger.info("Publication in time.");
                    delay = alertAfterInterval_ms - timeSinceLastPublication;
                } else {
                    logger.info("Missed publication!");
                    delay = alertAfterInterval_ms - timeSinceLastExpectedPublication(timeSinceLastPublication);
                    callback.onError();
                }
                logger.info("Rescheduling MissedPublicationTimer with delay: " + delay);
                rescheduleTimer(delay);
            } else {
                logger.info("Subscription expired. MissedPublicationTimer is not rescheduled.");
            }
        }

        private long timeSinceLastExpectedPublication(long timeSinceLastPublication) {
            return timeSinceLastPublication % expectedInterval_ms;
        }
    }

    @Override
    protected TimerTask getTimerTask() {
        return new MissedPublicationTask();
    };

}

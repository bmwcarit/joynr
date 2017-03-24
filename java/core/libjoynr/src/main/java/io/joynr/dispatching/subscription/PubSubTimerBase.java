package io.joynr.dispatching.subscription;

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

import io.joynr.pubsub.SubscriptionQos;

import java.util.Timer;
import java.util.TimerTask;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class PubSubTimerBase {
    protected long expiryDate;
    protected PubSubState state;
    protected Timer timer = new Timer();
    private static final Logger logger = LoggerFactory.getLogger(PubSubTimerBase.class);

    public PubSubTimerBase(long expiryDate, PubSubState state) {
        this.state = state;
        this.expiryDate = expiryDate;

    }

    public void startTimer() {
        startTimer(0);
    }

    public void startTimer(long delay) {
        rescheduleTimer(delay);
    }

    protected boolean isExpiredInMs(long delay_ms) {

        return expiryDate != SubscriptionQos.NO_EXPIRY_DATE && (System.currentTimeMillis() + delay_ms) > expiryDate;
    }

    public void cancel() {
        synchronized (timer) {
            state.stop();
            timer.cancel();
        }
    }

    protected void rescheduleTimer(long delay) {
        synchronized (timer) {
            boolean isExpiredNow = isExpiredInMs(0);
            boolean isExpiredBeforeNextPublication = isExpiredInMs(delay);
            if (!isExpiredNow && !isExpiredBeforeNextPublication && !state.isStopped()) {
                logger.trace("Rescheduling PubSubTimer with delay {}.", delay);
                timer.schedule(getTimerTask(), delay);
            } else {
                logger.trace("Will not reschedule PubSubTimer: "
                        + (isExpiredNow ? "endDate is reached"
                                : (isExpiredBeforeNextPublication ? "endDate will be reached before next publication"
                                        : "publication stopped")) + ".");
                logger.trace("SubscriptionEndDate: "
                        + (expiryDate == SubscriptionQos.NO_EXPIRY_DATE ? "never" : expiryDate));
                logger.trace("CurrentSystemTime: " + System.currentTimeMillis());
                logger.trace("Delay: ", delay);
            }
        }

    }

    protected abstract TimerTask getTimerTask();

}

package joynr;

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

import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.SubscriptionQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.annotation.JsonIgnore;

/**
 * Class representing the quality of service settings for subscriptions based on
 * time periods.
 * <br>
 * This class stores quality of service settings used for subscriptions to
 * <b>attributes</b> in generated proxy objects. Notifications will only be sent
 * if the period has expired. The subscription will automatically expire after
 * the expiry date is reached. If no publications were received for alertAfter
 * interval, publicationMissed will be called.
 */
public class PeriodicSubscriptionQos extends SubscriptionQos implements HeartbeatSubscriptionInformation {
    private static final long serialVersionUID = 1L;

    private static final Logger logger = LoggerFactory.getLogger(PeriodicSubscriptionQos.class);

    /**
     * Minimum value for period in milliseconds: 50
     */
    private static final long MIN_PERIOD = 50L;
    /**
     * Maximum value for period in milliseconds: 2.592.000.000 (30 days)
     */
    private static final long MAX_PERIOD = 30L * 24L * 60L * 60L * 1000L; // 30 days

    /**
     * Minimum value for alertAfterInterval in milliseconds: MIN_PERIOD: 50
     */
    private static final long MIN_ALERT_AFTER_INTERVAL = MIN_PERIOD;
    /**
     * Maximum value for alertAfterInterval in milliseconds: 2.592.000.000 (30 days)
     */
    private static final long MAX_ALERT_AFTER_INTERVAL = 30L * 24L * 60L * 60L * 1000L; // 30 days
    /**
     * Default value for alertAfterInterval in milliseconds: 0 (no alert)
     */
    private static final long DEFAULT_ALERT_AFTER_INTERVAL = 0L; // no alert
    /**
     * Value for alertAfterInterval to disable alert: 0
     */
    private static final long NO_ALERT_AFTER_INTERVAL = 0L;

    private long period = MIN_PERIOD;
    private long alertAfterInterval = DEFAULT_ALERT_AFTER_INTERVAL;

    /**
     * Default Constructor
     */
    protected PeriodicSubscriptionQos() {
    }

    /**
     * Constructor of PeriodicSubscriptionQos objects with full parameter set.
     *
     * @param period
     *            The provider will send notifications every period in milliseconds
     *            independently of value changes.
     * @param expiryDate
     *            the end date of the subscription until which publications will
     *            be sent. This value is provided in milliseconds
     *            (since 1970-01-01T00:00:00.000).
     * @param alertAfterInterval
     *            defines how long to wait for an update before publicationMissed
     *            is called if no publications were received.
     * @param publicationTtl
     *            time to live for publication messages
     *
     * @see #setPeriod(long)
     * @see #setAlertAfterInterval(long)
     * @see SubscriptionQos#SubscriptionQos(long, long)
     *           SubscriptionQos.SubscriptionQos(long, long)
     *           for more information on expiryDate and publicationTtl
     */
    public PeriodicSubscriptionQos(long period, long expiryDate, long alertAfterInterval, long publicationTtl) {
        super(expiryDate, publicationTtl);
        setPeriod(period);
        setAlertAfterInterval(alertAfterInterval);
    }

    /**
     * Constructor of PeriodicSubscriptionQos objects with specified period, expiry
     * date, and publicationTtl.
     *
     * @param period
     *            The provider will send notifications every period in milliseconds
     *            independently of value changes.
     * @param expiryDate
     *            the end date of the subscription until which publications will
     *            be sent. This value is provided in milliseconds
     *            (since 1970-01-01T00:00:00.000).
     * @param publicationTtl
     *            time to live for publication messages
     *
     * @see #setPeriod(long)
     * @see SubscriptionQos#SubscriptionQos(long, long)
     *           SubscriptionQos.SubscriptionQos(long, long)
     *           for more information on expiryDate and publicationTtl
     * @see #setAlertAfterInterval(long) setAlertAfterInterval(long)
     *           (alertAfterInterval will be set to its default value)
     */
    public PeriodicSubscriptionQos(long period, long expiryDate, long publicationTtl) {
        this(period, expiryDate, DEFAULT_ALERT_AFTER_INTERVAL, publicationTtl);
    }

    /**
     * Get the alertAfterInterval in milliseconds. <br>
     * If no notification was received within the last alert interval, a missed
     * publication notification will be raised.
     *
     * @return The alertAfterInterval in milliseconds. If more than
     *         alertAfterInterval milliseconds pass without receiving a message,
     *         the subscriptionManager will issue a publicationMissed. If set
     *         to 0, never alert.
     */
    @Override
    public long getAlertAfterInterval() {
        return alertAfterInterval;
    }

    /**
     * Set the alertAfterInterval in milliseconds. <br>
     * If no notification was received within the last alert interval, a missed
     * publication notification will be raised.<br>
     * <br>
     * <b>Minimum, Maximum, and Default Values:</b>
     * <ul>
     * <li>The absolute <b>minimum</b> setting is the period value. <br>
     * Any value less than period will be replaced by the period setting.
     * <li>The absolute <b>maximum</b> setting is 2.592.000.000 milliseconds (30 days). <br>
     * Any value bigger than this maximum will be treated at the absolute maximum setting of
     * 2.592.000.000 milliseconds.
     * <li><b>Default</b> setting: 0 milliseconds (no alert).
     * (no alert).
     * </ul>
     * <p>
     * Use {@link #clearAlertAfterInterval()} to remove missed publication notifications.
     *
     * @param alertAfterInterval_ms
     *            If more than alertInterval_ms pass without receiving a message,
     *            subscriptionManager will issue a publication missed.
     *
     * @see #clearAlertAfterInterval()
     */
    public void setAlertAfterInterval(final long alertAfterInterval_ms) {
        if (alertAfterInterval_ms < period && alertAfterInterval_ms != NO_ALERT_AFTER_INTERVAL) {
            this.alertAfterInterval = MIN_ALERT_AFTER_INTERVAL;
            logger.warn("alertAfterInterval_ms < MIN_ALERT_AFTER_INTERVAL. Using MIN_ALERT_AFTER_INTERVAL: {}",
                        MIN_ALERT_AFTER_INTERVAL);
            return;
        }
        if (alertAfterInterval_ms > MAX_ALERT_AFTER_INTERVAL) {
            this.alertAfterInterval = MAX_ALERT_AFTER_INTERVAL;
            logger.warn("alertAfterInterval_ms > MAX_ALERT_AFTER_INTERVAL. Using MAX_ALERT_AFTER_INTERVAL: {}",
                        MAX_ALERT_AFTER_INTERVAL);
            return;
        }
        this.alertAfterInterval = alertAfterInterval_ms;
    }

    /**
     * Resets the alert interval to 0 milliseconds (no alert), which disables
     * alerts. No missed publication notifications will be raised.
     */
    public void clearAlertAfterInterval() {
        alertAfterInterval = NO_ALERT_AFTER_INTERVAL;
    }

    /**
     * Get the period in milliseconds. <br>
     * The provider will periodically send notifications every period milliseconds.
     * The period can thus be seen as a sort of heart beat.
     *
     * @return The period value of the subscription in milliseconds.
     */
    public long getPeriod() {
        return period;
    }

    /**
     * Set the period in milliseconds.
     * <br>
     * The provider will periodically send notifications every period milliseconds.
     * The period can thus be seen as a sort of heart beat.<br>
     * <br>
     * <b>Minimum, Maximum, and Default Values:</b>
     * <ul>
     * <li>The absolute <b>minimum</b> setting is 50 milliseconds.<br>
     * Any value less than this minimum will be treated at the absolute minimum
     * setting of 50 milliseconds.
     * <li>The absolute <b>maximum</b> setting is 2.592.000.000 milliseconds (30 days).<br>
     * Any value bigger than this maximum will be treated at the absolute maximum
     * setting of 2.592.000.000 milliseconds (30 days).
     * <li>The <b>default</b> setting is 50 milliseconds (MIN_PERIOD).
     * </ul>
     *
     * @param period_ms
     *            The publisher will send a notification at least every period_ms.
     */
    public void setPeriod(long period_ms) {
        if (period_ms < MIN_PERIOD) {
            this.period = MIN_PERIOD;
            logger.warn("alertAfterInterval_ms < MIN_PERIOD. Using MIN_PERIOD: {}", MIN_PERIOD);

            return;
        }
        if (period_ms > MAX_PERIOD) {
            this.period = MAX_PERIOD;
            logger.warn("alertAfterInterval_ms > MAX_PERIOD. Using MAX_PERIOD: {}", MAX_PERIOD);
            return;
        }
        this.period = period_ms;
        if (this.alertAfterInterval != NO_ALERT_AFTER_INTERVAL && this.alertAfterInterval < this.period) {
            this.alertAfterInterval = this.period;
        }
    }

    @Override
    @JsonIgnore
    public long getHeartbeat() {
        return period;
    }

    /**
     * Calculate code for hashing based on member contents
     *
     * @return The calculated hash code
     */
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + (int) (alertAfterInterval ^ (alertAfterInterval >>> 32));
        result = prime * result + (int) (period ^ (period >>> 32));
        return result;
    }

    /**
     * Check for equality
     *
     * @param obj Reference to the object to compare to
     * @return true, if objects are equal, false otherwise
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        PeriodicSubscriptionQos other = (PeriodicSubscriptionQos) obj;
        if (alertAfterInterval != other.alertAfterInterval) {
            return false;
        }
        if (period != other.period) {
            return false;
        }
        return true;
    }

}

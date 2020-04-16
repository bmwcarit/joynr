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
package joynr;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.pubsub.HeartbeatSubscriptionInformation;

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
public class PeriodicSubscriptionQos extends UnicastSubscriptionQos implements HeartbeatSubscriptionInformation {
    private static final long serialVersionUID = 1L;
    private static final Logger logger = LoggerFactory.getLogger(PeriodicSubscriptionQos.class);

    /**
     * Value for alertAfterInterval to disable alert: 0
     */
    private static final long NO_ALERT_AFTER_INTERVAL = 0L;
    /**
     * Minimum value for period in milliseconds: 50
     */
    private static final long MIN_PERIOD_MS = 50L;
    /**
     * Maximum value for period in milliseconds: 2.592.000.000 (30 days)
     */
    private static final long MAX_PERIOD_MS = 30L * 24L * 60L * 60L * 1000L; // 30 days
    /**
     * Default value for period in milliseconds: 60.000 (1 minute)
     */
    private static final long DEFAULT_PERIOD_MS = 60L * 1000L;
    /**
     * Maximum value for alertAfterInterval in milliseconds: 2.592.000.000 (30 days)
     */
    private static final long MAX_ALERT_AFTER_INTERVAL_MS = 30L * 24L * 60L * 60L * 1000L; // 30 days
    /**
     * Default value for alertAfterInterval in milliseconds: no alert
     */
    private static final long DEFAULT_ALERT_AFTER_INTERVAL_MS = NO_ALERT_AFTER_INTERVAL;

    private long periodMs = DEFAULT_PERIOD_MS;
    private long alertAfterIntervalMs = DEFAULT_ALERT_AFTER_INTERVAL_MS;

    /**
     * Default Constructor
     */
    public PeriodicSubscriptionQos() {
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
    public long getAlertAfterIntervalMs() {
        return alertAfterIntervalMs;
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
     * @param alertAfterIntervalMs
     *            If more than alertInterval_ms pass without receiving a message,
     *            subscriptionManager will issue a publication missed.
     *
     * @see #clearAlertAfterInterval()
     * @return this (fluent interface).
     */
    public PeriodicSubscriptionQos setAlertAfterIntervalMs(final long alertAfterIntervalMs) {
        if (alertAfterIntervalMs > MAX_ALERT_AFTER_INTERVAL_MS) {
            this.alertAfterIntervalMs = MAX_ALERT_AFTER_INTERVAL_MS;
            logger.warn("AlertAfterInterval_ms > MAX_ALERT_AFTER_INTERVAL_MS. Using MAX_ALERT_AFTER_INTERVAL_MS: {}",
                        MAX_ALERT_AFTER_INTERVAL_MS);
        } else {
            this.alertAfterIntervalMs = alertAfterIntervalMs;
        }

        if (this.alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL && this.alertAfterIntervalMs < periodMs) {
            this.alertAfterIntervalMs = periodMs;
            logger.warn("AlertAfterInterval_ms < MIN_ALERT_AFTER_INTERVAL and will therefore be set to the period: {}",
                        periodMs);
        }
        return this;
    }

    /**
     * Resets the alert interval to 0 milliseconds (no alert), which disables
     * alerts. No missed publication notifications will be raised.
     */
    public void clearAlertAfterInterval() {
        alertAfterIntervalMs = NO_ALERT_AFTER_INTERVAL;
    }

    /**
     * Get the period in milliseconds. <br>
     * The provider will periodically send notifications every period milliseconds.
     * The period can thus be seen as a sort of heart beat.
     *
     * @return The period value of the subscription in milliseconds.
     */
    public long getPeriodMs() {
        return periodMs;
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
     * @param periodMs
     *            The publisher will send a notification at least every period_ms.
     * @return this (fluent interface).
     */
    public PeriodicSubscriptionQos setPeriodMs(long periodMs) {
        if (periodMs < MIN_PERIOD_MS) {
            this.periodMs = MIN_PERIOD_MS;
            logger.warn("PeriodMs < MIN_PERIOD_MS. Using MIN_PERIOD_MS: {}", MIN_PERIOD_MS);
        } else if (periodMs > MAX_PERIOD_MS) {
            this.periodMs = MAX_PERIOD_MS;
            logger.warn("PeriodMs > MAX_PERIOD_MS. Using MAX_PERIOD_MS: {}", MAX_PERIOD_MS);
        } else {
            this.periodMs = periodMs;
        }

        if (this.alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL && this.alertAfterIntervalMs < this.periodMs) {
            this.alertAfterIntervalMs = this.periodMs;
            logger.warn("AlertAfterIntervalMs < periodMs. Setting alertAfterIntervalMs = periodMs: {}", this.periodMs);
        }

        return this;
    }

    @Override
    public PeriodicSubscriptionQos setExpiryDateMs(long expiryDateMs) {
        return (PeriodicSubscriptionQos) super.setExpiryDateMs(expiryDateMs);
    }

    @Override
    public PeriodicSubscriptionQos setPublicationTtlMs(long publicationTtlMs) {
        return (PeriodicSubscriptionQos) super.setPublicationTtlMs(publicationTtlMs);
    }

    @Override
    public PeriodicSubscriptionQos setValidityMs(long validityMs) {
        return (PeriodicSubscriptionQos) super.setValidityMs(validityMs);
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
        result = prime * result + (int) (alertAfterIntervalMs ^ (alertAfterIntervalMs >>> 32));
        result = prime * result + (int) (periodMs ^ (periodMs >>> 32));
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
        if (alertAfterIntervalMs != other.alertAfterIntervalMs) {
            return false;
        }
        if (periodMs != other.periodMs) {
            return false;
        }
        return true;
    }

}

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
 * Class representing the quality of service settings for subscriptions
 * based on changes and time periods
 * <br>
 * This class stores quality of service settings used for subscriptions to
 * <b>attributes</b> in generated proxy objects. Using it for subscriptions to
 * broadcasts is theoretically possible because of inheritance but makes no
 * sense (in this case the additional members will be ignored).
 * <br>
 * Notifications will be sent if the subscribed value has changed or a time
 * interval without notifications has expired. The subscription will
 * automatically expire after the expiry date is reached. If no publications
 * were received for alertAfter Interval, publicationMissed will be called.
 * <br>
 * minInterval can be used to prevent too many messages being sent.
 */
public class OnChangeWithKeepAliveSubscriptionQos extends OnChangeSubscriptionQos implements
        HeartbeatSubscriptionInformation {
    private static final Logger logger = LoggerFactory.getLogger(OnChangeWithKeepAliveSubscriptionQos.class);

    private static final long MIN_MAX_INTERVAL = 0L;
    private static final long MAX_MAX_INTERVAL = 2592000000L; // 30 days

    private static final long MAX_ALERT_AFTER_INTERVAL = 2592000000L; // 30 days
    private static final long NO_ALERT_AFTER_INTERVAL = 0;
    private static final long DEFAULT_ALERT_AFTER_INTERVAL = NO_ALERT_AFTER_INTERVAL;

    private long maxInterval;
    private long alertAfterInterval;

    /**
     * Default Constructor
     */
    protected OnChangeWithKeepAliveSubscriptionQos() {
    }

    /**
     * Constructor of OnChangeWithKeepAliveSubscriptionQos object with specified
     * minInterval, maxInterval, and expiry date.
     *
     * @param minInterval_ms
     *            defines how often an update may be sent
     * @param maxInterval_ms
     *            defines how long to wait before sending an update even if the
     *            value did not change
     * @param expiryDate
     *            how long is the subscription valid
     *
     * @see OnChangeSubscriptionQos#OnChangeSubscriptionQos(long, long, long)
     *            OnChangeSubscriptionQos.OnChangeSubscriptionQos(long, long, long)
     *            for more information about <b>minInterval</b>
     * @see #setMaxInterval(long)
     * @see SubscriptionQos#SubscriptionQos(long, long)
     *            SubscriptionQos.SubscriptionQos(long, long)
     *            for more information on <b>expiryDate</b> and <b>publicationTtl</b>
     *            (publicationTtl will be set to its default value)
     * @see #setAlertAfterInterval(long) setAlertAfterInterval(long)
     *            (alertAfterInterval will be set to its default value)
     */
    public OnChangeWithKeepAliveSubscriptionQos(long minInterval_ms, long maxInterval_ms, long expiryDate) {
        this(minInterval_ms,
             maxInterval_ms,
             expiryDate,
             DEFAULT_ALERT_AFTER_INTERVAL,
             SubscriptionQos.DEFAULT_PUBLICATION_TTL);
    }

    /**
     * Constructor of OnChangeWithKeepAliveSubscriptionQos object with specified
     * minInterval, maxInterval, expiry date, and alertAfterInterval.
     *
     * @param minInterval_ms
     *            defines how often an update may be sent
     * @param maxInterval_ms
     *            defines how long to wait before sending an update even if the
     *            value did not change
     * @param expiryDate
     *            how long is the subscription valid
     * @param alertAfterInterval_ms
     *            defines how long to wait for an update before publicationMissed
     *            is called if no publications were received.
     *
     * @see OnChangeSubscriptionQos#OnChangeSubscriptionQos(long, long, long)
     *            OnChangeSubscriptionQos.OnChangeSubscriptionQos(long, long, long)
     *            for more information about <b>minInterval</b>
     * @see #setMaxInterval(long)
     * @see SubscriptionQos#SubscriptionQos(long, long)
     *            SubscriptionQos.SubscriptionQos(long, long)
     *            for more information on <b>expiryDate</b> and <b>publicationTtl</b>
     *            (publicationTtl will be set to its default value)
     * @see #setAlertAfterInterval(long)
     */
    public OnChangeWithKeepAliveSubscriptionQos(long minInterval_ms,
                                                long maxInterval_ms,
                                                long expiryDate,
                                                long alertAfterInterval_ms) {
        this(minInterval_ms, maxInterval_ms, expiryDate, alertAfterInterval_ms, SubscriptionQos.DEFAULT_PUBLICATION_TTL);
    }

    /**
     * Constructor of OnChangeWithKeepAliveSubscriptionQos object with specified
     * minInterval, maxInterval, expiry date, alertAfterInterval, and
     * publicationTtl (full parameter set).
     *
     * @param minInterval_ms
     *            defines how often an update may be sent
     * @param maxInterval_ms
     *            defines how long to wait before sending an update even if the
     *            value did not change
     * @param expiryDate_ms
     *            how long is the subscription valid
     * @param alertAfterInterval_ms
     *            defines how long to wait for an update before publicationMissed
     *            is called if no publications were received.
     * @param publicationTtl_ms
     *            time to live for publication messages
     *
     * @see OnChangeSubscriptionQos#OnChangeSubscriptionQos(long, long, long)
     *            OnChangeSubscriptionQos.OnChangeSubscriptionQos(long, long, long)
     *            for more information about <b>minInterval</b>
     * @see #setMaxInterval(long)
     * @see SubscriptionQos#SubscriptionQos(long, long)
     *            SubscriptionQos.SubscriptionQos(long, long)
     *            for more information on <b>expiryDate</b> and <b>publicationTtl</b>
     * @see #setAlertAfterInterval(long)
     */
    public OnChangeWithKeepAliveSubscriptionQos(long minInterval_ms,
                                                long maxInterval_ms,
                                                long expiryDate_ms,
                                                long alertAfterInterval_ms,
                                                long publicationTtl_ms) {
        super(minInterval_ms, expiryDate_ms, publicationTtl_ms);
        setMaxInterval(maxInterval_ms);
        setAlertAfterInterval(alertAfterInterval_ms);
    }

    /**
     * Get the maximum interval in milliseconds.
     * <br>
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change. It will send notifications more often if
     * on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus
     * be seen as a sort of heart beat or keep alive interval, if no other
     * publication has been sent within that time.
     *
     * @return The maxInterval in milliseconds. The publisher will send a
     *            notification at least every maxInterval milliseconds.
     */
    public long getMaxInterval() {
        return maxInterval;
    }

    /**
     * Set the maximum interval in milliseconds.
     * <br>
     * The provider will send publications every maximum interval in milliseconds,
     * even if the value didn't change. It will send notifications more often if
     * on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus
     * be seen as a sort of heart beat or keep alive interval, if no other
     * publication has been sent within that time.
     *
     * @param maxInterval_ms
     *            The publisher will send a notification at least every
     *            maxInterval_ms.<br>
     *            <br>
     *            <b>Minimum and Maximum Values</b>
     *            <ul>
     *            <li>The absolute <b>minimum</b> setting is
     *            {@value #MIN_MAX_INTERVAL} milliseconds. <br>
     *            Any value less than this minimum will be treated at the absolute
     *            minimum setting of{@value #MIN_MAX_INTERVAL} milliseconds.
     *            <li>The absolute <b>maximum</b> setting is
     *            {@value #MAX_MAX_INTERVAL} milliseconds. <br>
     *            Any value bigger than this maximum will be treated as the absolute
     *            maximum setting of {@value #MAX_MAX_INTERVAL} milliseconds.
     *            </ul>
     */
    public void setMaxInterval(long maxInterval_ms) {
        if (maxInterval_ms < this.getMinInterval()) {
            maxInterval_ms = this.getMinInterval();
        }
        if (maxInterval_ms < MIN_MAX_INTERVAL) {
            this.maxInterval = MIN_MAX_INTERVAL;
            return;
        }

        if (maxInterval_ms > MAX_MAX_INTERVAL) {
            this.maxInterval = MAX_MAX_INTERVAL;
            return;
        }

        this.maxInterval = maxInterval_ms;

    }

    /**
     * Get the alertAfterInterval in milliseconds.
     * <br>
     * If no notification was received within the last alert interval, a missed
     * publication notification will be raised.
     *
     * @return The alertAfterInterval in milliseconds. If more than
     *         alertAfterInterval milliseconds pass without receiving a message,
     *         the subscriptionManager will issue a publicationMissed. If set
     *         to 0 (NO_ALERT_AFTER_INTERVAL), never alert.
     */
    public long getAlertAfterInterval() {
        return alertAfterInterval;
    }

    /**
     * Set the alertAfterInterval in milliseconds.
     * <br>
     * If no notification was received within the last alert interval, a missed
     * publication notification will be raised.
     *
     * @param alertAfterInterval_ms
     *            the max time that can expire without receiving a publication
     *            before an alert will be generated. If more than alertInterval_ms
     *            pass without receiving a message, subscriptionManager will issue
     *            a publicationMissed.
     *            <ul>
     *            <li><b>Minimum</b> setting: The value cannot be set below the
     *            value of maxInterval <br>
     *            Any value less than this minimum will cause no alerts to be generated
     *            by setting the alertAfterInterval to NO_ALERT_AFTER_INTERVAL (0).
     *            <li>The absolute <b>maximum</b> setting is 2.592.000.000
     *            milliseconds (30 days). <br>
     *            Any value bigger than this maximum will be treated as the
     *            absolute maximum setting of 2.592.000.000 milliseconds.
     *            </ul>
     */
    public void setAlertAfterInterval(final long alertAfterInterval_ms) {
        if (alertAfterInterval_ms < maxInterval) {
            this.alertAfterInterval = NO_ALERT_AFTER_INTERVAL;
            logger.warn("alertAfterInterval_ms < maxInterval. AlertAfter deactivated");
            return;
        }

        if (alertAfterInterval_ms > MAX_ALERT_AFTER_INTERVAL) {
            this.alertAfterInterval = MAX_ALERT_AFTER_INTERVAL;
            logger.warn("alertAfterInterval_ms > maxInterval. Using MAX_ALERT_AFTER_INTERVAL: {}",
                        MAX_ALERT_AFTER_INTERVAL);
            return;
        }

        this.alertAfterInterval = alertAfterInterval_ms;
    }

    @JsonIgnore
    public long getHeartbeat() {
        return maxInterval;
    }

    /**
     * Calculate code for hashing based on member contents
     *
     * @return The calculated hash code
     */
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (alertAfterInterval ^ (alertAfterInterval >>> 32));
        result = prime * result + (int) (maxInterval ^ (maxInterval >>> 32));
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
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        OnChangeWithKeepAliveSubscriptionQos other = (OnChangeWithKeepAliveSubscriptionQos) obj;
        if (alertAfterInterval != other.alertAfterInterval)
            return false;
        if (maxInterval != other.maxInterval)
            return false;
        return true;
    }

}

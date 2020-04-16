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
public class OnChangeWithKeepAliveSubscriptionQos extends OnChangeSubscriptionQos
        implements HeartbeatSubscriptionInformation {
    private static final long serialVersionUID = 1L;

    private static final Logger logger = LoggerFactory.getLogger(OnChangeWithKeepAliveSubscriptionQos.class);

    private static final long MIN_MAX_INTERVAL_MS = 50L;
    private static final long MAX_MAX_INTERVAL_MS = 2592000000L; // 30 days
    private static final long DEFAULT_MAX_INTERVAL_MS = 60000L; // 1 minute
    private static final long NO_ALERT_AFTER_INTERVAL = 0;
    private static final long MAX_ALERT_AFTER_INTERVAL_MS = 2592000000L; // 30 days

    private long maxIntervalMs = DEFAULT_MAX_INTERVAL_MS;
    private long alertAfterIntervalMs = NO_ALERT_AFTER_INTERVAL;

    /**
     * Default Constructor
     */
    public OnChangeWithKeepAliveSubscriptionQos() {
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
    public long getMaxIntervalMs() {
        return maxIntervalMs;
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
    @Override
    public long getPeriodMs() {
        return getMaxIntervalMs();
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
     * @param maxIntervalMs
     *            The publisher will send a notification at least every
     *            maxIntervalMs.<br>
     *            <br>
     *            <b>Minimum and Maximum Values</b>
     *            <ul>
     *            <li>The absolute <b>minimum</b> setting is
     *            {@value #MIN_MAX_INTERVAL_MS} milliseconds. <br>
     *            Any value less than this minimum will be treated at the absolute
     *            minimum setting of{@value #MIN_MAX_INTERVAL_MS} milliseconds.
     *            <li>The absolute <b>maximum</b> setting is
     *            {@value #MAX_MAX_INTERVAL_MS} milliseconds. <br>
     *            Any value bigger than this maximum will be treated as the absolute
     *            maximum setting of {@value #MAX_MAX_INTERVAL_MS} milliseconds.
     *            </ul>
     * @return this (fluent interface).
     */
    public OnChangeWithKeepAliveSubscriptionQos setMaxIntervalMs(long maxIntervalMs) {
        if (maxIntervalMs < MIN_MAX_INTERVAL_MS) {
            this.maxIntervalMs = MIN_MAX_INTERVAL_MS;
            logger.warn("MaxIntervalMs < MIN_MAX_INTERVAL_MS. Using MIN_MAX_INTERVAL_MS: {}", MIN_MAX_INTERVAL_MS);
        } else if (maxIntervalMs > MAX_MAX_INTERVAL_MS) {
            this.maxIntervalMs = MAX_MAX_INTERVAL_MS;
            logger.warn("MaxIntervalMs > MAX_MAX_INTERVAL_MS. Using MAX_MAX_INTERVAL_MS: {}", MAX_MAX_INTERVAL_MS);
        } else {
            this.maxIntervalMs = maxIntervalMs;
        }

        if (this.maxIntervalMs < this.getMinIntervalMs()) {
            this.maxIntervalMs = this.getMinIntervalMs();
        }

        if (alertAfterIntervalMs != 0 && alertAfterIntervalMs < this.maxIntervalMs) {
            alertAfterIntervalMs = this.maxIntervalMs;
            logger.warn("AlertAfterIntervalMs < maxIntervalMs. Setting alertAfterIntervalMs = maxIntervalMs: {}",
                        this.maxIntervalMs);
        }
        return this;
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
    @Override
    public long getAlertAfterIntervalMs() {
        return alertAfterIntervalMs;
    }

    /**
     * Set the alertAfterInterval in milliseconds.
     * <br>
     * If no notification was received within the last alert interval, a missed
     * publication notification will be raised.
     *
     * @param alertAfterIntervalMs
     *            the max time that can expire without receiving a publication
     *            before an alert will be generated. If more than alertIntervalMs
     *            pass without receiving a message, subscriptionManager will issue
     *            a publicationMissed.
     *            <ul>
     *            <li><b>Minimum</b> setting: The value cannot be set below the
     *            value of maxInterval <br>
     *            If a value is passed that is less than this minimum, maxInterval
     *            will be used instead.
     *            <li>The absolute <b>maximum</b> setting is 2.592.000.000
     *            milliseconds (30 days). <br>
     *            Any value bigger than this maximum will be treated as the
     *            absolute maximum setting of 2.592.000.000 milliseconds.
     *            </ul>
     * @return this (fluent interface).
     */
    public OnChangeWithKeepAliveSubscriptionQos setAlertAfterIntervalMs(final long alertAfterIntervalMs) {
        if (alertAfterIntervalMs > MAX_ALERT_AFTER_INTERVAL_MS) {
            this.alertAfterIntervalMs = MAX_ALERT_AFTER_INTERVAL_MS;
            logger.warn("AlertAfterIntervalMs > maxInterval. Using MAX_ALERT_AFTER_INTERVAL_MS: {}",
                        MAX_ALERT_AFTER_INTERVAL_MS);
        } else {
            this.alertAfterIntervalMs = alertAfterIntervalMs;
        }

        if (this.alertAfterIntervalMs != 0 && this.alertAfterIntervalMs < maxIntervalMs) {
            this.alertAfterIntervalMs = maxIntervalMs;
            logger.warn("Attempt to set alertAfterIntervalMs to a value smaller than maxInterval; setting to maxInterval instead");
        }
        return this;
    }

    @Override
    public OnChangeWithKeepAliveSubscriptionQos setExpiryDateMs(long expiryDateMs) {
        return (OnChangeWithKeepAliveSubscriptionQos) super.setExpiryDateMs(expiryDateMs);
    }

    @Override
    public OnChangeWithKeepAliveSubscriptionQos setMinIntervalMs(long minIntervalMs) {
        super.setMinIntervalMs(minIntervalMs);
        // adjust maxInterval to match new minInterval
        return setMaxIntervalMs(maxIntervalMs);
    }

    @Override
    public OnChangeWithKeepAliveSubscriptionQos setPublicationTtlMs(long publicationTtlMs) {
        return (OnChangeWithKeepAliveSubscriptionQos) super.setPublicationTtlMs(publicationTtlMs);
    }

    @Override
    public OnChangeWithKeepAliveSubscriptionQos setValidityMs(long validityMs) {
        return (OnChangeWithKeepAliveSubscriptionQos) super.setValidityMs(validityMs);
    }

    public void clearAlertAfterInterval() {
        this.alertAfterIntervalMs = NO_ALERT_AFTER_INTERVAL;
    }

    /**
     * Calculate code for hashing based on member contents and superclass
     *
     * @return The calculated hash code
     */
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + (int) (alertAfterIntervalMs ^ (alertAfterIntervalMs >>> 32));
        result = prime * result + (int) (maxIntervalMs ^ (maxIntervalMs >>> 32));
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
        if (obj == null) {
            return false;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        OnChangeWithKeepAliveSubscriptionQos other = (OnChangeWithKeepAliveSubscriptionQos) obj;
        if (alertAfterIntervalMs != other.alertAfterIntervalMs) {
            return false;
        }
        if (maxIntervalMs != other.maxIntervalMs) {
            return false;
        }
        return true;
    }
}

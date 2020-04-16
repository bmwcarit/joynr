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

/**
 * Class representing the quality of service settings for subscriptions
 * based on changes.
 * <br>
 * This class stores quality of service settings used for subscriptions to
 * <b>broadcasts and attributes</b> in generated proxy objects. Notifications
 * will only be sent if the subscribed value has changed. The subscription will
 * automatically expire after the expiry date is reached. If no publications
 * were received for alertAfterInterval, publicationMissed will be called.
 * <br>
 * minInterval can be used to prevent too many messages being sent.
 */
public class OnChangeSubscriptionQos extends UnicastSubscriptionQos {
    private static final Logger logger = LoggerFactory.getLogger(OnChangeSubscriptionQos.class);

    private static final long serialVersionUID = 1L;
    private static final long DEFAULT_MIN_INTERVAL_MS = 1000;
    private static final long MIN_MIN_INTERVAL_MS = 0L;
    private static final long MAX_MIN_INTERVAL_MS = 2592000000L; // 30 days;

    private long minIntervalMs = DEFAULT_MIN_INTERVAL_MS;

    /**
     * Default Constructor
     */
    public OnChangeSubscriptionQos() {
    }

    /**
     * Get the minimum interval in milliseconds.
     * <br>
     * Publications will be sent maintaining this minimum interval provided,
     * even if the value changes more often. This prevents the consumer from
     * being flooded by updated values. The filtering happens on the provider's
     * side, thus also preventing excessive network traffic. This value is
     * provided in milliseconds.
     *
     * @return The minInterval in milliseconds. The publisher will keep a minimum
     *         idle time of minInterval milliseconds between two successive
     *         notifications.
     */
    public long getMinIntervalMs() {
        return minIntervalMs;
    }

    /**
     * Set the minimum interval in milliseconds.
     * <br>
     * Publications will be sent maintaining this minimum interval provided,
     * even if the value changes more often. This prevents the consumer from
     * being flooded by updated values. The filtering happens on the provider's
     * side, thus also preventing excessive network traffic. This value is
     * provided in milliseconds.<br>
     * <br>
     * <b>Minimum and Maximum Values</b>
     * <ul>
     * <li><b>Minimum</b> minInterval: {@value #MIN_MIN_INTERVAL_MS}. Smaller values will be rounded up.
     * <li><b>Maximum</b> minInterval: {@value #MAX_MIN_INTERVAL_MS}. Larger values
     * will be rounded down.
     * </ul>
     *
     * @param minIntervalMs
     *            The publisher will keep a minimum idle time of minIntervalMs
     *            between two successive notifications.
     * @return this (fluent interface).
     */
    public OnChangeSubscriptionQos setMinIntervalMs(final long minIntervalMs) {
        return setMinIntervalMsInternal(minIntervalMs);
    }

    @Override
    public OnChangeSubscriptionQos setExpiryDateMs(long expiryDateMs) {
        return (OnChangeSubscriptionQos) super.setExpiryDateMs(expiryDateMs);
    }

    @Override
    public OnChangeSubscriptionQos setPublicationTtlMs(long publicationTtlMs) {
        return (OnChangeSubscriptionQos) super.setPublicationTtlMs(publicationTtlMs);
    }

    @Override
    public OnChangeSubscriptionQos setValidityMs(long validityMs) {
        return (OnChangeSubscriptionQos) super.setValidityMs(validityMs);
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
        result = prime * result + (int) (minIntervalMs ^ (minIntervalMs >>> 32));
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
        OnChangeSubscriptionQos other = (OnChangeSubscriptionQos) obj;
        if (minIntervalMs != other.minIntervalMs) {
            return false;
        }
        return true;
    }

    // internal method required to prevent findbugs warning
    private OnChangeSubscriptionQos setMinIntervalMsInternal(final long minIntervalMs) {
        if (minIntervalMs < MIN_MIN_INTERVAL_MS) {
            this.minIntervalMs = MIN_MIN_INTERVAL_MS;
            logger.warn("MinIntervalMs < MIN_MIN_INTERVAL_MS. Using MIN_MIN_INTERVAL_MS: {}", MIN_MIN_INTERVAL_MS);
        } else if (minIntervalMs > MAX_MIN_INTERVAL_MS) {
            this.minIntervalMs = MAX_MIN_INTERVAL_MS;
            logger.warn("MinIntervalMs > MAX_MIN_INTERVAL_MS. Using MAX_MIN_INTERVAL_MS: {}", MAX_MIN_INTERVAL_MS);
        } else {
            this.minIntervalMs = minIntervalMs;
        }

        return this;
    }

}

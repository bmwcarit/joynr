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

import io.joynr.pubsub.SubscriptionQos;

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
public class OnChangeSubscriptionQos extends SubscriptionQos {

    private static final long MIN_MIN_INTERVAL = 50L;
    private static final long MAX_MIN_INTERVAL = 2592000000L; // 30 days;

    private long minInterval;

    /**
     * Default Constructor
     */
    protected OnChangeSubscriptionQos() {
    }

    /**
     * Constructor of OnChangeSubscriptionQos object used for subscriptions on
     * broadcasts in generated proxy objects
     *
     * @param minInterval_ms
     *            is used to prevent flooding. Publications will be sent
     *            maintaining this minimum interval provided, even if the value
     *            changes more often. This prevents the consumer from being
     *            flooded by updated values. The filtering happens on the
     *            provider's side, thus also preventing excessive network
     *            traffic. This value is provided in milliseconds.
     * @param expiryDate
     *            The expiryDate is the end date of the subscription. This value
     *            is provided in milliseconds (since 1970-01-01T00:00:00.000).
     * @param publicationTtl_ms
     *            is the time-to-live for publication messages.
     *            NOTE minimum and maximum values apply.
     *
     * @see #setMinInterval(long)
     * @see SubscriptionQos#SubscriptionQos(long, long)
     *           SubscriptionQos.SubscriptionQos(long, long)
     *           for more information on expiryDate and publicationTtl
     */
    public OnChangeSubscriptionQos(long minInterval_ms, long expiryDate, long publicationTtl_ms) {
        super(expiryDate, publicationTtl_ms);
        setMinInterval(minInterval_ms);

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
    public long getMinInterval() {
        return minInterval;
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
     * <li><b>Minimum</b> minInterval: 50. Smaller values will be rounded up.
     * <li><b>Maximum</b> minInterval: 2.592.000.000 (30 days). Larger values
     * will be rounded down.
     * </ul>
     *
     * @param minInterval_ms
     *            The publisher will keep a minimum idle time of minInterval_ms
     *            between two successive notifications.
     */
    public void setMinInterval(final long minInterval_ms) {
        if (minInterval_ms < MIN_MIN_INTERVAL) {
            this.minInterval = MIN_MIN_INTERVAL;
            return;
        }

        if (minInterval_ms > MAX_MIN_INTERVAL) {
            this.minInterval = MAX_MIN_INTERVAL;
            return;
        }

        this.minInterval = minInterval_ms;

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
        result = prime * result + (int) (minInterval ^ (minInterval >>> 32));
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
        OnChangeSubscriptionQos other = (OnChangeSubscriptionQos) obj;
        if (minInterval != other.minInterval)
            return false;
        return true;
    }

}

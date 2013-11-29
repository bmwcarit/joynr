package joynr;

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

import io.joynr.pubsub.SubscriptionQos;

public class OnChangeSubscriptionQos extends SubscriptionQos {

    private static final long MIN_MIN_INTERVAL = 50L;
    private static final long MAX_MIN_INTERVAL = 2592000000L; // 30 days;

    private long minInterval;

    protected OnChangeSubscriptionQos() {
    }

    /**
     * @param minInterval_ms
     *            is used to prevent flooding. Publications will be sent maintaining this minimum interval provided,
     *            even if the value changes more often. This prevents the consumer from being flooded by updated values.
     *            The filtering happens on the provider's side, thus also preventing excessive network traffic. This
     *            value is provided in milliseconds.
     * @param expiryDate
     *            The expiryDate is the end date of the subscription. This value is provided in milliseconds (since
     *            1970-01-01T00:00:00.000).
     * @param publicationTtl
     *            is the time-to-live for publication messages. NOTE minimum and maximum values apply.
     * @see SubscriptionQos#SubscriptionQos(long, long)
     */
    public OnChangeSubscriptionQos(long minInterval_ms, long expiryDate, long publicationTtl_ms) {
        super(expiryDate, publicationTtl_ms);
        setMinInterval(minInterval_ms);

    }

    /**
     * Publications will be sent maintaining this minimum interval provided, even if the value changes more often. This
     * prevents the consumer from being flooded by updated values. The filtering happens on the provider's side, thus
     * also preventing excessive network traffic. This value is provided in milliseconds.
     * 
     * @return minInterval_ms The publisher will keep a minimum idle time of minInterval_ms between two successive
     *         notifications.
     */
    public long getMinInterval() {
        return minInterval;
    }

    /**
     * Publications will be sent maintaining this minimum interval provided, even if the value changes more often. This
     * prevents the consumer from being flooded by updated values. The filtering happens on the provider's side, thus
     * also preventing excessive network traffic. This value is provided in milliseconds.
     * 
     * @param minInterval_ms
     *            The publisher will keep a minimum idle time of minInterval_ms between two successive notifications.
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
}

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
package io.joynr.pubsub;

import io.joynr.subtypes.JoynrType;

/**
 * Base class representing the subscription quality of service settings.
 * <br>
 * This class stores quality of service settings used for subscriptions to
 * <b>attributes and broadcasts</b> in generated proxy objects.
 * The subscription will automatically expire after the expiry date is reached.
 */
public abstract class SubscriptionQos implements JoynrType {
    private static final long serialVersionUID = 1L;

    private long expiryDateMs = NO_EXPIRY_DATE;
    public static final int IGNORE_VALUE = -1;
    public static final long INFINITE_SUBSCRIPTION = 9007199254740991L;

    /**
     * Expiry date value to disable expiration: {@value #NO_EXPIRY_DATE}.
     */
    public static final long NO_EXPIRY_DATE = 0L;

    /**
     * Default Constructor
     */
    public SubscriptionQos() {
    }

    /**
     * Get the end date of the subscription.
     * <br>
     * The provider will send notifications until the expiry date is reached.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this date.
     *
     * @return the end date of the subscription. <br>This value is provided in
     *            milliseconds (since 1970-01-01T00:00:00.000).
     */
    public long getExpiryDateMs() {
        return expiryDateMs;
    }

    /**
     * Set the end date of the subscription, in milliseconds (since 1970-01-01T00:00:00.000 ).
     * The publications will automatically expire at that date.
     * <br>
     * The provider will send notifications until the expiry date is reached.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this date.
     *
     * @param expiryDateMs
     *            is the end date of the subscription. <br>
     *            This value is provided in milliseconds (since 1970-01-01T00:00:00.000).
     *            {@value #NO_EXPIRY_DATE} means NO_EXPIRY_DATE.
     * @return the subscriptionQos (fluent interface)
     */
    public SubscriptionQos setExpiryDateMs(final long expiryDateMs) {
        long now = System.currentTimeMillis();
        if (expiryDateMs <= now && expiryDateMs != NO_EXPIRY_DATE) {
            throw new IllegalArgumentException("Subscription ExpiryDate " + expiryDateMs + " in the past. Now: " + now);
        }
        this.expiryDateMs = expiryDateMs;
        return this;
    }

    /**
     * Set how long the subscription should run for, in milliseconds.
     * This is a helper method that allows setting the expiryDate using
     * a relative time.
     *
     * @param validityMs
     *            is the number of milliseconds until the subscription will expire
     * @return the subscriptionQos (fluent interface)
     */
    public SubscriptionQos setValidityMs(final long validityMs) {
        if (validityMs == -1) {
            setExpiryDateMs(NO_EXPIRY_DATE);
        } else {
            long now = System.currentTimeMillis();
            this.expiryDateMs = now + validityMs;
        }
        return this;
    }

    /**
     * Resets the expiry date to the default value {@value #NO_EXPIRY_DATE}
     * (NO_EXPIRY_DATE).
     */
    public void clearExpiryDate() {
        this.expiryDateMs = NO_EXPIRY_DATE;
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
        result = prime * result + (int) (expiryDateMs ^ (expiryDateMs >>> 32));
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
        if (getClass() != obj.getClass()) {
            return false;
        }
        SubscriptionQos other = (SubscriptionQos) obj;
        if (expiryDateMs != other.expiryDateMs) {
            return false;
        }
        return true;
    }

}

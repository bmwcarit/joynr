package io.joynr.pubsub;

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

import io.joynr.subtypes.JoynrType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base class representing the subscription quality of service settings.
 * <br>
 * This class stores quality of service settings used for subscriptions to
 * <b>attributes and broadcasts</b> in generated proxy objects.
 * The subscription will automatically expire after the expiry date is reached.
 */
public abstract class SubscriptionQos implements JoynrType {
    private static final long serialVersionUID = 1L;

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionQos.class);

    private long expiryDateMs = NO_EXPIRY_DATE;
    private long publicationTtlMs = DEFAULT_PUBLICATION_TTL_MS;

    public static final int IGNORE_VALUE = -1;
    public static final long INFINITE_SUBSCRIPTION = Long.MAX_VALUE;
    /**
     * Minimum value for publicationTtl in milliseconds: 100.
     */
    private static final long MIN_PUBLICATION_TTL_MS = 100L;
    /**
     * Maximum value for publicationTtl in milliseconds: 2.592.000.000 (30 days).
     */
    private static final long MAX_PUBLICATION_TTL_MS = 2592000000L; // 30 days

    /**
     * Default value for publicationTtl in milliseconds: 10 000 (10 secs).
     */
    protected static final long DEFAULT_PUBLICATION_TTL_MS = 10000;

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
     * Constructor of SubscriptionQos object with specified expiry date.
     *
     * @deprecated This constructor will be deleted by 2017-01-01.
     * Use the fluent interface instead:
     *  new SubscriptionQos().setExpiryDate() or
     *  new SubscriptionQos().setValidity()
     * @param expiryDateMs
     *            The expiryDate is the end date of the subscription. This value
     *            is provided in milliseconds (since 1970-01-01T00:00:00.000).
     *
     * @see #setPublicationTtl(long) setPublicationTtl(long)
     *            (publicationTtl will be set to its default value)
     */
    @Deprecated
    public SubscriptionQos(long expiryDateMs) {
        this(expiryDateMs, DEFAULT_PUBLICATION_TTL_MS);
    }

    /**
     * Constructor of SubscriptionQos object with specified expiry date and
     * publication ttl (full parameter set).
     *
     * @deprecated Use the fluent interface:
     *  new SubscriptionQos().setExpiryDate() or
     *  new SubscriptionQos().setValidity()
     *
     * @param expiryDateMs
     *            the end date of the subscription until which publications will
     *            be sent. This value is provided in milliseconds
     *            (since 1970-01-01T00:00:00.000).
     * @param publicationTtlMs
     *            is the time-to-live for publication messages.<br>
     * <br>
     *            If a notification message can not be delivered within its time
     *            to live, it will be deleted from the system. This value is
     *            provided in milliseconds.
     *
     * @see #setExpiryDateMs(long)
     * @see #setPublicationTtl(long)
     */
    @Deprecated
    public SubscriptionQos(long expiryDateMs, long publicationTtlMs) {
        setExpiryDateMs(expiryDateMs);
        setPublicationTtl(publicationTtlMs);
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
    public long getExpiryDate() {
        return expiryDateMs;
    }

    /**
     * @deprecated Use setExpiryDateMs instead
     *
     * Set the end date of the subscription, in milliseconds (since 1970-01-01T00:00:00.000 ).
     * The publications will automatically expire at that date.
     * <br>
     * The provider will send notifications until the expiry date is reached.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this date.
     * @param expiryDateMs
     *            is the end date of the subscription. <br>
     *            This value is provided in milliseconds (since 1970-01-01T00:00:00.000).
     *            {@value #NO_EXPIRY_DATE} means NO_EXPIRY_DATE.
     * @return the subscriptionQos (fluent interface)
     */
    @Deprecated
    public SubscriptionQos setExpiryDate(final long expiryDateMs) {
        return setExpiryDateMs(expiryDateMs);
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
     * Get the time-to-live for notification messages.
     * <br>
     * Notification messages will be sent with this time-to-live.<br>
     * <br>
     * If a notification message can not be delivered within its time to live,
     * it will be deleted from the system. This value is provided in milliseconds.
     *
     * @return the publication time-to-live in milliseconds.
     */
    public long getPublicationTtl() {
        return publicationTtlMs;
    }

    /**
     * Set the time-to-live for notification messages.
     * <br>
     * Notification messages will be sent with this time-to-live. If a notification message can not be delivered within
     * its time to live, it will be deleted from the system. This value is provided in milliseconds.
     *
     * @param publicationTtlMs
     *            publicationTtlMs time-to-live in milliseconds.<br>
     *            <br>
     *            <b>Minimum and Default Values:</b>
     *            <ul>
     *            <li><b>Minimum</b> publicationTtlMs = 100.
     *            Smaller values will be rounded up.
     *            <li><b>Maximum</b> publicationTtlMs = 2.592.000.000 (30 days)
     *            Larger values will be rounded down.
     *            <li><b>Default</b> publicationTtlMs = 10 000 (10 secs)
     *            </ul>
     * @return the subscriptionQos (fluent interface)
     */
    public SubscriptionQos setPublicationTtl(final long publicationTtlMs) {
        if (publicationTtlMs < MIN_PUBLICATION_TTL_MS) {
            this.publicationTtlMs = MIN_PUBLICATION_TTL_MS;
            logger.warn("publicationTtlMs < MIN_PUBLICATION_TTL. Using MIN_PUBLICATION_TTL: {}", MIN_PUBLICATION_TTL_MS);
        } else if (publicationTtlMs > MAX_PUBLICATION_TTL_MS) {
            this.publicationTtlMs = MAX_PUBLICATION_TTL_MS;
            logger.warn("publicationTtlMs > MAX_PUBLICATION_TTL. Using MAX_PUBLICATION_TTL: {}", MAX_PUBLICATION_TTL_MS);
        } else {
            this.publicationTtlMs = publicationTtlMs;
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
        result = prime * result + (int) (publicationTtlMs ^ (publicationTtlMs >>> 32));
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
        if (publicationTtlMs != other.publicationTtlMs) {
            return false;
        }
        return true;
    }

}

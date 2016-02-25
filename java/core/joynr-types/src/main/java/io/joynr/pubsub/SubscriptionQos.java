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

    private long expiryDate;
    private long publicationTtl;

    public static final int IGNORE_VALUE = -1;
    public static final long INFINITE_SUBSCRIPTION = Long.MAX_VALUE;
    /**
     * Minimum value for publicationTtl in milliseconds: 100.
     */
    private static final long MIN_PUBLICATION_TTL = 100L;
    /**
     * Maximum value for publicationTtl in milliseconds: 2.592.000.000 (30 days).
     */
    private static final long MAX_PUBLICATION_TTL = 2592000000L; // 30 days

    /**
     * Default value for publicationTtl in milliseconds: 10 000 (10 secs).
     */
    protected static final long DEFAULT_PUBLICATION_TTL = 10000;

    /**
     * Expiry date value to disable expiration: {@value #NO_EXPIRY_DATE}.
     */
    public static final long NO_EXPIRY_DATE = 0L;

    /**
     * Default Constructor
     */
    protected SubscriptionQos() {
        this(NO_EXPIRY_DATE);
    }

    /**
     * Constructor of SubscriptionQos object with specified expiry date.
     *
     * @param expiryDate
     *            The expiryDate is the end date of the subscription. This value
     *            is provided in milliseconds (since 1970-01-01T00:00:00.000).
     *
     * @see #setPublicationTtl(long) setPublicationTtl(long)
     *            (publicationTtl will be set to its default value)
     */
    public SubscriptionQos(long expiryDate) {
        this(expiryDate, DEFAULT_PUBLICATION_TTL);
    }

    /**
     * Constructor of SubscriptionQos object with specified expiry date and
     * publication ttl (full parameter set).
     *
     * @param expiryDate
     *            the end date of the subscription until which publications will
     *            be sent. This value is provided in milliseconds
     *            (since 1970-01-01T00:00:00.000).
     * @param publicationTtl
     *            is the time-to-live for publication messages.<br>
     * <br>
     *            If a notification message can not be delivered within its time
     *            to live, it will be deleted from the system. This value is
     *            provided in milliseconds.
     *
     * @see #setExpiryDate(long)
     * @see #setPublicationTtl(long)
     */
    public SubscriptionQos(long expiryDate, long publicationTtl) {
        setExpiryDate(expiryDate);
        setPublicationTtl(publicationTtl);
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
        return expiryDate;
    }

    /**
     * Set the end date of the subscription. The publications will automatically
     * expire at that date.
     * <br>
     * The provider will send notifications until the expiry date is reached.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this date.
     *
     * @param expiryDate_ms
     *            is the end date of the subscription. <br>
     *            This value is provided in milliseconds (since 1970-01-01T00:00:00.000).
     *            {@value #NO_EXPIRY_DATE} means NO_EXPIRY_DATE.
     */
    public void setExpiryDate(final long expiryDate_ms) {
        long now = System.currentTimeMillis();
        if (expiryDate_ms <= now && expiryDate_ms != NO_EXPIRY_DATE) {
            throw new IllegalArgumentException("Subscription ExpiryDate " + expiryDate_ms + " in the past. Now: " + now);
        }
        this.expiryDate = expiryDate_ms;
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
        long now = System.currentTimeMillis();
        this.expiryDate = now + validityMs;
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
        return publicationTtl;
    }

    /**
     * Set the time-to-live for notification messages.
     * <br>
     * Notification messages will be sent with this time-to-live. If a notification message can not be delivered within
     * its time to live, it will be deleted from the system. This value is provided in milliseconds.
     *
     * @param publicationTtl_ms
     *            publicationTtl_ms time-to-live in milliseconds.<br>
     *            <br>
     *            <b>Minimum and Default Values:</b>
     *            <ul>
     *            <li><b>Minimum</b> publicationTtl_ms = 100.
     *            Smaller values will be rounded up.
     *            <li><b>Maximum</b> publicationTtl_ms = 2.592.000.000 (30 days)
     *            Larger values will be rounded down.
     *            <li><b>Default</b> publicationTtl_ms = 10 000 (10 secs)
     *            </ul>
     */
    public void setPublicationTtl(final long publicationTtl_ms) {
        if (publicationTtl_ms < MIN_PUBLICATION_TTL) {
            this.publicationTtl = MIN_PUBLICATION_TTL;
            logger.warn("publicationTtl_ms < MIN_PUBLICATION_TTL. Using MIN_PUBLICATION_TTL: {}", MIN_PUBLICATION_TTL);
            return;
        }
        if (publicationTtl_ms > MAX_PUBLICATION_TTL) {
            this.publicationTtl = MAX_PUBLICATION_TTL;
            logger.warn("publicationTtl_ms > MAX_PUBLICATION_TTL. Using MAX_PUBLICATION_TTL: {}", MAX_PUBLICATION_TTL);
            return;
        }
        this.publicationTtl = publicationTtl_ms;
    }

    /**
     * Resets the expiry date to the default value {@value #NO_EXPIRY_DATE}
     * (NO_EXPIRY_DATE).
     */
    public void clearExpiryDate() {
        this.expiryDate = NO_EXPIRY_DATE;
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
        result = prime * result + (int) (expiryDate ^ (expiryDate >>> 32));
        result = prime * result + (int) (publicationTtl ^ (publicationTtl >>> 32));
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
        if (expiryDate != other.expiryDate) {
            return false;
        }
        if (publicationTtl != other.publicationTtl) {
            return false;
        }
        return true;
    }

}

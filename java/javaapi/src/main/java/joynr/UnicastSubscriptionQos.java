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

import io.joynr.pubsub.SubscriptionQos;

/**
 * Class representing the quality of service settings for non-selective broadcasts
 */
public class UnicastSubscriptionQos extends SubscriptionQos {
    private static final Logger logger = LoggerFactory.getLogger(UnicastSubscriptionQos.class);

    private static final long serialVersionUID = 1L;
    /**
     * Minimum value for publicationTtl in milliseconds: 100.
     */
    protected static final long MIN_PUBLICATION_TTL_MS = 100L;
    /**
     * Maximum value for publicationTtl in milliseconds: 2.592.000.000 (30 days).
     */
    protected static final long MAX_PUBLICATION_TTL_MS = 2592000000L; // 30 days

    /**
     * Default value for publicationTtl in milliseconds: 10 000 (10 secs).
     */
    protected static final long DEFAULT_PUBLICATION_TTL_MS = 10000;
    private long publicationTtlMs = DEFAULT_PUBLICATION_TTL_MS;

    /**
     * Default Constructor
     */
    public UnicastSubscriptionQos() {
    }

    @Override
    public UnicastSubscriptionQos setExpiryDateMs(long expiryDateMs) {
        return (UnicastSubscriptionQos) super.setExpiryDateMs(expiryDateMs);
    }

    @Override
    public UnicastSubscriptionQos setValidityMs(long validityMs) {
        return (UnicastSubscriptionQos) super.setValidityMs(validityMs);
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
    public long getPublicationTtlMs() {
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
     *            <b>Minimum, Maximum and Default Values:</b>
     *            <ul>
     *            <li><b>Minimum</b> publicationTtlMs = {@value #MIN_PUBLICATION_TTL_MS}.
     *            Smaller values will be rounded up.
     *            <li><b>Maximum</b> publicationTtlMs = {@value #MAX_PUBLICATION_TTL_MS}.
     *            Larger values will be rounded down.
     *            <li><b>Default</b> publicationTtlMs = {@value #DEFAULT_PUBLICATION_TTL_MS}.
     *            </ul>
     * @return the subscriptionQos (fluent interface)
     */
    public SubscriptionQos setPublicationTtlMs(final long publicationTtlMs) {
        if (publicationTtlMs < MIN_PUBLICATION_TTL_MS) {
            this.publicationTtlMs = MIN_PUBLICATION_TTL_MS;
            logger.warn("PublicationTtlMs < MIN_PUBLICATION_TTL. Using MIN_PUBLICATION_TTL: {}",
                        MIN_PUBLICATION_TTL_MS);
        } else if (publicationTtlMs > MAX_PUBLICATION_TTL_MS) {
            this.publicationTtlMs = MAX_PUBLICATION_TTL_MS;
            logger.warn("PublicationTtlMs > MAX_PUBLICATION_TTL. Using MAX_PUBLICATION_TTL: {}",
                        MAX_PUBLICATION_TTL_MS);
        } else {
            this.publicationTtlMs = publicationTtlMs;
        }

        return this;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + (int) (publicationTtlMs ^ (publicationTtlMs >>> 32));
        return result;
    }

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
        UnicastSubscriptionQos other = (UnicastSubscriptionQos) obj;
        if (publicationTtlMs != other.publicationTtlMs) {
            return false;
        }
        return true;
    }
}

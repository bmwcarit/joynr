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
#ifndef SUBSCRIPTIONQOS_H
#define SUBSCRIPTIONQOS_H

#include <cstdint>

#include "joynr/JoynrCommonExport.h"

namespace joynr
{

/**
 * @brief Base class representing the subscription quality of service settings
 *
 * Class that stores quality of service settings for subscriptions to
 * <b>attributes and broadcasts</b>.
 *
 * The subscription will automatically expire after the expiryDate is reached.
 */
class JOYNRCOMMON_EXPORT SubscriptionQos
{

public:
    /** @brief Default constructor */
    SubscriptionQos();

    /**
     * @brief Copy constructor
     * @param subscriptionQos The SubscriptionQos object to be copied from.
     */
    SubscriptionQos(const SubscriptionQos& subscriptionQos) = default;

    /**
     * @brief Constructor SubscriptionQos objects with specified validity
     * @param validity Time span in milliseconds during which publications will be sent
     *
     * @see SubscriptionQos#setValidityMs
     * @see SubscriptionQos#setPublicationTtlMs
     */
    explicit SubscriptionQos(const std::int64_t& validityMs);

    /** Destructor */
    virtual ~SubscriptionQos() = default;

    /**
     * @brief Gets the expiry date of the subscription.
     *
     * The provider will send notifications until the expiry date is reached.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this date.
     *
     * @return The expiry date in milliseconds.
     * The publication will automatically expire at that EndDate.
     *
     * @see SubscriptionQos#setValidityMs
     */
    std::int64_t getExpiryDateMs() const;

    /**
     * @deprecated
     * @see SubscriptionQos#getExpiryDateMs
     */
    [[deprecated("Will be removed by end of the year 2016. Use getExpiryDateMs instead.")]] std::
            int64_t
            getExpiryDate() const;

    /**
     * @brief Clears the current expiry date and disables it, by setting
     * the value to NO_EXPIRY_DATE.
     */
    virtual void clearExpiryDate();

    /**
     * @brief Sets the expiry date
     *
     * The provider will send notifications until the the expiry date
     * is reached. You will not receive any notifications (neither
     * value notifications nor missed publication notifications) after
     * this date.
     *
     * @param expiryDate The expiry date in milliseconds
     * The publication will automatically expire at that date.
     *
     * @see SubscriptionQos#setValidityMs
     */
    virtual void setExpiryDateMs(const std::int64_t& expiryDateMs);

    /**
     * @deprecated
     * @see SubscriptionQos#setExpiryDateMs
     */
    [[deprecated(
            "Will be removed by end of the year 2016. Use setExpiryDateMs instead.")]] virtual void
    setExpiryDate(const std::int64_t& expiryDateMs);

    /**
     * @brief Gets the time to live value for publication messages.
     *
     * Notification messages will be sent with this time-to-live.
     * If a notification message can not be delivered within its TTL,
     * it will be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an
     * expired TTL, it might raise a missed publication notification
     * (depending on the value of the alert interval QoS).
     *
     * @return Returns the TTL of the publication Messages in milliseconds.
     *
     * @see SubscriptionQos#setExpiryDateMs
     */
    virtual std::int64_t getPublicationTtlMs() const;

    /**
     * @deprecated
     * @see SubscriptionQos#getPublicationTtlMs
     */
    [[deprecated("Will be removed by end of the year 2016. Use getPublicationTtlMs "
                 "instead.")]] virtual std::int64_t
    getPublicationTtl() const;

    /**
     * @brief Sets the validity of the subscription in milliseconds.
     *
     * The provider will send notifications for the next validity ms.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this time span expired.
     *
     * <p>This is equivalent to setting the expiry date QoS to NOW_ms + validity_ms.
     *
     * @param validity Time span in milliseconds during which publications will be sent
     *
     * @see SubscriptionQos#setExpiryDateMs
     */
    virtual void setValidityMs(const std::int64_t& validityMs);

    /**
     * @deprecated
     * @see SubscriptionQos#setValidityMs
     */
    [[deprecated(
            "Will be removed by end of the year 2016. Use setValidityMs instead.")]] virtual void
    setValidity(const std::int64_t& validityMs);

    /**
     * @brief Sets the time to live for publication messages in milliseconds
     *
     * Notification messages will be sent with this time-to-live. If a
     * notification message can not be delivered within its TTL, it will
     * be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an
     * expired TTL, it might raise a missed publication notification
     * (depending on the value of the alert interval QoS).
     *
     * @param publicationTtl_ms TTL of the publication Messages in milliseconds.
     * <br/><br>
     * <b>Minimum and Maximum Values:</b>
     * <ul>
     * <li>minimum publicationTtl_ms = 100. Smaller values will be rounded up.
     * <li>maximum publicationTtl_ms = 2 592 000 000 (30 days). Larger values will be rounded down.
     * </ul>
     *
     * @see SubscriptionQos#setExpiryDateMs
     */
    virtual void setPublicationTtlMs(const std::int64_t& publicationTtlMs);

    /**
     * @deprecated
     * @see SubscriptionQos#setPublicationTtlMs
     */
    [[deprecated("Will be removed by end of the year 2016. Use setPublicationTtlMs "
                 "instead.")]] virtual void
    setPublicationTtl(const std::int64_t& publicationTtlMs);

    /** @brief Assignment operator */
    SubscriptionQos& operator=(const SubscriptionQos& subscriptionQos);

    /** @brief Equality operator */
    bool operator==(const SubscriptionQos& subscriptionQos) const;

    /**
     * @brief Returns the default publication time to live value in milliseconds:
     * 10 000 (10 secs)
     */
    static const std::int64_t& DEFAULT_PUBLICATION_TTL_MS();

    /**
     * @deprecated
     * @see SubscriptionQos#DEFAULT_PUBLICATION_TTL_MS
     */
    [[deprecated("Will be removed by end of the year 2016. Use DEFAULT_PUBLICATION_TTL_MS "
                 "instead.")]] static const std::int64_t&
    DEFAULT_PUBLICATION_TTL();

    /**
     * @brief Returns the minimum publication time to live value in milliseconds:
     * 100
     */
    static const std::int64_t& MIN_PUBLICATION_TTL_MS();

    /**
     * @deprecated
     * @see SubscriptionQos#MIN_PUBLICATION_TTL_MS
     */
    [[deprecated("Will be removed by end of the year 2016. Use MIN_PUBLICATION_TTL_MS "
                 "instead.")]] static const std::int64_t&
    MIN_PUBLICATION_TTL();

    /**
     * @brief Returns the maximum publication time to live value in milliseconds:
     * 2 592 000 000 (30 days)
     */
    static const std::int64_t& MAX_PUBLICATION_TTL_MS();

    /**
     * @deprecated
     * @see SubscriptionQos#MAX_PUBLICATION_TTL_MS
     */
    [[deprecated("Will be removed by end of the year 2016. Use MAX_PUBLICATION_TTL_MS "
                 "instead.")]] static const std::int64_t&
    MAX_PUBLICATION_TTL();

    /**
     * @brief Returns the value for no expiry date time to live in milliseconds:
     * 0; used only internally
     */
    static const std::int64_t& NO_EXPIRY_DATE_TTL();

    /** @brief Returns the value for no expiry date in milliseconds: 0 */
    static const std::int64_t& NO_EXPIRY_DATE();

protected:
    /** @brief The expiry date in milliseconds */
    std::int64_t expiryDateMs;

    /** @brief The publication time to live in milliseconds */
    std::int64_t publicationTtlMs;
};

} // namespace joynr

#endif // SUBSCRIPTIONQOS_H

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
#ifndef SUBSCRIPTIONQOS_H
#define SUBSCRIPTIONQOS_H

#include <stdint.h>
#include <QDate>

#include "joynr/JoynrCommonExport.h"

namespace joynr
{

/**
 * @brief Base class representing the subscription quality of service settings
 *
 * Class that stores quality of service settings for subscriptions.
 * The subscription will automatically expire after the expiryDate is reached.
 */
class JOYNRCOMMON_EXPORT SubscriptionQos
{

public:
    /** @brief Default constructor */
    SubscriptionQos();

    /** @brief Copy constructor */
    SubscriptionQos(const SubscriptionQos& subscriptionQos);

    /**
     * @brief Constructor SubscriptionQos objects with specified validity
     * @param validity Time span in milliseconds during which publications will be sent
     */
    SubscriptionQos(const int64_t& validity);

    /** Destructor */
    virtual ~SubscriptionQos();

    /**
     * @brief Gets the expiry date
     *
     * The provider will send notifications until the expiry date is reached.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this date.
     *
     * @return The expiry date in milliseconds.
     * The publication will automatically expire at that EndDate.
     *
     * @see SubscriptionQos#setValidity_ms
     */
    int64_t getExpiryDate() const;

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
     * @see SubscriptionQos#setValidity_ms
     */
    virtual void setExpiryDate(const int64_t& expiryDate);

    /**
     * @brief Gets the publication time to live value
     *
     * Notification messages will be sent with this time-to-live.
     * If a notification message can not be delivered within its TTL,
     * it will be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an
     * expired TTL, it might raise a missed publication notification
     * (depending on the value of the alert interval QoS).
     *
     * @return Returns the TTL of the publication Messages in milliseconds.
     * -1 means, the message will be valid until the EndDate of the
     * Subscription.
     *
     * @see SubscriptionQos#setAlertInterval_ms
     * @see SubscriptionQos#setEndDate_ms
     */
    virtual int64_t getPublicationTtl() const;

    /**
     * @brief Sets the validity in milliseconds.
     *
     * The provider will send notifications for the next validity ms.
     * You will not receive any notifications (neither value notifications
     * nor missed publication notifications) after this time span expired.
     *
     * <p>This is equivalent to setting the expiry date QoS to NOW_ms + validity_ms.
     *
     * @param validity Time span in milliseconds during which publications will be sent
     *
     * @see SubscriptionQos#setEndDate_ms
     */
    virtual void setValidity(const int64_t& validity);

    /**
     * @brief Sets publication time to live in milliseconds
     *
     * Notification messages will be sent with this time-to-live. If a
     * notification message can not be delivered within its TTL, it will
     * be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an
     * expired TTL, it might raise a missed publication notification
     * (depending on the value of the alert interval QoS).
     *
     * @param publicationTtl_ms Sets the TTL of the publication Messages
     * -1 means, the message will be valid until the EndDate of the Subscription.
     *
     * @see SubscriptionQos#setAlertInterval_ms
     */
    virtual void setPublicationTtl(const int64_t& publicationTtl_ms);

    /** @brief Assignment operator */
    SubscriptionQos& operator=(const SubscriptionQos& subscriptionQos);

    /** @brief Equality operator */
    virtual bool operator==(const SubscriptionQos& subscriptionQos) const;

    /** @brief Gets the default publication time to live value */
    static const int64_t& DEFAULT_PUBLICATION_TTL();

    /** @brief Gets the minimum publication time to live value */
    static const int64_t& MIN_PUBLICATION_TTL();

    /** @brief Gets the maximum publication time to live value */
    static const int64_t& MAX_PUBLICATION_TTL();

    /** @brief Gets the value for no expiry date time to live; used only internally */
    static const int64_t& NO_EXPIRY_DATE_TTL();

    /** @brief Gets the value for no expiry date */
    static const int64_t& NO_EXPIRY_DATE();

protected:
    /** @brief The expiry date in milliseconds */
    int64_t expiryDate;

    /** @brief The publication time to live in milliseconds */
    int64_t publicationTtl;
};

} // namespace joynr

#endif // SUBSCRIPTIONQOS_H

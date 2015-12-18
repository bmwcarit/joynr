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
#ifndef PERIODICSUBSCRIPTIONQOS_H
#define PERIODICSUBSCRIPTIONQOS_H

#include "joynr/SubscriptionQos.h"

namespace joynr
{

/**
 * @brief Class representing the quality of service settings for subscriptions
 * based on time periods
 *
 * Class that stores quality of service settings for subscriptions to
 * <b>attributes</b>.
 *
 * Notifications will only be sent if a period has expired. The subscription
 * will automatically expire after the expiry date is reached. If no publications
 * were received for alertAfter interval, publicationMissed will be called.
 */
class JOYNRCOMMON_EXPORT PeriodicSubscriptionQos : public SubscriptionQos
{
public:
    /** @brief Default constructor */
    PeriodicSubscriptionQos();

    /**
     * @brief Copy constructor
     * @param other The PeriodicSubscriptionQos object to be copied from
     */
    PeriodicSubscriptionQos(const PeriodicSubscriptionQos& other);

    /**
     * @brief Constructor with full parameter set
     *
     * @param validity Time span in milliseconds during which publications will be sent
     * @param period interval in milliseconds.
     * The provider will send notifications every period in milliseconds
     * independently of value changes.
     * @param alertAfterInterval Time span in milliseconds after which publicationMissed
     * will be called if no publications were received.
     *
     * @see SubscriptionQos#setValidity
     * @see PeriodicSubscriptionQos#setPeriod
     * @see PeriodicSubscriptionQos#setAlertAfterInterval
     * @see SubscriptionQos#setPublicationTtl
     */
    PeriodicSubscriptionQos(const int64_t& validity,
                            const int64_t& period,
                            const int64_t& alertAfterInterval);

    /**
     * @brief Gets the period in milliseconds
     *
     * The provider will send notifications every period milliseconds,
     *
     * @return The period in milliseconds. The publisher will send a
     *            notification every period ms.
     */
    virtual int64_t getPeriod() const;

    /**
     * @brief Sets the period in milliseconds
     *
     * The provider will send notifications every period milliseconds.<br>
     * <br>
     * <b>Minimum and Maximum Values:</b>
     * <ul>
     * <li>The absolute <b>minimum</b> setting is 50 milliseconds.<br>
     * Any value less than this minimum will be treated at the absolute minimum
     * setting of 50 milliseconds.
     * <li>The absolute <b>maximum</b> setting is 2 592 000 000 milliseconds (30 days).<br>
     * Any value bigger than this maximum will be treated at the absolute maximum
     * setting of 2 592 000 000 milliseconds.
     * </ul>
     *
     * @param period
     *            The publisher will send a notification every period ms.
     */
    virtual void setPeriod(const int64_t& period);

    /**
     * @brief Gets the alertAfter interval in milliseconds
     *
     * If no notification was received within the last alertAfter interval,
     * a missed publication notification will be raised.
     *
     * @return alertAfterInterval (time span in milliseconds after which publicationMissed
     * will be called if no publications were received).
     */
    virtual int64_t getAlertAfterInterval() const;

    /**
     * @brief Sets the alertAfter interval in milliseconds
     *
     * If no notification was received within the last alertAfter interval, a missed publication
     * notification will be raised by the Subscription Manager.<br>
     * <br>
     * <b>Minimum, Maximum, and Default Values:</b>
     * <ul>
     * <li>The absolute <b>minimum</b> setting is the period value.<br>
     * Any value less than this minimum will be replaced by the period value.
     * <li>The absolute <b>maximum</b> setting is 2 592 000 000 milliseconds (30 days).<br>
     * Any value bigger than this maximum will be treated at the absolute maximum
     * setting of 2 592 000 000 milliseconds.
     * <li><b>Default</b> setting: 0 milliseconds (no alert)
     * </ul>
     *
     * @param alertAfterInterval Time span in milliseconds after which a publicationMissed
     * will be called if no publications were received.
     */
    virtual void setAlertAfterInterval(const int64_t& alertAfterInterval);

    /**
     * @brief Resets alert after interval
     *
     * Resets the alertAfterInterval and disables the alert by setting its value to
     * NO_ALERT_AFTER_INTERVAL.
     *
     * alertAfterInterval defines the time span in milliseconds after which a publicationMissed
     * will be called if no publications were received.
     */
    virtual void clearAlertAfterInterval();

    /** @brief Assignment operator */
    PeriodicSubscriptionQos& operator=(const PeriodicSubscriptionQos& other);

    /** @brief Equality operator */
    bool operator==(const PeriodicSubscriptionQos& other) const;

    /** @brief Returns the minimum value for the period in milliseconds: 50 */
    static const int64_t& MIN_PERIOD();

    /**
     * @brief Returns the maximum value for the period in milliseconds:
     * 2 592 000 000 (30 days)
     */
    static const int64_t& MAX_PERIOD();

    /**
     * @brief Returns the maximum value for the alertAfter interval in
     * milliseconds: 2 592 000 000 (30 days)
     */
    static const int64_t& MAX_ALERT_AFTER_INTERVAL();

    /**
     * @brief Returns the default value for the alertAfter interval in
     * milliseconds: 0 (NO_ALERT_AFTER_INTERVAL)
     */
    static const int64_t& DEFAULT_ALERT_AFTER_INTERVAL();

    /** @brief Returns the value for no alertAfter interval in milliseconds: 0 */
    static const int64_t& NO_ALERT_AFTER_INTERVAL();

protected:
    /**
     * @brief The period in milliseconds.
     *
     * The provider will send notifications every period milliseconds,
     */
    int64_t period;

    /**
     * @brief Time span in milliseconds after which a publicationMissed
     * will be called if no publications were received.
     */
    int64_t alertAfterInterval;
};

} // namespace joynr
#endif // PERIODICSUBSCRIPTIONQOS_H

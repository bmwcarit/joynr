/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

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
class JOYNR_EXPORT SubscriptionQos
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
     */
    explicit SubscriptionQos(std::int64_t validityMs);

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
    virtual void setExpiryDateMs(std::int64_t expiryDateMs);

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
    virtual void setValidityMs(std::int64_t validityMs);

    /** @brief Assignment operator */
    SubscriptionQos& operator=(const SubscriptionQos& subscriptionQos);

    /** @brief Equality operator */
    bool operator==(const SubscriptionQos& subscriptionQos) const;

    /** @brief Returns the value for no expiry date in milliseconds: 0 */
    static std::int64_t NO_EXPIRY_DATE();

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(_expiryDateMs));
    }

protected:
    /** @brief The expiry date in milliseconds */
    std::int64_t _expiryDateMs;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::SubscriptionQos, "joynr.SubscriptionQos")

#endif // SUBSCRIPTIONQOS_H

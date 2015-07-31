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
#ifndef ONCHANGESUBSCRIPTIONQOS_H
#define ONCHANGESUBSCRIPTIONQOS_H

#include <stdint.h>
#include "joynr/SubscriptionQos.h"
#include "joynr/JoynrCommonExport.h"

namespace joynr
{

/**
 * @brief Class representing the quality of service settings for subscriptions based
 * on changes
 *
 * Class that stores quality of service settings for subscriptions that will only
 * send a notification if the subscribed value has changed. The subscription will
 * automatically expire after validity ms. If no publications were received for
 * alertInterval, a publicationMissed will be called.
 * minInterval can be used to prevent too many messages being sent.
 */
class JOYNRCOMMON_EXPORT OnChangeSubscriptionQos : public SubscriptionQos
{

public:
    /** @brief Default constructor */
    OnChangeSubscriptionQos();

    /**
     * @brief Copy constructor for OnChangeSubscriptionQos object
     * @param other The object instance to be copied from
     */
    OnChangeSubscriptionQos(const OnChangeSubscriptionQos& other);

    /**
     * @brief Constructor with full parameter set.
     * @param validity Time span in milliseconds during which publications will be sent
     * @param minInterval Minimum interval in milliseconds.
     *
     * It is used to prevent flooding. Publications will be sent maintaining
     * this minimum interval provided, even if the value changes more often.
     * This prevents the consumer from being flooded by updated values.
     * The filtering happens on the provider's side, thus also preventing
     * excessive network traffic.
     */
    OnChangeSubscriptionQos(const int64_t& validity, const int64_t& minInterval);

    /**
     * @brief Gets the minimum interval in milliseconds
     *
     * The provider will maintain at least a minimum interval idle time in milliseconds between
     * successive notifications, even if on-change notifications are enabled and the value
     * changes more often. This prevents the consumer from being flooded by updated values.
     * The filtering happens on the provider's side, thus also preventing excessive network
     * traffic.
     *
     * @return Minimum interval in milliseconds
     */
    virtual int64_t getMinInterval() const;

    /**
     * @brief Sets minimum interval in milliseconds
     *
     * The provider will maintain at least a minimum interval idle time in milliseconds
     * between successive notifications, even if on-change notifications are enabled
     * and the value changes more often. This prevents the consumer from being flooded
     * by updated values. The filtering happens on the provider's side, thus also
     * preventing excessive network traffic.
     *
     * @param minInterval Minimum interval in milliseconds
     */
    virtual void setMinInterval(const int64_t& minInterval);

    /** @brief Assignment operator */
    OnChangeSubscriptionQos& operator=(const OnChangeSubscriptionQos& other);

    /** @brief Equality operator */
    virtual bool operator==(const OnChangeSubscriptionQos& other) const;

    /** @brief Returns the default value for the minimum interval setting */
    static const int64_t& DEFAULT_MIN_INTERVAL();

    /** @brief Returns the minimum value for the minimum interval setting */
    static const int64_t& MIN_MIN_INTERVAL();

    /** @brief Returns the maximum value for the minimum interval setting */
    static const int64_t& MAX_MIN_INTERVAL();

protected:
    /**
     * @brief The minimum interval in milliseconds
     *
     * It is used to prevent flooding. Publications will be sent maintaining
     * this minimum interval provided, even if the value changes more often.
     * This prevents the consumer from being flooded by updated values.
     * The filtering happens on the provider's side, thus also preventing
     * excessive network traffic.
     */
    int64_t minInterval;
};

} // namespace joynr

#endif // ONCHANGESUBSCRIPTIONQOS_H

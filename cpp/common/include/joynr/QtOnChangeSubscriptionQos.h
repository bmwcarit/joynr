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
#ifndef QTONCHANGESUBSCRIPTIONQOS_H
#define QTONCHANGESUBSCRIPTIONQOS_H

#include "joynr/QtSubscriptionQos.h"
#include "joynr/JoynrCommonExport.h"

/*
*  A subscription that will only send a notification if the subscribed value has changed
* minInterval_ms can be used to prevent too many message being sent.
* The subscription will automatically expire after invalidty_ms
* If no publications is received for alertInterval a publicationMissed will be called.
*/

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
class JOYNRCOMMON_EXPORT QtOnChangeSubscriptionQos : public QtSubscriptionQos
{

    Q_OBJECT

    Q_PROPERTY(qint64 minInterval READ getMinInterval WRITE setMinInterval)

public:
    /** @brief Default constructor */
    QtOnChangeSubscriptionQos();

    /**
     * @brief Copy constructor for OnChangeSubscriptionQos object
     * @param other The object instance to be copied from
     */
    QtOnChangeSubscriptionQos(const QtOnChangeSubscriptionQos& other);

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
    QtOnChangeSubscriptionQos(const qint64& validity, const qint64& minInterval);

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
    virtual qint64 getMinInterval() const;

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
    virtual void setMinInterval(const qint64& minInterval);

    /** @brief Assignment operator */
    QtOnChangeSubscriptionQos& operator=(const QtOnChangeSubscriptionQos& other);

    /** @brief Equality operator */
    virtual bool operator==(const QtOnChangeSubscriptionQos& other) const;

    /** @brief Returns the default value for the minimum interval setting */
    static const qint64& DEFAULT_MIN_INTERVAL();

    /** @brief Returns the minimum value for the minimum interval setting */
    static const qint64& MIN_MIN_INTERVAL();

    /** @brief Returns the maximum value for the minimum interval setting */
    static const qint64& MAX_MIN_INTERVAL();

    /** @brief equality operator */
    virtual bool equals(const QObject& other) const;

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
    qint64 minInterval;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::QtOnChangeSubscriptionQos)

#endif // QTONCHANGESUBSCRIPTIONQOS_H

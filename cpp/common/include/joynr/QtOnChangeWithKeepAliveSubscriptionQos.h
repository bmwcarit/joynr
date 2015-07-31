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
#ifndef QTONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H
#define QTONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H

#include "joynr/QtOnChangeSubscriptionQos.h"

namespace joynr
{

/**
 * @brief Class representing the quality of service settings for subscriptions
 * based on changes and time periods
 *
 * Class that stores quality of service settings for subscriptions that will only
 * send a notification if the subscribed value has changed or an interval without
 * notifications has expired. The subscription will automatically expire after validity ms.
 * If no publications were received for alertAfter Interval, a publicationMissed will be
 * called. minInterval can be used to prevent too many messages being sent.
 */
class JOYNRCOMMON_EXPORT QtOnChangeWithKeepAliveSubscriptionQos : public QtOnChangeSubscriptionQos
{

    Q_OBJECT

    Q_PROPERTY(qint64 maxInterval READ getMaxInterval WRITE setMaxInterval)
    Q_PROPERTY(qint64 alertAfterInterval READ getAlertAfterInterval WRITE setAlertAfterInterval)

public:
    /** @brief Default constructor */
    QtOnChangeWithKeepAliveSubscriptionQos();

    /**
     * @brief Copy constructor
     * @param other Object to copy from
     */
    QtOnChangeWithKeepAliveSubscriptionQos(const QtOnChangeWithKeepAliveSubscriptionQos& other);

    /**
     * @brief Constructor with full parameter set
     *
     * @param validity Time span in milliseconds during which publications will be sent
     * @param minInterval Minimum interval in milliseconds.
     * It is used to prevent flooding. Publications will be sent maintaining
     * this minimum interval provided, even if the value changes more often.
     * This prevents the consumer from being flooded by updated values.
     * The filtering happens on the provider's side, thus also preventing
     * excessive network traffic.
     * @param maxInterval Maximum interval in milliseconds.
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change.
     * @param alertAfterInterval Time span in milliseconds after which a publicationMissed
     * will be called if no publications were received.
     */
    QtOnChangeWithKeepAliveSubscriptionQos(const qint64& validity,
                                           const qint64& minInterval,
                                           const qint64& maxInterval,
                                           const qint64& alertAfterInterval);

    /**
     * @brief Sets minimum interval in milliseconds
     *
     * The provider will maintain at least a minimum interval idle time in milliseconds between
     * successive notifications, even if on-change notifications are enabled and the value changes
     * more often. This prevents the consumer from being flooded by updated values. The filtering
     * happens on the provider's side, thus also preventing excessive network traffic.
     *
     * @param minInterval Minimum interval in milliseconds
     */
    virtual void setMinInterval(const qint64& minInterval);

    /**
     * @brief Gets the maximum interval in milliseconds
     *
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change. It will send notifications more often if
     * on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus be
     * seen as a sort of heart beat.
     *
     * @return maxInterval
     *            The publisher will send a notification at least every maxInterval ms.
     */
    virtual qint64 getMaxInterval() const;

    /**
     * @brief Sets maximum interval in milliseconds
     *
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change. It will send notifications more often if
     * on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus be
     * seen as a sort of heart beat.
     *
     * @param period
     *            The publisher will send a notification at least every maxInterval_ms.
     */
    virtual void setMaxInterval(const qint64& period);

    /**
     * @brief Gets the alertAfter interval in milliseconds
     *
     * If no notification was received within the last alertAfter interval, a missed publication
     * notification will be raised by the Subscription Manager.
     *
     * @return alertAfterInterval (time span in milliseconds after which a publicationMissed
     * will be called if no publications were received)
     */
    virtual qint64 getAlertAfterInterval() const;

    /**
     * @brief Sets the alertAfter interval in milliseconds
     *
     * If no notification was received within the last alertAfter interval, a missed publication
     * notification will be raised by the Subscription Manager.
     *
     * @param alertAfterInterval Time span in milliseconds after which a publicationMissed
     * will be called if no publications were received.
     */
    virtual void setAlertAfterInterval(const qint64& alertAfterInterval);

    /** @brief Assignment operator */
    QtOnChangeWithKeepAliveSubscriptionQos& operator=(
            const QtOnChangeWithKeepAliveSubscriptionQos& other);

    /** @brief Equality operator */
    virtual bool operator==(const QtOnChangeWithKeepAliveSubscriptionQos& other) const;

    /** @brief Gets the maximum value for the maximum interval */
    static const qint64& MAX_MAX_INTERVAL();

    /** @brief Gets the maximum value for the alertAfter interval */
    static const qint64& MAX_ALERT_AFTER_INTERVAL();

    /** @brief Gets the default value for the alertAfter interval */
    static const qint64& DEFAULT_ALERT_AFTER_INTERVAL();

    /** @brief Gets the value for no alertAfter interval */
    static const qint64& NO_ALERT_AFTER_INTERVAL();

    /** @brief equality operator */
    virtual bool equals(const QObject& other) const;

protected:
    /**
     * @brief The maximum interval in milliseconds.
     *
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change.
     */
    qint64 maxInterval;

    /**
     * @brief time span in milliseconds after which a publicationMissed
     * will be called if no publications were received
     */
    qint64 alertAfterInterval;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::QtOnChangeWithKeepAliveSubscriptionQos)
Q_DECLARE_METATYPE(QSharedPointer<joynr::QtOnChangeWithKeepAliveSubscriptionQos>)

#endif // QTONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H

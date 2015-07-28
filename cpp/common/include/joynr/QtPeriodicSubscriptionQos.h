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
#ifndef PERIODICSUBSCRIPTIONQOS_H
#define PERIODICSUBSCRIPTIONQOS_H

#include "joynr/JoynrCommonExport.h"
#include "joynr/QtSubscriptionQos.h"

namespace joynr
{

class JOYNRCOMMON_EXPORT QtPeriodicSubscriptionQos : public QtSubscriptionQos
{

    Q_OBJECT

    Q_PROPERTY(qint64 period READ getPeriod WRITE setPeriod)
    Q_PROPERTY(qint64 alertAfterInterval READ getAlertAfterInterval WRITE setAlertAfterInterval)

public:
    QtPeriodicSubscriptionQos();
    QtPeriodicSubscriptionQos(const QtPeriodicSubscriptionQos& other);
    QtPeriodicSubscriptionQos(const qint64& validity,
                              const qint64& period,
                              const qint64& alertAfterInterval);

    /**
    * The provider will send notifications every maximum interval in milliseconds, even if the value
    *didn't
    * change. It will send notifications more often if on-change notifications are enabled,
    * the value changes more often, and the minimum interval QoS does not prevent it. The maximum
    *interval
    * can thus be seen as a sort of heart beat.
    *
    * @return qint64 period
    *            The publisher will send a notification at least every maxInterval_ms.
    */
    virtual qint64 getPeriod() const;

    /**
     * The provider will send notifications every maximum interval in milliseconds, even if the
     *value didn't
     * change. It will send notifications more often if on-change notifications are enabled,
     * the value changes more often, and the minimum interval QoS does not prevent it. The maximum
     *interval
     * can thus be seen as a sort of heart beat.
     *
     * @param period
     *            The publisher will send a notification at least every maxInterval_ms.
     */
    virtual void setPeriod(const qint64& period);

    /**
     * If no notification was received within the last alert interval, a missed publication
     * notification will be raised.
     *
     * @return alertInterval_ms
     *            If more than alertInterval_ms pass without receiving a message,
     *subscriptionManager will issue a
     *            publicationMissed.
     */
    virtual qint64 getAlertAfterInterval() const;

    /**
     * If no notification was received within the last alert interval, a missed publication
     * notification will be raised.
     *
     * @param alertInterval_ms
     *            If more than alertInterval pass without receiving a message, subscriptionManager
     *will issue a
     *            publicationMissed..
     */
    virtual void setAlertAfterInterval(const qint64& alertAfterInterval);

    /**
     * Resets the alertAfterInterval and disables the alert by setting its value to
     * NO_ALERT_AFTER_INTERVAL.
     */
    virtual void clearAlertAfterInterval();

    QtPeriodicSubscriptionQos& operator=(const QtPeriodicSubscriptionQos& other);
    virtual bool operator==(const QtPeriodicSubscriptionQos& other) const;

    static const qint64& MIN_PERIOD();
    static const qint64& MAX_PERIOD();

    static const qint64& MAX_ALERT_AFTER_INTERVAL();
    static const qint64& DEFAULT_ALERT_AFTER_INTERVAL();
    static const qint64& NO_ALERT_AFTER_INTERVAL();

    virtual bool equals(const QObject& other) const;

protected:
    qint64 period;
    qint64 alertAfterInterval;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::QtPeriodicSubscriptionQos)

#endif // PERIODICSUBSCRIPTIONQOS_H

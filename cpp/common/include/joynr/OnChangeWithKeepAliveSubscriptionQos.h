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
#ifndef ONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H
#define ONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H

#include "joynr/OnChangeSubscriptionQos.h"

namespace joynr {


class JOYNRCOMMON_EXPORT OnChangeWithKeepAliveSubscriptionQos : public OnChangeSubscriptionQos {

    Q_OBJECT

    Q_PROPERTY(qint64 maxInterval READ getMaxInterval WRITE setMaxInterval)
    Q_PROPERTY(qint64 alertAfterInterval READ getAlertAfterInterval  WRITE setAlertAfterInterval)

public:

    OnChangeWithKeepAliveSubscriptionQos();
    OnChangeWithKeepAliveSubscriptionQos(const OnChangeWithKeepAliveSubscriptionQos& other);
    OnChangeWithKeepAliveSubscriptionQos(const qint64& validity, const qint64& minIntervals, const qint64& maxInterval, const qint64& alertAfterInterval);

    /**
     * The provider will maintain at least a minimum interval idle time in milliseconds between
     * successive notifications, even if on-change notifications are enabled and the value changes more
     * often. This prevents the consumer from being flooded by updated values. The filtering happens on
     * the provider's side, thus also preventing excessive network traffic.
     *
     * @param minInterval
     *            The publisher will keep a minimum idle time of minInterval between two successive notifications.
     */
    virtual void setMinInterval(const qint64& minInterval);

    /**
    * The provider will send notifications every maximum interval in milliseconds, even if the value didn't
    * change. It will send notifications more often if on-change notifications are enabled,
    * the value changes more often, and the minimum interval QoS does not prevent it. The maximum interval
    * can thus be seen as a sort of heart beat.
    *
    * @return qint64 maxInterval
    *            The publisher will send a notification at least every maxInterval_ms.
    */
    virtual qint64 getMaxInterval() const;

    /**
     * The provider will send notifications every maximum interval in milliseconds, even if the value didn't
     * change. It will send notifications more often if on-change notifications are enabled,
     * the value changes more often, and the minimum interval QoS does not prevent it. The maximum interval
     * can thus be seen as a sort of heart beat.
     *
     * @param maxInterval
     *            The publisher will send a notification at least every maxInterval_ms.
     */
    virtual void setMaxInterval(const qint64& period);

    /**
     * If no notification was received within the last alert interval, a missed publication
     * notification will be raised.
     *
     * @return alertInterval_ms
     *            If more than alertInterval_ms pass without receiving a message, subscriptionManager will issue a
     *            publicationMissed.
     */
    virtual qint64 getAlertAfterInterval() const;

    /**
     * If no notification was received within the last alert interval, a missed publication
     * notification will be raised.
     *
     * @param alertInterval_ms
     *            If more than alertInterval pass without receiving a message, subscriptionManager will issue a
     *            publicationMissed..
     */
    virtual void setAlertAfterInterval(const qint64& alertAfterInterval);

    OnChangeWithKeepAliveSubscriptionQos& operator=(const OnChangeWithKeepAliveSubscriptionQos& other);
    virtual bool operator==(const OnChangeWithKeepAliveSubscriptionQos& other) const;

    static const qint64& MAX_MAX_INTERVAL();

    static const qint64& MAX_ALERT_AFTER_INTERVAL();
    static const qint64& DEFAULT_ALERT_AFTER_INTERVAL();
    static const qint64& NO_ALERT_AFTER_INTERVAL();

    virtual bool equals(const QObject& other) const;

protected:
    qint64 maxInterval;
    qint64 alertAfterInterval;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::OnChangeWithKeepAliveSubscriptionQos)
Q_DECLARE_METATYPE(QSharedPointer<joynr::OnChangeWithKeepAliveSubscriptionQos>)

#endif // ONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H

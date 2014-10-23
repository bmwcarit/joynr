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

#include <QObject>
#include <QDate>

#include "joynr/JoynrCommonExport.h"

namespace joynr
{

class JOYNRCOMMON_EXPORT SubscriptionQos : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qint64 expiryDate READ getExpiryDate WRITE setExpiryDate)
    Q_PROPERTY(qint64 publicationTtl READ getPublicationTtl WRITE setPublicationTtl)

public:
    SubscriptionQos();
    SubscriptionQos(const SubscriptionQos& subscriptionQos);
    SubscriptionQos(const qint64& validity);

    virtual ~SubscriptionQos();

    /**
     * The provider will send notifications until the end date is reached. You will not receive any
     * notifications (neither value notifications nor missed publication notifications) after
     * this date.
     *
     * @return endDate_ms
     * 		The publication will automatically expire at that EndDate.
     *
     * @see SubscriptionQos#setValidity_ms
     */
    qint64 getExpiryDate() const;

    /**
     * Clears the current expiry date and disables it, by setting the value to NO_EXPIRY_DATE.
     */
    virtual void clearExpiryDate();

    /**
     * The provider will send notifications until the the end date is reached. You will not receive
     *any
     * notifications (neither value notifications nor missed publication notifications) after
     * this date.
     *
     * @param endDate_ms
     * 		The publication will automatically expire at that date.
     *
     * @see SubscriptionQos#setValidity_ms
     */
    virtual void setExpiryDate(const qint64& expiryDate);

    /**
     * Notification messages will be sent with this time-to-live. If a notification message can not
     *be
     * delivered within its TTL, it will be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an expired TTL, it might raise a
     *       missed publication notification (depending on the value of the alert interval QoS).
     *
     * @return publicationTtl
     *            Returns the TTL of the publication Messages -1 means, the message will be valid
     *until the EndDate of the
     *            Subscription.
     *
     * @see SubscriptionQos#setAlertInterval_ms
     * @see SubscriptionQos#setEndDate_ms
     */
    virtual qint64 getPublicationTtl() const;

    /**
     * The provider will send notifications for the next validity ms. You will not receive any
     * notifications (neither value notifications nor missed publication notifications) after
     * this time span expired.
     *
     * <p>This is equivalent to setting the end date QoS to NOW_ms + validity_ms.
     *
     * @param validity
     *            The publication will automatically expire in validty_ms milliseconds
     *
     * @see SubscriptionQos#setEndDate_ms
     */
    virtual void setValidity(const qint64& validity);

    /**
     * Notification messages will be sent with this time-to-live. If a notification message can not
     *be
     * delivered within its TTL, it will be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an expired TTL, it might raise a
     *       missed publication notification (depending on the value of the alert interval QoS).
     *
     * @param publicationTtl
     *            Sets the TTL of the publication Messages -1 means, the message will be valid until
     *the EndDate of the
     *            Subscription.
     *
     * @see SubscriptionQos#setAlertInterval_ms
     */
    virtual void setPublicationTtl(const qint64& publicationTtl_ms);

    SubscriptionQos& operator=(const SubscriptionQos& subscriptionQos);
    virtual bool operator==(const SubscriptionQos& subscriptionQos) const;

    static const qint64& DEFAULT_PUBLICATION_TTL();
    static const qint64& MIN_PUBLICATION_TTL();
    static const qint64& MAX_PUBLICATION_TTL();
    static const qint64& NO_EXPIRY_DATE_TTL();

    static const qint64& NO_EXPIRY_DATE();

    virtual bool equals(const QObject& other) const;

protected:
    qint64 expiryDate;
    qint64 publicationTtl;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::SubscriptionQos)
Q_DECLARE_METATYPE(QSharedPointer<joynr::SubscriptionQos>)

#endif // SUBSCRIPTIONQOS_H

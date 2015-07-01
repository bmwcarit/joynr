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
#include "joynr/SubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/TypeUtil.h"

namespace joynr
{
class QtPeriodicSubscriptionQos;
class QtOnChangeSubscriptionQos;
class QtOnChangeWithKeepAliveSubscriptionQos;

/**
 * @brief Base class representing the subscription quality of service settings
 *
 * Class that stores quality of service settings for subscriptions.
 * The subscription will automatically expire after the expiryDate is reached.
 */
class JOYNRCOMMON_EXPORT QtSubscriptionQos : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qint64 expiryDate READ getExpiryDate WRITE setExpiryDate)
    Q_PROPERTY(qint64 publicationTtl READ getPublicationTtl WRITE setPublicationTtl)

public:
    /** @brief Default constructor */
    QtSubscriptionQos();

    /** @brief Copy constructor */
    QtSubscriptionQos(const QtSubscriptionQos& subscriptionQos);

    /**
     * @brief Constructor SubscriptionQos objects with specified validity
     * @param validity Time span in milliseconds during which publications will be sent
     */
    QtSubscriptionQos(const qint64& validity);

    /** Destructor */
    virtual ~QtSubscriptionQos();

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
     * @see QtSubscriptionQos#setValidity_ms
     */
    qint64 getExpiryDate() const;

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
     * @see QtSubscriptionQos#setValidity_ms
     */
    virtual void setExpiryDate(const qint64& expiryDate);

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
     * @see QtSubscriptionQos#setAlertInterval_ms
     * @see QtSubscriptionQos#setEndDate_ms
     */
    virtual qint64 getPublicationTtl() const;

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
     * @see QtSubscriptionQos#setEndDate_ms
     */
    virtual void setValidity(const qint64& validity);

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
     * @see QtSubscriptionQos#setAlertInterval_ms
     */
    virtual void setPublicationTtl(const qint64& publicationTtl_ms);

    /** @brief Assignment operator */
    QtSubscriptionQos& operator=(const QtSubscriptionQos& subscriptionQos);

    /** @brief Equality operator */
    virtual bool operator==(const QtSubscriptionQos& subscriptionQos) const;

    /** @brief Gets the default publication time to live value */
    static const qint64& DEFAULT_PUBLICATION_TTL();

    /** @brief Gets the minimum publication time to live value */
    static const qint64& MIN_PUBLICATION_TTL();

    /** @brief Gets the maximum publication time to live value */
    static const qint64& MAX_PUBLICATION_TTL();

    /** @brief Gets the value for no expiry date time to live; used only internally */
    static const qint64& NO_EXPIRY_DATE_TTL();

    /** @brief Gets the value for no expiry date */
    static const qint64& NO_EXPIRY_DATE();

    /** @brief equality operator */
    virtual bool equals(const QObject& other) const;

    /**
     * @brief convert standard C++ type value to QT specific type value
     * @param from variable of std type joynr::types::SubcriptionQos whose contents should be
     * converted
     * @return converted value of QT specific type QtSubscriptionQos
     */
    static QtSubscriptionQos* createQt(const SubscriptionQos& from);

    /**
     * @brief convert standard C++ type value to QT specific type value
     * @param from variable of std type joynr::types::OnChangeSubscriptionQos whose contents should
     * be
     * converted
     * @return converted value of QT specific type QtOnChangeSubscriptionQos
     */
    static QtOnChangeSubscriptionQos* createQt(const OnChangeSubscriptionQos& from);

protected:
    /** @brief The expiry date in milliseconds */
    qint64 expiryDate;

    /** @brief The publication time to live in milliseconds */
    qint64 publicationTtl;

private:
    static void createQtInternal(const SubscriptionQos& from, QtSubscriptionQos& to);
    static void createQtInternal(const OnChangeSubscriptionQos& from,
                                 QtOnChangeSubscriptionQos& to);
    static void createQtInternal(const OnChangeWithKeepAliveSubscriptionQos& from,
                                 QtOnChangeWithKeepAliveSubscriptionQos& to);
    static void createQtInternal(const PeriodicSubscriptionQos& from,
                                 QtPeriodicSubscriptionQos& to);
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::QtSubscriptionQos)
Q_DECLARE_METATYPE(QSharedPointer<joynr::QtSubscriptionQos>)

#endif // SUBSCRIPTIONQOS_H

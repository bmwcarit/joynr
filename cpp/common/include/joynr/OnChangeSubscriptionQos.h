/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

#include "joynr/SubscriptionQos.h"
#include "joynr/JoynrCommonExport.h"

/*
*  A subscription that will only send a notification if the subscribed value has changed
* minInterval_ms can be used to prevent too many message being sent.
* The subscription will automatically expire after invalidty_ms
* If no publications is received for alertInterval a publicationMissed will be called.
*/

namespace joynr {

class JOYNRCOMMON_EXPORT OnChangeSubscriptionQos : public SubscriptionQos {

    Q_OBJECT

    Q_PROPERTY(qint64 minInterval READ getMinInterval WRITE setMinInterval)

public:

    OnChangeSubscriptionQos();
    OnChangeSubscriptionQos(const OnChangeSubscriptionQos& other);
    OnChangeSubscriptionQos(const qint64& validity, const qint64& minInterval);

    /**
    * The provider will maintain at least a minimum interval idle time in milliseconds between
    * successive notifications, even if on-change notifications are enabled and the value changes more
    * often. This prevents the consumer from being flooded by updated values. The filtering happens on
    * the provider's side, thus also preventing excessive network traffic.
    *
    * @return qint64 minInterval
    *            The publisher will keep a minimum idle time of minInterval between two successive notifications.
    */
    virtual qint64 getMinInterval() const;

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

    OnChangeSubscriptionQos& operator=(const OnChangeSubscriptionQos& other);
    virtual bool operator==(const OnChangeSubscriptionQos& other) const;

    static const qint64& DEFAULT_MIN_INTERVAL();
    static const qint64& MIN_MIN_INTERVAL();
    static const qint64& MAX_MIN_INTERVAL();

    virtual bool equals(const QObject& other) const;

protected:
    qint64 minInterval;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::OnChangeSubscriptionQos)
Q_DECLARE_METATYPE(QSharedPointer<joynr::OnChangeSubscriptionQos>)

#endif // ONCHANGESUBSCRIPTIONQOS_H

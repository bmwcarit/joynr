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
#ifndef STDONCHANGESUBSCRIPTIONQOS_H
#define STDONCHANGESUBSCRIPTIONQOS_H

#include <stdint.h>
#include "joynr/SubscriptionQos.h"
#include "joynr/JoynrCommonExport.h"

/*
*  A subscription that will only send a notification if the subscribed value has changed
* minInterval_ms can be used to prevent too many message being sent.
* The subscription will automatically expire after invalidty_ms
* If no publications is received for alertInterval a publicationMissed will be called.
*/

namespace joynr
{

class JOYNRCOMMON_EXPORT OnChangeSubscriptionQos : public SubscriptionQos
{

public:
    OnChangeSubscriptionQos();
    OnChangeSubscriptionQos(const OnChangeSubscriptionQos& other);
    OnChangeSubscriptionQos(const int64_t& validity, const int64_t& minInterval);

    /**
    * The provider will maintain at least a minimum interval idle time in milliseconds between
    * successive notifications, even if on-change notifications are enabled and the value changes
    *more
    * often. This prevents the consumer from being flooded by updated values. The filtering happens
    *on
    * the provider's side, thus also preventing excessive network traffic.
    *
    * @return int64_t minInterval
    *            The publisher will keep a minimum idle time of minInterval between two successive
    *notifications.
    */
    virtual int64_t getMinInterval() const;

    /**
     * The provider will maintain at least a minimum interval idle time in milliseconds between
     * successive notifications, even if on-change notifications are enabled and the value changes
     *more
     * often. This prevents the consumer from being flooded by updated values. The filtering happens
     *on
     * the provider's side, thus also preventing excessive network traffic.
     *
     * @param minInterval
     *            The publisher will keep a minimum idle time of minInterval between two successive
     *notifications.
     */
    virtual void setMinInterval(const int64_t& minInterval);

    OnChangeSubscriptionQos& operator=(const OnChangeSubscriptionQos& other);
    virtual bool operator==(const OnChangeSubscriptionQos& other) const;

    static const int64_t& DEFAULT_MIN_INTERVAL();
    static const int64_t& MIN_MIN_INTERVAL();
    static const int64_t& MAX_MIN_INTERVAL();

protected:
    int64_t minInterval;
};

} // namespace joynr

#endif // STDONCHANGESUBSCRIPTIONQOS_H

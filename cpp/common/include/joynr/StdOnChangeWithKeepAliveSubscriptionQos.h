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
#ifndef STDONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H
#define STDONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H

#include "joynr/StdOnChangeSubscriptionQos.h"
#include <stdint.h>

namespace joynr
{

class JOYNRCOMMON_EXPORT StdOnChangeWithKeepAliveSubscriptionQos : public StdOnChangeSubscriptionQos
{
public:
    StdOnChangeWithKeepAliveSubscriptionQos();
    StdOnChangeWithKeepAliveSubscriptionQos(const StdOnChangeWithKeepAliveSubscriptionQos& other);
    StdOnChangeWithKeepAliveSubscriptionQos(const int64_t& validity,
                                            const int64_t& minInterval,
                                            const int64_t& maxInterval,
                                            const int64_t& alertAfterInterval);

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

    /**
    * The provider will send notifications every maximum interval in milliseconds, even if the value
    *didn't
    * change. It will send notifications more often if on-change notifications are enabled,
    * the value changes more often, and the minimum interval QoS does not prevent it. The maximum
    *interval
    * can thus be seen as a sort of heart beat.
    *
    * @return int64_t maxInterval
    *            The publisher will send a notification at least every maxInterval_ms.
    */
    virtual int64_t getMaxInterval() const;

    /**
     * The provider will send notifications every maximum interval in milliseconds, even if the
     *value didn't
     * change. It will send notifications more often if on-change notifications are enabled,
     * the value changes more often, and the minimum interval QoS does not prevent it. The maximum
     *interval
     * can thus be seen as a sort of heart beat.
     *
     * @param maxInterval
     *            The publisher will send a notification at least every maxInterval_ms.
     */
    virtual void setMaxInterval(const int64_t& period);

    /**
     * If no notification was received within the last alert interval, a missed publication
     * notification will be raised.
     *
     * @return alertInterval_ms
     *            If more than alertInterval_ms pass without receiving a message,
     *subscriptionManager will issue a
     *            publicationMissed.
     */
    virtual int64_t getAlertAfterInterval() const;

    /**
     * If no notification was received within the last alert interval, a missed publication
     * notification will be raised.
     *
     * @param alertInterval_ms
     *            If more than alertInterval pass without receiving a message, subscriptionManager
     *will issue a
     *            publicationMissed..
     */
    virtual void setAlertAfterInterval(const int64_t& alertAfterInterval);

    StdOnChangeWithKeepAliveSubscriptionQos& operator=(
            const StdOnChangeWithKeepAliveSubscriptionQos& other);
    virtual bool operator==(const StdOnChangeWithKeepAliveSubscriptionQos& other) const;

    static const int64_t& MAX_MAX_INTERVAL();

    static const int64_t& MAX_ALERT_AFTER_INTERVAL();
    static const int64_t& DEFAULT_ALERT_AFTER_INTERVAL();
    static const int64_t& NO_ALERT_AFTER_INTERVAL();

protected:
    int64_t maxInterval;
    int64_t alertAfterInterval;
};

} // namespace joynr

#endif // STDONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H

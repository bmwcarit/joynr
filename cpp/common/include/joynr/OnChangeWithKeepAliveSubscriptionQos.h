/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#include <cstdint>
#include "joynr/JoynrCommonExport.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Logger.h"

namespace joynr
{

/**
 * @brief Class representing the quality of service settings for subscriptions
 * based on changes and time periods
 *
 * Class that stores quality of service settings for subscriptions to
 * <b>attributes</b>. Using it for subscriptions to broadcasts is theoretically
 * possible because of inheritance but makes no sense (in this case the
 * additional members will be ignored).
 *
 * Notifications will be sent if the subscribed value has changed or a time
 * interval without notifications has expired. The subscription will
 * automatically expire after validity ms. If no publications were received for
 * alertAfter Interval, publicationMissed will be called.
 * minInterval can be used to prevent too many messages being sent.
 */
class JOYNRCOMMON_EXPORT OnChangeWithKeepAliveSubscriptionQos : public OnChangeSubscriptionQos
{
public:
    /** @brief Default constructor */
    OnChangeWithKeepAliveSubscriptionQos();

    /**
     * @brief Copy constructor
     * @param other The OnChangeWithKeepAliveSubscriptionQos object to copy from
     */
    OnChangeWithKeepAliveSubscriptionQos(const OnChangeWithKeepAliveSubscriptionQos& other);

    /**
     * @brief Constructor with full parameter set
     *
     * @param validityMs Time span in milliseconds during which publications will be sent
     * @param minIntervalMs Minimum interval in milliseconds.
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
     *
     * @see SubscriptionQos#setValidityMs
     * @see OnChangeSubscriptionQos#setMinIntervalMs
     * @see OnChangeWithKeepAliveSubscriptionQos#setMaxIntervalMs
     * @see OnChangeWithKeepAliveSubscriptionQos#setAlertAfterInterval
     * @see SubscriptionQos#setPublicationTtlMs
     */
    OnChangeWithKeepAliveSubscriptionQos(const std::int64_t& validityMs,
                                         const std::int64_t& minIntervalMs,
                                         const std::int64_t& maxIntervalMs,
                                         const std::int64_t& alertAfterInterval);

    /**
     * @brief Sets minimum interval in milliseconds
     *
     * Calls the setter in OnChangeSubscriptionQos and updates maxInterval if
     * necessary (maxInterval must not be less than minInterval).
     *
     * @param minInterval Minimum interval in milliseconds
     *
     * @see OnChangeSubscriptionQos#setMinIntervalMs
     * @see OnChangeWithKeepAliveSubscriptionQos#setMaxIntervalMs
     */
    void setMinIntervalMs(const std::int64_t& minIntervalMs) override;

    /**
     * @deprecated
     * @see OnChangeWithKeepAliveSubscriptionQos#setMinIntervalMs
     */
    [[deprecated(
            "Will be removed by end of the year 2016. Use setMinIntervalMs instead.")]] virtual void
    setMinInterval(const std::int64_t& minIntervalMs) override;

    /**
     * @brief Gets the maximum interval in milliseconds
     *
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change. It will send notifications more often if
     * on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus be
     * seen as a sort of heart beat.
     *
     * @return maxIntervalMs
     *            The publisher will send a notification at least every maxInterval ms.
     */
    virtual std::int64_t getMaxIntervalMs() const;

    /**
     * @deprecated
     * @see OnChangeWithKeepAliveSubscriptionQos#getMaxIntervalMs
     */
    [[deprecated("Will be removed by end of the year 2016. Use getMaxIntervalMs "
                 "instead.")]] virtual std::int64_t
    getMaxInterval() const;

    /**
     * @brief Sets maximum interval in milliseconds
     *
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change. It will send notifications more often if
     * on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus be
     * seen as a sort of heart beat.<br>
     * <br>
     * <b>Minimum and Maximum Values:</b>
     * <ul>
     * <li><b>Minimum</b> maxInterval: the minInterval value. Smaller values will
     * be rounded up.
     * <li><b>Maximum</b> minInterval: 2.592.000.000 (30 days). Larger values
     * will be rounded down.
     * </ul>
     *
     * @param maxIntervalMs
     *            The publisher will send a notification at least every maxInterval_ms.
     */
    virtual void setMaxIntervalMs(const std::int64_t& maxIntervalMs);

    /**
     * @deprecated
     * @see OnChangeWithKeepAliveSubscriptionQos#setMaxIntervalMs
     */
    [[deprecated("Will be removed by end of the year 2016. Use setMaxIntervalMs "
                 "instead.")]] virtual void
    setMaxInterval(const std::int64_t& maxIntervalMs);

    /**
     * @brief Gets the alertAfter interval in milliseconds
     *
     * If no notification was received within the last alertAfter interval, a
     * missed publication notification will be raised by the Subscription Manager.
     *
     * @return alertAfterInterval (time span in milliseconds after which a
     * publicationMissed will be called if no publications were received)
     */
    virtual std::int64_t getAlertAfterInterval() const;

    /**
     * @brief Sets the alertAfter interval in milliseconds
     *
     * If no notification was received within the last alertAfter interval, a
     * missed publication notification will be raised by the Subscription
     * Manager.<br>
     * <br>
     * <b>Minimum and Maximum Values:</b>
     * <ul>
     * <li><b>Minimum</b> alertAfterInterval: the maxInterval value or 0
     * (NO_ALERT_AFTER_INTERVAL). Smaller values, except NO_ALERT_AFTER_INTERVAL,
     * will be rounded up.
     * <li><b>Maximum</b> alertAfterInterval: 2.592.000.000 (30 days). Larger
     * values will be rounded down.
     * </ul>
     *
     * @param alertAfterInterval Time span in milliseconds after which a
     * publicationMissed will be called if no publications were received.
     */
    virtual void setAlertAfterInterval(const std::int64_t& alertAfterInterval);

    /** @brief Assignment operator */
    OnChangeWithKeepAliveSubscriptionQos& operator=(
            const OnChangeWithKeepAliveSubscriptionQos& other);

    /** @brief Equality operator */
    bool operator==(const OnChangeWithKeepAliveSubscriptionQos& other) const;

    /** @brief
     * Returns the maximum value for the maximum interval in milliseconds:
     * 2 592 000 000 (30 days)
     */
    static const std::int64_t& MAX_MAX_INTERVAL_MS();

    /**
     * @deprecated
     * @see OnChangeWithKeepAliveSubscriptionQos#MAX_MAX_INTERVAL_MS
     */
    [[deprecated("Will be removed by end of the year 2016. Use MAX_MAX_INTERVAL_MS "
                 "instead.")]] static const std::int64_t&
    MAX_MAX_INTERVAL();

    /**
     * @brief Returns the maximum value for the alertAfter interval in
     * milliseconds: 2 592 000 000 (30 days)
     */
    static const std::int64_t& MAX_ALERT_AFTER_INTERVAL();

    /**
     * @brief Returns the default value for the alertAfter interval in
     * milliseconds: 0 (NO_ALERT_AFTER_INTERVAL)
     */
    static const std::int64_t& DEFAULT_ALERT_AFTER_INTERVAL();

    /** @brief Returns the value for no alertAfter interval in milliseconds: 0 */
    static const std::int64_t& NO_ALERT_AFTER_INTERVAL();

protected:
    /**
     * @brief The maximum interval in milliseconds.
     *
     * The provider will send notifications every maximum interval in milliseconds,
     * even if the value didn't change.
     */
    std::int64_t maxIntervalMs;

    /**
     * @brief time span in milliseconds after which a publicationMissed
     * will be called if no publications were received
     */
    std::int64_t alertAfterInterval;

private:
    ADD_LOGGER(OnChangeWithKeepAliveSubscriptionQos);
};

} // namespace joynr

#endif // ONCHANGEWITHKEEPALIVESUBSCRIPTIONQOS_H

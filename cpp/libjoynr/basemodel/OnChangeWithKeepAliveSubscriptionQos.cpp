/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

namespace joynr
{

std::int64_t OnChangeWithKeepAliveSubscriptionQos::MIN_MAX_INTERVAL_MS()
{
    static std::int64_t minMaxInterval = 50UL;
    return minMaxInterval;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::MAX_MAX_INTERVAL_MS()
{
    static std::int64_t maxMaxInterval = 2592000000UL;
    return maxMaxInterval;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::DEFAULT_MAX_INTERVAL_MS()
{
    static std::int64_t defaultMaxInterval = 60000UL;
    return defaultMaxInterval;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::MAX_ALERT_AFTER_INTERVAL_MS()
{
    static std::int64_t maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL_MS()
{
    return NO_ALERT_AFTER_INTERVAL();
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::NO_ALERT_AFTER_INTERVAL()
{
    static std::int64_t noAlertAfterInterval = 0;
    return noAlertAfterInterval;
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos()
        : OnChangeSubscriptionQos(),
          maxIntervalMs(DEFAULT_MAX_INTERVAL_MS()),
          alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos(
        std::int64_t validityMs,
        std::int64_t publicationTtlMsLocal,
        std::int64_t minIntervalMsLocal,
        std::int64_t maxIntervalMsLocal,
        std::int64_t alertAfterIntervalMsLocal)
        : OnChangeSubscriptionQos(validityMs, publicationTtlMsLocal, minIntervalMsLocal),
          maxIntervalMs(DEFAULT_MAX_INTERVAL_MS()),
          alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
    setMaxIntervalMs(maxIntervalMsLocal);
    setAlertAfterIntervalMs(alertAfterIntervalMsLocal);
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos(
        const OnChangeWithKeepAliveSubscriptionQos& other)
        : OnChangeSubscriptionQos(other),
          maxIntervalMs(other.getMaxIntervalMs()),
          alertAfterIntervalMs(other.getAlertAfterIntervalMs())
{
}

void OnChangeWithKeepAliveSubscriptionQos::setMaxIntervalMs(std::int64_t maxIntervalMsLocal)
{
    if (maxIntervalMsLocal < MIN_MAX_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid maxIntervalMs ({} ms), which is smaller than "
                       "MIN_MAX_INTERVAL_MS ({} ms). MIN_MAX_INTERVAL_MS will be used instead.",
                       maxIntervalMsLocal,
                       MIN_MAX_INTERVAL_MS());
        this->maxIntervalMs = MIN_MAX_INTERVAL_MS();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else if (maxIntervalMsLocal > MAX_MAX_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid maxIntervalMs ({} ms), which is bigger than "
                       "MAX_MAX_INTERVAL_MS "
                       "({} ms). MAX_MAX_INTERVAL_MS will be used instead.",
                       maxIntervalMsLocal,
                       MAX_MAX_INTERVAL_MS());
        this->maxIntervalMs = MAX_MAX_INTERVAL_MS();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else {
        // default case
        this->maxIntervalMs = maxIntervalMsLocal;
    }
    // check dependendencies: maxIntervalMs is not smaller than minIntervalMs
    if (this->maxIntervalMs < getMinIntervalMs()) {
        JOYNR_LOG_WARN(logger(),
                       "maxIntervalMs ({} ms) is smaller than minIntervalMs ({} ms). Setting "
                       "maxIntervalMs to minIntervalMs.",
                       maxIntervalMsLocal,
                       getMinIntervalMs());
        this->maxIntervalMs = getMinIntervalMs();
    }
    // check dependendencies: allertAfterIntervalMs is not smaller than maxIntervalMs
    if (alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() &&
        alertAfterIntervalMs < getMaxIntervalMs()) {
        JOYNR_LOG_WARN(
                logger(),
                "alertAfterIntervalMs ({} ms) is smaller than maxIntervalMs ({} ms). Setting "
                "alertAfterIntervalMs to maxIntervalMs.",
                alertAfterIntervalMs,
                getMaxIntervalMs());
        alertAfterIntervalMs = getMaxIntervalMs();
    }
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getMaxIntervalMs() const
{
    return this->maxIntervalMs;
}

void OnChangeWithKeepAliveSubscriptionQos::setMinIntervalMs(std::int64_t minIntervalMsLocal)
{
    OnChangeSubscriptionQos::setMinIntervalMs(minIntervalMsLocal);
    // corrects the maxinterval if minIntervalMs changes
    setMaxIntervalMs(this->maxIntervalMs);
}

void OnChangeWithKeepAliveSubscriptionQos::setAlertAfterIntervalMs(
        std::int64_t alertAfterIntervalMsLocal)
{
    if (alertAfterIntervalMsLocal > MAX_ALERT_AFTER_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid alertAfterIntervalMs ({} ms), which is bigger than "
                       "MAX_ALERT_AFTER_INTERVAL_MS ({} ms). MAX_ALERT_AFTER_INTERVAL_MS will be "
                       "used instead.",
                       alertAfterIntervalMsLocal,
                       MAX_ALERT_AFTER_INTERVAL_MS());
        this->alertAfterIntervalMs = MAX_ALERT_AFTER_INTERVAL_MS();
        return;
    }
    if (alertAfterIntervalMsLocal != NO_ALERT_AFTER_INTERVAL() &&
        alertAfterIntervalMsLocal < getMaxIntervalMs()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid alertAfterIntervalMs ({} ms), which is smaller than "
                       "maxIntervalMs ({} ms). maxIntervalMs will be used instead.",
                       alertAfterIntervalMsLocal,
                       getMaxIntervalMs());
        this->alertAfterIntervalMs = getMaxIntervalMs();
        return;
    }
    this->alertAfterIntervalMs = alertAfterIntervalMsLocal;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getAlertAfterIntervalMs() const
{
    return alertAfterIntervalMs;
}

void OnChangeWithKeepAliveSubscriptionQos::clearAlertAfterInterval()
{
    alertAfterIntervalMs = NO_ALERT_AFTER_INTERVAL();
}

OnChangeWithKeepAliveSubscriptionQos& OnChangeWithKeepAliveSubscriptionQos::operator=(
        const OnChangeWithKeepAliveSubscriptionQos& other)
{
    expiryDateMs = other.getExpiryDateMs();
    publicationTtlMs = other.getPublicationTtlMs();
    minIntervalMs = other.getMinIntervalMs();
    maxIntervalMs = other.getMaxIntervalMs();
    alertAfterIntervalMs = other.getAlertAfterIntervalMs();
    return *this;
}

bool OnChangeWithKeepAliveSubscriptionQos::operator==(
        const OnChangeWithKeepAliveSubscriptionQos& other) const
{
    return expiryDateMs == other.getExpiryDateMs() &&
           publicationTtlMs == other.getPublicationTtlMs() &&
           minIntervalMs == other.getMinIntervalMs() && maxIntervalMs == other.getMaxIntervalMs() &&
           alertAfterIntervalMs == other.getAlertAfterIntervalMs();
}

} // namespace joynr

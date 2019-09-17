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
          _maxIntervalMs(DEFAULT_MAX_INTERVAL_MS()),
          _alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos(
        std::int64_t validityMs,
        std::int64_t publicationTtlMs,
        std::int64_t minIntervalMs,
        std::int64_t maxIntervalMs,
        std::int64_t alertAfterIntervalMs)
        : OnChangeSubscriptionQos(validityMs, publicationTtlMs, minIntervalMs),
          _maxIntervalMs(DEFAULT_MAX_INTERVAL_MS()),
          _alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
    setMaxIntervalMs(maxIntervalMs);
    setAlertAfterIntervalMs(alertAfterIntervalMs);
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos(
        const OnChangeWithKeepAliveSubscriptionQos& other)
        : OnChangeSubscriptionQos(other),
          _maxIntervalMs(other.getMaxIntervalMs()),
          _alertAfterIntervalMs(other.getAlertAfterIntervalMs())
{
}

void OnChangeWithKeepAliveSubscriptionQos::setMaxIntervalMs(std::int64_t maxIntervalMs)
{
    if (maxIntervalMs < MIN_MAX_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid maxIntervalMs ({} ms), which is smaller than "
                       "MIN_MAX_INTERVAL_MS ({} ms). MIN_MAX_INTERVAL_MS will be used instead.",
                       maxIntervalMs,
                       MIN_MAX_INTERVAL_MS());
        this->_maxIntervalMs = MIN_MAX_INTERVAL_MS();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else if (maxIntervalMs > MAX_MAX_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid maxIntervalMs ({} ms), which is bigger than "
                       "MAX_MAX_INTERVAL_MS "
                       "({} ms). MAX_MAX_INTERVAL_MS will be used instead.",
                       maxIntervalMs,
                       MAX_MAX_INTERVAL_MS());
        this->_maxIntervalMs = MAX_MAX_INTERVAL_MS();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else {
        // default case
        this->_maxIntervalMs = maxIntervalMs;
    }
    // check dependendencies: maxIntervalMs is not smaller than minIntervalMs
    if (this->_maxIntervalMs < getMinIntervalMs()) {
        JOYNR_LOG_WARN(logger(),
                       "maxIntervalMs ({} ms) is smaller than minIntervalMs ({} ms). Setting "
                       "maxIntervalMs to minIntervalMs.",
                       maxIntervalMs,
                       getMinIntervalMs());
        this->_maxIntervalMs = getMinIntervalMs();
    }
    // check dependendencies: allertAfterIntervalMs is not smaller than maxIntervalMs
    if (_alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() &&
        _alertAfterIntervalMs < getMaxIntervalMs()) {
        JOYNR_LOG_WARN(
                logger(),
                "alertAfterIntervalMs ({} ms) is smaller than maxIntervalMs ({} ms). Setting "
                "alertAfterIntervalMs to maxIntervalMs.",
                _alertAfterIntervalMs,
                getMaxIntervalMs());
        _alertAfterIntervalMs = getMaxIntervalMs();
    }
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getMaxIntervalMs() const
{
    return this->_maxIntervalMs;
}

void OnChangeWithKeepAliveSubscriptionQos::setMinIntervalMs(std::int64_t minIntervalMs)
{
    OnChangeSubscriptionQos::setMinIntervalMs(minIntervalMs);
    // corrects the maxinterval if minIntervalMs changes
    setMaxIntervalMs(this->_maxIntervalMs);
}

void OnChangeWithKeepAliveSubscriptionQos::setAlertAfterIntervalMs(
        std::int64_t alertAfterIntervalMs)
{
    if (alertAfterIntervalMs > MAX_ALERT_AFTER_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid alertAfterIntervalMs ({} ms), which is bigger than "
                       "MAX_ALERT_AFTER_INTERVAL_MS ({} ms). MAX_ALERT_AFTER_INTERVAL_MS will be "
                       "used instead.",
                       alertAfterIntervalMs,
                       MAX_ALERT_AFTER_INTERVAL_MS());
        this->_alertAfterIntervalMs = MAX_ALERT_AFTER_INTERVAL_MS();
        return;
    }
    if (alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() &&
        alertAfterIntervalMs < getMaxIntervalMs()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid alertAfterIntervalMs ({} ms), which is smaller than "
                       "maxIntervalMs ({} ms). maxIntervalMs will be used instead.",
                       alertAfterIntervalMs,
                       getMaxIntervalMs());
        this->_alertAfterIntervalMs = getMaxIntervalMs();
        return;
    }
    this->_alertAfterIntervalMs = alertAfterIntervalMs;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getAlertAfterIntervalMs() const
{
    return _alertAfterIntervalMs;
}

void OnChangeWithKeepAliveSubscriptionQos::clearAlertAfterInterval()
{
    _alertAfterIntervalMs = NO_ALERT_AFTER_INTERVAL();
}

OnChangeWithKeepAliveSubscriptionQos& OnChangeWithKeepAliveSubscriptionQos::operator=(
        const OnChangeWithKeepAliveSubscriptionQos& other)
{
    _expiryDateMs = other.getExpiryDateMs();
    _publicationTtlMs = other.getPublicationTtlMs();
    _minIntervalMs = other.getMinIntervalMs();
    _maxIntervalMs = other.getMaxIntervalMs();
    _alertAfterIntervalMs = other.getAlertAfterIntervalMs();
    return *this;
}

bool OnChangeWithKeepAliveSubscriptionQos::operator==(
        const OnChangeWithKeepAliveSubscriptionQos& other) const
{
    return _expiryDateMs == other.getExpiryDateMs() &&
           _publicationTtlMs == other.getPublicationTtlMs() &&
           _minIntervalMs == other.getMinIntervalMs() &&
           _maxIntervalMs == other.getMaxIntervalMs() &&
           _alertAfterIntervalMs == other.getAlertAfterIntervalMs();
}

} // namespace joynr

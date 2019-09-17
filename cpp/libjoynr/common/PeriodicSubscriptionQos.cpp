/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#include "joynr/PeriodicSubscriptionQos.h"

namespace joynr
{

std::int64_t PeriodicSubscriptionQos::MIN_PERIOD_MS()
{
    return 50;
}

std::int64_t PeriodicSubscriptionQos::MAX_PERIOD_MS()
{
    return 2592000000UL;
}

std::int64_t PeriodicSubscriptionQos::DEFAULT_PERIOD_MS()
{
    return 60000UL;
}

std::int64_t PeriodicSubscriptionQos::MAX_ALERT_AFTER_INTERVAL_MS()
{
    return 2592000000UL;
}

std::int64_t PeriodicSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL_MS()
{
    return NO_ALERT_AFTER_INTERVAL();
}

std::int64_t PeriodicSubscriptionQos::NO_ALERT_AFTER_INTERVAL()
{
    return 0;
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos()
        : UnicastSubscriptionQos(),
          _periodMs(DEFAULT_PERIOD_MS()),
          _alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(std::int64_t validityMs,
                                                 std::int64_t publicationTtlMs,
                                                 std::int64_t periodMs,
                                                 std::int64_t alertAfterIntervalMs)
        : UnicastSubscriptionQos(validityMs, publicationTtlMs),
          _periodMs(DEFAULT_PERIOD_MS()),
          _alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
    setPeriodMs(periodMs);
    setAlertAfterIntervalMs(alertAfterIntervalMs);
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(const PeriodicSubscriptionQos& other)
        : UnicastSubscriptionQos(other),
          _periodMs(other.getPeriodMs()),
          _alertAfterIntervalMs(other.getAlertAfterIntervalMs())
{
}

void PeriodicSubscriptionQos::setPeriodMs(std::int64_t periodMs)
{
    if (periodMs > MAX_PERIOD_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid periodMs ({} ms), which is bigger than MAX_PERIOD_MS "
                       "({} ms). MAX_PERIOD_MS will be used instead.",
                       periodMs,
                       MAX_PERIOD_MS());
        this->_periodMs = MAX_PERIOD_MS();
        // note: don't return here as we need to check dependent values at the end of this method
    } else if (periodMs < MIN_PERIOD_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid periodMs ({} ms), which is smaller than "
                       "MIN_PERIOD_MS ({} ms). MIN_PERIOD_MS will be used instead.",
                       periodMs,
                       MIN_PERIOD_MS());
        this->_periodMs = MIN_PERIOD_MS();
        // note: don't return here as we need to check dependent values at the end of this method
    } else {
        // default case
        this->_periodMs = periodMs;
    }
    // check dependencies: alertAfterIntervalMs is not smaller than periodMs
    if (_alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() &&
        _alertAfterIntervalMs < getPeriodMs()) {
        JOYNR_LOG_WARN(logger(),
                       "alertAfterIntervalMs ({} ms) is smaller than periodMs ({} ms). Setting "
                       "alertAfterIntervalMs to periodMs.",
                       _alertAfterIntervalMs,
                       getPeriodMs());
        _alertAfterIntervalMs = getPeriodMs();
    }
}

std::int64_t PeriodicSubscriptionQos::getPeriodMs() const
{
    return this->_periodMs;
}

void PeriodicSubscriptionQos::setAlertAfterIntervalMs(std::int64_t alertAfterIntervalMs)
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
    if (alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() && alertAfterIntervalMs < getPeriodMs()) {
        JOYNR_LOG_WARN(logger(),
                       "alertAfterIntervalMs ({} ms) is smaller than periodMs ({} ms). Setting "
                       "alertAfterIntervalMs to periodMs.",
                       alertAfterIntervalMs,
                       getPeriodMs());
        this->_alertAfterIntervalMs = _periodMs;
        return;
    }
    this->_alertAfterIntervalMs = alertAfterIntervalMs;
}

std::int64_t PeriodicSubscriptionQos::getAlertAfterIntervalMs() const
{
    return _alertAfterIntervalMs;
}

void PeriodicSubscriptionQos::clearAlertAfterInterval()
{
    this->_alertAfterIntervalMs = NO_ALERT_AFTER_INTERVAL();
}

PeriodicSubscriptionQos& PeriodicSubscriptionQos::operator=(const PeriodicSubscriptionQos& other)
{
    _expiryDateMs = other.getExpiryDateMs();
    _publicationTtlMs = other.getPublicationTtlMs();
    _periodMs = other.getPeriodMs();
    _alertAfterIntervalMs = other.getAlertAfterIntervalMs();
    return *this;
}

bool PeriodicSubscriptionQos::operator==(const PeriodicSubscriptionQos& other) const
{
    return _expiryDateMs == other.getExpiryDateMs() &&
           _publicationTtlMs == other.getPublicationTtlMs() && _periodMs == other.getPeriodMs() &&
           _alertAfterIntervalMs == other.getAlertAfterIntervalMs();
}

} // namespace joynr

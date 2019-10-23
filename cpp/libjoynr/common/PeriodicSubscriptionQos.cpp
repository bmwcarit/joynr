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
          periodMs(DEFAULT_PERIOD_MS()),
          alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(std::int64_t validityMs,
                                                 std::int64_t publicationTtlMsLocal,
                                                 std::int64_t periodMsLocal,
                                                 std::int64_t alertAfterIntervalMsLocal)
        : UnicastSubscriptionQos(validityMs, publicationTtlMsLocal),
          periodMs(DEFAULT_PERIOD_MS()),
          alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
    setPeriodMs(periodMsLocal);
    setAlertAfterIntervalMs(alertAfterIntervalMsLocal);
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(const PeriodicSubscriptionQos& other)
        : UnicastSubscriptionQos(other),
          periodMs(other.getPeriodMs()),
          alertAfterIntervalMs(other.getAlertAfterIntervalMs())
{
}

void PeriodicSubscriptionQos::setPeriodMs(std::int64_t periodMsLocal)
{
    if (periodMsLocal > MAX_PERIOD_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid periodMs ({} ms), which is bigger than MAX_PERIOD_MS "
                       "({} ms). MAX_PERIOD_MS will be used instead.",
                       periodMsLocal,
                       MAX_PERIOD_MS());
        this->periodMs = MAX_PERIOD_MS();
        // note: don't return here as we need to check dependent values at the end of this method
    } else if (periodMsLocal < MIN_PERIOD_MS()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to set invalid periodMs ({} ms), which is smaller than "
                       "MIN_PERIOD_MS ({} ms). MIN_PERIOD_MS will be used instead.",
                       periodMsLocal,
                       MIN_PERIOD_MS());
        this->periodMs = MIN_PERIOD_MS();
        // note: don't return here as we need to check dependent values at the end of this method
    } else {
        // default case
        this->periodMs = periodMsLocal;
    }
    // check dependencies: alertAfterIntervalMs is not smaller than periodMs
    if (alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() && alertAfterIntervalMs < getPeriodMs()) {
        JOYNR_LOG_WARN(logger(),
                       "alertAfterIntervalMs ({} ms) is smaller than periodMs ({} ms). Setting "
                       "alertAfterIntervalMs to periodMs.",
                       alertAfterIntervalMs,
                       getPeriodMs());
        alertAfterIntervalMs = getPeriodMs();
    }
}

std::int64_t PeriodicSubscriptionQos::getPeriodMs() const
{
    return this->periodMs;
}

void PeriodicSubscriptionQos::setAlertAfterIntervalMs(std::int64_t alertAfterIntervalMsLocal)
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
        alertAfterIntervalMsLocal < getPeriodMs()) {
        JOYNR_LOG_WARN(logger(),
                       "alertAfterIntervalMs ({} ms) is smaller than periodMs ({} ms). Setting "
                       "alertAfterIntervalMs to periodMs.",
                       alertAfterIntervalMsLocal,
                       getPeriodMs());
        this->alertAfterIntervalMs = periodMs;
        return;
    }
    this->alertAfterIntervalMs = alertAfterIntervalMsLocal;
}

std::int64_t PeriodicSubscriptionQos::getAlertAfterIntervalMs() const
{
    return alertAfterIntervalMs;
}

void PeriodicSubscriptionQos::clearAlertAfterInterval()
{
    this->alertAfterIntervalMs = NO_ALERT_AFTER_INTERVAL();
}

PeriodicSubscriptionQos& PeriodicSubscriptionQos::operator=(const PeriodicSubscriptionQos& other)
{
    expiryDateMs = other.getExpiryDateMs();
    publicationTtlMs = other.getPublicationTtlMs();
    periodMs = other.getPeriodMs();
    alertAfterIntervalMs = other.getAlertAfterIntervalMs();
    return *this;
}

bool PeriodicSubscriptionQos::operator==(const PeriodicSubscriptionQos& other) const
{
    return expiryDateMs == other.getExpiryDateMs() &&
           publicationTtlMs == other.getPublicationTtlMs() && periodMs == other.getPeriodMs() &&
           alertAfterIntervalMs == other.getAlertAfterIntervalMs();
}

} // namespace joynr

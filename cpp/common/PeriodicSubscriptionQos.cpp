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
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/Variant.h"

using namespace joynr;

INIT_LOGGER(PeriodicSubscriptionQos);

static bool isPeriodicSubscriptionQosRegistered =
        Variant::registerType<PeriodicSubscriptionQos>("joynr.PeriodicSubscriptionQos");

const std::int64_t& PeriodicSubscriptionQos::MIN_PERIOD_MS()
{
    static std::int64_t minPeriod = 50;
    return minPeriod;
}

const std::int64_t& PeriodicSubscriptionQos::MIN_PERIOD()
{
    return MIN_PERIOD_MS();
}

const std::int64_t& PeriodicSubscriptionQos::MAX_PERIOD_MS()
{
    static std::int64_t maxPeriod = 2592000000UL;
    return maxPeriod;
}

const std::int64_t& PeriodicSubscriptionQos::MAX_PERIOD()
{
    return MAX_PERIOD_MS();
}

const std::int64_t& PeriodicSubscriptionQos::DEFAULT_PERIOD_MS()
{
    static std::int64_t maxPeriod = 60000UL;
    return maxPeriod;
}

const std::int64_t& PeriodicSubscriptionQos::MAX_ALERT_AFTER_INTERVAL()
{
    static std::int64_t maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

const std::int64_t& PeriodicSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL()
{
    return NO_ALERT_AFTER_INTERVAL();
}

const std::int64_t& PeriodicSubscriptionQos::NO_ALERT_AFTER_INTERVAL()
{
    static std::int64_t noAlertAfterInterval = 0;
    return noAlertAfterInterval;
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos()
        : SubscriptionQos(),
          periodMs(DEFAULT_PERIOD_MS()),
          alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(const std::int64_t& validityMs,
                                                 const std::int64_t& periodMs,
                                                 const std::int64_t& alertAfterInterval)
        : SubscriptionQos(validityMs),
          periodMs(DEFAULT_PERIOD_MS()),
          alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
    setPeriodMs(periodMs);
    setAlertAfterInterval(alertAfterInterval);
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(const PeriodicSubscriptionQos& other)
        : SubscriptionQos(other),
          periodMs(other.getPeriodMs()),
          alertAfterInterval(other.getAlertAfterInterval())
{
}

void PeriodicSubscriptionQos::setPeriodMs(const std::int64_t& periodMs)
{
    if (periodMs > MAX_PERIOD_MS()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to set invalid periodMs ({} ms), which is bigger than MAX_PERIOD_MS "
                       "({} ms). MAX_PERIOD_MS will be used instead.",
                       periodMs,
                       MAX_PERIOD_MS());
        this->periodMs = MAX_PERIOD_MS();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else if (periodMs < MIN_PERIOD_MS()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to set invalid periodMs ({} ms), which is smaller than "
                       "MIN_PERIOD_MS ({} ms). MIN_PERIOD_MS will be used instead.",
                       periodMs,
                       MIN_PERIOD_MS());
        this->periodMs = MIN_PERIOD_MS();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else {
        // default case
        this->periodMs = periodMs;
    }
    // check dependendencies: allertAfterIntervalMs is not smaller than periodMs
    if (alertAfterInterval != NO_ALERT_AFTER_INTERVAL() && alertAfterInterval < getPeriodMs()) {
        JOYNR_LOG_WARN(logger,
                       "alertAfterInterval ({} ms) is smaller than periodMs ({} ms). Setting "
                       "alertAfterInterval to periodMs.",
                       alertAfterInterval,
                       getPeriodMs());
        alertAfterInterval = getPeriodMs();
    }
}

void PeriodicSubscriptionQos::setPeriod(const std::int64_t& periodMs)
{
    setPeriodMs(periodMs);
}

std::int64_t PeriodicSubscriptionQos::getPeriodMs() const
{
    return this->periodMs;
}

std::int64_t PeriodicSubscriptionQos::getPeriod() const
{
    return getPeriodMs();
}

void PeriodicSubscriptionQos::setAlertAfterInterval(const std::int64_t& alertAfterInterval)
{
    if (alertAfterInterval > MAX_ALERT_AFTER_INTERVAL()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to set invalid alertAfterInterval ({} ms), which is bigger than "
                       "MAX_ALERT_AFTER_INTERVAL ({} ms). MAX_ALERT_AFTER_INTERVAL will be "
                       "used instead.",
                       alertAfterInterval,
                       MAX_ALERT_AFTER_INTERVAL());
        this->alertAfterInterval = MAX_ALERT_AFTER_INTERVAL();
        return;
    }
    if (alertAfterInterval != NO_ALERT_AFTER_INTERVAL() && alertAfterInterval < getPeriodMs()) {
        JOYNR_LOG_WARN(logger,
                       "alertAfterInterval ({} ms) is smaller than periodMs ({} ms). Setting "
                       "alertAfterInterval to periodMs.",
                       alertAfterInterval,
                       getPeriodMs());
        this->alertAfterInterval = periodMs;
        return;
    }
    this->alertAfterInterval = alertAfterInterval;
}

std::int64_t PeriodicSubscriptionQos::getAlertAfterInterval() const
{
    return alertAfterInterval;
}

void PeriodicSubscriptionQos::clearAlertAfterInterval()
{
    this->alertAfterInterval = NO_ALERT_AFTER_INTERVAL();
}

PeriodicSubscriptionQos& PeriodicSubscriptionQos::operator=(const PeriodicSubscriptionQos& other)
{
    expiryDateMs = other.getExpiryDateMs();
    publicationTtlMs = other.getPublicationTtlMs();
    periodMs = other.getPeriodMs();
    alertAfterInterval = other.getAlertAfterInterval();
    return *this;
}

bool PeriodicSubscriptionQos::operator==(const PeriodicSubscriptionQos& other) const
{
    return expiryDateMs == other.getExpiryDateMs() &&
           publicationTtlMs == other.getPublicationTtlMs() && periodMs == other.getPeriodMs() &&
           alertAfterInterval == other.getAlertAfterInterval();
}

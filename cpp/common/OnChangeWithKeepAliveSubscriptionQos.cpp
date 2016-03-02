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
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/Variant.h"
#include "joynr/Logger.h"

using namespace joynr;

INIT_LOGGER(OnChangeWithKeepAliveSubscriptionQos);

static bool isOnChangeWithKeepAliveSubscriptionQosSerializer =
        Variant::registerType<OnChangeWithKeepAliveSubscriptionQos>(
                "joynr.OnChangeWithKeepAliveSubscriptionQos");

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::MAX_MAX_INTERVAL_MS()
{
    static std::int64_t maxMaxInterval = 2592000000UL;
    return maxMaxInterval;
}

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::MAX_MAX_INTERVAL()
{
    return MAX_MAX_INTERVAL_MS();
}

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::DEFAULT_MAX_INTERVAL_MS()
{
    static std::int64_t defaultMaxInterval = 60000UL;
    return defaultMaxInterval;
}

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::MAX_ALERT_AFTER_INTERVAL_MS()
{
    static std::int64_t maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::MAX_ALERT_AFTER_INTERVAL()
{
    return MAX_ALERT_AFTER_INTERVAL_MS();
}

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL_MS()
{
    return NO_ALERT_AFTER_INTERVAL();
}

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL()
{
    return DEFAULT_ALERT_AFTER_INTERVAL_MS();
}

const std::int64_t& OnChangeWithKeepAliveSubscriptionQos::NO_ALERT_AFTER_INTERVAL()
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
        const std::int64_t& validityMs,
        const std::int64_t& minIntervalMs,
        const std::int64_t& maxIntervalMs,
        const std::int64_t& alertAfterInterval)
        : OnChangeSubscriptionQos(validityMs, minIntervalMs),
          maxIntervalMs(DEFAULT_MAX_INTERVAL_MS()),
          alertAfterIntervalMs(DEFAULT_ALERT_AFTER_INTERVAL_MS())
{
    setMaxIntervalMs(maxIntervalMs);
    setAlertAfterIntervalMs(alertAfterInterval);
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos(
        const OnChangeWithKeepAliveSubscriptionQos& other)
        : OnChangeSubscriptionQos(other),
          maxIntervalMs(other.getMaxIntervalMs()),
          alertAfterIntervalMs(other.getAlertAfterIntervalMs())
{
}

void OnChangeWithKeepAliveSubscriptionQos::setMaxIntervalMs(const std::int64_t& maxIntervalMs)
{
    if (maxIntervalMs < getMinIntervalMs()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to set invalid maxIntervalMs ({} ms), which is smaller than "
                       "MIN_MAX_INTERVAL_MS ({} ms). MIN_MAX_INTERVAL_MS will be used instead.",
                       maxIntervalMs,
                       MIN_MAX_INTERVAL_MS());
        this->maxIntervalMs = getMinIntervalMs();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else if (maxIntervalMs > MAX_MAX_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to set invalid maxIntervalMs ({} ms), which is bigger than "
                       "MAX_MAX_INTERVAL_MS "
                       "({} ms). MAX_MAX_INTERVAL_MS will be used instead.",
                       maxIntervalMs,
                       MAX_MAX_INTERVAL_MS());
        this->maxIntervalMs = MAX_MAX_INTERVAL_MS();
        // note: don't return here as we nned to check dependend values at the end of this method
    } else {
        // default case
        this->maxIntervalMs = maxIntervalMs;
    }
    // check dependendencies: allertAfterIntervalMs is not smaller than maxIntervalMs
    if (alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() &&
        alertAfterIntervalMs < getMaxIntervalMs()) {
        JOYNR_LOG_WARN(
                logger,
                "alertAfterIntervalMs ({} ms) is smaller than maxIntervalMs ({} ms). Setting "
                "alertAfterIntervalMs to maxIntervalMs.",
                alertAfterIntervalMs,
                getMaxIntervalMs());
        alertAfterIntervalMs = getMaxIntervalMs();
    }
}

void OnChangeWithKeepAliveSubscriptionQos::setMaxInterval(const std::int64_t& maxIntervalMs)
{
    setMaxIntervalMs(maxIntervalMs);
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getMaxIntervalMs() const
{
    return this->maxIntervalMs;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getMaxInterval() const
{
    return getMaxIntervalMs();
}

void OnChangeWithKeepAliveSubscriptionQos::setMinIntervalMs(const std::int64_t& minIntervalMs)
{
    OnChangeSubscriptionQos::setMinIntervalMs(minIntervalMs);
    // corrects the maxinterval if minIntervalMs changes
    setMaxIntervalMs(this->maxIntervalMs);
}

void OnChangeWithKeepAliveSubscriptionQos::setMinInterval(const std::int64_t& minIntervalMs)
{
    setMinIntervalMs(minIntervalMs);
}

void OnChangeWithKeepAliveSubscriptionQos::setAlertAfterIntervalMs(
        const std::int64_t& alertAfterIntervalMs)
{
    if (alertAfterIntervalMs > MAX_ALERT_AFTER_INTERVAL_MS()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to set invalid alertAfterIntervalMs ({} ms), which is bigger than "
                       "MAX_ALERT_AFTER_INTERVAL_MS ({} ms). MAX_ALERT_AFTER_INTERVAL_MS will be "
                       "used instead.",
                       alertAfterIntervalMs,
                       MAX_ALERT_AFTER_INTERVAL_MS());
        this->alertAfterIntervalMs = MAX_ALERT_AFTER_INTERVAL_MS();
        return;
    }
    if (alertAfterIntervalMs != NO_ALERT_AFTER_INTERVAL() &&
        alertAfterIntervalMs < getMaxIntervalMs()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to set invalid alertAfterIntervalMs ({} ms), which is smaller than "
                       "maxIntervalMs ({} ms). maxIntervalMs will be used instead.",
                       alertAfterIntervalMs,
                       getMaxIntervalMs());
        this->alertAfterIntervalMs = getMaxIntervalMs();
        return;
    }
    this->alertAfterIntervalMs = alertAfterIntervalMs;
}

void OnChangeWithKeepAliveSubscriptionQos::setAlertAfterInterval(
        const std::int64_t& alertAfterIntervalMs)
{
    setAlertAfterIntervalMs(alertAfterIntervalMs);
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getAlertAfterIntervalMs() const
{
    return alertAfterIntervalMs;
}

std::int64_t OnChangeWithKeepAliveSubscriptionQos::getAlertAfterInterval() const
{
    return getAlertAfterIntervalMs();
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

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
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Variant.h"

using namespace joynr;

// Register the OnChangeSubscriptionQos type id
static const bool isOnChangeSubscriptionQosRegistered =
        Variant::registerType<OnChangeSubscriptionQos>("joynr.OnChangeSubscriptionQos");

const std::int64_t& OnChangeSubscriptionQos::DEFAULT_MIN_INTERVAL_MS()
{
    static std::int64_t defaultMinInterval = 1000;
    return defaultMinInterval;
}

const std::int64_t& OnChangeSubscriptionQos::DEFAULT_MIN_INTERVAL()
{
    return DEFAULT_MIN_INTERVAL_MS();
}

const std::int64_t& OnChangeSubscriptionQos::MIN_MIN_INTERVAL_MS()
{
    static std::int64_t minMinInterval = 0;
    return minMinInterval;
}

const std::int64_t& OnChangeSubscriptionQos::MIN_MIN_INTERVAL()
{
    return MIN_MIN_INTERVAL_MS();
}

const std::int64_t& OnChangeSubscriptionQos::MAX_MIN_INTERVAL_MS()
{
    static std::int64_t maxMinInterval = 2592000000UL;
    return maxMinInterval;
}

const std::int64_t& OnChangeSubscriptionQos::MAX_MIN_INTERVAL()
{
    return MAX_MIN_INTERVAL_MS();
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos()
        : SubscriptionQos(), minIntervalMs(MIN_MIN_INTERVAL_MS())
{
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos(const std::int64_t& validityMs,
                                                 const std::int64_t& minIntervalMs)
        : SubscriptionQos(validityMs), minIntervalMs(DEFAULT_MIN_INTERVAL_MS())
{
    setMinIntervalMs(minIntervalMs);
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos(const OnChangeSubscriptionQos& other)
        : SubscriptionQos(other), minIntervalMs(other.getMinIntervalMs())
{
}

std::int64_t OnChangeSubscriptionQos::getMinIntervalMs() const
{
    return minIntervalMs;
}

std::int64_t OnChangeSubscriptionQos::getMinInterval() const
{
    return getMinIntervalMs();
}

void OnChangeSubscriptionQos::setMinIntervalMs(const std::int64_t& minInterval)
{
    if (minIntervalMs < MIN_MIN_INTERVAL_MS()) {
        this->minIntervalMs = MIN_MIN_INTERVAL_MS();
        return;
    }
    if (minIntervalMs > MAX_MIN_INTERVAL_MS()) {
        this->minIntervalMs = MAX_MIN_INTERVAL_MS();
        return;
    }

    this->minIntervalMs = minInterval;
}

void OnChangeSubscriptionQos::setMinInterval(const std::int64_t& minIntervalMs)
{
    setMinIntervalMs(minIntervalMs);
}

OnChangeSubscriptionQos& OnChangeSubscriptionQos::operator=(const OnChangeSubscriptionQos& other)
{
    expiryDateMs = other.getExpiryDateMs();
    publicationTtlMs = other.getPublicationTtlMs();
    minIntervalMs = other.getMinIntervalMs();
    return *this;
}

bool OnChangeSubscriptionQos::operator==(const OnChangeSubscriptionQos& other) const
{
    return expiryDateMs == other.getExpiryDateMs() &&
           publicationTtlMs == other.getPublicationTtlMs() &&
           minIntervalMs == other.getMinIntervalMs();
}

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
#include "joynr/OnChangeSubscriptionQos.h"

namespace joynr
{

const std::int64_t& OnChangeSubscriptionQos::DEFAULT_MIN_INTERVAL_MS()
{
    static std::int64_t defaultMinInterval = 1000;
    return defaultMinInterval;
}

const std::int64_t& OnChangeSubscriptionQos::MIN_MIN_INTERVAL_MS()
{
    static std::int64_t minMinInterval = 0;
    return minMinInterval;
}

const std::int64_t& OnChangeSubscriptionQos::MAX_MIN_INTERVAL_MS()
{
    static std::int64_t maxMinInterval = 2592000000UL;
    return maxMinInterval;
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos() : minIntervalMs(MIN_MIN_INTERVAL_MS())
{
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos(const std::int64_t validityMs,
                                                 const std::int64_t publicationTtlMs,
                                                 const std::int64_t minIntervalMs)
        : UnicastSubscriptionQos(validityMs, publicationTtlMs),
          minIntervalMs(DEFAULT_MIN_INTERVAL_MS())
{
    setMinIntervalMs(minIntervalMs);
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos(const OnChangeSubscriptionQos& other)
        : UnicastSubscriptionQos::UnicastSubscriptionQos(other),
          minIntervalMs(other.getMinIntervalMs())
{
}

std::int64_t OnChangeSubscriptionQos::getMinIntervalMs() const
{
    return minIntervalMs;
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

} // namespace joynr

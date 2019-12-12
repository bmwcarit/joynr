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

std::int64_t OnChangeSubscriptionQos::DEFAULT_MIN_INTERVAL_MS()
{
    static std::int64_t defaultMinInterval = 1000;
    return defaultMinInterval;
}

std::int64_t OnChangeSubscriptionQos::MIN_MIN_INTERVAL_MS()
{
    static std::int64_t minMinInterval = 0;
    return minMinInterval;
}

std::int64_t OnChangeSubscriptionQos::MAX_MIN_INTERVAL_MS()
{
    static std::int64_t maxMinInterval = 2592000000UL;
    return maxMinInterval;
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos() : minIntervalMs(MIN_MIN_INTERVAL_MS())
{
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos(std::int64_t validityMs,
                                                 std::int64_t publicationTtlMsLocal,
                                                 std::int64_t minIntervalMsLocal)
        : UnicastSubscriptionQos(validityMs, publicationTtlMsLocal),
          minIntervalMs(DEFAULT_MIN_INTERVAL_MS())
{
    setMinIntervalMs(minIntervalMsLocal);
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

void OnChangeSubscriptionQos::setMinIntervalMs(std::int64_t minIntervalMsLocal)
{
    if (minIntervalMsLocal < MIN_MIN_INTERVAL_MS()) {
        this->minIntervalMs = MIN_MIN_INTERVAL_MS();
        return;
    }
    if (minIntervalMsLocal > MAX_MIN_INTERVAL_MS()) {
        this->minIntervalMs = MAX_MIN_INTERVAL_MS();
        return;
    }

    this->minIntervalMs = minIntervalMsLocal;
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

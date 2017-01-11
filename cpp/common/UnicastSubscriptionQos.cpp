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

#include "joynr/UnicastSubscriptionQos.h"

namespace joynr
{

const std::int64_t& UnicastSubscriptionQos::DEFAULT_PUBLICATION_TTL_MS()
{
    static const std::int64_t defaultPublicationTtl = 10000;
    return defaultPublicationTtl;
}

const std::int64_t& UnicastSubscriptionQos::MIN_PUBLICATION_TTL_MS()
{
    static const std::int64_t minPublicationTtl = 100;
    return minPublicationTtl;
}

const std::int64_t& UnicastSubscriptionQos::MAX_PUBLICATION_TTL_MS()
{
    static const std::int64_t maxPublicationTtl = 2592000000UL;
    return maxPublicationTtl;
}

UnicastSubscriptionQos::UnicastSubscriptionQos()
        : SubscriptionQos(), publicationTtlMs(DEFAULT_PUBLICATION_TTL_MS())
{
}

UnicastSubscriptionQos::UnicastSubscriptionQos(const UnicastSubscriptionQos& other)
        : SubscriptionQos::SubscriptionQos(other), publicationTtlMs(other.publicationTtlMs)
{
}

UnicastSubscriptionQos::UnicastSubscriptionQos(const std::int64_t validityMs,
                                               const std::int64_t publicationTtlMs)
        : SubscriptionQos(validityMs), publicationTtlMs(DEFAULT_PUBLICATION_TTL_MS())
{
    setPublicationTtlMs(publicationTtlMs);
}

std::int64_t UnicastSubscriptionQos::getPublicationTtlMs() const
{
    return publicationTtlMs;
}

void UnicastSubscriptionQos::setPublicationTtlMs(const std::int64_t& publicationTtlMs)
{
    this->publicationTtlMs = publicationTtlMs;
    if (this->publicationTtlMs > MAX_PUBLICATION_TTL_MS()) {
        this->publicationTtlMs = MAX_PUBLICATION_TTL_MS();
    }
    if (this->publicationTtlMs < MIN_PUBLICATION_TTL_MS()) {
        this->publicationTtlMs = MIN_PUBLICATION_TTL_MS();
    }
}

UnicastSubscriptionQos& UnicastSubscriptionQos::operator=(
        const UnicastSubscriptionQos& subscriptionQos)
{
    expiryDateMs = subscriptionQos.getExpiryDateMs();
    publicationTtlMs = subscriptionQos.getPublicationTtlMs();
    return *this;
}

bool UnicastSubscriptionQos::operator==(const UnicastSubscriptionQos& subscriptionQos) const
{
    return expiryDateMs == subscriptionQos.getExpiryDateMs() &&
           publicationTtlMs == subscriptionQos.getPublicationTtlMs();
}

} // namespace joynr

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

std::int64_t UnicastSubscriptionQos::DEFAULT_PUBLICATION_TTL_MS()
{
    return 10000;
}

std::int64_t UnicastSubscriptionQos::MIN_PUBLICATION_TTL_MS()
{
    return 100;
}

std::int64_t UnicastSubscriptionQos::MAX_PUBLICATION_TTL_MS()
{
    return 2592000000UL;
}

UnicastSubscriptionQos::UnicastSubscriptionQos()
        : SubscriptionQos(), _publicationTtlMs(DEFAULT_PUBLICATION_TTL_MS())
{
}

UnicastSubscriptionQos::UnicastSubscriptionQos(const UnicastSubscriptionQos& other)
        : SubscriptionQos::SubscriptionQos(other), _publicationTtlMs(other._publicationTtlMs)
{
}

UnicastSubscriptionQos::UnicastSubscriptionQos(std::int64_t validityMs,
                                               std::int64_t publicationTtlMs)
        : SubscriptionQos(validityMs), _publicationTtlMs(DEFAULT_PUBLICATION_TTL_MS())
{
    setPublicationTtlMs(publicationTtlMs);
}

std::int64_t UnicastSubscriptionQos::getPublicationTtlMs() const
{
    return _publicationTtlMs;
}

void UnicastSubscriptionQos::setPublicationTtlMs(std::int64_t publicationTtlMs)
{
    this->_publicationTtlMs = publicationTtlMs;
    if (this->_publicationTtlMs > MAX_PUBLICATION_TTL_MS()) {
        this->_publicationTtlMs = MAX_PUBLICATION_TTL_MS();
    }
    if (this->_publicationTtlMs < MIN_PUBLICATION_TTL_MS()) {
        this->_publicationTtlMs = MIN_PUBLICATION_TTL_MS();
    }
}

UnicastSubscriptionQos& UnicastSubscriptionQos::operator=(
        const UnicastSubscriptionQos& subscriptionQos)
{
    _expiryDateMs = subscriptionQos.getExpiryDateMs();
    _publicationTtlMs = subscriptionQos.getPublicationTtlMs();
    return *this;
}

bool UnicastSubscriptionQos::operator==(const UnicastSubscriptionQos& subscriptionQos) const
{
    return _expiryDateMs == subscriptionQos.getExpiryDateMs() &&
           _publicationTtlMs == subscriptionQos.getPublicationTtlMs();
}

} // namespace joynr

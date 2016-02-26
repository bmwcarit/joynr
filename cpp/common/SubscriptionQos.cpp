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
#include "joynr/SubscriptionQos.h"
#include <limits>
#include <chrono>
#include "joynr/Variant.h"

namespace joynr
{

static const bool isSubscriptionQosRegistered =
        Variant::registerType<SubscriptionQos>("joynr.SubscriptionQos");

const std::int64_t& SubscriptionQos::DEFAULT_PUBLICATION_TTL_MS()
{
    static const std::int64_t defaultPublicationTtl = 10000;
    return defaultPublicationTtl;
}

const std::int64_t& SubscriptionQos::DEFAULT_PUBLICATION_TTL()
{
    return DEFAULT_PUBLICATION_TTL_MS();
}

const std::int64_t& SubscriptionQos::MIN_PUBLICATION_TTL_MS()
{
    static const std::int64_t minPublicationTtl = 100;
    return minPublicationTtl;
}

const std::int64_t& SubscriptionQos::MIN_PUBLICATION_TTL()
{
    return MIN_PUBLICATION_TTL_MS();
}

const std::int64_t& SubscriptionQos::MAX_PUBLICATION_TTL_MS()
{
    static const std::int64_t maxPublicationTtl = 2592000000UL;
    return maxPublicationTtl;
}

const std::int64_t& SubscriptionQos::MAX_PUBLICATION_TTL()
{
    return MAX_PUBLICATION_TTL_MS();
}

const std::int64_t& SubscriptionQos::NO_EXPIRY_DATE_TTL()
{
    static const std::int64_t noExpiryDateTTL = std::numeric_limits<std::int64_t>::max(); // 2^63-1
    return noExpiryDateTTL;
}

const std::int64_t& SubscriptionQos::NO_EXPIRY_DATE()
{
    static std::int64_t noExpiryDate = 0;
    return noExpiryDate;
}

SubscriptionQos::SubscriptionQos()
        : expiryDateMs(-1), publicationTtlMs(DEFAULT_PUBLICATION_TTL_MS())
{
    setValidityMs(1000);
}

SubscriptionQos::SubscriptionQos(const std::int64_t& validityMs)
        : expiryDateMs(-1), publicationTtlMs(DEFAULT_PUBLICATION_TTL_MS())
{
    setValidityMs(validityMs);
}

std::int64_t SubscriptionQos::getPublicationTtlMs() const
{
    return publicationTtlMs;
}

std::int64_t SubscriptionQos::getPublicationTtl() const
{
    return getPublicationTtlMs();
}

void SubscriptionQos::setPublicationTtlMs(const std::int64_t& publicationTtlMs)
{
    this->publicationTtlMs = publicationTtlMs;
    if (this->publicationTtlMs > MAX_PUBLICATION_TTL_MS()) {
        this->publicationTtlMs = MAX_PUBLICATION_TTL_MS();
    }
    if (this->publicationTtlMs < MIN_PUBLICATION_TTL_MS()) {
        this->publicationTtlMs = MIN_PUBLICATION_TTL_MS();
    }
}

void SubscriptionQos::setPublicationTtl(const std::int64_t& publicationTtlMs)
{
    setPublicationTtlMs(publicationTtlMs);
}

std::int64_t SubscriptionQos::getExpiryDateMs() const
{
    return expiryDateMs;
}

std::int64_t SubscriptionQos::getExpiryDate() const
{
    return getExpiryDateMs();
}

void SubscriptionQos::setExpiryDateMs(const std::int64_t& expiryDateMs)
{
    this->expiryDateMs = expiryDateMs;
}

void SubscriptionQos::setExpiryDate(const std::int64_t& expiryDateMs)
{
    setExpiryDateMs(expiryDateMs);
}

void SubscriptionQos::clearExpiryDate()
{
    this->expiryDateMs = NO_EXPIRY_DATE();
}

void SubscriptionQos::setValidityMs(const std::int64_t& validityMs)
{
    if (validityMs == -1) {
        setExpiryDateMs(SubscriptionQos::NO_EXPIRY_DATE());
    } else {
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        setExpiryDateMs(now + validityMs);
    }
}

void SubscriptionQos::setValidity(const std::int64_t& validityMs)
{
    setValidityMs(validityMs);
}

SubscriptionQos& SubscriptionQos::operator=(const SubscriptionQos& subscriptionQos)
{
    expiryDateMs = subscriptionQos.getExpiryDateMs();
    publicationTtlMs = subscriptionQos.getPublicationTtlMs();
    return *this;
}

bool SubscriptionQos::operator==(const SubscriptionQos& subscriptionQos) const
{
    return getExpiryDateMs() == subscriptionQos.getExpiryDateMs() &&
           publicationTtlMs == subscriptionQos.getPublicationTtlMs();
}

} // namespace joynr

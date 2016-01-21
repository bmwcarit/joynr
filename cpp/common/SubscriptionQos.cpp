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
#include "joynr/DispatcherUtils.h"
#include <limits>
#include <chrono>
#include <stdexcept>
#include "joynr/Variant.h"

namespace joynr
{

static const bool isSubscriptionQosRegistered =
        Variant::registerType<SubscriptionQos>("joynr.SubscriptionQos");

const std::int64_t& SubscriptionQos::DEFAULT_PUBLICATION_TTL()
{
    static const std::int64_t defaultPublicationTtl = 10000;
    return defaultPublicationTtl;
}

const std::int64_t& SubscriptionQos::MIN_PUBLICATION_TTL()
{
    static const std::int64_t minPublicationTtl = 100;
    return minPublicationTtl;
}

const std::int64_t& SubscriptionQos::MAX_PUBLICATION_TTL()
{
    static const std::int64_t maxPublicationTtl = 2592000000UL;
    return maxPublicationTtl;
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

SubscriptionQos::SubscriptionQos() : expiryDate(-1), publicationTtl(DEFAULT_PUBLICATION_TTL())
{
    setValidity(1000);
}

SubscriptionQos::SubscriptionQos(const std::int64_t& validity)
        : expiryDate(-1), publicationTtl(DEFAULT_PUBLICATION_TTL())
{
    setValidity(validity);
}

std::int64_t SubscriptionQos::getPublicationTtl() const
{
    return publicationTtl;
}

void SubscriptionQos::setPublicationTtl(const std::int64_t& publicationTtl_ms)
{
    this->publicationTtl = publicationTtl_ms;
    if (this->publicationTtl > MAX_PUBLICATION_TTL()) {
        this->publicationTtl = MAX_PUBLICATION_TTL();
    }
    if (this->publicationTtl < MIN_PUBLICATION_TTL()) {
        this->publicationTtl = MIN_PUBLICATION_TTL();
    }
}

std::int64_t SubscriptionQos::getExpiryDate() const
{
    return expiryDate;
}

void SubscriptionQos::setExpiryDate(const std::int64_t& expiryDate)
{
    this->expiryDate = expiryDate;
}

void SubscriptionQos::clearExpiryDate()
{
    this->expiryDate = NO_EXPIRY_DATE();
}

void SubscriptionQos::setValidity(const std::int64_t& validity)
{
    if (validity == -1) {
        setExpiryDate(SubscriptionQos::NO_EXPIRY_DATE());
    } else {
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        setExpiryDate(now + validity);
    }
}

SubscriptionQos& SubscriptionQos::operator=(const SubscriptionQos& subscriptionQos)
{
    expiryDate = subscriptionQos.getExpiryDate();
    publicationTtl = subscriptionQos.getPublicationTtl();
    return *this;
}

bool SubscriptionQos::operator==(const SubscriptionQos& subscriptionQos) const
{
    return getExpiryDate() == subscriptionQos.getExpiryDate() &&
           publicationTtl == subscriptionQos.getPublicationTtl();
}

} // namespace joynr

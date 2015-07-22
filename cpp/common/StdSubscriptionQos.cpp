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
#include "joynr/StdSubscriptionQos.h"
#include "joynr/DispatcherUtils.h"
#include <limits>

namespace joynr
{

const int64_t& StdSubscriptionQos::DEFAULT_PUBLICATION_TTL()
{
    static const int64_t defaultPublicationTtl = 10000;
    return defaultPublicationTtl;
}

const int64_t& StdSubscriptionQos::MIN_PUBLICATION_TTL()
{
    static const int64_t minPublicationTtl = 100;
    return minPublicationTtl;
}

const int64_t& StdSubscriptionQos::MAX_PUBLICATION_TTL()
{
    static const int64_t maxPublicationTtl = 2592000000UL;
    return maxPublicationTtl;
}

const int64_t& StdSubscriptionQos::NO_EXPIRY_DATE_TTL()
{
    static const int64_t noExpiryDateTTL = std::numeric_limits<int64_t>::max(); // 2^63-1
    return noExpiryDateTTL;
}

const int64_t& StdSubscriptionQos::NO_EXPIRY_DATE()
{
    static int64_t noExpiryDate = 0;
    return noExpiryDate;
}

StdSubscriptionQos::StdSubscriptionQos() : expiryDate(-1), publicationTtl(DEFAULT_PUBLICATION_TTL())
{
    setValidity(1000);
}

StdSubscriptionQos::StdSubscriptionQos(const int64_t& validity)
        : expiryDate(-1), publicationTtl(DEFAULT_PUBLICATION_TTL())
{
    setValidity(validity);
}

StdSubscriptionQos::StdSubscriptionQos(const StdSubscriptionQos& subscriptionQos)
        : expiryDate(subscriptionQos.expiryDate), publicationTtl(subscriptionQos.publicationTtl)
{
}

StdSubscriptionQos::~StdSubscriptionQos()
{
}

int64_t StdSubscriptionQos::getPublicationTtl() const
{
    return publicationTtl;
}

void StdSubscriptionQos::setPublicationTtl(const int64_t& publicationTtl)
{
    this->publicationTtl = publicationTtl;
    if (this->publicationTtl > MAX_PUBLICATION_TTL()) {
        this->publicationTtl = MAX_PUBLICATION_TTL();
    }
    if (this->publicationTtl < MIN_PUBLICATION_TTL()) {
        this->publicationTtl = MIN_PUBLICATION_TTL();
    }
}

int64_t StdSubscriptionQos::getExpiryDate() const
{
    return expiryDate;
}

void StdSubscriptionQos::setExpiryDate(const int64_t& expiryDate)
{
    this->expiryDate = expiryDate;
    if (this->expiryDate < QDateTime::currentMSecsSinceEpoch()) {
        clearExpiryDate();
    }
}

void StdSubscriptionQos::clearExpiryDate()
{
    this->expiryDate = NO_EXPIRY_DATE();
}

void StdSubscriptionQos::setValidity(const int64_t& validity)
{
    if (validity == -1) {
        setExpiryDate(joynr::StdSubscriptionQos::NO_EXPIRY_DATE());
    } else {
        setExpiryDate(QDateTime::currentMSecsSinceEpoch() + validity);
    }
}

StdSubscriptionQos& StdSubscriptionQos::operator=(const StdSubscriptionQos& subscriptionQos)
{
    expiryDate = subscriptionQos.getExpiryDate();
    publicationTtl = subscriptionQos.getPublicationTtl();
    return *this;
}

bool StdSubscriptionQos::operator==(const StdSubscriptionQos& subscriptionQos) const
{
    return getExpiryDate() == subscriptionQos.getExpiryDate() &&
           publicationTtl == subscriptionQos.getPublicationTtl();
}

} // namespace joynr

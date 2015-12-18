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

const int64_t& OnChangeSubscriptionQos::DEFAULT_MIN_INTERVAL()
{
    static int64_t defaultMinInterval = 1000;
    return defaultMinInterval;
}

const int64_t& OnChangeSubscriptionQos::MIN_MIN_INTERVAL()
{
    static int64_t minMinInterval = 0;
    return minMinInterval;
}

const int64_t& OnChangeSubscriptionQos::MAX_MIN_INTERVAL()
{
    static int64_t maxMinInterval = 2592000000UL;
    return maxMinInterval;
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos()
        : SubscriptionQos(), minInterval(MIN_MIN_INTERVAL())
{
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos(const int64_t& validity,
                                                 const int64_t& minInterval)
        : SubscriptionQos(validity), minInterval(DEFAULT_MIN_INTERVAL())
{
    setMinInterval(minInterval);
}

OnChangeSubscriptionQos::OnChangeSubscriptionQos(const OnChangeSubscriptionQos& other)
        : SubscriptionQos(other), minInterval(other.getMinInterval())
{
}

int64_t OnChangeSubscriptionQos::getMinInterval() const
{
    return minInterval;
}

void OnChangeSubscriptionQos::setMinInterval(const int64_t& minInterval)
{
    this->minInterval = minInterval;
    if (this->minInterval < MIN_MIN_INTERVAL()) {
        this->minInterval = MIN_MIN_INTERVAL();
    }
    if (this->minInterval > MAX_MIN_INTERVAL()) {
        this->minInterval = MAX_MIN_INTERVAL();
    }
}

OnChangeSubscriptionQos& OnChangeSubscriptionQos::operator=(const OnChangeSubscriptionQos& other)
{
    expiryDate = other.getExpiryDate();
    publicationTtl = other.getPublicationTtl();
    minInterval = other.getMinInterval();
    return *this;
}

bool OnChangeSubscriptionQos::operator==(const OnChangeSubscriptionQos& other) const
{
    return expiryDate == other.getExpiryDate() && publicationTtl == other.getPublicationTtl() &&
           minInterval == other.getMinInterval();
}
